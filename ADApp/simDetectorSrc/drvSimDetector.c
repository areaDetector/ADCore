/* drvSimDetector.c
 *
 * This is a driver for a simulated area detector.
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  March 20, 2008
 *
 */
 
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <cantProceed.h>

#include <asynStandardInterfaces.h>

/* Defining this will create the static table of standard parameters in ADInterface.h */
#define DEFINE_STANDARD_PARAM_STRINGS 1
#include "ADParamLib.h"
#include "ADUtils.h"
#include "ADInterface.h"
#include "NDArrayBuff.h"

#include "drvSimDetector.h"


static char *driverName = "drvSimDetector";

/* If we have any private driver parameters they begin with ADFirstDriverParam and should end
   with ADLastDriverParam, which is used for setting the size of the parameter library table */
typedef enum {
   SimGainX = ADFirstDriverParam,
   SimGainY,
   SimResetImage,
   ADLastDriverParam
} SimDetParam_t;

/* The command strings are the input to ADUtils->FindParam, which returns the corresponding parameter enum value */
static ADParamString_t SimDetParamString[] = {
    {SimGainX,      "SIM_GAINX"},  
    {SimGainY,      "SIM_GAINY"},  
    {SimResetImage, "RESET_IMAGE"}  
};

#define NUM_SIM_DET_PARAMS (sizeof(SimDetParamString)/sizeof(SimDetParamString[0]))



typedef struct drvADPvt {
    /* The first set of items in this structure will be needed by all drivers */
    char *portName;
    epicsMutexId mutexId;              /* A mutex to lock access to data structures. */
    PARAMS params;

    /* The asyn interfaces this driver implements */
    asynStandardInterfaces asynStdInterfaces;

    /* asynUser connected to ourselves for asynTrace */
    asynUser *pasynUser;

    /* These items are specific to the Simulator driver */
    int imagesRemaining;
    epicsEventId startEventId;
    epicsEventId stopEventId;
    NDArray_t *pRaw;
    NDArray_t *pImage;
} drvADPvt;


static int simAllocateBuffer(drvADPvt *pPvt)
{
    int status = asynSuccess;
    NDArrayInfo_t arrayInfo;
    
    /* Make sure the raw array we have allocated is large enough. 
     * We are allowed to change its size because we have exclusive use of it */
    NDArrayBuff->getInfo(pPvt->pRaw, &arrayInfo);
    if (arrayInfo.totalBytes > pPvt->pRaw->dataSize) {
        free(pPvt->pRaw->pData);
        pPvt->pRaw->pData  = malloc(arrayInfo.totalBytes);
        pPvt->pRaw->dataSize = arrayInfo.totalBytes;
    }
    return(status);
}

static int simComputeImage(drvADPvt *pPvt)
{
    int status = asynSuccess;
    int dataType;
    int binX, binY, minX, minY, sizeX, sizeY, reverseX, reverseY;
    int maxSizeX, maxSizeY, resetImage;
    NDDimension_t dimsOut[2];
    NDArrayInfo_t arrayInfo;
    double exposureTime, gain, gainX, gainY, scaleX, scaleY;
    double increment;
    int i, j;
    const char* functionName = "simComputeImage";

    /* NOTE: The caller of this function must have taken the mutex */
    
    status |= ADParam->getInteger(pPvt->params, ADBinX,        &binX);
    status |= ADParam->getInteger(pPvt->params, ADBinY,        &binY);
    status |= ADParam->getInteger(pPvt->params, ADMinX,        &minX);
    status |= ADParam->getInteger(pPvt->params, ADMinY,        &minY);
    status |= ADParam->getInteger(pPvt->params, ADSizeX,       &sizeX);
    status |= ADParam->getInteger(pPvt->params, ADSizeY,       &sizeY);
    status |= ADParam->getInteger(pPvt->params, ADReverseX,    &reverseX);
    status |= ADParam->getInteger(pPvt->params, ADReverseY,    &reverseY);
    status |= ADParam->getInteger(pPvt->params, ADMaxSizeX,    &maxSizeX);
    status |= ADParam->getInteger(pPvt->params, ADMaxSizeY,    &maxSizeY);
    status |= ADParam->getInteger(pPvt->params, ADDataType,    &dataType);
    status |= ADParam->getInteger(pPvt->params, SimResetImage, &resetImage);
    status |= ADParam->getDouble (pPvt->params, ADAcquireTime, &exposureTime);
    status |= ADParam->getDouble (pPvt->params, ADGain,        &gain);
    status |= ADParam->getDouble (pPvt->params, SimGainX,      &gainX);
    status |= ADParam->getDouble (pPvt->params, SimGainY,      &gainY);

    /* Make sure parameters are consistent, fix them if they are not */
    if (binX < 0) {binX = 0; status |= ADParam->setInteger(pPvt->params, ADBinX, binX);}
    if (binY < 0) {binY = 0; status |= ADParam->setInteger(pPvt->params, ADBinY, binY);}
    if (minX < 0) {minX = 0; status |= ADParam->setInteger(pPvt->params, ADMinX, minX);}
    if (minY < 0) {minY = 0; status |= ADParam->setInteger(pPvt->params, ADMinY, minY);}
    if (minX > maxSizeX-1) {minX = maxSizeX-1; status |= ADParam->setInteger(pPvt->params, ADMinX, minX);}
    if (minY > maxSizeY-1) {minY = maxSizeY-1; status |= ADParam->setInteger(pPvt->params, ADMinY, minY);}
    if (minX+sizeX > maxSizeX) {sizeX = maxSizeX-minX; status |= ADParam->setInteger(pPvt->params, ADSizeX, sizeX);}
    if (minY+sizeY > maxSizeY) {sizeY = maxSizeY-minY; status |= ADParam->setInteger(pPvt->params, ADSizeY, sizeY);}

    /* Make sure the buffer we have allocated is large enough. */
    pPvt->pRaw->dataType = dataType;
    status |= simAllocateBuffer(pPvt);
    /* The intensity at each pixel[i,j] is:
     * (i * gainX + j* gainY) + imageCounter * gain * exposureTime * 1000. */
    increment = gain * exposureTime * 1000.;
    scaleX = 0.;
    scaleY = 0.;
    
    /* The following macro simplifies the code */

    #define COMPUTE_ARRAY(DATA_TYPE) {                      \
        DATA_TYPE *pData = (DATA_TYPE *)pPvt->pRaw->pData; \
        DATA_TYPE inc = (DATA_TYPE)increment;               \
        if (resetImage) {                                   \
            for (i=0; i<maxSizeY; i++) {                    \
                scaleX = 0.;                                \
                for (j=0; j<maxSizeX; j++) {                \
                    (*pData++) = (DATA_TYPE)(scaleX + scaleY + inc);  \
                    scaleX += gainX;                        \
                }                                           \
                scaleY += gainY;                            \
            }                                               \
        } else {                                            \
            for (i=0; i<maxSizeY; i++) {                    \
                for (j=0; j<maxSizeX; j++) {                \
                     *pData++ += inc;                       \
                }                                           \
            }                                               \
        }                                                   \
    }
        

    switch (dataType) {
        case NDInt8: 
            COMPUTE_ARRAY(epicsInt8);
            break;
        case NDUInt8: 
            COMPUTE_ARRAY(epicsUInt8);
            break;
        case NDInt16: 
            COMPUTE_ARRAY(epicsInt16);
            break;
        case NDUInt16: 
            COMPUTE_ARRAY(epicsUInt16);
            break;
        case NDInt32: 
            COMPUTE_ARRAY(epicsInt32);
            break;
        case NDUInt32: 
            COMPUTE_ARRAY(epicsUInt32);
            break;
        case NDFloat32: 
            COMPUTE_ARRAY(epicsFloat32);
            break;
        case NDFloat64: 
            COMPUTE_ARRAY(epicsFloat64);
            break;
    }
    
    /* Extract the region of interest with binning.  
     * If the entire image is being used (no ROI or binning) that's OK because
     * convertImage detects that case and is very efficient */
    NDArrayBuff->initDimension(&dimsOut[0], sizeX);
    dimsOut[0].binning = binX;
    dimsOut[0].offset = minX;
    dimsOut[0].reverse = reverseX;
    NDArrayBuff->initDimension(&dimsOut[1], sizeY);
    dimsOut[1].binning = binY;
    dimsOut[1].offset = minY;
    dimsOut[1].reverse = reverseY;
    status |= NDArrayBuff->convert(pPvt->pRaw,
                                   &pPvt->pImage,
                                   dataType,
                                   dimsOut);
    NDArrayBuff->getInfo(pPvt->pImage, &arrayInfo);
    status |= ADParam->setInteger(pPvt->params, ADImageSize,  arrayInfo.totalBytes);
    status |= ADParam->setInteger(pPvt->params, ADImageSizeX, pPvt->pImage->dims[0].size);
    status |= ADParam->setInteger(pPvt->params, ADImageSizeY, pPvt->pImage->dims[1].size);
    status |= ADParam->setInteger(pPvt->params, SimResetImage, 0);
    if (status) asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                    "%s:%s: ERROR, status=%d\n",
                    driverName, functionName, status);
    return(status);
}

static void simTask(drvADPvt *pPvt)
{
    /* This thread computes new image data and does the callbacks to send it to higher layers */
    int status = asynSuccess;
    int dataType;
    int imageSizeX, imageSizeY, imageSize;
    int imageCounter;
    int acquire, autoSave;
    ADStatus_t acquiring;
    double acquireTime, acquirePeriod, delay;
    epicsTimeStamp startTime, endTime;
    double elapsedTime;
    static char *functionName = "simTask";

    /* Loop forever */
    while (1) {
    
        epicsMutexLock(pPvt->mutexId);

        /* Is acquisition active? */
        ADParam->getInteger(pPvt->params, ADAcquire, &acquire);
        
        /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquire) {
            ADParam->setInteger(pPvt->params, ADStatus, ADStatusIdle);
            ADParam->callCallbacks(pPvt->params);
            /* Release the lock while we wait for an event that says acquire has started, then lock again */
            epicsMutexUnlock(pPvt->mutexId);
            asynPrint(pPvt->pasynUser, ASYN_TRACE_FLOW, 
                "%s:%s: waiting for acquire to start\n", driverName, functionName);
            status = epicsEventWait(pPvt->startEventId);
            epicsMutexLock(pPvt->mutexId);
        }
        
        /* We are acquiring. */
        /* Get the current time */
        epicsTimeGetCurrent(&startTime);
        
        /* Get the exposure parameters */
        ADParam->getDouble(pPvt->params, ADAcquireTime, &acquireTime);
        ADParam->getDouble(pPvt->params, ADAcquirePeriod, &acquirePeriod);
        
        acquiring = ADStatusAcquire;
        ADParam->setInteger(pPvt->params, ADStatus, acquiring);

        /* Call the callbacks to update any changes */
        ADParam->callCallbacks(pPvt->params);

        /* Simulate being busy during the exposure time.  Use epicsEventWaitWithTimeout so that
         * manually stopping the acquisition will work */
        if (acquireTime >= epicsThreadSleepQuantum()) {
            epicsMutexUnlock(pPvt->mutexId);
            status = epicsEventWaitWithTimeout(pPvt->stopEventId, acquireTime);
            epicsMutexLock(pPvt->mutexId);
        }
        
        /* We save the most recent image buffer so it can be used in the read() function.
         * Now release it.  simComputeImage will get a new one. */
        if (pPvt->pImage) NDArrayBuff->release(pPvt->pImage);

        /* Update the image */
        simComputeImage(pPvt);
        
        epicsTimeGetCurrent(&endTime);
        elapsedTime = epicsTimeDiffInSeconds(&endTime, &startTime);

        /* Get the current parameters */
        ADParam->getInteger(pPvt->params, ADImageSizeX, &imageSizeX);
        ADParam->getInteger(pPvt->params, ADImageSizeY, &imageSizeY);
        ADParam->getInteger(pPvt->params, ADImageSize,  &imageSize);
        ADParam->getInteger(pPvt->params, ADDataType,   &dataType);
        ADParam->getInteger(pPvt->params, ADAutoSave,   &autoSave);
        ADParam->getInteger(pPvt->params, ADImageCounter, &imageCounter);
        imageCounter++;
        ADParam->setInteger(pPvt->params, ADImageCounter, imageCounter);
        
        /* Put the frame number and time stamp into the buffer */
        pPvt->pImage->uniqueId = imageCounter;
        pPvt->pImage->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;
        
        /* Call the NDArray callback */
        /* Must release the lock here, or we can get into a deadlock, because we can
         * block on the plugin lock, and the plugin can be calling us */
        epicsMutexUnlock(pPvt->mutexId);
        asynPrint(pPvt->pasynUser, ASYN_TRACE_FLOW, 
             "%s:%s: calling imageData callback\n", driverName, functionName);
        ADUtils->handleCallback(pPvt->asynStdInterfaces.handleInterruptPvt, pPvt->pImage);
        epicsMutexLock(pPvt->mutexId);

        /* See if acquisition is done */
        if (pPvt->imagesRemaining > 0) pPvt->imagesRemaining--;
        if (pPvt->imagesRemaining == 0) {
            acquiring = ADStatusIdle;
            ADParam->setInteger(pPvt->params, ADAcquire, acquiring);
            asynPrint(pPvt->pasynUser, ASYN_TRACE_FLOW, 
                  "%s:%s: acquisition completed\n", driverName, functionName);
        }
        
        /* Call the callbacks to update any changes */
        ADParam->callCallbacks(pPvt->params);
        
        /* We are done accessing data structures, release the lock */
        epicsMutexUnlock(pPvt->mutexId);
        
        /* If we are acquiring then sleep for the acquire period minus elapsed time. */
        if (acquiring) {
            /* We set the status to readOut to indicate we are in the period delay */
            ADParam->setInteger(pPvt->params, ADStatus, ADStatusReadout);
            ADParam->callCallbacks(pPvt->params);
            delay = acquirePeriod - elapsedTime;
            asynPrint(pPvt->pasynUser, ASYN_TRACE_FLOW, 
                     "%s:%s: delay=%f\n",
                      driverName, functionName, delay);            
            if (delay >= epicsThreadSleepQuantum())
                status = epicsEventWaitWithTimeout(pPvt->stopEventId, delay);

        }
    }
}


/* asynInt32 interface functions */
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
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
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
    int adstatus;
    asynStatus status = asynSuccess;
    int reset=0;

    epicsMutexLock(pPvt->mutexId);

    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= ADParam->setInteger(pPvt->params, function, value);

    /* For a real detector this is where the parameter is sent to the hardware */
    switch (function) {
    case ADAcquire:
        ADParam->getInteger(pPvt->params, ADStatus, &adstatus);
        if (value && (adstatus == ADStatusIdle)) {
            /* We need to set the number of images we expect to collect, so the image callback function
               can know when acquisition is complete.  We need to find out what mode we are in and how
               many images have been requested.  If we are in continuous mode then set the number of
               remaining images to -1. */
            int imageMode, numImages;
            status |= ADParam->getInteger(pPvt->params, ADImageMode, &imageMode);
            status |= ADParam->getInteger(pPvt->params, ADNumImages, &numImages);
            switch(imageMode) {
            case ADImageSingle:
                pPvt->imagesRemaining = 1;
                break;
            case ADImageMultiple:
                pPvt->imagesRemaining = numImages;
                break;
            case ADImageContinuous:
                pPvt->imagesRemaining = -1;
                break;
            }
            /* Send an event to wake up the simulation task.  
             * It won't actually start generating new images until we release the lock below */
            epicsEventSignal(pPvt->startEventId);
        } 
        if (!value && (adstatus != ADStatusIdle)) {
            /* This was a command to stop acquisition */
            /* Send the stop event */
            epicsEventSignal(pPvt->stopEventId);
        }
        break;
    case ADBinX:
    case ADBinY:
    case ADMinX:
    case ADMinY:
    case ADSizeX:
    case ADSizeY:
    case ADDataType:
        reset = 1;
        break;
    case SimResetImage:
        if (value) reset = 1;
        break;
    case ADImageMode: 
        /* The image mode may have changed while we are acquiring, 
         * set the images remaining appropriately. */
        switch (value) {
        case ADImageSingle:
            pPvt->imagesRemaining = 1;
            break;
        case ADImageMultiple: {
            int numImages;
            ADParam->getInteger(pPvt->params, ADNumImages, &numImages);
            pPvt->imagesRemaining = numImages; }
            break;
        case ADImageContinuous:
            pPvt->imagesRemaining = -1;
            break;
        }
        break;
    }
    
    /* Reset the image if the reset flag was set above */
    if (reset) {
        status |= ADParam->setInteger(pPvt->params, SimResetImage, 1);
        /* Compute the image when parameters change.  
         * This won't post data, but will cause any parameter changes to be computed and readbacks to update. */
        simComputeImage(pPvt);
    }
    
    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(pPvt->params);
    
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
              "%s::getBounds,low=%d, high=%d\n", driverName, *low, *high);
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

    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= ADParam->setDouble(pPvt->params, function, value);

    /* Changing any of the following parameters requires recomputing the base image */
    switch (function) {
    case ADAcquireTime:
    case ADGain:
    case SimGainX:
    case SimGainY:
        status |= ADParam->setInteger(pPvt->params, SimResetImage, 1);
        /* Compute the image.  This won't post data, but will cause any readbacks to update */
        simComputeImage(pPvt);
        break;
    }

    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(pPvt->params);
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
    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= ADParam->setString(pPvt->params, function, (char *)value);
    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(pPvt->params);
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

/* asynHandle interface methods */
static asynStatus readADImage(void *drvPvt, asynUser *pasynUser, void *handle)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    NDArray_t *pImage = handle;
    NDArrayInfo_t arrayInfo;
    int dataSize=0;
    int status = asynSuccess;
    const char* functionName = "readADImage";
    
    epicsMutexLock(pPvt->mutexId);
    if (!pPvt->pImage) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:functionName error, no valid image available\n", 
              driverName, functionName);
        status = asynError;
    } else {
        pImage->ndims = pPvt->pImage->ndims;
        memcpy(pImage->dims, pPvt->pImage->dims, sizeof(pImage->dims));
        pImage->dataType = pPvt->pImage->dataType;
        NDArrayBuff->getInfo(pPvt->pImage, &arrayInfo);
        dataSize = arrayInfo.totalBytes;
        if (dataSize > pImage->dataSize) dataSize = pImage->dataSize;
        memcpy(pImage->pData, pPvt->pImage->pData, dataSize);
    }
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d pData=%p\n", 
              driverName, functionName, status, pImage->pData);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s error, maxBytes=%d, data=%p\n", 
              driverName, functionName, dataSize, pImage->pData);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

static asynStatus writeADImage(void *drvPvt, asynUser *pasynUser, void *handle)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    NDArray_t *pImage = handle;
    int status = asynSuccess;
    
    if (pPvt == NULL) return asynError;
    epicsMutexLock(pPvt->mutexId);

    /* The simDetector does not allow downloading image data */    
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
          "%s:ADSetImage not currently supported, pImage=%p\n", 
          driverName, pImage);
    status = asynError;
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


/* asynDrvUser routines */
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
    if (status) status = ADUtils->findParam(SimDetParamString, NUM_SIM_DET_PARAMS, 
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
              "%s::drvUserGetType entered",
              driverName);
    *pptypeName = NULL;
    *psize = 0;
    return(asynError);
}

static asynStatus drvUserDestroy(void *drvPvt, asynUser *pasynUser)
{
    /* Nothing to do because we did not allocate any resources */
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::drvUserDestroy, drvPvt=%p, pasynUser=%p\n",
              driverName, drvPvt, pasynUser);
    return(asynSuccess);
}


/* asynCommon interface methods */

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

static void report(void *drvPvt, FILE *fp, int details)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;

    fprintf(fp, "Simulation detector %s\n", pPvt->portName);
    if (details > 0) {
        int nx, ny, dataType;
        ADParam->getInteger(pPvt->params, ADSizeX, &nx);
        ADParam->getInteger(pPvt->params, ADSizeY, &ny);
        ADParam->getInteger(pPvt->params, ADDataType, &dataType);
        fprintf(fp, "  NX, NY:            %d  %d\n", nx, ny);
        fprintf(fp, "  Data type:         %d\n", dataType);
    }
    if (details > 5) {
        fprintf(fp, "\nParameter library contents:\n");
        ADParam->dump(pPvt->params);
        fprintf(fp, "\nImage buffer library:\n");
        NDArrayBuff->report(details);
    }
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

static asynDrvUser ifaceDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};

static asynHandle ifaceHandle = {
    writeADImage,
    readADImage
};

int simDetectorConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType)

{
    drvADPvt *pPvt;
    int status = asynSuccess;
    char *functionName = "simDetectorConfig";
    asynStandardInterfaces *pInterfaces;
    int dims[2];

    pPvt = callocMustSucceed(1, sizeof(*pPvt), functionName);
    pPvt->portName = epicsStrDup(portName);

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register port\n", functionName);
        return(asynError);
    }

    /* Create asynUser for debugging */
    pPvt->pasynUser = pasynManager->createAsynUser(0, 0);

    pInterfaces = &pPvt->asynStdInterfaces;
    
    /* Initialize interface pointers */
    pInterfaces->common.pinterface        = (void *)&ifaceCommon;
    pInterfaces->drvUser.pinterface       = (void *)&ifaceDrvUser;
    pInterfaces->octet.pinterface         = (void *)&ifaceOctet;
    pInterfaces->int32.pinterface         = (void *)&ifaceInt32;
    pInterfaces->float64.pinterface       = (void *)&ifaceFloat64;
    pInterfaces->handle.pinterface        = (void *)&ifaceHandle;

    /* Define which interfaces can generate interrupts */
    pInterfaces->octetCanInterrupt        = 1;
    pInterfaces->int32CanInterrupt        = 1;
    pInterfaces->float64CanInterrupt      = 1;
    pInterfaces->handleCanInterrupt       = 1;

    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     pPvt->pasynUser, pPvt);
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register interfaces: %s.\n",
               functionName, pPvt->pasynUser->errorMessage);
        return(asynError);
    }
    
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
    
    /* Create the epicsEvents for signaling to the simulate task when acquisition starts and stops */
    pPvt->startEventId = epicsEventCreate(epicsEventEmpty);
    if (!pPvt->startEventId) {
        printf("%s: epicsEventCreate failure for start event\n", functionName);
        return asynError;
    }
    pPvt->stopEventId = epicsEventCreate(epicsEventEmpty);
    if (!pPvt->stopEventId) {
        printf("%s: epicsEventCreate failure for stop event\n", functionName);
        return asynError;
    }
    
    /* Initialize the parameter library */
    pPvt->params = ADParam->create(0, ADLastDriverParam, &pPvt->asynStdInterfaces);
    if (!pPvt->params) {
        printf("%s: unable to create parameter library\n", functionName);
        return asynError;
    }

    /* Allocate the raw buffer we use to compute images.  Only do this once */
    dims[0] = maxSizeX;
    dims[1] = maxSizeY;
    pPvt->pRaw = NDArrayBuff->alloc(2, dims, dataType, 0, NULL);

    /* Use the utility library to set some defaults */
    status = ADUtils->setParamDefaults(pPvt->params);
    
    /* Set some default values for parameters */
    status =  ADParam->setString (pPvt->params, ADManufacturer, "Simulated detector");
    status |= ADParam->setString (pPvt->params, ADModel, "Basic simulator");
    status |= ADParam->setInteger(pPvt->params, ADMaxSizeX, maxSizeX);
    status |= ADParam->setInteger(pPvt->params, ADMaxSizeY, maxSizeY);
    status |= ADParam->setInteger(pPvt->params, ADSizeX, maxSizeX);
    status |= ADParam->setInteger(pPvt->params, ADSizeY, maxSizeY);
    status |= ADParam->setInteger(pPvt->params, ADImageSizeX, maxSizeX);
    status |= ADParam->setInteger(pPvt->params, ADImageSizeY, maxSizeY);
    status |= ADParam->setInteger(pPvt->params, ADDataType, dataType);
    status |= ADParam->setInteger(pPvt->params, ADImageMode, ADImageContinuous);
    status |= ADParam->setDouble (pPvt->params, ADAcquireTime, .001);
    status |= ADParam->setDouble (pPvt->params, ADAcquirePeriod, .005);
    status |= ADParam->setInteger(pPvt->params, ADNumImages, 100);
    status |= ADParam->setInteger(pPvt->params, SimResetImage, 1);
    status |= ADParam->setDouble (pPvt->params, SimGainX, 1);
    status |= ADParam->setDouble (pPvt->params, SimGainY, 1);
    if (status) {
        printf("%s: unable to set camera parameters\n", functionName);
        return asynError;
    }
    
    /* Create the thread that updates the images */
    status = (epicsThreadCreate("SimDetTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)simTask,
                                pPvt) == NULL);
    if (status) {
        printf("%s: epicsThreadCreate failure for image task\n", functionName);
        return asynError;
    }

    /* Compute the first image */
    simComputeImage(pPvt);
    
    return asynSuccess;
}
