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
#include <epicsExport.h>

#include "NDArray.h"
#include "NDPluginROI.h"

static const char *driverName="NDPluginROI";

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)


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

    int use;
    int dataType;
    int dim;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS], tempDim, *pDim;
    int userDims[ND_ARRAY_MAX_DIMS];
    int colorMode = NDColorModeMono;
    NDAttribute *pAttribute;
    //const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

    /* We always keep the last array so read() can use it.
     * Release previous one. Reserve new one below. */
    if (this->pArrays[0]) {
        this->pArrays[0]->release();
        this->pArrays[0] = NULL;
    }
    getIntegerParam(NDPluginROIUse, &use);
    if (!use) return;

    /* Need to fetch all of these parameters while we still have the mutex */
    getIntegerParam(NDPluginROIDataType,           &dataType);

    /* Make sure dimensions are valid, fix them if they are not */
    /* We treat the case of RGB1 data specially, so that NX and NY are the X and Y dimensions of the
     * image, not the first 2 dimensions.  This makes it much easier to switch back and forth between
     * RGB1 and mono mode when using an ROI. */
    if (colorMode == NDColorModeRGB1) {
        userDims[0] = 1;
        userDims[1] = 2;
        userDims[2] = 0;
    }
    else if (colorMode == NDColorModeRGB2) {
        userDims[0] = 0;
        userDims[1] = 2;
        userDims[2] = 1;
    }
    else {
        for (dim=0; dim<ND_ARRAY_MAX_DIMS; dim++) userDims[dim] = dim;
    }
    for (dim=0; dim<pArray->ndims; dim++) {
        pDim = &this->dims[dim];
        pDim->offset  = MAX(pDim->offset, 0);
        pDim->offset  = MIN(pDim->offset, pArray->dims[userDims[dim]].size-1);
        pDim->size    = MAX(pDim->size, 1);
        pDim->size    = MIN(pDim->size, pArray->dims[userDims[dim]].size - pDim->offset);
        pDim->binning = MAX(pDim->binning, 1);
        pDim->binning = MIN(pDim->binning, pDim->size);
    }

    /* Make a local copy of the fixed dimensions so we can release the mutex */
    memcpy(dims, this->dims, pArray->ndims*sizeof(NDDimension_t));

    /* Update the parameters that may have changed */
    pDim = &dims[0];
    setIntegerParam(NDPluginROIDim0Min,  pDim->offset);
    setIntegerParam(NDPluginROIDim0Size, pDim->size);
    setIntegerParam(NDPluginROIDim0MaxSize, pArray->dims[userDims[0]].size);
    setIntegerParam(NDPluginROIDim0Bin,  pDim->binning);
    pDim = &dims[1];
    setIntegerParam(NDPluginROIDim1Min,  pDim->offset);
    setIntegerParam(NDPluginROIDim1Size, pDim->size);
    setIntegerParam(NDPluginROIDim1MaxSize, pArray->dims[userDims[1]].size);
    setIntegerParam(NDPluginROIDim1Bin,  pDim->binning);
    pDim = &dims[2];
    setIntegerParam(NDPluginROIDim2Min,  pDim->offset);
    setIntegerParam(NDPluginROIDim2Size, pDim->size);
    setIntegerParam(NDPluginROIDim2MaxSize, pArray->dims[userDims[2]].size);
    setIntegerParam(NDPluginROIDim2Bin,  pDim->binning);

    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing elements of
     * pPvt that other threads can access. */
    this->unlock();

    /* Extract this ROI from the input array.  The convert() function allocates
     * a new array and it is reserved (reference count = 1) */
    if (dataType == -1) dataType = (int)pArray->dataType;
    /* We treat the case of RGB1 data specially, so that NX and NY are the X and Y dimensions of the
     * image, not the first 2 dimensions.  This makes it much easier to switch back and forth between
     * RGB1 and mono mode when using an ROI. */
    if (colorMode == NDColorModeRGB1) {
        tempDim = dims[0];
        dims[0] = dims[2];
        dims[2] = dims[1];
        dims[1] = tempDim;
    }
    else if (colorMode == NDColorModeRGB2) {
        tempDim = dims[1];
        dims[1] = dims[2];
        dims[2] = tempDim;
    }
    this->pNDArrayPool->convert(pArray, &this->pArrays[0], (NDDataType_t)dataType, dims);

    /* Set the image size of the ROI image data */
    setIntegerParam(NDArraySizeX, this->pArrays[0]->dims[userDims[0]].size);
    setIntegerParam(NDArraySizeY, this->pArrays[0]->dims[userDims[1]].size);
    setIntegerParam(NDArraySizeZ, this->pArrays[0]->dims[userDims[2]].size);

    /* We must enter the loop and exit with the mutex locked */
    this->lock();
    callParamCallbacks();

    /* Get the attributes for this driver */
    this->getAttributes(this->pArrays[0]->pAttributeList);
    /* Call any clients who have registered for NDArray callbacks */
    this->unlock();
    doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);
    this->lock();
    callParamCallbacks();

}


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including minimum, size, binning, etc. for each ROI.
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginROI::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char* functionName = "writeInt32";

    /* Set parameter and readback in parameter library */
    status = setIntegerParam(function, value);
    if (function == NDPluginROIDim0Min)
            this->dims[0].offset = value;
    else if (function == NDPluginROIDim0Size)
            this->dims[0].size = value;
    else if (function == NDPluginROIDim0Bin)
            this->dims[0].binning = value;
    else if (function == NDPluginROIDim0Reverse)
            this->dims[0].reverse = value;
    else if (function == NDPluginROIDim1Min)
            this->dims[1].offset = value;
    else if (function == NDPluginROIDim1Size)
            this->dims[1].size = value;
    else if (function == NDPluginROIDim1Bin)
            this->dims[1].binning = value;
    else if (function == NDPluginROIDim1Reverse)
            this->dims[1].reverse = value;
    else if (function == NDPluginROIDim2Min)
            this->dims[2].offset = value;
    else if (function == NDPluginROIDim2Size)
            this->dims[2].size = value;
    else if (function == NDPluginROIDim2Bin)
            this->dims[2].binning = value;
    else if (function == NDPluginROIDim2Reverse)
            this->dims[2].reverse = value;
    else {
        /* This was not a parameter that this driver understands, try the base class */
        status = NDPluginDriver::writeInt32(pasynUser, value);
    }
    /* Do callbacks so higher layers see any changes */
    status = callParamCallbacks();

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, value=%d",
                  driverName, functionName, status, function, value);
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
    asynStatus status;
    //const char *functionName = "NDPluginROI";

    /* ROI general parameters */
    createParam(NDPluginROINameString,              asynParamOctet, &NDPluginROIName);
    createParam(NDPluginROIUseString,               asynParamInt32, &NDPluginROIUse);

     /* ROI definition */
    createParam(NDPluginROIDim0MinString,           asynParamInt32, &NDPluginROIDim0Min);
    createParam(NDPluginROIDim0SizeString,          asynParamInt32, &NDPluginROIDim0Size);
    createParam(NDPluginROIDim0MaxSizeString,       asynParamInt32, &NDPluginROIDim0MaxSize);
    createParam(NDPluginROIDim0BinString,           asynParamInt32, &NDPluginROIDim0Bin);
    createParam(NDPluginROIDim0ReverseString,       asynParamInt32, &NDPluginROIDim0Reverse);
    createParam(NDPluginROIDim1MinString,           asynParamInt32, &NDPluginROIDim1Min);
    createParam(NDPluginROIDim1SizeString,          asynParamInt32, &NDPluginROIDim1Size);
    createParam(NDPluginROIDim1MaxSizeString,       asynParamInt32, &NDPluginROIDim1MaxSize);
    createParam(NDPluginROIDim1BinString,           asynParamInt32, &NDPluginROIDim1Bin);
    createParam(NDPluginROIDim1ReverseString,       asynParamInt32, &NDPluginROIDim1Reverse);
    createParam(NDPluginROIDim2MinString,           asynParamInt32, &NDPluginROIDim2Min);
    createParam(NDPluginROIDim2SizeString,          asynParamInt32, &NDPluginROIDim2Size);
    createParam(NDPluginROIDim2MaxSizeString,       asynParamInt32, &NDPluginROIDim2MaxSize);
    createParam(NDPluginROIDim2BinString,           asynParamInt32, &NDPluginROIDim2Bin);
    createParam(NDPluginROIDim2ReverseString,       asynParamInt32, &NDPluginROIDim2Reverse);
    createParam(NDPluginROIDataTypeString,          asynParamInt32, &NDPluginROIDataType);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginROI");

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDROIConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginROI *pPlugin =
        new NDPluginROI(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                        maxBuffers, maxMemory, priority, stackSize);
    pPlugin = NULL;  /* This is just to eliminate compiler warning about unused variables/objects */
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
