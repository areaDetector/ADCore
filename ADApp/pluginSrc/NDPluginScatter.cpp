/*
 * NDPluginScatter.cpp
 *
 * Do callback only to next registered client, not to all clients
 * Author: Mark Rivers
 *
 * March 2014.
 */

#include <stdlib.h>

#include <iocsh.h>

#include "NDPluginScatter.h"

#include <epicsExport.h>

static const char *driverName="NDPluginScatter";

/**
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginScatter::processCallbacks(NDArray *pArray)
{
    /*
     * This function is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
    int arrayCallbacks;

    static const char *functionName = "NDPluginScatter::processCallbacks";

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);

    getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
    if (arrayCallbacks == 1) {
        NDArray *pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 1);
        if (NULL != pArrayOut) {
            this->getAttributes(pArrayOut->pAttributeList);
            this->unlock();
            doNDArrayCallbacks(pArrayOut, NDArrayData, 0);
            this->lock();
            if (this->pArrays[0]) this->pArrays[0]->release();
            this->pArrays[0] = pArrayOut;
        }
        else {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s: Couldn't allocate output array. Further processing terminated.\n",
                driverName, functionName);
        }
    }
}

/** Called by driver to do the callbacks to the next registered client on the asynGenericPointer interface.
  * \param[in] pArray Pointer to the NDArray
  * \param[in] reason A client will be called if reason matches pasynUser->reason registered for that client.
  * \param[in] address A client will be called if address matches the address registered for that client. */
asynStatus NDPluginScatter::doNDArrayCallbacks(NDArray *pArray, int reason, int address)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    int addr;
    int numNodes;
    int i;
    //static const char *functionName = "doNDArrayCallbacks";

    pasynManager->interruptStart(this->asynStdInterfaces.genericPointerInterruptPvt, &pclientList);
    numNodes = ellCount(pclientList);
    for (i=0; i<numNodes; i++) {
        if (nextClient_ > numNodes) nextClient_ = 1;
        pnode = (interruptNode *)ellNth(pclientList, nextClient_);
        nextClient_++;
        asynGenericPointerInterrupt *pInterrupt = (asynGenericPointerInterrupt *)pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &addr);
        /* If this is not a multi-device then address is -1, change to 0 */
        if (addr == -1) addr = 0;
        if ((pInterrupt->pasynUser->reason != reason) || (address != addr)) continue;
        /* Set pasynUser->auxStatus to asynOverflow.
         * This is a flag that means return without generating an error if the queue is full.
         * We don't set this for the last node because if the last node cannot queue the array
         * then the array will be dropped */
        pInterrupt->pasynUser->auxStatus = asynOverflow;
        if (i == numNodes-1) pInterrupt->pasynUser->auxStatus = asynSuccess;
        pInterrupt->callback(pInterrupt->userPvt, pInterrupt->pasynUser, pArray);
        if (pInterrupt->pasynUser->auxStatus == asynSuccess) break;
    }
    pasynManager->interruptEnd(this->asynStdInterfaces.genericPointerInterruptPvt);
    return asynSuccess;
}

/** Constructor for NDPluginScatter; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  *
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
NDPluginScatter::NDPluginScatter(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr,
                                     int maxBuffers, size_t maxMemory,
                                     int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(static_cast<NDPluginDriverParamSet*>(this), portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize, 1),
    nextClient_(1)
{
    //static const char *functionName = "NDPluginScatter::NDPluginScatter";

    createParam(NDPluginScatterMethodString,         asynParamInt32,        &NDPluginScatterMethod);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginScatter");

    /* Try to connect to the array port */
    connectToArrayPort();

}

/** Configuration command */
extern "C" int NDScatterConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                  const char *NDArrayPort, int NDArrayAddr,
                                  int maxBuffers, size_t maxMemory,
                                  int priority, int stackSize)
{
    NDPluginScatter *pPlugin = new NDPluginScatter(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                                   maxBuffers, maxMemory, priority, stackSize);
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
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDScatterConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDScatterConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDScatterRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDScatterRegister);
}
