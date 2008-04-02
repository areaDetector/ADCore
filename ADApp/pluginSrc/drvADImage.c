/*
 * drvADAsynImage.c
 * 
 * Asyn driver for area detectors
 *
 * Original Author: Mark Rivers
 * Current Author: Mark Rivers
 *
 * Created March 6, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iocsh.h>
#include <epicsExport.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsMessageQueue.h>
#include <cantProceed.h>

#include <asynStandardInterfaces.h>

#include "ADInterface.h"
#include "ADParamLib.h"
#include "ADUtils.h"
#include "asynADImage.h"

#define RATE_TIME 2.0  /* Time between computing frame and image rates */

#define driverName "drvADImage"

typedef enum
{
    ADCmdUpdateTime,          /* (float64, r/w) Minimum time between image updates */
    ADCmdPostImages,          /* (int32,   r/w) Post images (1=Yes, 0=No) */
    ADCmdImageCounter,        /* (int32,   r/w) Image counter.  Increments by 1 when image posted */
    ADCmdImageRate,           /* (float64, r/o) Image rate.  Rate at which images are being posted */
    ADCmdFrameCounter,        /* (int32,   r/w) Frame counter.  Increments by 1 when image callback */
    ADCmdFrameRate,           /* (float64, r/o) Frame rate.  Rate at which images are being received by driver */
    ADCmdImageData            /* (void*,   r/w) Image data waveform */
} DetParam_t;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t DetParamString[] = {
    {ADCmdUpdateTime,  "IMAGE_UPDATE_TIME" },
    {ADCmdPostImages,  "POST_IMAGES"  },
    {ADCmdImageCounter,"IMAGE_COUNTER"},
    {ADCmdImageRate,   "IMAGE_RATE"   },
    {ADCmdImageData,   "IMAGE_DATA"   }
};

#define NUM_DET_PARAMS (sizeof(DetParamString)/sizeof(DetParamString[0]))

typedef struct drvADPvt {
    /* These fields will be needed by most asyn plug-in drivers */
    char *portName;
    char *detectorPortName;
    epicsMutexId mutexId;
    epicsEventId eventId;
    PARAMS params;
    /* Asyn interfaces */
    asynStandardInterfaces asynInterfaces;
    asynADImage *pasynADImage;
    void *asynADImagePvt;
    void *asynADImageInterruptPvt;
    asynUser *pasynUserADImage;
    asynUser *pasynUser;
    
    /* These fields are specific to the ADImage driver */
    void *inputBuffer;
    size_t inputSize;
    void *outputBuffer;
    size_t outputSize;
    /* Image data posting */
    int nx;
    int ny;
    int imageRateCounter;
    int frameRateCounter;
    ADDataType_t dataType;
    epicsTimeStamp lastRateTime;
    epicsTimeStamp lastImagePostTime;
} drvADPvt;


/* Local functions, not in any interface */

static int allocateImageBuffer(void **buffer, size_t newSize, size_t *oldSize)
{
    if (newSize > *oldSize) {
        free(*buffer);
        *buffer = malloc(newSize*sizeof(char));
        *oldSize = newSize;
        if (!*buffer) {
            *oldSize=0;
            return(asynError);
        }
    }
    return(asynSuccess);
}


static void ADImageCallback(void *drvPvt, asynUser *pasynUser, void *value,  
                            int dataType, int nx, int ny)
{
    /* This callback function is called from the detector driver when a new image arrives.
     * It copies the data to a private buffer and then signals the background thread to call
     * back any registered clients.
     */
     
    drvADPvt *pPvt = drvPvt;
    epicsTimeStamp tCheck;
    double minImageUpdateTime, deltaTime;
    int imageCounter;
    int status;
    int imageSize, bytesPerPixel;

    epicsMutexLock(pPvt->mutexId);

    status |= ADParam->getDouble(pPvt->params, ADCmdUpdateTime, &minImageUpdateTime);
    status |= ADParam->getInteger(pPvt->params, ADCmdImageCounter, &imageCounter);
    
    epicsTimeGetCurrent(&tCheck);
    deltaTime = epicsTimeDiffInSeconds(&tCheck, &pPvt->lastImagePostTime);

    if (deltaTime < minImageUpdateTime) goto skipPost;  /* Not time to post the next image */

    pPvt->dataType = dataType;
    pPvt->nx = nx;
    pPvt->ny = ny;
    ADUtils->bytesPerPixel(dataType, &bytesPerPixel);
    imageSize = bytesPerPixel * nx * ny;

    /* Make sure our buffer is large enough to hold data.  If not free it and allocate a new one. */
    status = allocateImageBuffer(&pPvt->inputBuffer, imageSize, &pPvt->inputSize);
    
    /* Copy the data from the callback buffer to our private buffer */
    memcpy(pPvt->inputBuffer, value, imageSize);
    
    /* Send an event to the background task so it processes the callbacks for this image */
    epicsEventSignal(pPvt->eventId);
    
    skipPost:
    
    epicsMutexUnlock(pPvt->mutexId);
}

static void ADImageDoCallbacks(drvADPvt *pPvt)
{
    /* This function calls back any registered clients on the standard asyn array interfaces with 
     * the data in our private buffer.
     * It is called with the mutex already locked.
     */
     
    ELLLIST *pclientList;
    int nPixels = pPvt->nx * pPvt->ny;
    int nxt, nyt;
    interruptNode *pnode;
    epicsTimeStamp tCheck;
    double deltaTime, rate;
    int imageCounter;
    int int8Initialized=0;
    int int16Initialized=0;
    int int32Initialized=0;
    int float32Initialized=0;
    int float64Initialized=0;
    asynStandardInterfaces *pInterfaces = &pPvt->asynInterfaces;

    ADParam->getInteger(pPvt->params, ADCmdImageCounter, &imageCounter);

    /* This macro saves a lot of code, since there are 5 types of array interrupts that we support */
    #define ARRAY_INTERRUPT_CALLBACK(INTERRUPT_PVT, INTERRUPT_TYPE, INITIALIZED, \
                                     SIGNED_TYPE, UNSIGNED_TYPE, EPICS_TYPE) { \
        EPICS_TYPE *pData=NULL;\
        pasynManager->interruptStart(pInterfaces->INTERRUPT_PVT, &pclientList); \
        pnode = (interruptNode *)ellFirst(pclientList); \
        while (pnode) { \
            INTERRUPT_TYPE *pInterrupt = pnode->drvPvt; \
            if (!INITIALIZED) { \
                INITIALIZED = 1; \
                allocateImageBuffer(&pPvt->outputBuffer, nPixels*sizeof(EPICS_TYPE), &pPvt->outputSize); \
                ADUtils->convertImage(pPvt->inputBuffer, pPvt->dataType, \
                                      pPvt->nx, pPvt->ny, \
                                      pPvt->outputBuffer, SIGNED_TYPE, \
                                      1, 1, 0, 0, \
                                      pPvt->nx, pPvt->ny, &nxt, &nyt); \
                pData = (EPICS_TYPE *)pPvt->outputBuffer; \
             } \
            pInterrupt->callback(pInterrupt->userPvt, \
                                 pInterrupt->pasynUser, \
                                 pData, nPixels); \
            pnode = (interruptNode *)ellNext(&pnode->node); \
        } \
        pasynManager->interruptEnd(pInterfaces->INTERRUPT_PVT); \
    }

    /* Pass interrupts for int8Array data*/
    ARRAY_INTERRUPT_CALLBACK(int8ArrayInterruptPvt, asynInt8ArrayInterrupt,
                             int8Initialized, ADInt8, ADUInt8, epicsInt8);
    
    /* Pass interrupts for int16Array data*/
    ARRAY_INTERRUPT_CALLBACK(int16ArrayInterruptPvt, asynInt16ArrayInterrupt,
                             int16Initialized, ADInt16, ADUInt16, epicsInt16);
    
    /* Pass interrupts for int32Array data*/
    ARRAY_INTERRUPT_CALLBACK(int32ArrayInterruptPvt, asynInt32ArrayInterrupt,
                             int32Initialized, ADInt32, ADUInt32, epicsInt32);
    
    /* Pass interrupts for float32Array data*/
    ARRAY_INTERRUPT_CALLBACK(float32ArrayInterruptPvt, asynFloat32ArrayInterrupt,
                             float32Initialized, ADFloat32, ADFloat32, epicsFloat32);
    
    /* Pass interrupts for float64Array data*/
    ARRAY_INTERRUPT_CALLBACK(float64ArrayInterruptPvt, asynFloat64ArrayInterrupt,
                             float64Initialized, ADFloat64, ADFloat64, epicsFloat64);
    
    epicsTimeGetCurrent(&tCheck);
    deltaTime = epicsTimeDiffInSeconds(&tCheck, &pPvt->lastImagePostTime);
    memcpy(&pPvt->lastImagePostTime, &tCheck, sizeof(epicsTimeStamp));
    imageCounter++;
    pPvt->imageRateCounter++;
    ADParam->setInteger(pPvt->params, ADCmdImageCounter, imageCounter);

    /* See if it is time to compute the rates */
    deltaTime = epicsTimeDiffInSeconds(&tCheck, &pPvt->lastRateTime);
    if (deltaTime > RATE_TIME) {
        /* Now do the image rate */
        rate = pPvt->imageRateCounter / deltaTime;
        ADParam->setDouble(pPvt->params, ADCmdImageRate, rate);
        pPvt->imageRateCounter = 0;
        memcpy(&pPvt->lastRateTime, &tCheck, sizeof(epicsTimeStamp));
    }
    
    ADParam->callCallbacks(pPvt->params);
}

static void ADImageTask(drvADPvt *pPvt)
{
    /* This thread does the callbacks to the clients when a new frame arrives */

    /* Loop forever */
    while (1) {
    
        epicsEventWait(pPvt->eventId);
        epicsMutexLock(pPvt->mutexId);
        
        /* Call the function that does the callbacks */
        ADImageDoCallbacks(pPvt);
        
       /* We are done accessing data structures, release the lock */
        epicsMutexUnlock(pPvt->mutexId);
        
    }
}



/* asynInt32 interface methods */
static asynStatus readInt32(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 *value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(pPvt->mutexId);
    
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getInteger(pPvt->params, function, value);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:readInt32 error, status=%d function=%d, value=%d\n", 
                  driverName, status, function, *value);
    else        
        asynPrint(pPvt->pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readInt32: function=%d, value=%d\n", 
              driverName, function, *value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeInt32(void *drvPvt, asynUser *pasynUser, 
                             epicsInt32 value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(pPvt->mutexId);

    /* Set the parameter in the parameter library. */
    status |= ADParam->setInteger(pPvt->params, function, value);

    switch(function) {
        case ADCmdPostImages:
            if (value) {
                /* Register to be called with interrupts from the detector driver on the asynADImage interface */
                status = pPvt->pasynADImage->registerInterruptUser(
                            pPvt->asynADImagePvt, pPvt->pasynUserADImage,
                            ADImageCallback, pPvt, &pPvt->asynADImageInterruptPvt);
                if (status!=asynSuccess) {
                    asynPrint(pasynUser, ASYN_TRACE_ERROR,
                        "%s::writeInt32 ERROR: Can't register for interrupt callbacks on detector port: %s.\n",
                        driverName, pPvt->detectorPortName);
                    status = asynError;
                }
            } else {
                /* Cancel request to be called with interrupts from the detector driver on the asynADImage interface */
                status = pPvt->pasynADImage->cancelInterruptUser(
                            pPvt->asynADImagePvt, pPvt->pasynUserADImage,
                            pPvt->asynADImageInterruptPvt);
                if (status!=asynSuccess) {
                    asynPrint(pasynUser, ASYN_TRACE_ERROR,
                        "%s::writeInt32 ERROR: Can't register for interrupt callbacks on detector port: %s.\n",
                        driverName, pPvt->detectorPortName);
                    status = asynError;
                }
            }
            break;
        default:
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeInt32 error, status=%d function=%d, value=%d\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeInt32: function=%d, value=%d\n", 
              driverName, function, value);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

static asynStatus getBounds(void *drvPvt, asynUser *pasynUser,
                            epicsInt32 *low, epicsInt32 *high)
{
    /* This is only needed for the asynInt32 interface when the device uses raw units.
       Our interface is using engineering units. */
    *low = 0;
    *high = 65535;
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s::getBounds,low=%d, high=%d\n", 
              driverName, *low, *high);
    return(asynSuccess);
}


/* asynFloat64 interface methods */
static asynStatus readFloat64(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 *value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    
    epicsMutexLock(pPvt->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getDouble(pPvt->params, function, value);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:readFloat64 error, status=%d function=%d, value=%f\n", 
              driverName, status, function, *value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readFloat64: function=%d, value=%f\n", 
              driverName, function, *value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeFloat64(void *drvPvt, asynUser *pasynUser, 
                               epicsFloat64 value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(pPvt->mutexId);

    /* Set the parameter in the parameter library. */
    status |= ADParam->setDouble(pPvt->params, function, value);

    switch(function) {
        /* We don't currently need to do anything special when these functions are received */
        default:
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeFloat64 error, status=%d function=%d, value=%f\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeFloat64: function=%d, value=%f\n", 
              driverName, function, value);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


/* asynOctet interface methods */
static asynStatus readOctet(void *drvPvt, asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
   
    epicsMutexLock(pPvt->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getString(pPvt->params, function, maxChars, value);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:readOctet error, status=%d function=%d, value=%s\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readOctet: function=%d, value=%s\n", 
              driverName, function, value);
    *eomReason = ASYN_EOM_END;
    *nActual = strlen(value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeOctet(void *drvPvt, asynUser *pasynUser,
                             const char *value, size_t nChars, size_t *nActual)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(pPvt->mutexId);
    /* Set the parameter in the parameter library. */
    status |= ADParam->setString(pPvt->params, function, (char *)value);

    switch(function) {
        /* We don't currently need to do anything special when these functions are received */
        default:
            break;
    }
    
     /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);

    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeOctet error, status=%d function=%d, value=%s\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeOctet: function=%d, value=%s\n", 
              driverName, function, value);
    *nActual = nChars;
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


/* The following macros save a lot of code, since we have 5 array types to support */
#define DEFINE_READ_ARRAY(FUNCTION_NAME, EPICS_TYPE, AD_TYPE) \
static asynStatus FUNCTION_NAME(void *drvPvt, asynUser *pasynUser, \
                                EPICS_TYPE *value, size_t nelements, size_t *nIn) \
{ \
    drvADPvt *pPvt = (drvADPvt *)drvPvt; \
    DetParam_t command = pasynUser->reason; \
    asynStatus status = asynSuccess; \
    int nPixels; \
    int nx=pPvt->nx, ny=pPvt->ny, nxt, nyt; \
    \
    epicsMutexLock(pPvt->mutexId); \
    nPixels = pPvt->nx * pPvt->ny; \
    if (nPixels > nelements) { \
        /* We have been requested fewer pixels than we have.  \
         * Just pass the first nelements pixels.  Do this by \
         * faking the values for convertImage, which will just copy the \
         * first nelements pixels. */ \
         nx = nelements; \
         ny = 1; \
    } \
    switch(command) { \
        case ADCmdImageData: \
            /* We are guaranteed to have the most recent data in our buffer.  No need to call the driver, \
             * just copy the data from our buffer */ \
            /* Convert data from its actual data type.  */ \
            status = ADUtils->convertImage(pPvt->inputBuffer, pPvt->dataType, nx, ny, \
                                           value, AD_TYPE, \
                                           1, 1, 0, 0, \
                                           nx, ny, &nxt, &nyt); \
            break; \
        default: \
            asynPrint(pasynUser, ASYN_TRACE_ERROR, \
                      "%s::readArray, unknown command %d\n" \
                      driverName, command); \
            status = asynError; \
    } \
    epicsMutexUnlock(pPvt->mutexId); \
    return(asynSuccess); \
}

#define DEFINE_WRITE_ARRAY(FUNCTION_NAME, EPICS_TYPE, AD_TYPE) \
static asynStatus FUNCTION_NAME(void *drvPvt, asynUser *pasynUser, \
                                EPICS_TYPE *value, size_t nelements) \
{ \
    /* Note yet implemented */ \
    asynPrint(pasynUser, ASYN_TRACE_ERROR, \
              "%s::WRITE_ARRAY not yet implemented\n", driverName); \
    return(asynError); \
}

/* asynInt8Array interface methods */
DEFINE_READ_ARRAY(readInt8Array, epicsInt8, ADInt8)
DEFINE_WRITE_ARRAY(writeInt8Array, epicsInt8, ADInt8)
    
/* asynInt16Array interface methods */
DEFINE_READ_ARRAY(readInt16Array, epicsInt16, ADInt16)
DEFINE_WRITE_ARRAY(writeInt16Array, epicsInt16, ADInt16)
    
/* asynInt32Array interface methods */
DEFINE_READ_ARRAY(readInt32Array, epicsInt32, ADInt32)
DEFINE_WRITE_ARRAY(writeInt32Array, epicsInt32, ADInt32)
    
/* asynFloat32Array interface methods */
DEFINE_READ_ARRAY(readFloat32Array, epicsFloat32, ADFloat32)
DEFINE_WRITE_ARRAY(writeFloat32Array, epicsFloat32, ADFloat32)
    
/* asynFloat64Array interface methods */
DEFINE_READ_ARRAY(readFloat64Array, epicsFloat64, ADFloat64)
DEFINE_WRITE_ARRAY(writeFloat64Array, epicsFloat64, ADFloat64)
    

/* asynDrvUser routines */
static asynStatus drvUserCreate(void *drvPvt, asynUser *pasynUser,
                                const char *drvInfo, 
                                const char **pptypeName, size_t *psize)
{
    int status;
    int param;

    status = ADUtils->findParam(DetParamString, NUM_DET_PARAMS, 
                                drvInfo, &param);
    if (status == asynSuccess) {
        pasynUser->reason = param;
        if (pptypeName) {
            *pptypeName = epicsStrDup(drvInfo);
        }
        if (psize) {
            *psize = sizeof(param);
        }
        asynPrint(pasynUser, ASYN_TRACE_FLOW,
                  "%s::drvUserCreate, drvInfo=%s, param=%d\n", 
                  driverName, drvInfo, param);
        return(asynSuccess);
    } else {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "%s::drvUserCreate, unknown drvInfo=%s", 
                     driverName, drvInfo);
        return(asynError);
    }
}

    
static asynStatus drvUserGetType(void *drvPvt, asynUser *pasynUser,
                                 const char **pptypeName, size_t *psize)
{
    /* This is not currently supported, because we can't get the strings for driver-specific commands */

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::drvUserGetType entered", driverName);

    *pptypeName = NULL;
    *psize = 0;
    return(asynError);
}

static asynStatus drvUserDestroy(void *drvPvt, asynUser *pasynUser)
{
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::drvUserDestroy, drvPvt=%p, pasynUser=%p\n",
              driverName, drvPvt, pasynUser);

    return(asynSuccess);
}

/* asynCommon routines */

static void report(void *drvPvt, FILE *fp, int details)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    interruptNode *pnode;
    ELLLIST *pclientList;
    asynStandardInterfaces *pInterfaces = &pPvt->asynInterfaces;

    fprintf(fp, "Port: %s\n", pPvt->portName);
    if (details >= 1) {
        /* Report int32 interrupts */
        pasynManager->interruptStart(pInterfaces->int32InterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynInt32Interrupt *pint32Interrupt = pnode->drvPvt;
            fprintf(fp, "    int32 callback client address=%p, addr=%d, reason=%d\n",
                    pint32Interrupt->callback, pint32Interrupt->addr, 
                    pint32Interrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pInterfaces->int32InterruptPvt);

        /* Report float64 interrupts */
        pasynManager->interruptStart(pInterfaces->float64InterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynFloat64Interrupt *pfloat64Interrupt = pnode->drvPvt;
            fprintf(fp, "    float64 callback client address=%p, addr=%d, reason=%d\n",
                    pfloat64Interrupt->callback, pfloat64Interrupt->addr, 
                    pfloat64Interrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pInterfaces->float64InterruptPvt);

        /* Report asynInt32Array interrupts */
        pasynManager->interruptStart(pInterfaces->int32ArrayInterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynInt32ArrayInterrupt *pint32ArrayInterrupt = pnode->drvPvt;
            fprintf(fp, "    int32Array callback client address=%p, addr=%d, reason=%d\n",
                    pint32ArrayInterrupt->callback, pint32ArrayInterrupt->addr, 
                    pint32ArrayInterrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pInterfaces->int32ArrayInterruptPvt);

    }
}

static asynStatus connect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionConnect(pasynUser);

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::connect, pasynUser=%p\n", 
              driverName, pasynUser);
    return(asynSuccess);
}


static asynStatus disconnect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionDisconnect(pasynUser);
    return(asynSuccess);
}


/* Configuration routine, called from startup script either directly (on vxWorks) or from shell */

static asynCommon drvADCommon = {
    report,
    connect,
    disconnect
};

static asynInt32 drvADInt32 = {
    writeInt32,
    readInt32,
    getBounds
};

static asynFloat64 drvADFloat64 = {
    writeFloat64,
    readFloat64
};

static asynOctet drvADOctet = {
    writeOctet,
    NULL,
    readOctet,
};

static asynInt8Array drvADInt8Array = {
    writeInt8Array,
    readInt8Array,
};

static asynInt16Array drvADInt16Array = {
    writeInt16Array,
    readInt16Array,
};

static asynInt32Array drvADInt32Array = {
    writeInt32Array,
    readInt32Array,
};

static asynFloat32Array drvADFloat32Array = {
    writeFloat32Array,
    readFloat32Array,
};

static asynFloat64Array drvADFloat64Array = {
    writeFloat64Array,
    readFloat64Array,
};

static asynDrvUser drvADDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};

int drvADImageConfigure(const char *portName, const char *detectorPortName)
{
    drvADPvt *pPvt;
    asynStatus status;
    char *functionName = "drvADImageConfigure";
    asynStandardInterfaces *pInterfaces;
    asynUser *pasynUser;
    asynInterface *pasynInterface;

    pPvt = callocMustSucceed(1, sizeof(*pPvt), functionName);
    pPvt->portName = epicsStrDup(portName);
    pPvt->detectorPortName = epicsStrDup(detectorPortName);

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register port\n", functionName);
        return -1;
    }

    /* Create asynUser for debugging and for standardBases */
    pPvt->pasynUser = pasynManager->createAsynUser(0, 0);

    /* Set addresses of asyn interfaces */
    pInterfaces = &pPvt->asynInterfaces;
    
    pInterfaces->common.pinterface        = (void *)&drvADCommon;
    pInterfaces->drvUser.pinterface       = (void *)&drvADDrvUser;
    pInterfaces->octet.pinterface         = (void *)&drvADOctet;
    pInterfaces->int32.pinterface         = (void *)&drvADInt32;
    pInterfaces->float64.pinterface       = (void *)&drvADFloat64;
    pInterfaces->int8Array.pinterface     = (void *)&drvADInt8Array;
    pInterfaces->int16Array.pinterface    = (void *)&drvADInt16Array;
    pInterfaces->int32Array.pinterface    = (void *)&drvADInt32Array;
    pInterfaces->float32Array.pinterface  = (void *)&drvADFloat32Array;
    pInterfaces->float64Array.pinterface  = (void *)&drvADFloat64Array;

    /* Define which interfaces can generate interrupts */
    pInterfaces->octetCanInterrupt        = 1;
    pInterfaces->int32CanInterrupt        = 1;
    pInterfaces->float64CanInterrupt      = 1;
    pInterfaces->int8ArrayCanInterrupt    = 1;
    pInterfaces->int16ArrayCanInterrupt   = 1;
    pInterfaces->int32ArrayCanInterrupt   = 1;
    pInterfaces->float32ArrayCanInterrupt = 1;
    pInterfaces->float64ArrayCanInterrupt = 1;

    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     pPvt->pasynUser, pPvt);
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register interfaces: %s.\n",
                functionName, pPvt->pasynUser->errorMessage);
        return -1;
    }
    
    /* Create asynUser for communicating with detector driver */
    pasynUser = pasynManager->createAsynUser(0, 0);
    pPvt->pasynUserADImage = pasynUser;

    /* Connect to the detector port driver */
    status = pasynManager->connectDevice(pasynUser, detectorPortName, 0);
    if (status != asynSuccess) {
        printf("%s ERROR: Can't connect to detector port: %s.\n",
                functionName, detectorPortName);
        return -1;
    }
    
    /* Find the asynADImage interface in that driver */
    pasynInterface = pasynManager->findInterface(pasynUser, asynADImageType, 1);
    if (!pasynInterface) {
         printf("%s ERROR: Can't find asynADImage interface on detector port: %s.\n",
                functionName, detectorPortName);
        return -1;
    }
    pPvt->pasynADImage = pasynInterface->pinterface;
    pPvt->asynADImagePvt = pasynInterface->drvPvt;
    
    /* Connect to our device for asynTrace */
    status = pasynManager->connectDevice(pPvt->pasynUser, portName, 0);
    if (status != asynSuccess) {
        printf("%s, connectDevice failed\n", functionName);
        return -1;
    }

    /* Create the epicsMutex for locking access to data structures from other threads */
    pPvt->mutexId = epicsMutexCreate();
    if (!pPvt->mutexId) {
        printf("%s: epicsMutexCreate failure\n", functionName);
        return asynError;
    }

    /* Create the epicsEvent for signaling to the background task when new data has arrived */
    pPvt->eventId = epicsEventCreate(epicsEventEmpty);
    if (!pPvt->eventId) {
        printf("%s: epicsEventCreate failure\n", functionName);
        return asynError;
    }
    
    /* Initialize the parameter library */
    pPvt->params = ADParam->create(0, NUM_DET_PARAMS, &pPvt->asynInterfaces);
    if (!pPvt->params) {
        printf("%s: unable to create parameter library\n", functionName);
        return asynError;
    }
    
    /* Create the thread that does the image callbacks */
    status = (epicsThreadCreate("ADImageTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)ADImageTask,
                                pPvt) == NULL);
    if (status) {
        printf("%s: epicsThreadCreate failure\n", functionName);
        return asynError;
    }

    /* Set the initial values of some parameters */
    ADParam->setInteger(pPvt->params, ADCmdImageCounter, 0);
    ADParam->setInteger(pPvt->params, ADCmdFrameCounter, 0);
    ADParam->setDouble(pPvt->params, ADCmdImageRate, 0.0);
    ADParam->setDouble(pPvt->params, ADCmdFrameRate, 0.0);
    
    return asynSuccess;
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "detectorPortName",iocshArgString};
static const iocshArg * const initArgs[2] = {&initArg0,
                                             &initArg1};
static const iocshFuncDef initFuncDef = {"drvADImageConfigure",2,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    drvADImageConfigure(args[0].sval, args[1].sval);
}

void ADImageRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

epicsExportRegistrar(ADImageRegister);
