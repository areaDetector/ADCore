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
#include "NDArrayBuff.h"
#include "ADParamLib.h"
#include "ADUtils.h"
#include "asynHandle.h"
#include "drvADImage.h"

#define driverName "drvADImage"


typedef enum
{
    ADImgImagePort=ADFirstDriverParam, /* (asynOctet,    r/w) The port for the ADImage interface */
    ADImgImageAddr,           /* (asynInt32,    r/w) The address on the port */
    ADImgUpdateTime,          /* (asynFloat64,  r/w) Minimum time between image updates */
    ADImgDroppedImages,       /* (asynInt32,    r/w) Number of dropped images */
    ADImgPostImages,          /* (asynInt32,    r/w) Post images (1=Yes, 0=No) */
    ADImgBlockingCallbacks,   /* (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
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
    {ADImgDroppedImages,"DROPPED_IMAGES" },
    {ADImgPostImages,  "POST_IMAGES"  },
    {ADImgBlockingCallbacks,  "BLOCKING_CALLBACKS"  },
    {ADImgImageData,   "IMAGE_DATA"   }
};

#define NUM_AD_IMAGE_PARAMS (sizeof(ADImageParamString)/sizeof(ADImageParamString[0]))

typedef struct drvADPvt {
    /* These fields will be needed by most asyn plug-in drivers */
    char *portName;
    epicsMutexId mutexId;
    epicsMessageQueueId msgQId;
    PARAMS params;
    NDArray_t *pImage;
    
    /* The asyn interfaces this driver implements */
    asynStandardInterfaces asynStdInterfaces;
    
    /* The asyn interfaces we access as a client */
    asynHandle *pasynHandle;
    void *asynHandlePvt;
    void *asynHandleInterruptPvt;
    asynUser *pasynUserHandle;
    
    /* asynUser connected to ourselves for asynTrace */
    asynUser *pasynUser;
    
    /* These fields are specific to the ADImage driver */
    epicsTimeStamp lastImagePostTime;
} drvADPvt;


/* Local functions, not in any interface */


static void ADImageDoCallbacks(drvADPvt *pPvt, NDArray_t *pImage)
{
    /* This function calls back any registered clients on the standard asyn array interfaces with 
     * the data in our private buffer.
     * It is called with the mutex already locked.
     */
     
    ELLLIST *pclientList;
    int nPixels;
    interruptNode *pnode;
    int imageCounter;
    int int8Initialized=0;
    int int16Initialized=0;
    int int32Initialized=0;
    int float32Initialized=0;
    int float64Initialized=0;
    int i;
    asynStandardInterfaces *pInterfaces = &pPvt->asynStdInterfaces;
    /* const char* functionName = "ADImageDoCallbacks"; */

    for (i=0, nPixels=1; i<pImage->ndims; i++) nPixels = nPixels * pImage->dims[i].size;
    
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing pPvt */
    epicsMutexUnlock(pPvt->mutexId);

    /* This macro saves a lot of code, since there are 5 types of array interrupts that we support */
    #define ARRAY_INTERRUPT_CALLBACK(INTERRUPT_PVT, INTERRUPT_TYPE, INITIALIZED, \
                                     SIGNED_TYPE, UNSIGNED_TYPE, EPICS_TYPE) { \
        EPICS_TYPE *pData=NULL;\
        NDArray_t *pOutput=NULL; \
        NDDimension_t outDims[ND_ARRAY_MAX_DIMS]; \
         \
        pasynManager->interruptStart(pInterfaces->INTERRUPT_PVT, &pclientList); \
        pnode = (interruptNode *)ellFirst(pclientList); \
        while (pnode) { \
            INTERRUPT_TYPE *pInterrupt = pnode->drvPvt; \
            if (!INITIALIZED) { \
                INITIALIZED = 1; \
                for (i=0; i<pImage->ndims; i++)  {\
                    NDArrayBuff->initDimension(&outDims[i], pImage->dims[i].size); \
                } \
                NDArrayBuff->convert(pImage, &pOutput, \
                                     SIGNED_TYPE, \
                                     outDims); \
                pData = (EPICS_TYPE *)pOutput->pData; \
             } \
            pInterrupt->callback(pInterrupt->userPvt, \
                                 pInterrupt->pasynUser, \
                                 pData, nPixels); \
            pnode = (interruptNode *)ellNext(&pnode->node); \
        } \
        pasynManager->interruptEnd(pInterfaces->INTERRUPT_PVT); \
        if (pOutput) NDArrayBuff->release(pOutput); \
    }

    /* Pass interrupts for int8Array data*/
    ARRAY_INTERRUPT_CALLBACK(int8ArrayInterruptPvt, asynInt8ArrayInterrupt,
                             int8Initialized, NDInt8, NDUInt8, epicsInt8);
    
    /* Pass interrupts for int16Array data*/
    ARRAY_INTERRUPT_CALLBACK(int16ArrayInterruptPvt, asynInt16ArrayInterrupt,
                             int16Initialized, NDInt16, NDUInt16, epicsInt16);
    
    /* Pass interrupts for int32Array data*/
    ARRAY_INTERRUPT_CALLBACK(int32ArrayInterruptPvt, asynInt32ArrayInterrupt,
                             int32Initialized, NDInt32, NDUInt32, epicsInt32);
    
    /* Pass interrupts for float32Array data*/
    ARRAY_INTERRUPT_CALLBACK(float32ArrayInterruptPvt, asynFloat32ArrayInterrupt,
                             float32Initialized, NDFloat32, NDFloat32, epicsFloat32);
    
    /* Pass interrupts for float64Array data*/
    ARRAY_INTERRUPT_CALLBACK(float64ArrayInterruptPvt, asynFloat64ArrayInterrupt,
                             float64Initialized, NDFloat64, NDFloat64, epicsFloat64);

    /* We must exit with the mutex locked */
    epicsMutexLock(pPvt->mutexId);
    /* We always keep the last image so read() can use it.  Release it now */
    if (pPvt->pImage) NDArrayBuff->release(pPvt->pImage);
    pPvt->pImage = pImage;
    /* Update the parameters.  The counter should be updated after data are posted
     * because we are using that with ezca to detect new data */
    ADParam->setInteger(pPvt->params, ADImageSizeX, pImage->dims[0].size);
    ADParam->setInteger(pPvt->params, ADImageSizeY, pImage->dims[1].size);
    ADParam->setInteger(pPvt->params, ADDataType, pImage->dataType);
    ADParam->getInteger(pPvt->params, ADImageCounter, &imageCounter);    
    imageCounter++;
    ADParam->setInteger(pPvt->params, ADImageCounter, imageCounter);    
    ADParam->callCallbacks(pPvt->params);
    
}

static void ADImageCallback(void *drvPvt, asynUser *pasynUser, void *handle)
{
    /* This callback function is called from the detector driver when a new image arrives.
     * It calls back registered clients on the standard asynXXXArray interfaces.
     * It can either do the callbacks directly (if BlockingCallbacks=1) or by queueing
     * the images to be processed by a background task (if BlockingCallbacks=0).
     * In the latter case images can be dropped if the queue is full.
     */
     
    drvADPvt *pPvt = drvPvt;
    NDArray_t *pImage = handle;
    epicsTimeStamp tNow;
    double minImageUpdateTime, deltaTime;
    int status;
    int blockingCallbacks;
    int imageCounter, droppedImages;
    char *functionName = "ADImageCallback";

    epicsMutexLock(pPvt->mutexId);

    status |= ADParam->getDouble(pPvt->params, ADImgUpdateTime, &minImageUpdateTime);
    status |= ADParam->getInteger(pPvt->params, ADImageCounter, &imageCounter);
    status |= ADParam->getInteger(pPvt->params, ADImgDroppedImages, &droppedImages);
    status |= ADParam->getInteger(pPvt->params, ADImgBlockingCallbacks, &blockingCallbacks);
    
    epicsTimeGetCurrent(&tNow);
    deltaTime = epicsTimeDiffInSeconds(&tNow, &pPvt->lastImagePostTime);

    if (deltaTime > minImageUpdateTime) {  
        /* Time to post the next image */
        
        /* The callbacks can operate in 2 modes: blocking or non-blocking.
         * If blocking we call ADImageDoCallbacks directly, executing them
         * in the detector callback thread.
         * If non-blocking we put the image on the queue and it executes
         * in our background thread. */
        /* Reserve (increase reference count) on new image */
        NDArrayBuff->reserve(pImage);
        /* Update the time we last posted an image */
        epicsTimeGetCurrent(&tNow);
        memcpy(&pPvt->lastImagePostTime, &tNow, sizeof(tNow));
        if (blockingCallbacks) {
            ADImageDoCallbacks(pPvt, pImage);
        } else {
            /* Increase the reference count again on this image
             * It will be released in the background task when processing is done */
            NDArrayBuff->reserve(pImage);
            /* Try to put this image on the message queue.  If there is no room then return
             * immediately. */
            status = epicsMessageQueueTrySend(pPvt->msgQId, &pImage, sizeof(&pImage));
            if (status) {
                asynPrint(pasynUser, ASYN_TRACE_FLOW, 
                    "%s:%s message queue full, dropped image %d\n",
                    driverName, functionName, imageCounter);
                droppedImages++;
                status |= ADParam->setInteger(pPvt->params, ADImgDroppedImages, droppedImages);
                 /* This buffer needs to be released twice, because it never made it onto the queue where
                 * it would be released later */
                NDArrayBuff->release(pImage);
                NDArrayBuff->release(pImage);
            }
        }
    }
    ADParam->callCallbacks(pPvt->params);
    epicsMutexUnlock(pPvt->mutexId);
}


static void ADImageTask(drvADPvt *pPvt)
{
    /* This thread does the callbacks to the clients when a new image arrives */

    /* Loop forever */
    NDArray_t *pImage;
    
    while (1) {
        /* Wait for an image to arrive from the queue */    
        epicsMessageQueueReceive(pPvt->msgQId, &pImage, sizeof(&pImage));
        
        /* Take the lock.  The function we are calling must release the lock
         * during time-consuming operations when it does not need it. */
        epicsMutexLock(pPvt->mutexId);
        /* Call the function that does the callbacks to standard asyn interfaces. */
        ADImageDoCallbacks(pPvt, pImage);
        epicsMutexUnlock(pPvt->mutexId); 
        
        /* We are done with this image buffer */       
        NDArrayBuff->release(pImage);
    }
}

static int setImageInterrupt(drvADPvt *pPvt, int connect)
{
    int status = asynSuccess;
    const char *functionName = "setImageInterrupt";
    
    /* Lock the port.  May not be necessary to do this. */
    status = pasynManager->lockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock image port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        return(status);
    }
    if (connect) {
        status = pPvt->pasynHandle->registerInterruptUser(
                    pPvt->asynHandlePvt, pPvt->pasynUserHandle,
                    ADImageCallback, pPvt, &pPvt->asynHandleInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't register for interrupt callbacks on detector port: %s\n",
                driverName, functionName, pPvt->pasynUserHandle->errorMessage);
            return(status);
        }
    } else {
        status = pPvt->pasynHandle->cancelInterruptUser(pPvt->asynHandlePvt, 
                        pPvt->pasynUserHandle, pPvt->asynHandleInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't unregister for interrupt callbacks on detector port: %s\n",
                driverName, functionName, pPvt->pasynUserHandle->errorMessage);
            return(status);
        }
    }
    /* Unlock the port.  May not be necessary to do this. */
    status = pasynManager->unlockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock image port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        return(status);
    }
    return(asynSuccess);
}

static int connectToImagePort(drvADPvt *pPvt)
{
    asynStatus status;
    asynInterface *pasynInterface;
    int currentlyPosting;
    NDArray_t image;
    int isConnected;
    char imagePort[20];
    int imageAddr;
    const char *functionName = "connectToImagePort";

    ADParam->getString(pPvt->params, ADImgImagePort, sizeof(imagePort), imagePort);
    ADParam->getInteger(pPvt->params, ADImgImageAddr, &imageAddr);
    status = ADParam->getInteger(pPvt->params, ADImgPostImages, &currentlyPosting);
    if (status) currentlyPosting = 0;
    status = pasynManager->isConnected(pPvt->pasynUserHandle, &isConnected);
    if (status) isConnected=0;

    /* If we are currently connected and there is a callback registered, cancel it */    
    if (isConnected && currentlyPosting) {
        status = setImageInterrupt(pPvt, 0);
    }
    
    /* Disconnect the image port from our asynUser.  Ignore error if there is no device
     * currently connected. */
    pasynManager->exceptionCallbackRemove(pPvt->pasynUserHandle);
    pasynManager->disconnect(pPvt->pasynUserHandle);

    /* Connect to the image port driver */
    status = pasynManager->connectDevice(pPvt->pasynUserHandle, imagePort, imageAddr);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::%s ERROR: Can't connect to image port %s address %d: %s\n",
                  driverName, functionName, imagePort, imageAddr, pPvt->pasynUserHandle->errorMessage);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return (status);
    }

    /* Find the asynHandle interface in that driver */
    pasynInterface = pasynManager->findInterface(pPvt->pasynUserHandle, asynHandleType, 1);
    if (!pasynInterface) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::connectToPort ERROR: Can't find asynHandle interface on image port %s address %d\n",
                  driverName, imagePort, imageAddr);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return(asynError);
    }
    pPvt->pasynHandle = pasynInterface->pinterface;
    pPvt->asynHandlePvt = pasynInterface->drvPvt;
    pasynManager->exceptionConnect(pPvt->pasynUser);

    /* Read the current image parameters from the image driver */
    /* Lock the port. Defintitely necessary to do this. */
    status = pasynManager->lockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock image port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
    }
    /* Read the current image, but only request 0 bytes so no data are actually transferred */
    image.dataSize = 0;
    status = pPvt->pasynHandle->read(pPvt->asynHandlePvt,pPvt->pasynUserHandle, &image);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: reading image data:%s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        status = asynError;
    } else {
        ADParam->setInteger(pPvt->params, ADImageSizeX, image.dims[0].size);
        ADParam->setInteger(pPvt->params, ADImageSizeY, image.dims[1].size);
        ADParam->setInteger(pPvt->params, ADDataType, image.dataType);
        ADParam->callCallbacks(pPvt->params);
    }
    /* Unlock the port.  Definitely necessary to do this. */
    status = pasynManager->unlockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock image port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
    }
    
    /* If we are posting enable interrupt callbacks */
    if (currentlyPosting) {
        status = setImageInterrupt(pPvt, 1);
    }

    return(status);
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

    epicsMutexLock(pPvt->mutexId);

    /* See if we are connected */
    status = pasynManager->isConnected(pPvt->pasynUserHandle, &isConnected);
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
                     * the asynHandle interface. */
                    status |= setImageInterrupt(pPvt, 1);
                }
            } else {
                /* If we are currently connected and there is a callback registered, cancel it */    
                if (isConnected && currentlyPosting) {
                    status |= setImageInterrupt(pPvt, 0);
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
    NDArray_t *pOutput; \
    NDArrayInfo_t arrayInfo; \
    NDDimension_t outDims[ND_ARRAY_MAX_DIMS]; \
    int i; \
    int dataType; \
    \
    epicsMutexLock(pPvt->mutexId); \
    dataType = pPvt->pImage->dataType; \
    NDArrayBuff->getInfo(pPvt->pImage, &arrayInfo); \
    if (arrayInfo.nElements > (int)nelements) { \
        /* We have been requested fewer pixels than we have.  \
         * Just pass the first nelements. */ \
         arrayInfo.nElements = nelements; \
    } \
    switch(command) { \
        case ADImgImageData: \
            /* We are guaranteed to have the most recent data in our buffer.  No need to call the driver, \
             * just copy the data from our buffer */ \
            /* Convert data from its actual data type.  */ \
            if (!pPvt->pImage || !pPvt->pImage->pData) break; \
            for (i=0; i<pPvt->pImage->ndims; i++)  {\
                NDArrayBuff->initDimension(&outDims[i], pPvt->pImage->dims[i].size); \
            } \
            status = NDArrayBuff->convert(pPvt->pImage, \
                                          &pOutput, \
                                          AD_TYPE, \
                                          outDims); \
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
DEFINE_READ_ARRAY(readInt8Array, epicsInt8, NDInt8)
DEFINE_WRITE_ARRAY(writeInt8Array, epicsInt8, NDInt8)
    
/* asynInt16Array interface methods */
DEFINE_READ_ARRAY(readInt16Array, epicsInt16, NDInt16)
DEFINE_WRITE_ARRAY(writeInt16Array, epicsInt16, NDInt16)
    
/* asynInt32Array interface methods */
DEFINE_READ_ARRAY(readInt32Array, epicsInt32, NDInt32)
DEFINE_WRITE_ARRAY(writeInt32Array, epicsInt32, NDInt32)
    
/* asynFloat32Array interface methods */
DEFINE_READ_ARRAY(readFloat32Array, epicsFloat32, NDFloat32)
DEFINE_WRITE_ARRAY(writeFloat32Array, epicsFloat32, NDFloat32)
    
/* asynFloat64Array interface methods */
DEFINE_READ_ARRAY(readFloat64Array, epicsFloat64, NDFloat64)
DEFINE_WRITE_ARRAY(writeFloat64Array, epicsFloat64, NDFloat64)
    

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
    asynStandardInterfaces *pInterfaces = &pPvt->asynStdInterfaces;

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
    if (details > 5) {
        ADParam->dump(pPvt->params);
        NDArrayBuff->report(details);
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

int drvADImageConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                        const char *imagePort, int imageAddr)
{
    drvADPvt *pPvt;
    asynStatus status;
    char *functionName = "drvADImageConfigure";
    asynStandardInterfaces *pInterfaces;
    asynUser *pasynUser;

    pPvt = callocMustSucceed(1, sizeof(*pPvt), functionName);
    pPvt->portName = epicsStrDup(portName);

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register port\n", functionName);
        return -1;
    }

    /* Create asynUser for debugging and for standardInterfacesBase */
    pPvt->pasynUser = pasynManager->createAsynUser(0, 0);

    /* Set addresses of asyn interfaces */
    pInterfaces = &pPvt->asynStdInterfaces;
    
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
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->userPvt = pPvt;
    pPvt->pasynUserHandle = pasynUser;

    /* Create the epicsMutex for locking access to data structures from other threads */
    pPvt->mutexId = epicsMutexCreate();
    if (!pPvt->mutexId) {
        printf("%s: epicsMutexCreate failure\n", functionName);
        return asynError;
    }

    /* Create the message queue for the input images */
    pPvt->msgQId = epicsMessageQueueCreate(queueSize, sizeof(NDArray_t*));
    if (!pPvt->msgQId) {
        printf("%s: epicsMessageQueueCreate failure\n", functionName);
        return asynError;
    }
    
    /* Initialize the parameter library */
    pPvt->params = ADParam->create(0, ADImgLastDriverParam, &pPvt->asynStdInterfaces);
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
    ADParam->setInteger(pPvt->params, ADImageCounter, 0);
    ADParam->setInteger(pPvt->params, ADImgDroppedImages, 0);
    ADParam->setString (pPvt->params, ADImgImagePort, imagePort);
    ADParam->setInteger(pPvt->params, ADImgImageAddr, imageAddr);
    ADParam->setInteger(pPvt->params, ADImgBlockingCallbacks, 0);
   
    /* Try to connect to the image port */
    status = connectToImagePort(pPvt);
    
    return asynSuccess;
}

