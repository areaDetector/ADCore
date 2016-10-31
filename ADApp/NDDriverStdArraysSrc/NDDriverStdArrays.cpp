/* NDDriverStdArrays.cpp
 * 
 * This is a driver for converting standard EPICS arrays (waveform records)
 * into NDArrays.
 *
 * It allows any Channel Access client to inject NDArrays into an areaDetector IOC.
 * 
 * Author: David J. Vine
 * Berkeley National Lab
 *
 * Mark Rivers
 * University of Chicago
 * 
 * Created: September 28, 2016
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
#include <cantProceed.h>
#include <iocsh.h>

#include "ADDriver.h"
#include <epicsExport.h>
#include "NDDriverStdArrays.h"

static const char *driverName = "NDDriverStdArrays";

/** Constructor for NDDriverStdArrays; most parameters are passed to ADDriver::ADDriver.
  * \param[in] portName The name of the asyn port to be created
  * \param[in] maxSizeX Maximum number of elements in x dimension
  * \param[in] maxSizeY Maximum number of elements in y dimension
  * \param[in] maxSizeZ Maximum number of elements in z dimension
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver
  *            is allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            is allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set
  *            in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in
  *            asynFlags.
  **/

NDDriverStdArrays::NDDriverStdArrays(const char *portName, int maxSizeX, int maxSizeY, int maxSizeZ,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize)

    : ADDriver(portName, 1, NUM_SOFT_DETECTOR_PARAMS,
               maxBuffers, maxMemory,
               asynFloat64ArrayMask | asynDrvUserMask, 
               asynFloat64ArrayMask, 
               0, 1, /* ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=0, autoConnect=1 */
               priority, stackSize)
{

    int status = asynSuccess;
    const char *functionName = "NDDriverStdArrays";
    /* In append mode we need to store the pixel number to write to next.  */

    /* Create the epicsEvents for signaling to the acquisition task when acquisition starts and
     * stops. */
     this->startEventId = epicsEventCreate(epicsEventEmpty);
     if (!this->startEventId)
     {
         asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s: epicsEventCreate failure for start event\n",
            driverName, functionName);
         return;
     }
     this->stopEventId = epicsEventCreate(epicsEventEmpty);
     if (!this->stopEventId)
     {
         asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s: epicsEventCreate failure for stop event\n",
            driverName, functionName);
         return;
     }
     this->imageEventId = epicsEventCreate(epicsEventEmpty);
     if (!this->imageEventId)
     {
         asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s: epicsEventCreate failure for image event\n",
            driverName, functionName);
         return;
     }

    createParam(arrayModeString,                asynParamInt32,        &ADSArrayMode);
    createParam(arrayModeRBVString,             asynParamInt32,        &ADSArrayMode_RBV);
    createParam(partialArrayCallbacksString,    asynParamInt32,        &ADSPartialArrayCallbacks);
    createParam(partialArrayCallbacksRBVString, asynParamInt32,        &ADSPartialArrayCallbacks_RBV);
    createParam(numElementsString,              asynParamInt32,        &ADSNumElements);
    createParam(numElementsRBVString,           asynParamInt32,        &ADSNumElements_RBV);
    createParam(currentPixelString,             asynParamInt32,        &ADSCurrentPixel);
    createParam(currentPixelRBVString,          asynParamInt32,        &ADSCurrentPixel_RBV);
    createParam(sizeZString,                    asynParamInt32,        &ADSSizeZ);
    createParam(sizeZRBVString,                 asynParamInt32,        &ADSSizeZ_RBV);
    createParam(maxSizeZRBVString,              asynParamInt32,        &ADSMaxSizeZ_RBV);
    createParam(arrayInString,                  asynParamFloat64Array, &ADSArrayIn);

    status  = setStringParam (ADManufacturer, "Soft Detector");
    status |= setStringParam (ADModel, "Software Detector");
    status |= setIntegerParam(ADMaxSizeX, maxSizeX);
    status |= setIntegerParam(ADMaxSizeY, maxSizeY);
    status |= setIntegerParam(ADSMaxSizeZ_RBV,   maxSizeZ);
    status |= setIntegerParam(ADSizeX, 256);
    status |= setIntegerParam(ADSizeY, 256);
    status |= setIntegerParam(ADSSizeZ, 1);
    status |= setIntegerParam(ADSSizeZ_RBV, 1);
    status |= setIntegerParam(NDArraySizeX, 256);
    status |= setIntegerParam(NDArraySizeY, 256);
    status |= setIntegerParam(NDArraySize, 65536); 
    status |= setIntegerParam(ADImageMode, ADImageSingle);
    status |= setDoubleParam (ADAcquireTime, .001);
    status |= setDoubleParam (ADAcquirePeriod, .005);
    status |= setIntegerParam(ADNumImages, 100);
    status |= setIntegerParam(ADSArrayMode, 0);
    status |= setIntegerParam(ADSArrayMode_RBV, 0);
    status |= setIntegerParam(ADSPartialArrayCallbacks, 1);
    status |= setIntegerParam(ADSPartialArrayCallbacks_RBV, 1);
    status |= setIntegerParam(ADSNumElements, 1);
    status |= setIntegerParam(ADSNumElements_RBV, 1);
    status |= setIntegerParam(ADSCurrentPixel, 0);
    status |= setIntegerParam(ADSCurrentPixel_RBV, 0);


    if (status) {
	printf("%s: Unable to set camera prameters.", functionName);
	return;
    }

    /* Create the thread that updates the images */
    status = (epicsThreadCreate("NDDriverStdArraysTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)startTaskC,
                                this)==NULL);

    if (status)
    {
        printf("%s:%s: epicsThreadCreate failure for image task\n",
            driverName, functionName);
        return;
    }
}

void NDDriverStdArrays::setShutter(int open)
{
    int shutterMode;
    getIntegerParam(ADShutterMode, &shutterMode);
    if (shutterMode == ADShutterModeDetector){
        setIntegerParam(ADShutterStatus, open);
    } else {
        ADDriver::setShutter(open);
    }
}

template <typename epicsType> int NDDriverStdArrays::computeImage()
{
    epicsType *pArray=NULL, *pRed=NULL, *pBlue=NULL, *pGreen=NULL;
    int columnStep=0, rowStep=0, colorMode;
    int status = asynSuccess;
    int sizeX, sizeY, sizeZ;
    int i, j, k;
    int i0, j0;
    int arrayMode; /* Overwrite (0) or append (1) */
    int numElements; /* Number of elements to write in append mode */
    int currentPixel; /* Append beginning at this pixel */
    int pixelCount=0; /* How many pixels have been written */

    status = getIntegerParam(NDColorMode, &colorMode);
    status |= getIntegerParam(ADSizeX, &sizeX);
    status |= getIntegerParam(ADSizeY, &sizeY);
    status |= getIntegerParam(ADSSizeZ_RBV, &sizeZ);
    status |= getIntegerParam(ADSNumElements, &numElements);
    status |= getIntegerParam(ADSArrayMode, &arrayMode);
    status |= getIntegerParam(ADSCurrentPixel, &currentPixel);
    if (status) asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:computeImage: error getting parameters",
                        driverName);

    switch (colorMode)
    {
        case NDColorModeMono:
            pArray = (epicsType *)this->pArrays[0]->pData;
            break;
        case NDColorModeRGB1:
            columnStep = 3;
            rowStep = 0;
            pRed   = (epicsType *)this->pArrays[0]->pData;
            pGreen = (epicsType *)this->pArrays[0]->pData+1;
            pBlue  = (epicsType *)this->pArrays[0]->pData+2;
            break;
        case NDColorModeRGB2:
            columnStep = 1;
            rowStep = 2*sizeX;
            pRed   = (epicsType *)this->pArrays[0]->pData;
            pGreen = (epicsType *)this->pArrays[0]->pData+sizeX;
            pBlue  = (epicsType *)this->pArrays[0]->pData+2*sizeX;
            break;
        case NDColorModeRGB3:
            columnStep = 1;
            rowStep = 0;
            pRed   = (epicsType *)this->pArrays[0]->pData;
            pGreen = (epicsType *)this->pArrays[0]->pData+sizeX*sizeY;
            pBlue  = (epicsType *)this->pArrays[0]->pData+2*sizeX*sizeY;
            break;
    }
    this->pArrays[0]->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32, &colorMode);

    if (arrayMode==0){ /* Overwrite */
        switch (colorMode){
            case NDColorModeMono:
                for (i=0;i<sizeX*sizeY*sizeZ;i++){
                    *(pArray++) = (epicsType) *(pRaw++);
                }
                break;
            case NDColorModeRGB1:
            case NDColorModeRGB2:
            case NDColorModeRGB3:
                for (i=0;i<sizeX*sizeY*3;i++){
                    *(pArray++) = (epicsType) *(pRaw++);
                }
                break;
        }

    } else if (arrayMode==1) { /* Append */
        i0 = floor(currentPixel/sizeX);
        j0 = currentPixel % sizeX;
        switch (colorMode){
            case NDColorModeMono:
                pArray += i0*sizeX+j0;
                for (i=0;i<numElements;i++){
                    for (j=0;j<sizeZ;j++){
                        *(pArray+j*sizeX*sizeY) = *(pRaw+j*numElements);
                    }
                    pArray++;
                    pRaw++;
                    currentPixel++;
                    if (currentPixel>=sizeX*sizeY){ /* Written one full array */
                        continue;
                    }
                }
                break;
            case NDColorModeRGB1:
            case NDColorModeRGB2:
            case NDColorModeRGB3:
                for (i=i0;i<sizeX;i++){
                    for (j=j0;j<sizeY;j++){
                        if ((i==i0)&&(j<j0)){
                            continue;
                        }
                        k = i*(sizeX*columnStep+rowStep)+j*columnStep;
                        *(pRed  + k) = *(pRaw);
                        *(pGreen+ k) = *(pRaw+numElements);
                        *(pBlue + k) = *(pRaw+2*numElements);
                        pRaw++;
                        currentPixel++;
                        pixelCount+=1;
                        if ((currentPixel>=sizeX*sizeY)||
                            (pixelCount>=numElements)){ /* Written one full array */
                            goto end;
                        }
                    }
                }
                end:
                break;
        }
        setIntegerParam(ADSCurrentPixel, currentPixel);
        setIntegerParam(ADSCurrentPixel_RBV, currentPixel);
    }
    callParamCallbacks();

    return(status);
}

int NDDriverStdArrays::updateImage()
{
    int status = asynSuccess;
    NDDataType_t dataType;
    int itemp;
    int xDim=0, yDim=1, zDim=2, colorDim=-1;
    int arrayMode = 0; /* Overwrite or append */
    NDArrayInfo arrayInfo;
    int sizeX, sizeY, sizeZ;
    int maxSizeX, maxSizeY, maxSizeZ;
    NDColorMode_t colorMode;
    int ndims=3;
    int currentPixel;
    size_t dims[3];
    NDArray *pImage = this->pArrays[0];
    const char* functionName = "updateImage";

    /* Get parameters from ADBase */
    callParamCallbacks();
    status |= getIntegerParam(ADSizeX,      &sizeX);
    status |= getIntegerParam(ADSizeY,      &sizeY);
    status |= getIntegerParam(ADSSizeZ_RBV, &sizeZ);
    status |= getIntegerParam(ADMaxSizeX,   &maxSizeX);
    status |= getIntegerParam(ADMaxSizeY,   &maxSizeY);
    status |= getIntegerParam(ADSMaxSizeZ_RBV, &maxSizeZ);
    status |= getIntegerParam(NDDataType,   &itemp); dataType = (NDDataType_t) itemp;
    status |= getIntegerParam(NDColorMode,  &itemp); colorMode = (NDColorMode_t) itemp;
    status |= getIntegerParam(ADSArrayMode,    &arrayMode);
    status |= getIntegerParam(ADSCurrentPixel, &currentPixel);

    if (status) asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: error getting parameters",
                        driverName, functionName);

    /* Make sure parameters are consistent, fix them if they're not. */
    if (sizeX > maxSizeX) {
        sizeX = maxSizeX;
        status |= setIntegerParam(ADSizeX, sizeX);
    }
    if (sizeY > maxSizeY) {
        sizeY = maxSizeY;
        status |= setIntegerParam(ADSizeY, sizeY);
    }
    if (sizeZ > maxSizeZ) {
        sizeZ = maxSizeZ;
        status |= setIntegerParam(ADSSizeZ, sizeZ);
    }

    /* This switch is used to determine the pixel order in the array */
    switch (colorMode) {
        case NDColorModeMono:
            xDim  = 0;
            yDim  = 1;
            zDim  = 2;
            break;
        case NDColorModeRGB1:
            colorDim = 0;
            xDim     = 1;
            yDim     = 2;
            break;
        case NDColorModeRGB2:
            colorDim = 1;
            xDim     = 0;
            yDim     = 2;
            break;
        case NDColorModeRGB3:
            colorDim = 2;
            xDim     = 0;
            yDim     = 1;
            break;
    }
    if (arrayMode==0 || currentPixel==0){ /* Overwrite */
        if (this->pArrays[0]) this->pArrays[0]->release();
        /* Allocate the raw buffer we use to compute images. */
        dims[xDim] = sizeX;
        dims[yDim] = sizeY;

        if (colorMode==NDColorModeMono){
            dims[zDim] = sizeZ;
        } else {
            dims[colorDim] = 3;
        }
        this->pArrays[0] = this->pNDArrayPool->alloc(ndims, dims, dataType, 0, NULL);

        if (!this->pArrays[0]) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s:%s: error allocating raw buffer\n",
                      driverName, functionName);
            return(status);
        }
    }

    switch (dataType){
        case NDInt8:
            status |= computeImage<epicsInt8>();
            break;
        case NDUInt8:
            status |= computeImage<epicsUInt8>();
            break;
        case NDInt16:
            status |= computeImage<epicsInt16>();
            break;
        case NDUInt16:
            status |= computeImage<epicsUInt16>();
            break;
        case NDInt32:
            status |= computeImage<epicsInt32>();
            break;
        case NDUInt32:
            status |= computeImage<epicsUInt32>();
            break;
        case NDFloat32:
            status |= computeImage<epicsFloat32>();
            break;
        case NDFloat64:
            status |= computeImage<epicsFloat64>();
            break;
    }
    
    pImage = this->pArrays[0];
    pImage->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
    pImage->getInfo(&arrayInfo);
    status = asynSuccess;
    status |= setIntegerParam(NDArraySize,  (int)arrayInfo.totalBytes);
    status |= setIntegerParam(NDArraySizeX, (int)pImage->dims[xDim].size);
    status |= setIntegerParam(NDArraySizeY, (int)pImage->dims[yDim].size);
    status |= setIntegerParam(NDArraySizeZ, (int)pImage->dims[zDim].size);
    if (status) asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: error setting parameters\n",
                    driverName, functionName);
    callParamCallbacks();
    return(status);
}

static void startTaskC(void *drvPvt)
{
    NDDriverStdArrays *pPvt = (NDDriverStdArrays *)drvPvt;
    pPvt->startTask();
}

void NDDriverStdArrays::startTask()
{
    int imageStatus;
    int imageCounter;
    int numImages, numImagesCounter;
    int imageMode;
    int partialArrayCallbacks;
    int arrayMode;
    int currentPixel;
    int sizeX, sizeY;
    int arrayCallbacks;
    int acquire;
    double acquirePeriod, delay;
    epicsTimeStamp startTime, endTime;
    double elapsedTime;
    const char* functionName = "startTask";
    NDArray *pImage;

    getIntegerParam(ADSArrayMode, &arrayMode);
    getIntegerParam(ADSCurrentPixel, &currentPixel);
    getIntegerParam(ADSPartialArrayCallbacks, &partialArrayCallbacks);

    this->lock();
    /* Loop forever */
    while(1) {
        /* Is acquisition active? */
        getIntegerParam(ADAcquire, &acquire);

        /* If we are not acquiring then wait for a semaphore that is given when acquisition is
         * started. */
        if (!acquire) {
            setIntegerParam(ADStatus, ADStatusIdle);
            callParamCallbacks();
            /* Release lock while we wait for an event that says acquire has started, then lock
             * again. */
             asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: waiting for acquire to start\n", driverName, functionName);
             this->unlock();
             epicsEventWait(this->startEventId);
             this->lock();
             setStringParam(ADStatusMessage, "Acquiring Data");
             setIntegerParam(ADNumImagesCounter, 0);
        }

        /* We are acquiring. */
        /* Get the current time. */
        epicsTimeGetCurrent(&startTime);

        /* Get exposure parameters. */
        getDoubleParam(ADAcquirePeriod, &acquirePeriod);

        setIntegerParam(ADStatus, ADStatusAcquire);

        /* Open the shutter. */
        setShutter(ADShutterOpen);

        /* Call the callbacks to update any changes. */
        callParamCallbacks();

        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s: waiting for user to put waveform data\n", driverName, functionName);

        this->unlock();
        imageStatus = epicsEventWait(this->imageEventId);
        this->lock();

        /* If user clicks stop while we were waiting for an array
           need to skip to the end. */
        getIntegerParam(ADAcquire, &acquire);
        if (!acquire){
            continue;
        }

        updateImage();

        /* Close the shutter. */
        setShutter(ADShutterClosed);

        /* Call the callbacks to update any changes. */
        callParamCallbacks();
        
        setIntegerParam(ADStatus, ADStatusReadout);
        callParamCallbacks();

        getIntegerParam(ADSPartialArrayCallbacks, &partialArrayCallbacks);
        if ((partialArrayCallbacks==1) ||
            (arrayMode==0) || 
            ((arrayMode==1) && 
             (currentPixel>=sizeX*sizeY)))
        {
            pImage = this->pArrays[0];
            /* Get current parameters. */
            getIntegerParam(NDArrayCounter, &imageCounter);
            getIntegerParam(ADNumImages, &numImages);
            getIntegerParam(ADNumImagesCounter, &numImagesCounter);
            getIntegerParam(ADImageMode, &imageMode);
            getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
            imageCounter++;
            numImagesCounter++;
            setIntegerParam(NDArrayCounter, imageCounter);
            setIntegerParam(ADNumImagesCounter, numImagesCounter);

            /* put the frame number and timestamp into the current buffer. */
            pImage->uniqueId = imageCounter;
            pImage->timeStamp = startTime.secPastEpoch+startTime.nsec/1.e9;
            updateTimeStamp(&pImage->epicsTS);

            
            this->getAttributes(pImage->pAttributeList);
            if (arrayCallbacks) {
                this->unlock();
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                        "%s:%s: calling imageData callback\n", driverName, functionName);
                doCallbacksGenericPointer(pImage, NDArrayData, 0);
                this->lock();
            }
        }
        
        getIntegerParam(ADSizeX, &sizeX);
        getIntegerParam(ADSizeY, &sizeY);
        getIntegerParam(ADSArrayMode, &arrayMode);
        getIntegerParam(ADSCurrentPixel_RBV, &currentPixel);
        /* Check if acquisition is done. */
        if ((imageMode==ADImageSingle) ||
            ((imageMode==ADImageMultiple) &&
            (numImagesCounter>=numImages))) 
        {
            /* Check if we are appending values to an array. If so
               we want to collect one whole array before acquisition is completed. */
            if ((arrayMode==0) || 
                ((arrayMode==1) && 
                (currentPixel>=sizeX*sizeY)))
            {
                setIntegerParam(ADAcquire, 0);
                setIntegerParam(ADStatus, ADStatusIdle);
                setIntegerParam(ADSCurrentPixel, 0);
                setIntegerParam(ADSCurrentPixel_RBV, 0);
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: acquisition completes\n", driverName, functionName);
            }
        }

        callParamCallbacks();
        getIntegerParam(ADAcquire, &acquire);

        if (acquire)
        {
            epicsTimeGetCurrent(&endTime);
            elapsedTime = epicsTimeDiffInSeconds(&endTime, &startTime);
            delay = acquirePeriod - elapsedTime;
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                     "%s:%s: delay=%f\n",
                     driverName, functionName, delay);
            if (delay>=0.0)
            {
                /* Set the status to readout to indicate we are in delay period  */
                setIntegerParam(ADStatus, ADStatusWaiting);
                callParamCallbacks();
                this->unlock();
                epicsEventWaitWithTimeout(this->stopEventId, delay);
                this->lock();
            }
        }

    }
}

asynStatus NDDriverStdArrays::writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements)
{
    int adstatus;
    asynStatus status = asynSuccess;
    status = getIntegerParam(ADStatus, &adstatus);

    if (!adstatus){
         asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:writeFloat64Array: Ignore input array while not acquiring.\n", driverName);
    } else {
        pRaw = value;
        epicsEventSignal(this->imageEventId);
    }
    return status;
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including ADAcquire.
  * \param[in] pasynUser pasynUser structure that encodes reason and address.
  * \param[in] value Value to write
  */
asynStatus NDDriverStdArrays::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int adstatus;
    int maxSizeZ;
    int sizeZ;
    asynStatus status = asynSuccess;

    status = setIntegerParam(function, value);

    if (function==ADAcquire)
    {
        getIntegerParam(ADStatus, &adstatus);
        if (value && (adstatus==ADStatusIdle))
        {
            /* Wake up acquisition task. */
            epicsEventSignal(this->startEventId);
        }
        if (!value && (adstatus != ADStatusIdle))
        {
            if (adstatus == ADStatusAcquire)
            {
                epicsEventSignal(this->imageEventId);
            } else if (adstatus == ADStatusWaiting) {
                epicsEventSignal(this->stopEventId);
            }
        }
    } else if (function==ADSArrayMode) {
        setIntegerParam(ADSArrayMode_RBV, value);
    } else if (function==ADSPartialArrayCallbacks) {
        setIntegerParam(ADSPartialArrayCallbacks_RBV, value);   
    } else if (function==ADSSizeZ){
        getIntegerParam(ADSMaxSizeZ_RBV, &maxSizeZ);
        if ((value>=1)&&(value<=maxSizeZ)){
            setIntegerParam(ADSSizeZ_RBV, value);
        } else {
            getIntegerParam(ADSSizeZ_RBV, &sizeZ); 
            setIntegerParam(ADSSizeZ, sizeZ);
        }
    } else if (function==ADSCurrentPixel){
        setIntegerParam(ADSCurrentPixel_RBV, value);
    } else {
        if (function < FIRST_SOFT_DETECTOR_PARAM) status = ADDriver::writeInt32(pasynUser, value);
    }

    callParamCallbacks();

    if (status)
    {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
            "%s:writeInt32 error, status=%d function=%d, value=%d\n",
            driverName, status, function, value);
    } else {
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
            "%s:writeInt32: function=%d, value=%d\n",
            driverName, function, value);
    }
    return status;
}


extern "C" int NDDriverStdArraysConfig(const char *portName, int maxBuffers, int maxMemory, int priority, int stackSize)
{
    new NDDriverStdArrays(portName,
                    (maxBuffers < 0) ? 0 : maxBuffers,
                    (maxMemory < 0) ? 0 : maxMemory,
                    priority, stackSize);
    return(asynSuccess);
}

/** Code for iocsh registration */
static const iocshArg NDDriverStdArraysConfigArg0 = {"Port name", iocshArgString};
static const iocshArg NDDriverStdArraysConfigArg1 = {"maxBuffers", iocshArgInt};
static const iocshArg NDDriverStdArraysConfigArg2 = {"maxMemory", iocshArgInt};
static const iocshArg NDDriverStdArraysConfigArg3 = {"priority", iocshArgInt};
static const iocshArg NDDriverStdArraysConfigArg4 = {"stackSize", iocshArgInt};
static const iocshArg * const NDDriverStdArraysConfigArgs[] =  {&NDDriverStdArraysConfigArg0,
                                                                &NDDriverStdArraysConfigArg1,
                                                                &NDDriverStdArraysConfigArg2,
                                                                &NDDriverStdArraysConfigArg3,
                                                                &NDDriverStdArraysConfigArg4};
static const iocshFuncDef configNDDriverStdArrays = {"NDDriverStdArraysConfig", 5, NDDriverStdArraysConfigArgs};
static void configNDDriverStdArraysCallFunc(const iocshArgBuf *args)
{
    NDDriverStdArraysConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival, args[4].ival);
}

static void NDDriverStdArraysRegister(void)
{

    iocshRegister(&configNDDriverStdArrays, configNDDriverStdArraysCallFunc);
}

extern "C" {
epicsExportRegistrar(NDDriverStdArraysRegister);
}

