/*
 * NDPluginBadPixel.cpp
 *
 * Bad pixel processing plugin
 * Author: Mark Rivers
 *
 * Created April 20, 2021
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <json.hpp>
using nlohmann::json;

#include <iocsh.h>

#include "NDPluginBadPixel.h"

#include <epicsExport.h>

static const char *driverName="NDPluginBadPixel";


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Does bad pixel processing.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginBadPixel::processCallbacks(NDArray *pArray)
{
    /* This function does array processing.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
    NDArray *pArrayOut = NULL;
    NDArrayInfo arrayInfo;
    size_t nElements;
    static const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);

    /* Release the lock now that we are only doing things that don't involve memory other thread
     * cannot access */

    pArray->getInfo(&arrayInfo);
    nElements = arrayInfo.nElements;

    this->unlock();

    /* Make a copy of the array because we cannot modify the input array */
    this->pNDArrayPool->copy(pArray, pArrayOut, 0);
    if (NULL == pArrayOut) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s Processing aborted; cannot allocate an NDArray for storage of temporary data.\n",
            driverName, functionName);
        goto doCallbacks;
    }

    doCallbacks:
    /* We must exit with the mutex locked */
    this->lock();

    if ((NULL != pArrayOut)) {
        NDPluginDriver::endProcessCallbacks(pArrayOut, false, true);
    }

    callParamCallbacks();
}


/** Constructor for NDPluginBadPixel; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
  */
NDPluginBadPixel::NDPluginBadPixel(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize, 1)
{
    //static const char *functionName = "NDPluginBadPixel";

    /* Background array subtraction */
    createParam(NDPluginBadPixelFileNameString, asynParamOctet, &NDPluginBadPixelFileName);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginBadPixel");

    // Enable ArrayCallbacks.
    // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
    setIntegerParam(NDArrayCallbacks, 1);

    /* Try to connect to the array port */
    connectToArrayPort();
}

asynStatus NDPluginBadPixel::readBadPixelFile(const char *fileName)
{
    json j;
    std::ifstream file(fileName);
    file >> j;
    auto badPixels = j["Bad pixels"];
    badPixelDef bp;
    for (size_t i=0; i<badPixels.size(); i++) {
        bp.coordinate.x = badPixels[i]["X"];
        bp.coordinate.y = badPixels[i]["Y"];
        badPixelList.push_back(bp);
        printf("Bad pixel %d, X=%d, Y=%d\n", (int)i, bp.coordinate.x, bp.coordinate.y);
        if (badPixels[i]["Median"] !=0) {
            std::cout << "Median " << badPixels[i]["Median"] << "\n";
        }
        if (badPixels[i]["Set"] !=0) {
            std::cout << "Set " << badPixels[i]["Set"] << "\n";
        }
        
        if (badPixels[i]["Replace"] !=0) {
            std::cout << "Replace " << badPixels[i]["Replace"] << "\n";
        }
    }
    return asynSuccess;
}

/** Called when asyn clients call pasynOctet->write().
  * This function performs actions for some parameters, including AttributesFile.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus NDPluginBadPixel::writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual)
{
  int addr=0;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  const char *functionName = "writeOctet";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
  // Set the parameter in the parameter library.
  status = (asynStatus)setStringParam(addr, function, (char *)value);
  if (status != asynSuccess) return(status);

  if (function == NDPluginBadPixelFileName) {
      if ((nChars > 0) && (value[0] != 0)) {
          status = this->readBadPixelFile(value);
      }
  }

  else if (function < FIRST_NDPLUGIN_BAD_PIXEL_PARAM) {
      /* If this parameter belongs to a base class call its method */
      status = NDPluginDriver::writeOctet(pasynUser, value, nChars, nActual);
  }

  // Do callbacks so higher layers see any changes
  callParamCallbacks(addr);

  if (status){
      epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                    "%s:%s: status=%d, function=%d, value=%s",
                    driverName, functionName, status, function, value);
  } else {
      asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s: function=%d, value=%s\n",
                driverName, functionName, function, value);
  }
  *nActual = nChars;
  return status;
}

/** Configuration command */
extern "C" int NDBadPixelConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int maxBuffers, size_t maxMemory,
                                   int priority, int stackSize)
{
    NDPluginBadPixel *pPlugin = new NDPluginBadPixel(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshFuncDef initFuncDef = {"NDBadPixelConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDBadPixelConfigure(args[0].sval, args[1].ival, args[2].ival,
                        args[3].sval, args[4].ival, args[5].ival,
                        args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDBadPixelRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDBadPixelRegister);
}
