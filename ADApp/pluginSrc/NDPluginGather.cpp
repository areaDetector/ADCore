/*
 * NDPluginGather.cpp
 *
 * A plugin that subscribes to callbacks from multiple ports, not just a single port 
 * Author: Mark Rivers
 *
 * February 27. 2017.
 */

#include <stdlib.h>

#include <epicsTypes.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginGather.h"

static const char *driverName="NDPluginGather";

/** 
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginGather::processCallbacks(NDArray *pArray)
{
    /* This function is called with the mutex already locked.  It unlocks it during long calculations when private
    * structures don't need to be protected. 
    */
    int arrayCallbacks;
    static const char *functionName = "NDPluginGather::processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
    if (arrayCallbacks == 1) {
        NDArray *pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 1);
        if (NULL != pArrayOut) {
            this->getAttributes(pArrayOut->pAttributeList);
            this->unlock();
            doCallbacksGenericPointer(pArrayOut, NDArrayData, 0);
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

/** Constructor for NDPluginGather; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
NDPluginGather::NDPluginGather(const char *portName, int queueSize, int blockingCallbacks,
                                     int maxPorts,
                                     int maxBuffers, size_t maxMemory,
                                     int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   "", 0, maxPorts, 0, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    //static const char *functionName = "NDPluginGather::NDPluginGather";

    createParam(NDPluginGatherDummyString,         asynParamInt32,        &NDPluginGatherDummy);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginGather");
}

/** Configuration command */
extern "C" int NDGatherConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                  int maxPorts,
                                  int maxBuffers, size_t maxMemory,
                                  int priority, int stackSize)
{
    NDPluginGather *pPlugin = new NDPluginGather(portName, queueSize, blockingCallbacks, maxPorts,
                                                 maxBuffers, maxMemory, priority, stackSize);
    return pPlugin->start();
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "maxPorts",iocshArgInt};
static const iocshArg initArg4 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg5 = { "maxMemory",iocshArgInt};
static const iocshArg initArg6 = { "priority",iocshArgInt};
static const iocshArg initArg7 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7};
static const iocshFuncDef initFuncDef = {"NDGatherConfigure",8,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDGatherConfigure(args[0].sval, args[1].ival, args[2].ival,
                    args[3].ival, args[4].ival, args[5].ival,
                    args[6].ival, args[7].ival);
}

extern "C" void NDGatherRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDGatherRegister);
}
