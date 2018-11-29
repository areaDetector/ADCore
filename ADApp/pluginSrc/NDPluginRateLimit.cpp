/*
 * NDPluginRateLimit.cpp
 *
 * Plugin to rate limit NDArrays
 *
 * Author: Bruno Martins
 *
 * Created November 28, 2018
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginRateLimit.h"

using std::string;

static const char *driverName="NDPluginRateLimit";

void NDPluginRateLimit::reset(void) {
    int mode, limit;

    getIntegerParam(NDRateLimitMode, &mode);
    getIntegerParam(NDRateLimitLimit, &limit);

    int refillTime, refillAmount;

    switch(mode) {
    case NDRATELIMIT_OFF:
        refillAmount = 0;
        refillTime = 0;
        break;

    case NDRATELIMIT_ARRAYRATE:
        /* Array Rate limit are expected to be in the order of 10^0 to 10^4,
         * so we fix the refillAmount to 1 and work out the time between
         * refills so there are no "fractional refills".
         */
        refillAmount = 1;
        refillTime = 1000.0 / ((double)limit);  // Limit is in arrays/sec
        break;

    case NDRATELIMIT_BYTERATE:
        /* Byte rate limits are expected to be much larger numbers, so we want
         * to avoid a "fractional" refillTime. Hence, we fix it to 10ms.
         */
        refillTime = 10;
        refillAmount = limit / (1000.0/refillTime);  // Limit is in bytes/sec
        break;

    default:
        return;
    }

    setIntegerParam(NDRateLimitNumTokens, limit);
    setIntegerParam(NDRateLimitRefillTime, refillTime);
    setIntegerParam(NDRateLimitRefillAmount, refillAmount);

    callParamCallbacks();

    epicsTimeGetCurrent(&lastUpdate_);
}

int NDPluginRateLimit::refill(void) {
    epicsTimeStamp now;
    epicsTimeGetCurrent(&now);

    int numTokens, refillTime, refillAmount, limit;
    getIntegerParam(NDRateLimitNumTokens, &numTokens);
    getIntegerParam(NDRateLimitRefillTime, &refillTime);
    getIntegerParam(NDRateLimitRefillAmount, &refillAmount);
    getIntegerParam(NDRateLimitLimit, &limit);

    double elapsedMs = epicsTimeDiffInSeconds(&now, &lastUpdate_)*1000;
    int refillCount = elapsedMs / refillTime;

    if (refillCount) {
        numTokens = std::min(limit, numTokens + refillCount*refillAmount);
        setIntegerParam(NDRateLimitNumTokens, numTokens);

        callParamCallbacks();

        lastUpdate_ = now;
    }

    return numTokens;
}

bool NDPluginRateLimit::tryTake(int tokens) {
    int numTokens = refill();

    if (tokens > numTokens)
        return false;

    setIntegerParam(NDRateLimitNumTokens, numTokens - tokens);
    callParamCallbacks();
    return true;
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Drops the array if a rate limit is reached.
  * If mode is off, no rate limiting happens.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginRateLimit::processCallbacks(NDArray *pArray)
{
    NDPluginDriver::beginProcessCallbacks(pArray);

    int mode;
    getIntegerParam(NDRateLimitMode, &mode);

    bool succeeded = true;

    if (mode == NDRATELIMIT_ARRAYRATE) {
        succeeded = tryTake(1);
    } else if (mode == NDRATELIMIT_BYTERATE) {
        if (pArray->codec.empty()) {
            NDArrayInfo info;

            pArray->getInfo(&info);
            succeeded = tryTake(info.totalBytes);
        } else {
            succeeded = tryTake(pArray->compressedSize);
        }
    }

    if (succeeded)
        NDPluginDriver::endProcessCallbacks(pArray, true, true);
    else {
        int dropped;
        getIntegerParam(NDRateLimitDropped, &dropped);
        setIntegerParam(NDRateLimitDropped, dropped + 1);
        callParamCallbacks();
    }
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including NDPluginDriverEnableCallbacks and
  * NDPluginDriverArrayAddr.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginRateLimit::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char* functionName = "writeInt32";

    if (function == NDRateLimitMode) {
        if (value < 0)
            value = 0;
        else if (value >= NDRATELIMIT_MODE_MAX)
            value = NDRATELIMIT_MODE_MAX;
    } else if (function == NDRateLimitLimit) {
        if (value < 0) {
            value = 0;
        }
    } else if (function < FIRST_NDCODEC_PARAM) {
        status = NDPluginDriver::writeInt32(pasynUser, value);
    }

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    if (function == NDRateLimitMode || function == NDRateLimitLimit)
        reset();

    const char* paramName;
    if (status) {
        getParamName( function, &paramName );
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: function=%d %s, value=%d\n",
              driverName, functionName, function, paramName, value);
    }
    else {
        if ( pasynTrace->getTraceMask(pasynUser) & ASYN_TRACEIO_DRIVER ) {
            getParamName( function, &paramName );
            asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                  "%s:%s: function=%d %s, paramvalue=%d\n",
                  driverName, functionName, function, paramName, value);
        }
    }
    return status;
}

/** Constructor for NDPluginRateLimit; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
  *            allowed to allocate. Set this to 0 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to 0 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] maxThreads The maximum number of threads this driver is allowed to use. If 0 then 1 will be used.
  */
NDPluginRateLimit::NDPluginRateLimit(const char *portName, int queueSize, int blockingCallbacks,
                                           const char *NDArrayPort, int NDArrayAddr,
                                           int maxBuffers, size_t maxMemory,
                                           int priority, int stackSize, int maxThreads)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
                   asynGenericPointerMask,
                   asynGenericPointerMask,
                   0, 1, priority, stackSize, maxThreads,
                   true)
{
    createParam(NDRateLimitModeString,          asynParamInt32, &NDRateLimitMode);
    createParam(NDRateLimitLimitString,         asynParamInt32, &NDRateLimitLimit);
    createParam(NDRateLimitRefillTimeString,    asynParamInt32, &NDRateLimitRefillTime);
    createParam(NDRateLimitRefillAmountString,  asynParamInt32, &NDRateLimitRefillAmount);
    createParam(NDRateLimitNumTokensString,     asynParamInt32, &NDRateLimitNumTokens);
    createParam(NDRateLimitDroppedString,       asynParamInt32, &NDRateLimitDropped);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginRateLimit");

    // Enable ArrayCallbacks.
    // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
    setIntegerParam(NDArrayCallbacks, 1);

    //reset();

    /* Try to connect to the array port */
    connectToArrayPort();
}

extern "C" int NDRateLimitConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                          const char *NDArrayPort, int NDArrayAddr,
                                          int maxBuffers, size_t maxMemory,
                                          int priority, int stackSize, int maxThreads)
{
    NDPluginRateLimit *pPlugin = new NDPluginRateLimit(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                                             maxBuffers, maxMemory, priority, stackSize, maxThreads);
    return pPlugin->start();
}

/** EPICS iocsh shell commands */
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
static const iocshFuncDef initFuncDef = {"NDRateLimitConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDRateLimitConfigure(args[0].sval, args[1].ival, args[2].ival,
                               args[3].sval, args[4].ival, args[5].ival,
                               args[6].ival, args[7].ival, args[8].ival,
                               args[9].ival);
}

extern "C" void NDRateLimitRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDRateLimitRegister);
}
