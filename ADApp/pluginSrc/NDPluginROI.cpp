/*
 * NDPluginROI.cpp
 *
 * Region-of-Interest (ROI) plugin
 * Author: Mark Rivers
 *
 * Created April 23, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginROI.h"

//static const char *driverName="NDPluginROI";

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

static const char *driverName="NDPluginROI";


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Extracts the NthrDArray data into each of the ROIs that are being used.
  * Computes statistics on the ROI if NDPluginROIComputeStatistics is 1.
  * Computes the histogram of ROI values if NDPluginROIComputeHistogram is 1.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginROI::processCallbacks(NDArray *pArray)
{
    /* This function computes the ROIs.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */

    int dataType;
    int dim;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS], tempDim, *pDim;
    size_t userDims[ND_ARRAY_MAX_DIMS];
    NDArrayInfo arrayInfo, scratchInfo;
    NDArray *pScratch, *pOutput;
    NDColorMode_t colorMode;
    double *pData;
    int enableScale, enableDim[3], autoSize[3];
    size_t i;
    double scale;
    //static const char* functionName = "processCallbacks";
    
    memset(dims, 0, sizeof(NDDimension_t) * ND_ARRAY_MAX_DIMS);

    /* Get all parameters while we have the mutex */
    getIntegerParam(NDPluginROIDim0Bin,      &dims[0].binning);
    getIntegerParam(NDPluginROIDim1Bin,      &dims[1].binning);
    getIntegerParam(NDPluginROIDim2Bin,      &dims[2].binning);
    getIntegerParam(NDPluginROIDim0Reverse,  &dims[0].reverse);
    getIntegerParam(NDPluginROIDim1Reverse,  &dims[1].reverse);
    getIntegerParam(NDPluginROIDim2Reverse,  &dims[2].reverse);
    getIntegerParam(NDPluginROIDim0Enable,   &enableDim[0]);
    getIntegerParam(NDPluginROIDim1Enable,   &enableDim[1]);
    getIntegerParam(NDPluginROIDim2Enable,   &enableDim[2]);
    getIntegerParam(NDPluginROIDim0AutoSize, &autoSize[0]);
    getIntegerParam(NDPluginROIDim1AutoSize, &autoSize[1]);
    getIntegerParam(NDPluginROIDim2AutoSize, &autoSize[2]);
    getIntegerParam(NDPluginROIDataType,     &dataType);
    getIntegerParam(NDPluginROIEnableScale,  &enableScale);
    getDoubleParam(NDPluginROIScale, &scale);

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    /* We always keep the last array so read() can use it.
     * Release previous one. Reserve new one below. */
    if (this->pArrays[0]) {
        this->pArrays[0]->release();
        this->pArrays[0] = NULL;
    }
    
    /* Get information about the array */
    pArray->getInfo(&arrayInfo);
    
    userDims[0] = arrayInfo.xDim;
    userDims[1] = arrayInfo.yDim;
    userDims[2] = arrayInfo.colorDim;

    /* Make sure dimensions are valid, fix them if they are not */
    for (dim=0; dim<pArray->ndims; dim++) {
        pDim = &dims[dim];
        if (enableDim[dim]) {
            size_t newDimSize = pArray->dims[userDims[dim]].size;
            pDim->offset  = requestedOffset_[dim];
            pDim->size    = requestedSize_[dim];
            pDim->offset  = MAX(pDim->offset,  0);
            pDim->offset  = MIN(pDim->offset,  newDimSize-1);
            if (autoSize[dim]) pDim->size = newDimSize;
            pDim->size    = MAX(pDim->size,    1);
            pDim->size    = MIN(pDim->size,    newDimSize - pDim->offset);
            pDim->binning = MAX(pDim->binning, 1);
            pDim->binning = MIN(pDim->binning, (int)pDim->size);
        } else {
            pDim->offset  = 0;
            pDim->size    = pArray->dims[userDims[dim]].size;
            pDim->binning = 1;
        }
    }

    /* Update the parameters that may have changed */
    setIntegerParam(NDPluginROIDim0MaxSize, 0);
    setIntegerParam(NDPluginROIDim1MaxSize, 0);
    setIntegerParam(NDPluginROIDim2MaxSize, 0);
    if (pArray->ndims > 0) {
        pDim = &dims[0];
        setIntegerParam(NDPluginROIDim0MaxSize, (int)pArray->dims[userDims[0]].size);
        if (enableDim[0]) {
            setIntegerParam(NDPluginROIDim0Min,  (int)pDim->offset);
            setIntegerParam(NDPluginROIDim0Size, (int)pDim->size);
            setIntegerParam(NDPluginROIDim0Bin,  pDim->binning);
        }
    }
    if (pArray->ndims > 1) {
        pDim = &dims[1];
        setIntegerParam(NDPluginROIDim1MaxSize, (int)pArray->dims[userDims[1]].size);
        if (enableDim[1]) {
            setIntegerParam(NDPluginROIDim1Min,  (int)pDim->offset);
            setIntegerParam(NDPluginROIDim1Size, (int)pDim->size);
            setIntegerParam(NDPluginROIDim1Bin,  pDim->binning);
        }
    }
    if (pArray->ndims > 2) {
        pDim = &dims[2];
        setIntegerParam(NDPluginROIDim2MaxSize, (int)pArray->dims[userDims[2]].size);
        if (enableDim[2]) {
            setIntegerParam(NDPluginROIDim2Min,  (int)pDim->offset);
            setIntegerParam(NDPluginROIDim2Size, (int)pDim->size);
            setIntegerParam(NDPluginROIDim2Bin,  pDim->binning);
        }
    }

    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing memory
     * that other threads can access. */
    this->unlock();

    /* Extract this ROI from the input array.  The convert() function allocates
     * a new array and it is reserved (reference count = 1) */
    if (dataType == -1) dataType = (int)pArray->dataType;
    /* We treat the case of RGB1 data specially, so that NX and NY are the X and Y dimensions of the
     * image, not the first 2 dimensions.  This makes it much easier to switch back and forth between
     * RGB1 and mono mode when using an ROI. */
    if (arrayInfo.colorMode == NDColorModeRGB1) {
        tempDim = dims[0];
        dims[0] = dims[2];
        dims[2] = dims[1];
        dims[1] = tempDim;
    }
    else if (arrayInfo.colorMode == NDColorModeRGB2) {
        tempDim = dims[1];
        dims[1] = dims[2];
        dims[2] = tempDim;
    }
    
    if (enableScale && (scale != 0) && (scale != 1)) {
        /* This is tricky.  We want to do the operation to avoid errors due to integer truncation.
         * For example, if an image with all pixels=1 is binned 3x3 with scale=9 (divide by 9), then
         * the output should also have all pixels=1. 
         * We do this by extracting the ROI and converting to double, do the scaling, then convert
         * to the desired data type. */
        this->pNDArrayPool->convert(pArray, &pScratch, NDFloat64, dims);
        pScratch->getInfo(&scratchInfo);
        pData = (double *)pScratch->pData;
        for (i=0; i<scratchInfo.nElements; i++) pData[i] = pData[i]/scale;
        this->pNDArrayPool->convert(pScratch, &this->pArrays[0], (NDDataType_t)dataType);
        pScratch->release();
    } 
    else {        
        this->pNDArrayPool->convert(pArray, &this->pArrays[0], (NDDataType_t)dataType, dims);
    }
    pOutput = this->pArrays[0];

    /* If we selected just one color from the array, then we need to change the
     * dimensions and the color mode */
    colorMode = NDColorModeMono;
    if ((pOutput->ndims == 3) && 
        (arrayInfo.colorMode == NDColorModeRGB1) && 
        (pOutput->dims[0].size == 1)) 
    {
        pOutput->ndims = 2;
        pOutput->dims[0] = pOutput->dims[1];
        pOutput->dims[1] = pOutput->dims[2];
        pOutput->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
    }
    else if ((pOutput->ndims == 3) && 
        (arrayInfo.colorMode == NDColorModeRGB2) && 
        (pOutput->dims[1].size == 1)) 
    {
        pOutput->ndims = 2;
        pOutput->dims[1] = pOutput->dims[2];
        pOutput->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
    }
    else if ((pOutput->ndims == 3) && 
        (pOutput->dims[2].size == 1)) 
    {
        pOutput->ndims = 2;
        pOutput->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
    }
    this->lock();

    /* Set the image size of the ROI image data */
    setIntegerParam(NDArraySizeX, 0);
    setIntegerParam(NDArraySizeY, 0);
    setIntegerParam(NDArraySizeZ, 0);
    if (pOutput->ndims > 0) setIntegerParam(NDArraySizeX, (int)this->pArrays[0]->dims[userDims[0]].size);
    if (pOutput->ndims > 1) setIntegerParam(NDArraySizeY, (int)this->pArrays[0]->dims[userDims[1]].size);
    if (pOutput->ndims > 2) setIntegerParam(NDArraySizeZ, (int)this->pArrays[0]->dims[userDims[2]].size);

    /* Get the attributes for this driver */
    this->getAttributes(this->pArrays[0]->pAttributeList);
    /* Call any clients who have registered for NDArray callbacks */
    this->unlock();
    doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);
    /* We must enter the loop and exit with the mutex locked */
    this->lock();
    callParamCallbacks();

}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including NDPluginDriverEnableCallbacks and
  * NDPluginDriverArrayAddr.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginROI::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char* functionName = "writeInt32";

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    if        (function == NDPluginROIDim0Min) {
        requestedOffset_[0] = value;
    } else if (function == NDPluginROIDim1Min) {
        requestedOffset_[1] = value;
    } else if (function == NDPluginROIDim2Min) {
        requestedOffset_[2] = value;
    } else if (function == NDPluginROIDim0Size) {
        requestedSize_[0] = value;
    } else if (function == NDPluginROIDim1Size) {
        requestedSize_[1] = value;
    } else if (function == NDPluginROIDim2Size) {
        requestedSize_[2] = value;
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_ROI_PARAM) 
            status = NDPluginDriver::writeInt32(pasynUser, value);
    }
    
    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    return status;
}


/** Constructor for NDPluginROI; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * ROI parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginROI::NDPluginROI(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_ROI_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    //static const char *functionName = "NDPluginROI";

    /* ROI general parameters */
    createParam(NDPluginROINameString,              asynParamOctet, &NDPluginROIName);

     /* ROI definition */
    createParam(NDPluginROIDim0MinString,           asynParamInt32, &NDPluginROIDim0Min);
    createParam(NDPluginROIDim1MinString,           asynParamInt32, &NDPluginROIDim1Min);
    createParam(NDPluginROIDim2MinString,           asynParamInt32, &NDPluginROIDim2Min);
    createParam(NDPluginROIDim0SizeString,          asynParamInt32, &NDPluginROIDim0Size);
    createParam(NDPluginROIDim1SizeString,          asynParamInt32, &NDPluginROIDim1Size);
    createParam(NDPluginROIDim2SizeString,          asynParamInt32, &NDPluginROIDim2Size);
    createParam(NDPluginROIDim0MaxSizeString,       asynParamInt32, &NDPluginROIDim0MaxSize);
    createParam(NDPluginROIDim1MaxSizeString,       asynParamInt32, &NDPluginROIDim1MaxSize);
    createParam(NDPluginROIDim2MaxSizeString,       asynParamInt32, &NDPluginROIDim2MaxSize);
    createParam(NDPluginROIDim0BinString,           asynParamInt32, &NDPluginROIDim0Bin);
    createParam(NDPluginROIDim1BinString,           asynParamInt32, &NDPluginROIDim1Bin);
    createParam(NDPluginROIDim2BinString,           asynParamInt32, &NDPluginROIDim2Bin);
    createParam(NDPluginROIDim0ReverseString,       asynParamInt32, &NDPluginROIDim0Reverse);
    createParam(NDPluginROIDim1ReverseString,       asynParamInt32, &NDPluginROIDim1Reverse);
    createParam(NDPluginROIDim2ReverseString,       asynParamInt32, &NDPluginROIDim2Reverse);
    createParam(NDPluginROIDim0EnableString,        asynParamInt32, &NDPluginROIDim0Enable);
    createParam(NDPluginROIDim1EnableString,        asynParamInt32, &NDPluginROIDim1Enable);
    createParam(NDPluginROIDim2EnableString,        asynParamInt32, &NDPluginROIDim2Enable);
    createParam(NDPluginROIDim0AutoSizeString,      asynParamInt32, &NDPluginROIDim0AutoSize);
    createParam(NDPluginROIDim1AutoSizeString,      asynParamInt32, &NDPluginROIDim1AutoSize);
    createParam(NDPluginROIDim2AutoSizeString,      asynParamInt32, &NDPluginROIDim2AutoSize);
    createParam(NDPluginROIDataTypeString,          asynParamInt32, &NDPluginROIDataType);
    createParam(NDPluginROIEnableScaleString,       asynParamInt32, &NDPluginROIEnableScale);
    createParam(NDPluginROIScaleString,             asynParamFloat64, &NDPluginROIScale);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginROI");

    // Enable ArrayCallbacks.  
    // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
    setIntegerParam(NDArrayCallbacks, 1);

    /* Try to connect to the array port */
    connectToArrayPort();
}

/** Configuration command */
extern "C" int NDROIConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    new NDPluginROI(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                    maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDROIConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDROIConfigure(args[0].sval, args[1].ival, args[2].ival,
                   args[3].sval, args[4].ival, args[5].ival,
                   args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDROIRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDROIRegister);
}
