/* simDetector.cpp
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

#include "ADParamLib.h"
#include "ADUtils.h"
#include "ADInterface.h"
#include "NDArrayBuff.h"
#include "ADDriverBase.h"

#include "drvSimDetector.h"


static char *driverName = "drvSimDetector";

class simDetector : public ADDriverBase {
public:
    simDetector(const char *portName, int maxSizeX, int maxSizeY, int dataType);
                 
    /* These are the methods that we override from ADDriverBase */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                     const char **pptypeName, size_t *psize);
    void report(FILE *fp, int details);
                                        
    /* These are the methods that are new to this class */
    template <typename epicsType> int computeArray(int maxSizeX, int maxSizeY);
    int allocateBuffer();
    int computeImage();
    void simTask();

    /* Our data */
    int imagesRemaining;
    epicsEventId startEventId;
    epicsEventId stopEventId;
    NDArray_t *pRaw;
};

/* If we have any private driver parameters they begin with ADFirstDriverParam and should end
   with ADLastDriverParam, which is used for setting the size of the parameter library table */
typedef enum {
   SimGainX = ADFirstDriverParam,
   SimGainY,
   SimResetImage,
   ADLastDriverParam
} SimDetParam_t;

/* The command strings are the input to ADUtils->FindParam, which returns the corresponding parameter enum value */
static asynParamString_t SimDetParamString[] = {
    {SimGainX,      "SIM_GAINX"},  
    {SimGainY,      "SIM_GAINY"},  
    {SimResetImage, "RESET_IMAGE"}  
};

#define NUM_SIM_DET_PARAMS (sizeof(SimDetParamString)/sizeof(SimDetParamString[0]))

template <typename epicsType> int simDetector::computeArray(int maxSizeX, int maxSizeY)
{
    epicsType *pData = (epicsType *)this->pRaw->pData;
    epicsType inc;
    int addr=0;
    int status = asynSuccess;
    double scaleX=0., scaleY=0.;
    double exposureTime, gain, gainX, gainY;
    int resetImage;
    int i, j;

    status = ADParam->getDouble (this->params[addr], ADGain,        &gain);
    status = ADParam->getDouble (this->params[addr], SimGainX,      &gainX);
    status = ADParam->getDouble (this->params[addr], SimGainY,      &gainY);
    status = ADParam->getInteger(this->params[addr], SimResetImage, &resetImage);
    status = ADParam->getDouble (this->params[addr], ADAcquireTime, &exposureTime);

    /* The intensity at each pixel[i,j] is:
     * (i * gainX + j* gainY) + imageCounter * gain * exposureTime * 1000. */
    inc = (epicsType) (gain * exposureTime * 1000.);

    if (resetImage) {
        for (i=0; i<maxSizeY; i++) {
            scaleX = 0.;
            for (j=0; j<maxSizeX; j++) {
                (*pData++) = (epicsType)(scaleX + scaleY + inc);
                scaleX += gainX;
            }
            scaleY += gainY;
        }
    } else {
        for (i=0; i<maxSizeY; i++) {
            for (j=0; j<maxSizeX; j++) {
                 *pData++ += inc;
            }
        }
    }
    return(status);
}


int simDetector::allocateBuffer()
{
    int status = asynSuccess;
    NDArrayInfo_t arrayInfo;
    
    /* Make sure the raw array we have allocated is large enough. 
     * We are allowed to change its size because we have exclusive use of it */
    NDArrayBuff->getInfo(this->pRaw, &arrayInfo);
    if (arrayInfo.totalBytes > this->pRaw->dataSize) {
        free(this->pRaw->pData);
        this->pRaw->pData  = malloc(arrayInfo.totalBytes);
        this->pRaw->dataSize = arrayInfo.totalBytes;
    }
    return(status);
}

int simDetector::computeImage()
{
    int status = asynSuccess;
    int dataType;
    int addr=0;
    int binX, binY, minX, minY, sizeX, sizeY, reverseX, reverseY;
    int maxSizeX, maxSizeY;
    NDDimension_t dimsOut[2];
    NDArrayInfo_t arrayInfo;
    NDArray_t *pImage;
    const char* functionName = "computeImage";

    /* NOTE: The caller of this function must have taken the mutex */
    
    status |= ADParam->getInteger(this->params[addr], ADBinX,        &binX);
    status |= ADParam->getInteger(this->params[addr], ADBinY,        &binY);
    status |= ADParam->getInteger(this->params[addr], ADMinX,        &minX);
    status |= ADParam->getInteger(this->params[addr], ADMinY,        &minY);
    status |= ADParam->getInteger(this->params[addr], ADSizeX,       &sizeX);
    status |= ADParam->getInteger(this->params[addr], ADSizeY,       &sizeY);
    status |= ADParam->getInteger(this->params[addr], ADReverseX,    &reverseX);
    status |= ADParam->getInteger(this->params[addr], ADReverseY,    &reverseY);
    status |= ADParam->getInteger(this->params[addr], ADMaxSizeX,    &maxSizeX);
    status |= ADParam->getInteger(this->params[addr], ADMaxSizeY,    &maxSizeY);
    status |= ADParam->getInteger(this->params[addr], ADDataType,    &dataType);

    /* Make sure parameters are consistent, fix them if they are not */
    if (binX < 1) {binX = 1; status |= ADParam->setInteger(this->params[addr], ADBinX, binX);}
    if (binY < 1) {binY = 1; status |= ADParam->setInteger(this->params[addr], ADBinY, binY);}
    if (minX < 0) {minX = 0; status |= ADParam->setInteger(this->params[addr], ADMinX, minX);}
    if (minY < 0) {minY = 0; status |= ADParam->setInteger(this->params[addr], ADMinY, minY);}
    if (minX > maxSizeX-1) {minX = maxSizeX-1; status |= ADParam->setInteger(this->params[addr], ADMinX, minX);}
    if (minY > maxSizeY-1) {minY = maxSizeY-1; status |= ADParam->setInteger(this->params[addr], ADMinY, minY);}
    if (minX+sizeX > maxSizeX) {sizeX = maxSizeX-minX; status |= ADParam->setInteger(this->params[addr], ADSizeX, sizeX);}
    if (minY+sizeY > maxSizeY) {sizeY = maxSizeY-minY; status |= ADParam->setInteger(this->params[addr], ADSizeY, sizeY);}

    /* Make sure the buffer we have allocated is large enough. */
    this->pRaw->dataType = dataType;
    status |= allocateBuffer();
    
    switch (dataType) {
        case NDInt8: 
            status |= computeArray<epicsInt8>(maxSizeX, maxSizeY);
            break;
        case NDUInt8: 
            status |= computeArray<epicsUInt8>(maxSizeX, maxSizeY);
            break;
        case NDInt16: 
            status |= computeArray<epicsInt16>(maxSizeX, maxSizeY);
            break;
        case NDUInt16: 
            status |= computeArray<epicsUInt16>(maxSizeX, maxSizeY);
            break;
        case NDInt32: 
            status |= computeArray<epicsInt32>(maxSizeX, maxSizeY);
            break;
        case NDUInt32: 
            status |= computeArray<epicsUInt32>(maxSizeX, maxSizeY);
            break;
        case NDFloat32: 
            status |= computeArray<epicsFloat32>(maxSizeX, maxSizeY);
            break;
        case NDFloat64: 
            status |= computeArray<epicsFloat64>(maxSizeX, maxSizeY);
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
    /* We save the most recent image buffer so it can be used in the read() function.
     * Now release it before getting a new version. */
    if (this->pArrays[addr]) NDArrayBuff->release(this->pArrays[addr]);
    status |= NDArrayBuff->convert(this->pRaw,
                                   &this->pArrays[addr],
                                   dataType,
                                   dimsOut);
    pImage = this->pArrays[addr];
    NDArrayBuff->getInfo(pImage, &arrayInfo);
    status |= ADParam->setInteger(this->params[addr], ADImageSize,  arrayInfo.totalBytes);
    status |= ADParam->setInteger(this->params[addr], ADImageSizeX, pImage->dims[0].size);
    status |= ADParam->setInteger(this->params[addr], ADImageSizeY, pImage->dims[1].size);
    status |= ADParam->setInteger(this->params[addr], SimResetImage, 0);
    if (status) asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                    "%s:%s: ERROR, status=%d\n",
                    driverName, functionName, status);
    return(status);
}

static void simTaskC(void *drvPvt)
{
    simDetector *pPvt = (simDetector *)drvPvt;
    
    pPvt->simTask();
}

void simDetector::simTask()
{
    /* This thread computes new image data and does the callbacks to send it to higher layers */
    int status = asynSuccess;
    int dataType;
    int addr=0;
    int imageSizeX, imageSizeY, imageSize;
    int imageCounter;
    int acquire, autoSave;
    ADStatus_t acquiring;
    NDArray_t *pImage;
    double acquireTime, acquirePeriod, delay;
    epicsTimeStamp startTime, endTime;
    double elapsedTime;
    static char *functionName = "simTask";

    /* Loop forever */
    while (1) {
    
        epicsMutexLock(this->mutexId);

        /* Is acquisition active? */
        ADParam->getInteger(this->params[addr], ADAcquire, &acquire);
        
        /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquire) {
            ADParam->setInteger(this->params[addr], ADStatus, ADStatusIdle);
            ADParam->callCallbacks(this->params[addr]);
            /* Release the lock while we wait for an event that says acquire has started, then lock again */
            epicsMutexUnlock(this->mutexId);
            asynPrint(this->pasynUser, ASYN_TRACE_FLOW, 
                "%s:%s: waiting for acquire to start\n", driverName, functionName);
            status = epicsEventWait(this->startEventId);
            epicsMutexLock(this->mutexId);
        }
        
        /* We are acquiring. */
        /* Get the current time */
        epicsTimeGetCurrent(&startTime);
        
        /* Get the exposure parameters */
        ADParam->getDouble(this->params[addr], ADAcquireTime, &acquireTime);
        ADParam->getDouble(this->params[addr], ADAcquirePeriod, &acquirePeriod);
        
        acquiring = ADStatusAcquire;
        ADParam->setInteger(this->params[addr], ADStatus, acquiring);

        /* Call the callbacks to update any changes */
        ADParam->callCallbacks(this->params[addr]);

        /* Simulate being busy during the exposure time.  Use epicsEventWaitWithTimeout so that
         * manually stopping the acquisition will work */
        if (acquireTime >= epicsThreadSleepQuantum()) {
            epicsMutexUnlock(this->mutexId);
            status = epicsEventWaitWithTimeout(this->stopEventId, acquireTime);
            epicsMutexLock(this->mutexId);
        }
        
        /* Update the image */
        computeImage();
        pImage = this->pArrays[addr];
        
        epicsTimeGetCurrent(&endTime);
        elapsedTime = epicsTimeDiffInSeconds(&endTime, &startTime);

        /* Get the current parameters */
        ADParam->getInteger(this->params[addr], ADImageSizeX, &imageSizeX);
        ADParam->getInteger(this->params[addr], ADImageSizeY, &imageSizeY);
        ADParam->getInteger(this->params[addr], ADImageSize,  &imageSize);
        ADParam->getInteger(this->params[addr], ADDataType,   &dataType);
        ADParam->getInteger(this->params[addr], ADAutoSave,   &autoSave);
        ADParam->getInteger(this->params[addr], ADImageCounter, &imageCounter);
        imageCounter++;
        ADParam->setInteger(this->params[addr], ADImageCounter, imageCounter);
        
        /* Put the frame number and time stamp into the buffer */
        pImage->uniqueId = imageCounter;
        pImage->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;
        
        /* Call the NDArray callback */
        /* Must release the lock here, or we can get into a deadlock, because we can
         * block on the plugin lock, and the plugin can be calling us */
        epicsMutexUnlock(this->mutexId);
        asynPrint(this->pasynUser, ASYN_TRACE_FLOW, 
             "%s:%s: calling imageData callback\n", driverName, functionName);
        doCallbacksNDArray(pImage, NDArrayData, addr);
        epicsMutexLock(this->mutexId);

        /* See if acquisition is done */
        if (this->imagesRemaining > 0) this->imagesRemaining--;
        if (this->imagesRemaining == 0) {
            acquiring = ADStatusIdle;
            ADParam->setInteger(this->params[addr], ADAcquire, acquiring);
            asynPrint(this->pasynUser, ASYN_TRACE_FLOW, 
                  "%s:%s: acquisition completed\n", driverName, functionName);
        }
        
        /* Call the callbacks to update any changes */
        ADParam->callCallbacks(this->params[addr]);
        
        /* We are done accessing data structures, release the lock */
        epicsMutexUnlock(this->mutexId);
        
        /* If we are acquiring then sleep for the acquire period minus elapsed time. */
        if (acquiring) {
            /* We set the status to readOut to indicate we are in the period delay */
            ADParam->setInteger(this->params[addr], ADStatus, ADStatusReadout);
            ADParam->callCallbacks(this->params[addr]);
            delay = acquirePeriod - elapsedTime;
            asynPrint(this->pasynUser, ASYN_TRACE_FLOW, 
                     "%s:%s: delay=%f\n",
                      driverName, functionName, delay);            
            if (delay >= epicsThreadSleepQuantum())
                status = epicsEventWaitWithTimeout(this->stopEventId, delay);

        }
    }
}


asynStatus simDetector::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int adstatus;
    int addr=0;
    asynStatus status = asynSuccess;
    int reset=0;

    epicsMutexLock(this->mutexId);

    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = ADParam->setInteger(this->params[addr], function, value);

    /* For a real detector this is where the parameter is sent to the hardware */
    switch (function) {
    case ADAcquire:
        ADParam->getInteger(this->params[addr], ADStatus, &adstatus);
        if (value && (adstatus == ADStatusIdle)) {
            /* We need to set the number of images we expect to collect, so the image callback function
               can know when acquisition is complete.  We need to find out what mode we are in and how
               many images have been requested.  If we are in continuous mode then set the number of
               remaining images to -1. */
            int imageMode, numImages;
            status = ADParam->getInteger(this->params[addr], ADImageMode, &imageMode);
            status = ADParam->getInteger(this->params[addr], ADNumImages, &numImages);
            switch(imageMode) {
            case ADImageSingle:
                this->imagesRemaining = 1;
                break;
            case ADImageMultiple:
                this->imagesRemaining = numImages;
                break;
            case ADImageContinuous:
                this->imagesRemaining = -1;
                break;
            }
            /* Send an event to wake up the simulation task.  
             * It won't actually start generating new images until we release the lock below */
            epicsEventSignal(this->startEventId);
        } 
        if (!value && (adstatus != ADStatusIdle)) {
            /* This was a command to stop acquisition */
            /* Send the stop event */
            epicsEventSignal(this->stopEventId);
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
            this->imagesRemaining = 1;
            break;
        case ADImageMultiple: {
            int numImages;
            ADParam->getInteger(this->params[addr], ADNumImages, &numImages);
            this->imagesRemaining = numImages; }
            break;
        case ADImageContinuous:
            this->imagesRemaining = -1;
            break;
        }
        break;
    }
    
    /* Reset the image if the reset flag was set above */
    if (reset) {
        status = ADParam->setInteger(this->params[addr], SimResetImage, 1);
        /* Compute the image when parameters change.  
         * This won't post data, but will cause any parameter changes to be computed and readbacks to update. */
        computeImage();
    }
    
    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(this->params[addr]);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeInt32 error, status=%d function=%d, value=%d\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeInt32: function=%d, value=%d\n", 
              driverName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}


asynStatus simDetector::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr=0;

    epicsMutexLock(this->mutexId);

    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = ADParam->setDouble(this->params[addr], function, value);

    /* Changing any of the following parameters requires recomputing the base image */
    switch (function) {
    case ADAcquireTime:
    case ADGain:
    case SimGainX:
    case SimGainY:
        status = ADParam->setInteger(this->params[addr], SimResetImage, 1);
        /* Compute the image.  This won't post data, but will cause any readbacks to update */
        computeImage();
        break;
    }

    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(this->params[addr]);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeFloat64 error, status=%d function=%d, value=%f\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeFloat64: function=%d, value=%f\n", 
              driverName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}


/* asynDrvUser routines */
asynStatus simDetector::drvUserCreate(asynUser *pasynUser,
                                      const char *drvInfo, 
                                      const char **pptypeName, size_t *psize)
{
    asynStatus status;
    int param;
    const char *functionName = "drvUserCreate";

    /* See if this is one of our standard parameters */
    status = findParam(SimDetParamString, NUM_SIM_DET_PARAMS, 
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
                  "%s:%s: drvInfo=%s, param=%d\n", 
                  driverName, functionName, drvInfo, param);
        return(asynSuccess);
    }
    
    /* If not, then see if it is a base class parameter */
    status = ADDriverBase::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return(status);  
}
    
void simDetector::report(FILE *fp, int details)
{
    int addr=0;

    fprintf(fp, "Simulation detector %s\n", this->portName);
    if (details > 0) {
        int nx, ny, dataType;
        ADParam->getInteger(this->params[addr], ADSizeX, &nx);
        ADParam->getInteger(this->params[addr], ADSizeY, &ny);
        ADParam->getInteger(this->params[addr], ADDataType, &dataType);
        fprintf(fp, "  NX, NY:            %d  %d\n", nx, ny);
        fprintf(fp, "  Data type:         %d\n", dataType);
    }
    /* Invoke the base class method */
    ADDriverBase::report(fp, details);
}

extern "C" int simDetectorConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType)
{
    new simDetector(portName, maxSizeX, maxSizeY, dataType);
    return(asynSuccess);
}

simDetector::simDetector(const char *portName, int maxSizeX, int maxSizeY, int dataType)

    : ADDriverBase(portName, 1, ADLastDriverParam), imagesRemaining(0), pRaw(NULL)

{
    int status = asynSuccess;
    char *functionName = "simDetector";
    int addr=0;
    asynStandardInterfaces *pInterfaces = &this->asynStdInterfaces;
    int dims[2];

    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     this->pasynUser, this);
    if (status != asynSuccess) {
        printf("%s:%s ERROR: Can't register interfaces: %s.\n",
               driverName, functionName, this->pasynUser->errorMessage);
        return;
    }
    
    /* Connect to our device for asynTrace */
    status = pasynManager->connectDevice(this->pasynUser, portName, 0);
    if (status != asynSuccess) {
        printf("%s:%s, connectDevice failed\n", 
            driverName, functionName);
        return;
    }

    /* Create the epicsEvents for signaling to the simulate task when acquisition starts and stops */
    this->startEventId = epicsEventCreate(epicsEventEmpty);
    if (!this->startEventId) {
        printf("%s:%s epicsEventCreate failure for start event\n", 
            driverName, functionName);
        return;
    }
    this->stopEventId = epicsEventCreate(epicsEventEmpty);
    if (!this->stopEventId) {
        printf("%s:%s epicsEventCreate failure for stop event\n", 
            driverName, functionName);
        return;
    }
    
    /* Allocate the raw buffer we use to compute images.  Only do this once */
    dims[0] = maxSizeX;
    dims[1] = maxSizeY;
    this->pRaw = NDArrayBuff->alloc(2, dims, dataType, 0, NULL);

    /* Set some default values for parameters */
    status =  ADParam->setString (this->params[addr], ADManufacturer, "Simulated detector");
    status |= ADParam->setString (this->params[addr], ADModel, "Basic simulator");
    status |= ADParam->setInteger(this->params[addr], ADMaxSizeX, maxSizeX);
    status |= ADParam->setInteger(this->params[addr], ADMaxSizeY, maxSizeY);
    status |= ADParam->setInteger(this->params[addr], ADSizeX, maxSizeX);
    status |= ADParam->setInteger(this->params[addr], ADSizeY, maxSizeY);
    status |= ADParam->setInteger(this->params[addr], ADImageSizeX, maxSizeX);
    status |= ADParam->setInteger(this->params[addr], ADImageSizeY, maxSizeY);
    status |= ADParam->setInteger(this->params[addr], ADDataType, dataType);
    status |= ADParam->setInteger(this->params[addr], ADImageMode, ADImageContinuous);
    status |= ADParam->setDouble (this->params[addr], ADAcquireTime, .001);
    status |= ADParam->setDouble (this->params[addr], ADAcquirePeriod, .005);
    status |= ADParam->setInteger(this->params[addr], ADNumImages, 100);
    status |= ADParam->setInteger(this->params[addr], SimResetImage, 1);
    status |= ADParam->setDouble (this->params[addr], SimGainX, 1);
    status |= ADParam->setDouble (this->params[addr], SimGainY, 1);
    if (status) {
        printf("%s: unable to set camera parameters\n", functionName);
        return;
    }
    
    /* Create the thread that updates the images */
    status = (epicsThreadCreate("SimDetTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)simTaskC,
                                this) == NULL);
    if (status) {
        printf("%s:%s epicsThreadCreate failure for image task\n", 
            driverName, functionName);
        return;
    }

    /* Compute the first image */
    computeImage();
}
