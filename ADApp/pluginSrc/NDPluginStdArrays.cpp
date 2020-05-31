/*
 * NDPluginStdArrays.cpp
 *
 * Asyn driver for callbacks to standard asyn array interfaces for NDArray drivers.
 * This is commonly used for EPICS waveform records.
 *
 * Author: Mark Rivers
 *
 * Created April 25, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsTime.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginStdArrays.h"

static const char *driverName="NDPluginStdArrays";

template <typename epicsType, typename interruptType>
void NDPluginStdArrays::arrayInterruptCallback(NDArray *pArray, NDArrayPool *pNDArrayPool,
                            void *interruptPvt, int *initialized, NDDataType_t signedType, bool *wasThrottled)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    int status;
    epicsType *pData=NULL;
    NDArray *pOutput=NULL;
    NDArrayInfo_t arrayInfo;
    size_t numElements = 0;
    static const char* functionName="arrayInterruptCallback";

    pasynManager->interruptStart(interruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        interruptType *pInterrupt = (interruptType *)pnode->drvPvt;
        if (pInterrupt->pasynUser->reason == NDPluginStdArraysData) {
            if (!*initialized) {
                *initialized = 1;
                pArray->getInfo(&arrayInfo);
                if (pArray->codec.empty()) {
                    status = pNDArrayPool->convert(pArray, &pOutput, signedType);
                    if (status) {
                        asynPrint(pInterrupt->pasynUser, ASYN_TRACE_ERROR,
                                  "%s::arrayInterruptCallback: error allocating array in convert()\n",
                                   driverName);
                        break;
                    }
                    pData = (epicsType *)pOutput->pData;
                    numElements = arrayInfo.nElements;
                } else {
                    // Need to handle compressed arrays differently
                    pData = (epicsType *)pArray->pData;
                    numElements = (pArray->compressedSize / arrayInfo.bytesPerElement) + 1;
                }
            }
            if (throttled(pOutput)) {
                int droppedOutputArrays;
                *wasThrottled = true;
                getIntegerParam(NDPluginDriverDroppedOutputArrays, &droppedOutputArrays);
                asynPrint(pasynUserSelf, ASYN_TRACE_WARNING,
                    "%s::%s maximum byte rate exceeded, dropped array uniqueId=%d\n",
                    driverName, functionName, pArray->uniqueId);
                droppedOutputArrays++;
                setIntegerParam(NDPluginDriverDroppedOutputArrays, droppedOutputArrays);
            } else {
                pInterrupt->pasynUser->timestamp = pArray->epicsTS;
                pInterrupt->callback(pInterrupt->userPvt,
                                     pInterrupt->pasynUser,
                                     pData, numElements);
            }
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(interruptPvt);
    if (pOutput) pOutput->release();
}

template <typename epicsType>
asynStatus NDPluginStdArrays::readArray(asynUser *pasynUser, epicsType *value, size_t nElements, size_t *nIn, NDDataType_t outputType)
{
    int command = pasynUser->reason;
    asynStatus status = asynSuccess;
    NDArray *pOutput, *myArray;
    NDArrayInfo_t arrayInfo;

    myArray = this->pArrays[0];
    if (command == NDPluginStdArraysData) {
        /* If there is valid data available we already have a copy of it.
         * No need to call driver just copy the data from our buffer */
        if (!myArray || !myArray->pData) {
            status = asynError;
            goto done;
        }
        myArray->getInfo(&arrayInfo);
        if (myArray->codec.empty()) {
            if (arrayInfo.nElements > nElements) {
                /* We have been requested fewer pixels than we have.
                 * Just pass the first nElements. */
                 arrayInfo.nElements = nElements;
            }
            status = (asynStatus)this->pNDArrayPool->convert(myArray, &pOutput, outputType);
            if (status) {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                          "%s::readArray: error allocating array in convert()\n",
                           driverName);
               goto done;
            }
            /* Copy the data */
            *nIn = arrayInfo.nElements;
            memcpy(value, pOutput->pData, *nIn*sizeof(epicsType));
            pOutput->release();
        } else {
            // This is compressed data
            size_t numCopy = sizeof(epicsType) * nElements;
            if (numCopy > myArray->compressedSize) numCopy = myArray->compressedSize;
            memcpy(value, myArray->pData, numCopy);
        }

        /* Set the timestamp */
        pasynUser->timestamp = myArray->epicsTS;
    } else {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s::readArray, unknown command %d",
                  driverName, command);
        status = asynError;
    }
    done:
    return(status);
}



/** Callback function that is called by the NDArray driver with new NDArray data.
  * It does callbacks with the array data to any registered asyn clients on any
  * of the asynXXXArray interfaces.  It converts the array data to the type required for that
  * interface.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginStdArrays::processCallbacks(NDArray *pArray)
{
    /* This function calls back any registered clients on the standard asyn array interfaces with
     * the data in our private buffer.
     * It is called with the mutex already locked.
     */

    int int8Initialized=0;
    int int16Initialized=0;
    int int32Initialized=0;
    int int64Initialized=0;
    int float32Initialized=0;
    int float64Initialized=0;
    bool wasThrottled=false;
    NDArrayInfo_t arrayInfo;
    asynStandardInterfaces *pInterfaces = this->getAsynStdInterfaces();
    /* static const char* functionName = "processCallbacks"; */

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);

    pArray->getInfo(&arrayInfo);

    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing pPvt */
    this->unlock();

    /* Pass interrupts for int8Array data*/
    arrayInterruptCallback<epicsInt8, asynInt8ArrayInterrupt>(pArray, this->pNDArrayPool,
                             pInterfaces->int8ArrayInterruptPvt,
                             &int8Initialized, NDInt8, &wasThrottled);

    /* Pass interrupts for int16Array data*/
    arrayInterruptCallback<epicsInt16,  asynInt16ArrayInterrupt>(pArray, this->pNDArrayPool,
                             pInterfaces->int16ArrayInterruptPvt,
                             &int16Initialized, NDInt16, &wasThrottled);

    /* Pass interrupts for int32Array data*/
    arrayInterruptCallback<epicsInt32, asynInt32ArrayInterrupt>(pArray, this->pNDArrayPool,
                             pInterfaces->int32ArrayInterruptPvt,
                             &int32Initialized, NDInt32, &wasThrottled);

    /* Pass interrupts for int64Array data*/
    arrayInterruptCallback<epicsInt64, asynInt64ArrayInterrupt>(pArray, this->pNDArrayPool,
                             pInterfaces->int64ArrayInterruptPvt,
                             &int64Initialized, NDInt64, &wasThrottled);

    /* Pass interrupts for float32Array data*/
    arrayInterruptCallback<epicsFloat32, asynFloat32ArrayInterrupt>(pArray, this->pNDArrayPool,
                             pInterfaces->float32ArrayInterruptPvt,
                             &float32Initialized, NDFloat32, &wasThrottled);

    /* Pass interrupts for float64Array data*/
    arrayInterruptCallback<epicsFloat64, asynFloat64ArrayInterrupt>(pArray, this->pNDArrayPool,
                             pInterfaces->float64ArrayInterruptPvt,
                             &float64Initialized, NDFloat64, &wasThrottled);

    /* We must exit with the mutex locked */
    this->lock();

    // If any of the callbacks were throttled decrement the array counter.
    // This gives the user feedback that the ArrayRate has dropped.
    // It is also important because clients like ImageJ monitor the ArrayCounter field to decide if new data is available
    // Not entirely accurate, but the best we can do.
    if (wasThrottled) {
        int arrayCounter;
        getIntegerParam(NDArrayCounter, &arrayCounter);
        arrayCounter--;
        setIntegerParam(NDArrayCounter, arrayCounter);
    }

    // Do NDArray callbacks (rarely needed for this plugin).  We need to copy the array and get the attributes
    NDPluginDriver::endProcessCallbacks(pArray, true, true);

    callParamCallbacks();
}


/** Called when asyn clients call pasynInt8Array->read().
  * Converts the last NDArray callback data to epicsInt8 (if necessary) and returns it.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readInt8Array(asynUser *pasynUser, epicsInt8 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt8>(pasynUser, value, nElements, nIn, NDInt8));
}

/** Called when asyn clients call pasynInt16Array->read().
  * Converts the last NDArray callback data to epicsInt16 (if necessary) and returns it.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readInt16Array(asynUser *pasynUser, epicsInt16 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt16>(pasynUser, value, nElements, nIn, NDInt16));
}

/** Called when asyn clients call pasynInt32Array->read().
  * Converts the last NDArray callback data to epicsInt32 (if necessary) and returns it.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
    asynStatus status;
    status = readArray<epicsInt32>(pasynUser, value, nElements, nIn, NDInt32);
    if (status != asynSuccess)
        status = NDPluginDriver::readInt32Array(pasynUser, value, nElements, nIn);
    return(status);

}

/** Called when asyn clients call pasynInt64Array->read().
  * Converts the last NDArray callback data to epicsInt64 (if necessary) and returns it.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readInt64Array(asynUser *pasynUser, epicsInt64 *value, size_t nElements, size_t *nIn)
{
    asynStatus status;
    status = readArray<epicsInt64>(pasynUser, value, nElements, nIn, NDInt64);
    if (status != asynSuccess)
        status = NDPluginDriver::readInt64Array(pasynUser, value, nElements, nIn);
    return(status);

}

/** Called when asyn clients call pasynFloat32Array->read().
  * Converts the last NDArray callback data to epicsFloat32 (if necessary) and returns it.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsFloat32>(pasynUser, value, nElements, nIn, NDFloat32));
}

/** Called when asyn clients call pasynFloat64Array->read().
  * Converts the last NDArray callback data to epicsFloat64 (if necessary) and returns it.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsFloat64>(pasynUser, value, nElements, nIn, NDFloat64));
}


/** Constructor for NDPluginStdArrays; all parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * This plugin cannot block (ASYN_CANBLOCK=0) and is not multi-device (ASYN_MULTIDEVICE=0).
  * It allocates a maximum of 2 NDArray buffers for internal use.
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
  *      allowed to allocate. Set this to 0 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to 0 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] maxThreads The maximum number of threads this driver is allowed to use. If 0 then 1 will be used.
  */
NDPluginStdArrays::NDPluginStdArrays(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
                                     int priority, int stackSize, int maxThreads)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,

                   asynInt8ArrayMask | asynInt16ArrayMask | asynInt32ArrayMask | asynInt64ArrayMask |
                   asynFloat32ArrayMask | asynFloat64ArrayMask,

                   asynInt8ArrayMask | asynInt16ArrayMask | asynInt32ArrayMask | asynInt64ArrayMask |
                   asynFloat32ArrayMask | asynFloat64ArrayMask,

                   /* asynFlags is set to 0, because this plugin cannot block and is not multi-device.
                    * It does autoconnect */
                   0, 1, priority, stackSize, maxThreads, true)
{
    //static const char *functionName = "NDPluginStdArrays";

    createParam(NDPluginStdArraysDataString, asynParamGenericPointer, &NDPluginStdArraysData);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginStdArrays");

    // Disable ArrayCallbacks.
    // This plugin currently does not do array callbacks, so make the setting reflect the behavior
    setIntegerParam(NDArrayCallbacks, 0);

    /* Try to connect to the NDArray port */
    connectToArrayPort();
}

/* Configuration routine.  Called directly, or from the iocsh function */
extern "C" int NDStdArraysConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                    const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
                                    int priority, int stackSize, int maxThreads)
{
    NDPluginStdArrays *pPlugin = new NDPluginStdArrays(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshArg initArg8 = { "stack size",iocshArgInt};
static const iocshArg initArg9 = { "# threads",iocshArgInt};
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
static const iocshFuncDef initFuncDef = {"NDStdArraysConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDStdArraysConfigure(args[0].sval, args[1].ival, args[2].ival,
                            args[3].sval, args[4].ival, args[5].ival,
                            args[6].ival, args[7].ival, args[8].ival,
                            args[9].ival);
}

extern "C" void NDStdArraysRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDStdArraysRegister);
}
