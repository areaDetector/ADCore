/*
 * NDPluginGather.cpp
 *
 * A plugin that subscribes to callbacks from multiple ports, not just a single port
 * Author: Mark Rivers
 *
 * February 27. 2017.
 */

#include <stdlib.h>
#include <stdio.h>

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsTime.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginGather.h"

static const char *driverName="NDPluginGather";

/** Constructor for NDPluginGather; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  *
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] maxPorts  Maximum number of ports that this plugin can connected to for callbacks
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
                   "", 0, maxPorts, maxBuffers, maxMemory,
                   asynInt32Mask | asynFloat64Mask | asynGenericPointerMask,
                   asynInt32Mask | asynFloat64Mask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize, 1),
    maxPorts_(maxPorts)
{
    int i;
    NDGatherNDArraySource_t *pArraySrc;
    //static const char *functionName = "NDPluginGather";

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginGather");

    if (maxPorts_ < 1) maxPorts_ = 1;
    NDArraySrc_ = (NDGatherNDArraySource_t *)calloc(sizeof(NDGatherNDArraySource_t), maxPorts_);
    pArraySrc = NDArraySrc_;
    for (i=0; i<maxPorts_; i++, pArraySrc++) {
        /* Create asynUser for communicating with NDArray port */
        pArraySrc->pasynUserGenericPointer = pasynManager->createAsynUser(0, 0);
        pArraySrc->pasynUserGenericPointer->userPvt = this;
        pArraySrc->pasynUserGenericPointer->reason = NDArrayData;
    }
}


extern "C" {static void driverCallback(void *drvPvt, asynUser *pasynUser, void *genericPointer)
{
    NDPluginDriver *pNDPluginDriver = (NDPluginDriver *)drvPvt;
    pNDPluginDriver->driverCallback(pasynUser, genericPointer);
}}


/**
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginGather::processCallbacks(NDArray *pArray)
{
    /* This function is called with the mutex already locked.  It unlocks it during long calculations when private
    * structures don't need to be protected.
    */
    //static const char *functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);

    NDPluginDriver::endProcessCallbacks(pArray, true, true);
}

/** Register or unregister to receive asynGenericPointer (NDArray) callbacks from the driver.
  * Note: this function must be called with the lock released, otherwise a deadlock can occur
  * in the call to cancelInterruptUser.
  * \param[in] enableCallbacks 1 to enable callbacks, 0 to disable callbacks */
asynStatus NDPluginGather::setArrayInterrupt(int enableCallbacks)
{
    asynStatus status = asynSuccess;
    int i;
    NDGatherNDArraySource_t *pArraySrc = NDArraySrc_;
    static const char *functionName = "setArrayInterrupt";

    for (i=0; i<maxPorts_; i++, pArraySrc++) {
        if (enableCallbacks && pArraySrc->connectedToArrayPort && !pArraySrc->asynGenericPointerInterruptPvt) {
            status = pArraySrc->pasynGenericPointer->registerInterruptUser(
                        pArraySrc->asynGenericPointerPvt, pArraySrc->pasynUserGenericPointer,
                        ::driverCallback, this, &pArraySrc->asynGenericPointerInterruptPvt);
            if (status != asynSuccess) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: Can't register for interrupt callbacks on detector port: %s\n",
                    driverName, functionName, pArraySrc->pasynUserGenericPointer->errorMessage);
                return(status);
            }
        }
        if (!enableCallbacks && pArraySrc->connectedToArrayPort && pArraySrc->asynGenericPointerInterruptPvt) {
            status = pArraySrc->pasynGenericPointer->cancelInterruptUser(pArraySrc->asynGenericPointerPvt,
                            pArraySrc->pasynUserGenericPointer, pArraySrc->asynGenericPointerInterruptPvt);
            pArraySrc->asynGenericPointerInterruptPvt = NULL;
            if (status != asynSuccess) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: Can't unregister for interrupt callbacks on detector port: %s\n",
                    driverName, functionName, pArraySrc->pasynUserGenericPointer->errorMessage);
                return(status);
            }
        }
    }
    return(asynSuccess);
}


/** Connect this plugin to an NDArray port driver; disconnect from any existing driver first, register
  * for callbacks if enabled. */
asynStatus NDPluginGather::connectToArrayPort(void)
{
    asynStatus status;
    asynInterface *pasynInterface;
    int enableCallbacks;
    char arrayPort[20];
    int arrayAddr;
    int i;
    NDGatherNDArraySource_t *pArraySrc = NDArraySrc_;
    static const char *functionName = "connectToArrayPort";

    getIntegerParam(NDPluginDriverEnableCallbacks, &enableCallbacks);
    for (i=0; i<maxPorts_; i++, pArraySrc++) {
        getStringParam(i, NDPluginDriverArrayPort, sizeof(arrayPort), arrayPort);
        getIntegerParam(i, NDPluginDriverArrayAddr, &arrayAddr);

        /* If we are currently connected to an array port cancel interrupt request */
        if (pArraySrc->connectedToArrayPort) {
            status = setArrayInterrupt(0);
        }

        /* Disconnect the array port from our asynUser.  Ignore error if there is no device
         * currently connected. */
        pasynManager->disconnect(pArraySrc->pasynUserGenericPointer);
        pArraySrc->connectedToArrayPort = false;

        /* Connect to the array port driver if the arrayPort string is not zero-length */
        if (strlen(arrayPort) == 0) continue;
        status = pasynManager->connectDevice(pArraySrc->pasynUserGenericPointer, arrayPort, arrayAddr);
        if (status != asynSuccess) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s::%s Error calling pasynManager->connectDevice to array port %s address %d, status=%d, error=%s\n",
                      driverName, functionName, arrayPort, arrayAddr, status, pArraySrc->pasynUserGenericPointer->errorMessage);
            return (status);
        }

        /* Find the asynGenericPointer interface in that driver */
        pasynInterface = pasynManager->findInterface(pArraySrc->pasynUserGenericPointer, asynGenericPointerType, 1);
        if (!pasynInterface) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s::connectToPort ERROR: Can't find asynGenericPointer interface on array port %s address %d\n",
                      driverName, arrayPort, arrayAddr);
            return(asynError);
        }
        pArraySrc->pasynGenericPointer = (asynGenericPointer *)pasynInterface->pinterface;
        pArraySrc->asynGenericPointerPvt = pasynInterface->drvPvt;
        pArraySrc->connectedToArrayPort = true;
    }
    /* Enable or disable interrupt callbacks */
    status = setArrayInterrupt(enableCallbacks);

    return(status);
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
