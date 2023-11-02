/*
 * NDPluginCentroid.cpp
 *
 * Image statistics plugin
 * Author: Mark Rivers
 *
 * Created March 12, 2010
 */

#include <stdlib.h>
#include <math.h>

#include <iocsh.h>

#include "NDPluginCentroid.h"

#include <epicsExport.h>

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

/* Some systems do not define M_PI in math.h */
#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

static const char *driverName="NDPluginCentroid";


template <typename epicsType>
asynStatus NDPluginCentroid::doComputeCentroidT(NDArray *pArray, NDCentroid_t *pCentroid)
{
    epicsType *pData = (epicsType *)pArray->pData;
    /* Scientific output parameters */
    pCentroid->centroidThreshold = 100;
    pCentroid->centroidX = 50;
    pCentroid->centroidY = 60;
    return(asynSuccess);
}

asynStatus NDPluginCentroid::doComputeCentroid(NDArray *pArray, NDCentroid_t *pCentroid)
{
    asynStatus status;

    switch(pArray->dataType) {
        case NDInt8:
            status = doComputeCentroidT<epicsInt8>(pArray, pCentroid);
            break;
        case NDUInt8:
            status = doComputeCentroidT<epicsUInt8>(pArray, pCentroid);
            break;
        case NDInt16:
            status = doComputeCentroidT<epicsInt16>(pArray, pCentroid);
            break;
        case NDUInt16:
            status = doComputeCentroidT<epicsUInt16>(pArray, pCentroid);
            break;
        case NDInt32:
            status = doComputeCentroidT<epicsInt32>(pArray, pCentroid);
            break;
        case NDUInt32:
            status = doComputeCentroidT<epicsUInt32>(pArray, pCentroid);
            break;
        case NDInt64:
            status = doComputeCentroidT<epicsInt64>(pArray, pCentroid);
            break;
        case NDUInt64:
            status = doComputeCentroidT<epicsUInt64>(pArray, pCentroid);
            break;
        case NDFloat32:
            status = doComputeCentroidT<epicsFloat32>(pArray, pCentroid);
            break;
        case NDFloat64:
            status = doComputeCentroidT<epicsFloat64>(pArray, pCentroid);
            break;
        default:
            status = asynError;
        break;
    }
    return(status);
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Does image statistics.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginCentroid::processCallbacks(NDArray *pArray)
{
    /* This function does array statistics.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
    int dim;
    NDCentroid_t  centroid, *pCentroid=&centroid;
    int computeCentroid;
    size_t sizeX=0, sizeY=0;
    int i;
    int itemp;
    NDArrayInfo arrayInfo;
    static const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);

    pArray->getInfo(&arrayInfo);
    getIntegerParam(NDPluginCentroidCompute,    &computeCentroid);
    getDoubleParam (NDPluginCentroidThreshold,  &pCentroid->centroidThreshold);

    if (pArray->ndims > 0) sizeX = pArray->dims[0].size;
    if (pArray->ndims == 1) sizeY = 1;
    if (pArray->ndims > 1)  sizeY = pArray->dims[1].size;

    // Release the lock.  While it is released we cannot access the parameter library or class member data.
    this->unlock();


    if (computeCentroid) {
         doComputeCentroid(pArray, pCentroid);
    }

    // Take the lock again.  The time-series data need to be protected.
    this->lock();
    size_t dims= pArray->dims[0].size * pArray->dims[1].size;
    NDArray *pTimeSeriesArray = this->pNDArrayPool->alloc(1, &dims, NDFloat64, 0, NULL);
    pTimeSeriesArray->uniqueId  = pArray->uniqueId;
    pTimeSeriesArray->timeStamp = pArray->timeStamp;
    pTimeSeriesArray->epicsTS   = pArray->epicsTS;

    doCallbacksGenericPointer(pTimeSeriesArray, NDArrayData, 1);
    pTimeSeriesArray->release();


    if (computeCentroid) {
        setDoubleParam(NDPluginCentroidX,     pCentroid->centroidX);
        setDoubleParam(NDPluginCentroidY,     pCentroid->centroidY);
    }


    NDPluginDriver::endProcessCallbacks(pArray, true, true);

    callParamCallbacks();
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginCentroid::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeInt32";


    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    
    /* If this parameter belongs to a base class call its method */
    if (function < FIRST_NDPLUGIN_CENTROID_PARAM)
        status = NDPluginDriver::writeInt32(pasynUser, value);
    

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks();

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

/** Called when asyn clients call pasynFloat64->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus  NDPluginCentroid::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeFloat64";

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(function, value);

    /* If this parameter belongs to a base class call its method */
    if (function < FIRST_NDPLUGIN_CENTROID_PARAM) status = NDPluginDriver::writeFloat64(pasynUser, value);

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();
    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, value=%f\n",
              driverName, functionName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%f\n",
              driverName, functionName, function, value);
    return status;
}



/** Constructor for NDPluginCentroid; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * parameters.
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
  * \param[in] maxThreads The maximum number of threads this driver is allowed to use. If 0 then 1 will be used.
  */
NDPluginCentroid::NDPluginCentroid(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize, int maxThreads)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 2, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   0, 1, priority, stackSize, maxThreads)
{
    //static const char *functionName = "NDPluginCentroid";

    /* Centroid */
    createParam(NDPluginCentroidComputeString,   asynParamInt32,      &NDPluginCentroidCompute);
    createParam(NDPluginCentroidThresholdString, asynParamFloat64,    &NDPluginCentroidThreshold);
    createParam(NDPluginCentroidXString,         asynParamFloat64,    &NDPluginCentroidX);
    createParam(NDPluginCentroidYString,         asynParamFloat64,    &NDPluginCentroidY);
    
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginCentroid");

    /* Try to connect to the array port */
    connectToArrayPort();
}

/** Configuration command */
extern "C" int NDCentroidConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize, int maxThreads)
{
    NDPluginCentroid *pPlugin = new NDPluginCentroid(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                              maxBuffers, maxMemory, priority, stackSize, maxThreads);
    return pPlugin->start();
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
static const iocshArg initArg9 = { "maxThreads",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9};
static const iocshFuncDef initFuncDef = {"NDCentroidConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDCentroidConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival,
                     args[9].ival);
}

extern "C" void NDCentroidRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDCentroidRegister);
}
