/**
 * NDPluginTimeSeries.cpp
 *
 * Plugin that creates time-series arrays from callback data.
 * Optionally computes the FFT of the time-series data. 
 * 
 * @author Mark Rivers 
 * @date February 2016
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <NDArray.h>

#include <epicsExport.h>

#include "NDPluginTimeSeries.h"

#define DEFAULT_NUM_TSPOINTS 2048

/** Constructor for NDPluginTimeSeries; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxSignals The maximum number of signals this plugin supports. 1 is minimum.
  * \param[in] drvInfoTime The drvInfo string to access the time interval parameter in the driver.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginTimeSeries::NDPluginTimeSeries(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, 
                         int maxSignals, const char *drvInfoTime,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
             NDArrayPort, NDArrayAddr, maxSignals, NUM_NDPLUGIN_TIME_SERIES_PARAMS, maxBuffers, maxMemory,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             ASYN_MULTIDEVICE, 1, priority, stackSize),
    timeArray_(0), freqArray_(0), timeStamp_(0), 
    timeSeries_(0), FFTReal_(0), FFTImaginary_(0), FFTAbsValue_(0)
{
  //const char *functionName = "NDPluginTimeSeries::NDPluginTimeSeries";

  if (maxSignals < 1) {
    maxSignals = 1;
  }
  maxSignals_ = maxSignals;
  
  /* Per-plugin parameters */
  createParam(TSControlString,                 asynParamInt32, &P_TSControl);
  createParam(TSNumPointsString,               asynParamInt32, &P_TSNumPoints);
  createParam(TSCurrentPointString,            asynParamInt32, &P_TSCurrentPoint);
  createParam(TSAcquiringString,               asynParamInt32, &P_TSAcquiring);
  createParam(TSAcquireModeString,             asynParamInt32, &P_TSAcquireMode);
  createParam(TSComputeFFTString,              asynParamInt32, &P_TSComputeFFT);
  createParam(TSTimeArrayString,        asynParamFloat64Array, &P_TSTimeArray);
  createParam(TSFreqArrayString,        asynParamFloat64Array, &P_TSFreqArray);
  createParam(TSTimestampString,        asynParamFloat64Array, &P_TSTimestamp);
  
  /* Per-signal parameters */
  createParam(TSSignalNameString,              asynParamOctet, &P_TSSignalName);
  createParam(TSSignalUseString,               asynParamInt32, &P_TSSignalUse);
  createParam(TSTimeSeriesString,       asynParamFloat64Array, &P_TSTimeSeries);
  createParam(TSFFTRealString,          asynParamFloat64Array, &P_TSFFTReal);
  createParam(TSFFTImaginaryString,     asynParamFloat64Array, &P_TSFFTImaginary);
  createParam(TSFFTAbsValueString,      asynParamFloat64Array, &P_TSFFTAbsValue);
 
  //Note: params set to a default value here will overwrite a default database value

  /* Set the plugin type string */
  setStringParam(NDPluginDriverPluginType, "NDPluginTimeSeries");
  
  numTimePoints_ = DEFAULT_NUM_TSPOINTS;
  setIntegerParam(P_TSNumPoints, numTimePoints_);
  allocateArrays();
  
  /* Try to connect to the array port */
  connectToArrayPort();

  callParamCallbacks();
  
}

void NDPluginTimeSeries::allocateArrays()
{
  if (timeArray_)     free(timeArray_);
  if (timeStamp_)     free(timeStamp_);
  if (timeSeries_)    free(timeSeries_);
  if (freqArray_)     free(freqArray_);
  if (FFTReal_)       free(FFTReal_);
  if (FFTImaginary_)  free(FFTImaginary_);
  if (FFTAbsValue_)   free(FFTAbsValue_);

  timeArray_    = (double *)calloc(maxSignals_*numTimePoints_, sizeof(double));
  timeStamp_    = (double *)calloc(maxSignals_*numTimePoints_, sizeof(double));
  timeSeries_   = (double *)calloc(maxSignals_*numTimePoints_, sizeof(double));
  freqArray_    = (double *)calloc(maxSignals_*numFreqPoints_, sizeof(double));
  FFTReal_      = (double *)calloc(maxSignals_*numFreqPoints_, sizeof(double));
  FFTImaginary_ = (double *)calloc(maxSignals_*numFreqPoints_, sizeof(double));
  FFTAbsValue_  = (double *)calloc(maxSignals_*numFreqPoints_, sizeof(double));
}

void NDPluginTimeSeries::zeroArrays()
{
  memset(timeSeries_,   0, numTimePoints_*sizeof(double));
  memset(FFTReal_,      0, numFreqPoints_*sizeof(double));
  memset(FFTImaginary_, 0, numFreqPoints_*sizeof(double));
  memset(FFTAbsValue_,  0, numFreqPoints_*sizeof(double));
}

/**
 * Templated function to calculate statistics on different NDArray data types.
 * \param[in] NDArray The pointer to the NDArray object
 * \param[in] NDROI The pointer to the NDROI object
 * \return asynStatus
 */
template <typename epicsType>
asynStatus NDPluginTimeSeries::doAddToTimeSeriesT(NDArray *pArray)
{
  epicsType *pData = (epicsType *)pArray->pData;
  int signal;
  int i;
  int numTimes = 1;
  
  if (pArray->ndims == 2) numTimes = pArray->dims[1].size;
  
  for (i=0; i<numTimes; i++) {
    for (signal=0; signal<maxSignals_; signal++) {
      timeSeries_[signal*numTimePoints_ + currentTimePoint_] = (epicsFloat64)*pData++;
    }
    timeStamp_[currentTimePoint_] = pArray->timeStamp;
    currentTimePoint_++;
    setIntegerParam(P_TSCurrentPoint, currentTimePoint_);
    if (currentTimePoint_ >= numTimePoints_) {
      setIntegerParam(P_TSAcquiring, 0);
      doTimeSeriesCallbacks();
      break;
    }
  }
  return asynSuccess;
}

     
/**
 * Call the templated doAddToTimeSeries so we can cast correctly. 
 * \param[in] NDArray The pointer to the NDArray object
 * \return asynStatus
 */
asynStatus NDPluginTimeSeries::addToTimeSeries(NDArray *pArray)
{
  asynStatus status = asynSuccess;
  
  switch(pArray->dataType) {
  case NDInt8:
    status = doAddToTimeSeriesT<epicsInt8>(pArray);
    break;
  case NDUInt8:
    status = doAddToTimeSeriesT<epicsUInt8>(pArray);
    break;
  case NDInt16:
    status = doAddToTimeSeriesT<epicsInt16>(pArray);
    break;
  case NDUInt16:
    status = doAddToTimeSeriesT<epicsUInt16>(pArray);
    break;
  case NDInt32:
    status = doAddToTimeSeriesT<epicsInt32>(pArray);
    break;
  case NDUInt32:
    status = doAddToTimeSeriesT<epicsUInt32>(pArray);
    break;
  case NDFloat32:
    status = doAddToTimeSeriesT<epicsFloat32>(pArray);
    break;
  case NDFloat64:
    status = doAddToTimeSeriesT<epicsFloat64>(pArray);
    break;
  default:
    return asynError;
    break;
  }
  return status;
}


/** 
 * Callback function that is called by the NDArray driver with new NDArray data.
 * Appends the new array data to the time series if possible.
 * \param[in] pArray The NDArray from the callback.
 */
void NDPluginTimeSeries::processCallbacks(NDArray *pArray)
{

  //This function is called with the mutex already locked.  
  //It unlocks it during long calculations when private structures don't need to be protected.
  
  int acquiring;
  const char* functionName = "NDPluginTimeSeries::processCallbacks";

  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);

  // This plugin only works with 1-D or 2-D arrays
  if ((pArray->ndims < 1) || (pArray->ndims > 2)) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s: error, number of array dimensions must be 1 or 2\n",
        functionName);
  }

  getIntegerParam(P_TSAcquiring, &acquiring);
  
  if (acquiring) {
      addToTimeSeries(pArray);
  }

  callParamCallbacks();
}

/** Called when asyn clients call pasynInt32->write().
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value The value to write. 
  * \return asynStatus
  */
asynStatus NDPluginTimeSeries::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    bool stat = true;
    int signal = 0;
    const char* functionName = "NDPluginTimeSeries::writeInt32";

    status = getAddress(pasynUser, &signal); 
    if (status != asynSuccess) {
      return status;
    }

    /* Set parameter and readback in parameter library */
    stat = (setIntegerParam(signal, function, value) == asynSuccess) && stat;
    
    if (function == P_TSNumPoints) {
      allocateArrays();
    } else if (function == P_TSControl) {
        switch (value) {
          case TSEraseStart:
            currentTimePoint_ = 0;
            setIntegerParam(P_TSCurrentPoint, currentTimePoint_);
            setIntegerParam(P_TSAcquiring, 1);
            zeroArrays();
            break;
          case TSStart:
            if (currentTimePoint_ < numTimePoints_) {
                setIntegerParam(P_TSAcquiring, 1);
            }
            break;
          case TSStop:
            setIntegerParam(P_TSAcquiring, 0);
            doTimeSeriesCallbacks();
            break;
          case TSRead:
            doTimeSeriesCallbacks();
            break;
        }
    } else if (function < FIRST_NDPLUGIN_TIME_SERIES_PARAM) {
      stat = (NDPluginDriver::writeInt32(pasynUser, value) == asynSuccess) && stat;
    }
    
    /* Do callbacks so higher layers see any changes */
    stat = (callParamCallbacks(signal) == asynSuccess) && stat;
    stat = (callParamCallbacks() == asynSuccess) && stat;

    if (!stat) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
              "%s: status=%d, function=%d, value=%d",
              functionName, status, function, value);
    status = asynError;
    } else {
      asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
        "%s: function=%d, signal=%d, value=%d\n",
        functionName, function, signal, value);
    }
    
    return status;
}

void NDPluginTimeSeries::doTimeSeriesCallbacks()
{
  int signal;
    
  /* Loop over the signals in this driver */
  for (signal=0; signal<maxSignals_; signal++) {
    doCallbacksFloat64Array(timeSeries_ + signal*numTimePoints_, currentTimePoint_, P_TSTimeSeries, signal);
  }
}



/** Configuration command */
extern "C" int NDTimeSeriesConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr, 
                                 int maxSignals, const char *drvInfoTime, 
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    new NDPluginTimeSeries(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 
                           maxSignals, drvInfoTime,
                           maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxSignals",iocshArgInt};
static const iocshArg initArg6 = { "devInfoTime",iocshArgString};
static const iocshArg initArg7 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg8 = { "maxMemory",iocshArgInt};
static const iocshArg initArg9 = { "priority",iocshArgInt};
static const iocshArg initArg10= { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9,
                                            &initArg10};
static const iocshFuncDef initFuncDef = {"NDTimeSeriesConfigure",11,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDTimeSeriesConfigure(args[0].sval, args[1].ival, args[2].ival,
                   args[3].sval, args[4].ival, args[5].ival,
                   args[6].sval, args[7].ival, args[8].ival, 
                   args[9].ival, args[10].ival);
}

extern "C" void NDTimeSeriesRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDTimeSeriesRegister);
}
