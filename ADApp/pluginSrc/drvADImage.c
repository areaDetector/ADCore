/*
 * drvADImage.c
 * 
 * Asyn driver for callbacks to asyn array interfaces for area detectors.
 * This is commonly used for EPICS waveform records.
 *
 * Author: Mark Rivers
 *
 * Created April 2, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsMessageQueue.h>
#include <cantProceed.h>

#include <asynStandardInterfaces.h>

#define DEFINE_STANDARD_PARAM_STRINGS 1
#include "ADInterface.h"
#include "ADParamLib.h"
#include "ADUtils.h"
#include "drvADImage.h"
#include "asynADImage.h"

#define driverName "drvADImage"


typedef enum
{
    ADImgEnableCallbacks,
    ADImgDisableCallbacks,
    ADImgReadParameters,
} ADCallbackReason_t;

typedef struct ADImageFrame {
    int nx;
    int ny;
    int dataType;
    int imageSize;
    void *pImage;
} ADImageFrame_t;

typedef enum
{
    ADImgImagePort=ADFirstDriverParam, /* (asynOctet,    r/w) The port for the ADImage interface */
    ADImgImageAddr,           /* (asynInt32,    r/w) The address on the port */
    ADImgUpdateTime,          /* (asynFloat64,  r/w) Minimum time between image updates */
    ADImgPostImages,          /* (asynInt32,    r/w) Post images (1=Yes, 0=No) */
    ADImgImageData,           /* (asynXXXArray, r/w) Image data waveform */
    ADImgLastDriverParam
} ADImageParam_t;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t ADImageParamString[] = {
    {ADImgImagePort,   "IMAGE_PORT" },
    {ADImgImageAddr,   "IMAGE_ADDR" },
    {ADImgUpdateTime,  "IMAGE_UPDATE_TIME" },
    {ADImgPostImages,  "POST_IMAGES"  },
    {ADImgImageData,   "IMAGE_DATA"   }
};

#define NUM_AD_IMAGE_PARAMS (sizeof(ADImageParamString)/sizeof(ADImageParamString[0]))

typedef struct drvADPvt {
    /* These fields will be needed by most asyn plug-in drivers */
    char *portName;
    epicsMutexId mutexId;
    epicsMessageQueueId msgQId;
    PARAMS params;
    /* Asyn interfaces */
    asynStandardInterfaces asynInterfaces;
    asynADImage *pasynADImage;
    void *asynADImagePvt;
    void *asynADImageInterruptPvt;
    asynUser *pasynUserADImage;
    asynUser *pasynUser;
    
    /* These fields are specific to the ADImage driver */
    ADImageFrame_t *pInputFrames;
    int maxFrames;
    int nextFrame;
    void *pCurrentData;
    void *outputBuffer;
    size_t outputSize;
    ADDataType_t dataType;
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
    int status;
    int imageSize, bytesPerPixel;
    int frameCounter;
    ADImageFrame_t *pFrame;
    char *functionName = "ADImageCallback";

    epicsMutexLock(pPvt->mutexId);

    status |= ADParam->getDouble(pPvt->params, ADImgUpdateTime, &minImageUpdateTime);
    status |= ADParam->getInteger(pPvt->params, ADFrameCounter, &frameCounter);
    
    epicsTimeGetCurrent(&tCheck);
    deltaTime = epicsTimeDiffInSeconds(&tCheck, &pPvt->lastImagePostTime);

    if (deltaTime > minImageUpdateTime) {  
    
        /* Time to post the next image */
        pFrame = &pPvt->pInputFrames[pPvt->nextFrame++];
        if (pPvt->nextFrame == pPvt->maxFrames) pPvt->nextFrame=0;
        pFrame->nx = nx;
        pFrame->ny = ny;
        pFrame->dataType = dataType;
        ADUtils->bytesPerPixel(dataType, &bytesPerPixel);
        imageSize = bytesPerPixel * nx * ny;

        /* Make sure our buffer is large enough to hold data.  If not free it and allocate a new one. */
        status = allocateImageBuffer(&pFrame->pImage, imageSize, &pFrame->imageSize);

        /* Copy the data from the callback buffer to our frame buffer */
        memcpy(pFrame->pImage, value, imageSize);

        /* Send try to put this frame on the message queue.  If there is no room then return
         * immediately. */
        status = epicsMessageQueueTrySend(pPvt->msgQId, pFrame, sizeof(ADImageFrame_t));
        if (status) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                "%s:%s message queue full, dropped frame %d\n",
                driverName, functionName, frameCounter);
        } else {    
            /* Update the time we last posted an image */
            memcpy(&pPvt->lastImagePostTime, &tCheck, sizeof(tCheck));
        }
    }
    epicsMutexUnlock(pPvt->mutexId);
}


static void ADImageDoCallbacks(drvADPvt *pPvt, ADImageFrame_t *pFrame)
{
    /* This function calls back any registered clients on the standard asyn array interfaces with 
     * the data in our private buffer.
     * It is called with the mutex already locked.
     */
     
    ELLLIST *pclientList;
    int nPixels;
    int nxt, nyt;
    interruptNode *pnode;
    int frameCounter;
    int int8Initialized=0;
    int int16Initialized=0;
    int int32Initialized=0;
    int float32Initialized=0;
    int float64Initialized=0;
    asynStandardInterfaces *pInterfaces = &pPvt->asynInterfaces;

    nPixels = pFrame->nx * pFrame->ny;
    
    /* The following code can be exected without the mutex because the only data in the driver structure
     * that we are accessing is outputBuffer, and we are the only function using that, and only in the
     * thread of the background task */

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
                ADUtils->convertImage(pFrame->pImage, pFrame->dataType, \
                                      pFrame->nx, pFrame->ny, \
                                      pPvt->outputBuffer, SIGNED_TYPE, \
                                      1, 1, 0, 0, \
                                      pFrame->nx, pFrame->ny, &nxt, &nyt); \
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

    /* Update the parameters. We need the mutex now, but it's quick. */
    epicsMutexLock(pPvt->mutexId);
    pPvt->pCurrentData = pFrame->pImage;    
    ADParam->setInteger(pPvt->params, ADImageSizeX, pFrame->nx);
    ADParam->setInteger(pPvt->params, ADImageSizeY, pFrame->ny);
    ADParam->setInteger(pPvt->params, ADDataType, pFrame->dataType);
    ADParam->getInteger(pPvt->params, ADFrameCounter, &frameCounter);
    frameCounter++;
    ADParam->setInteger(pPvt->params, ADFrameCounter, frameCounter);    
    ADParam->callCallbacks(pPvt->params);
    epicsMutexUnlock(pPvt->mutexId);
}


static void ADImageTask(drvADPvt *pPvt)
{
    /* This thread does the callbacks to the clients when a new frame arrives */

    /* Loop forever */
    ADImageFrame_t frame;
    
    while (1) {    
        epicsMessageQueueReceive(pPvt->msgQId, &frame, sizeof(frame));
        /* Call the function that does the callbacks */
        ADImageDoCallbacks(pPvt, &frame);        
    }
}
static void ADImageQueueCallback(asynUser *pasynUser)
{
    /* This is the callback routine from queueRequest when we want to enable/disable
     * interrupt callbacks from the ADImage port driver. It is called from the port thread
     * of the detector port driver. */
   
    drvADPvt *pPvt = (drvADPvt *)pasynUser->userPvt;
    int status;
    int nx, ny, dataType;
    char *functionName = "ADImageQueueCallback";
    
    epicsMutexLock(pPvt->mutexId);

    switch (pasynUser->reason) {
        case ADImgEnableCallbacks:
            status = pPvt->pasynADImage->registerInterruptUser(
                        pPvt->asynADImagePvt, pasynUser,
                        ADImageCallback, pPvt, &pPvt->asynADImageInterruptPvt);
            if (status!=asynSuccess) {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: Can't register for interrupt callbacks on detector port\n",
                    driverName, functionName);
                status = asynError;
            }
            break;
        case ADImgDisableCallbacks:
            status = pPvt->pasynADImage->cancelInterruptUser(
                        pPvt->asynADImagePvt, pasynUser,
                        pPvt->asynADImageInterruptPvt);
            if (status!=asynSuccess) {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: Can't unregister for interrupt callbacks on detector port\n",
                    driverName, functionName);
                status = asynError;
            }
            break;
        case ADImgReadParameters:
            /* Read the current image, but only request 0 bytes so no data are actually transferred */
            status = pPvt->pasynADImage->read(pPvt->asynADImagePvt, pasynUser, NULL, 0,
                                              &nx, &ny, &dataType);
            if (status!=asynSuccess) {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: reading image data\n",
                    driverName, functionName);
                status = asynError;
            } else {
                ADParam->setInteger(pPvt->params, ADImageSizeX, nx);
                ADParam->setInteger(pPvt->params, ADImageSizeY, ny);
                ADParam->setInteger(pPvt->params, ADDataType, dataType);
            }
            break;
        }
    /* This asynUser was duplicated before calling, free it */
    pasynManager->freeAsynUser(pasynUser);
    ADParam->callCallbacks(pPvt->params);
    epicsMutexUnlock(pPvt->mutexId);
}

static int connectToImagePort(drvADPvt *pPvt)
{
    asynStatus status;
    asynInterface *pasynInterface;
    int currentlyPosting;
    int isConnected;
    char imagePort[20];
    int imageAddr;
    asynUser *pasynUser;

    ADParam->getString(pPvt->params, ADImgImagePort, sizeof(imagePort), imagePort);
    ADParam->getInteger(pPvt->params, ADImgImageAddr, &imageAddr);
    status = ADParam->getInteger(pPvt->params, ADImgPostImages, &currentlyPosting);
    if (status) currentlyPosting = 0;
    status = pasynManager->isConnected(pPvt->pasynUserADImage, &isConnected);
    if (status) isConnected=0;

    /* If we are currently connected and there is a callback registered, cancel it */    
    if (isConnected && currentlyPosting) {
        /* Make a duplicate asynUser for the queue request */
        pasynUser = pasynManager->duplicateAsynUser(pPvt->pasynUserADImage, ADImageQueueCallback, 0);
        pasynUser->reason = ADImgDisableCallbacks;
        status = pasynManager->queueRequest(pasynUser, 0, 0);
        if (status!=asynSuccess) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s::connectToImagePort ERROR: Can't queue request on detector port\n",
                driverName);
            status = asynError;
        }
    }

    /* Disconnect any connected device.  Ignore error if there is no device
     * currently connected. */
    pasynManager->exceptionCallbackRemove(pPvt->pasynUserADImage);
    pasynManager->disconnect(pPvt->pasynUserADImage);

    /* Connect to the image port driver */
    status = pasynManager->connectDevice(pPvt->pasynUserADImage, imagePort, imageAddr);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::connectToPort ERROR: Can't connect to image port %s address %d\n",
                  driverName, imagePort, imageAddr);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return (status);
    }

    /* Find the asynADImage interface in that driver */
    pasynInterface = pasynManager->findInterface(pPvt->pasynUserADImage, asynADImageType, 1);
    if (!pasynInterface) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::connectToPort ERROR: Can't find asynADImage interface on image port %s address %d\n",
                  driverName, imagePort, imageAddr);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return(asynError);
    }
    pPvt->pasynADImage = pasynInterface->pinterface;
    pPvt->asynADImagePvt = pasynInterface->drvPvt;
    pasynManager->exceptionConnect(pPvt->pasynUser);

    if (currentlyPosting) {
        /* We need to register to be called with interrupts from the detector driver on 
         * the asynADImage interface.  We need to queue a request for this, because that
         * must execute in the port thread for that driver */
        /* Make a duplicate asynUser for the queue request */
        pasynUser = pasynManager->duplicateAsynUser(pPvt->pasynUserADImage, ADImageQueueCallback, 0);
        pasynUser->reason = ADImgEnableCallbacks;
        status = pasynManager->queueRequest(pasynUser, 0, 0);
        if (status!=asynSuccess) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s::connectToPort ERROR: Can't queue request on detector port\n",
                driverName);
            status = asynError;
        }
    }

    return asynSuccess;
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
    int isConnected;
    int currentlyPosting;
    asynUser *pasynUserADImage;

    epicsMutexLock(pPvt->mutexId);

    /* See if we are connected */
    status = pasynManager->isConnected(pPvt->pasynUserADImage, &isConnected);
    if (status) {isConnected=0; status=asynSuccess;}

    /* Get the current value of ADImgPost images, so we don't add more than 1 callback request */
    status = ADParam->getInteger(pPvt->params, ADImgPostImages, &currentlyPosting);
    if (status) {currentlyPosting = 0; status=asynSuccess;}

    /* Set the parameter in the parameter library. */
    status |= ADParam->setInteger(pPvt->params, function, value);

    switch(function) {
        case ADImgPostImages:
            if (value) {  
                if (isConnected && !currentlyPosting) {
                    /* We need to register to be called with interrupts from the detector driver on 
                     * the asynADImage interface.  We need to queue a request for this, because that
                     * must execute in the port thread for that driver */
                    /* Make a duplicate asynUser for the queue request */
                    pasynUserADImage = pasynManager->duplicateAsynUser(pPvt->pasynUserADImage, ADImageQueueCallback, 0);
                    pasynUserADImage->reason = ADImgEnableCallbacks;
                    status = pasynManager->queueRequest(pasynUserADImage, 0, 0);
                    if (status!=asynSuccess) {
                        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                            "%s::writeInt32 ERROR: Can't queue request on detector port\n",
                            driverName);
                        status = asynError;
                    }
                }
            } else {
                /* If we are currently connected and there is a callback registered, cancel it */    
                if (isConnected && currentlyPosting) {
                    /* Make a duplicate asynUser for the queue request */
                    pasynUserADImage = pasynManager->duplicateAsynUser(pPvt->pasynUserADImage, ADImageQueueCallback, 0);
                    pasynUserADImage->reason = ADImgDisableCallbacks;
                    status = pasynManager->queueRequest(pasynUserADImage, 0, 0);
                    if (status!=asynSuccess) {
                        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                            "%s::writeInt32 ERROR: Can't queue request on detector port\n",
                            driverName);
                        status = asynError;
                    }
                }
            }
            break;
       case ADImgImageAddr:
            connectToImagePort(pPvt);
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
    const char* functionName = "writeFloat64";

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
              "%s:%s error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
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
        case ADImgImagePort:
            connectToImagePort(pPvt);
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
    int command = pasynUser->reason; \
    asynStatus status = asynSuccess; \
    int nPixels; \
    int dataType; \
    int nx, ny, nxt, nyt; \
    \
    epicsMutexLock(pPvt->mutexId); \
    ADParam->getInteger(pPvt->params, ADImageSizeX, &nx); \
    ADParam->getInteger(pPvt->params, ADImageSizeY, &ny); \
    ADParam->getInteger(pPvt->params, ADDataType, &dataType); \
    nPixels = nx * ny; \
    if (nPixels > (int)nelements) { \
        /* We have been requested fewer pixels than we have.  \
         * Just pass the first nelements pixels.  Do this by \
         * faking the values for convertImage, which will just copy the \
         * first nelements pixels. */ \
         nx = nelements; \
         ny = 1; \
    } \
    switch(command) { \
        case ADImgImageData: \
            /* We are guaranteed to have the most recent data in our buffer.  No need to call the driver, \
             * just copy the data from our buffer */ \
            /* Convert data from its actual data type.  */ \
            if (!pPvt->pCurrentData) break; \
            status = ADUtils->convertImage(pPvt->pCurrentData, pPvt->dataType, nx, ny, \
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
    

/* asynDrvUser interface methods */
static asynStatus drvUserCreate(void *drvPvt, asynUser *pasynUser,
                                const char *drvInfo, 
                                const char **pptypeName, size_t *psize)
{
    int status;
    int param;

    /* See if this is one of the standard parameters */
    status = ADUtils->findParam(ADStandardParamString, NUM_AD_STANDARD_PARAMS,
                                drvInfo, &param);

    /* If we did not find it in that table try our driver-specific table */
    if (status) status = ADUtils->findParam(ADImageParamString, NUM_AD_IMAGE_PARAMS, 
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


/* asynCommon interface methods */

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


/* Structures with function pointers for each of the asyn interfaces */
static asynCommon ifaceCommon = {
    report,
    connect,
    disconnect
};

static asynInt32 ifaceInt32 = {
    writeInt32,
    readInt32,
    getBounds
};

static asynFloat64 ifaceFloat64 = {
    writeFloat64,
    readFloat64
};

static asynOctet ifaceOctet = {
    writeOctet,
    NULL,
    readOctet,
};

static asynInt8Array ifaceInt8Array = {
    writeInt8Array,
    readInt8Array,
};

static asynInt16Array ifaceInt16Array = {
    writeInt16Array,
    readInt16Array,
};

static asynInt32Array ifaceInt32Array = {
    writeInt32Array,
    readInt32Array,
};

static asynFloat32Array ifaceFloat32Array = {
    writeFloat32Array,
    readFloat32Array,
};

static asynFloat64Array ifaceFloat64Array = {
    writeFloat64Array,
    readFloat64Array,
};

static asynDrvUser ifaceDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};


/* Configuration routine.  Called directly, or from the iocsh function in drvADImageEpics */

int drvADImageConfigure(const char *portName, int maxFrames, const char *imagePort, int imageAddr)
{
    drvADPvt *pPvt;
    asynStatus status;
    char *functionName = "drvADImageConfigure";
    asynStandardInterfaces *pInterfaces;
    asynUser *pasynUser;

    pPvt = callocMustSucceed(1, sizeof(*pPvt), functionName);
    pPvt->portName = epicsStrDup(portName);
    pPvt->maxFrames = maxFrames;
    pPvt->nextFrame = 0;

printf("portName=%s, maxFrames=%d, imagePort=%s, imageAddr=%d\n",
portName, maxFrames, imagePort, imageAddr);
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
    
    pInterfaces->common.pinterface        = (void *)&ifaceCommon;
    pInterfaces->drvUser.pinterface       = (void *)&ifaceDrvUser;
    pInterfaces->octet.pinterface         = (void *)&ifaceOctet;
    pInterfaces->int32.pinterface         = (void *)&ifaceInt32;
    pInterfaces->float64.pinterface       = (void *)&ifaceFloat64;
    pInterfaces->int8Array.pinterface     = (void *)&ifaceInt8Array;
    pInterfaces->int16Array.pinterface    = (void *)&ifaceInt16Array;
    pInterfaces->int32Array.pinterface    = (void *)&ifaceInt32Array;
    pInterfaces->float32Array.pinterface  = (void *)&ifaceFloat32Array;
    pInterfaces->float64Array.pinterface  = (void *)&ifaceFloat64Array;

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
    
    /* Connect to our device for asynTrace */
    status = pasynManager->connectDevice(pPvt->pasynUser, portName, 0);
    if (status != asynSuccess) {
        printf("%s, connectDevice failed\n", functionName);
        return -1;
    }

    /* Create asynUser for communicating with image port */
    pasynUser = pasynManager->createAsynUser(ADImageQueueCallback, 0);
    pasynUser->userPvt = pPvt;
    pPvt->pasynUserADImage = pasynUser;

    /* Create the epicsMutex for locking access to data structures from other threads */
    pPvt->mutexId = epicsMutexCreate();
    if (!pPvt->mutexId) {
        printf("%s: epicsMutexCreate failure\n", functionName);
        return asynError;
    }

    /* Create the message queue for the input frames */
    pPvt->msgQId = epicsMessageQueueCreate(maxFrames, sizeof(ADImageFrame_t));
    if (!pPvt->msgQId) {
        printf("%s: epicsMessageQueueCreate failure\n", functionName);
        return asynError;
    }
    
    /* Initialize the parameter library */
    pPvt->params = ADParam->create(0, ADImgLastDriverParam, &pPvt->asynInterfaces);
    if (!pPvt->params) {
        printf("%s: unable to create parameter library\n", functionName);
        return asynError;
    }
    
    /* Allocate space for the frame buffers */
    pPvt->pInputFrames = (ADImageFrame_t *)calloc(maxFrames, sizeof(ADImageFrame_t));
    
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
    ADParam->setInteger(pPvt->params, ADFrameCounter, 0);
    ADParam->setString(pPvt->params, ADImgImagePort, imagePort);
    ADParam->setInteger(pPvt->params, ADImgImageAddr, imageAddr);
    
    /* Try to connect to the image port */
    status = connectToImagePort(pPvt);
    
    return asynSuccess;
}

