/**
 * NDPluginTimeSeries.cpp
 *
 * Plugin that creates time-series arrays from callback data.
 * 
 * @author Mark Rivers 
 * @date February 2016
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsTime.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <NDArray.h>

#include <epicsExport.h>

#include "NDPluginTimeSeries.h"

#define DEFAULT_NUM_TSPOINTS 2048

enum {
  TSAcquireModeFixed,
  TSAcquireModeCircular
};

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
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginTimeSeries::NDPluginTimeSeries(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, 
                         int maxSignals, int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor 
     * Note: maxAddr is maxSignals+1 because we do callbacks on the 2-D array on address maxSignals */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
             NDArrayPort, NDArrayAddr, maxSignals+1, NUM_NDPLUGIN_TIME_SERIES_PARAMS, maxBuffers, maxMemory,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             ASYN_MULTIDEVICE, 1, priority, stackSize),
    dataType_(NDFloat64), dataSize_(sizeof(epicsFloat64)), numTimePoints_(DEFAULT_NUM_TSPOINTS), currentTimePoint_(0),
    uniqueId_(0), numAverage_(1), timePerPoint_(0), signalData_(0), timeAxis_(0), timeStamp_(0), pTimeCircular_(0)
{
  //const char *functionName = "NDPluginTimeSeries::NDPluginTimeSeries";

  if (maxSignals < 1) {
    maxSignals = 1;
  }
  maxSignals_ = maxSignals;
  numSignals_ = maxSignals;
  averageStore_ = (double *)calloc(maxSignals_, sizeof(double));
  
  /* Per-plugin parameters */
  createParam(TSAcquireString,                 asynParamInt32, &P_TSAcquire);
  createParam(TSReadString,                    asynParamInt32, &P_TSRead);
  createParam(TSNumPointsString,               asynParamInt32, &P_TSNumPoints);
  createParam(TSCurrentPointString,            asynParamInt32, &P_TSCurrentPoint);
  createParam(TSTimePerPointString,          asynParamFloat64, &P_TSTimePerPoint);
  createParam(TSAveragingTimeString,         asynParamFloat64, &P_TSAveragingTime);
  createParam(TSNumAverageString,              asynParamInt32, &P_TSNumAverage);
  createParam(TSElapsedTimeString,           asynParamFloat64, &P_TSElapsedTime);
  createParam(TSAcquireModeString,             asynParamInt32, &P_TSAcquireMode);
  createParam(TSTimeAxisString,         asynParamFloat64Array, &P_TSTimeAxis);
  createParam(TSTimestampString,        asynParamFloat64Array, &P_TSTimestamp);
  
  /* Per-signal parameters */
  createParam(TSTimeSeriesString,       asynParamFloat64Array, &P_TSTimeSeries);
 
  /* Set the plugin type string */
  setStringParam(NDPluginDriverPluginType, "NDPluginTimeSeries");
  
  setIntegerParam(P_TSNumPoints, numTimePoints_);
  allocateArrays();
  
  /* Try to connect to the array port */
  connectToArrayPort();

  callParamCallbacks();
  
}

void NDPluginTimeSeries::computeNumAverage()
{
  if (timePerPoint_ == 0) {
    numAverage_ = 1;
    averagingTimeActual_ = averagingTimeRequested_;
  }
  else {
    numAverage_ = (int) (averagingTimeRequested_/timePerPoint_ + 0.5);
    if (numAverage_ < 1) numAverage_ = 1;
    averagingTimeActual_ = timePerPoint_ * numAverage_;
  }
  numAveraged_ = 0;
  setDoubleParam(P_TSAveragingTime, averagingTimeActual_);
  setIntegerParam(P_TSNumAverage, numAverage_);
  createAxisArray();
  callParamCallbacks();
}

void NDPluginTimeSeries::allocateArrays()
{
  int numPoints;
  int nDims=2;
  size_t dims[2];
  
  getIntegerParam(P_TSNumPoints, &numPoints);
  numTimePoints_ = numPoints;
  if (timeStamp_)  free(timeStamp_);
  if (signalData_) free (signalData_);
  if (pTimeCircular_) pTimeCircular_->release();

  timeStamp_  = (double *)calloc(numSignals_*numTimePoints_, sizeof(double));
  signalData_ = (double *)calloc(numSignals_*numTimePoints_, sizeof(double));
  nDims = 2;
  dims[0] = numTimePoints_;
  dims[1] = numSignals_;
  pTimeCircular_ = pNDArrayPool->alloc(nDims, dims, dataType_, 0, 0);
  createAxisArray();
  acquireReset();
}

void NDPluginTimeSeries::acquireReset()
{
  memset(signalData_,           0, numTimePoints_ * numSignals_ * sizeof(double));
  memset(timeStamp_,            0, numTimePoints_ * sizeof(double));
  memset(pTimeCircular_->pData, 0, numTimePoints_ * numSignals_ * dataSize_);
  currentTimePoint_ = 0;
  setIntegerParam(P_TSCurrentPoint, currentTimePoint_);
  epicsTimeGetCurrent(&startTime_);
}

void NDPluginTimeSeries::createAxisArray()
{
  int i;
  
  if (timeAxis_) free(timeAxis_);
  timeAxis_ = (double *)calloc(numTimePoints_, sizeof(double));
  for (i=0; i<numTimePoints_; i++) {
    if (acquireMode_ == TSAcquireModeFixed) {
      timeAxis_[i] = i*averagingTimeActual_;
    } else {
      timeAxis_[i] = -(numTimePoints_-1-i)*averagingTimeActual_;
    }
  }
  doCallbacksFloat64Array(timeAxis_, numTimePoints_, P_TSTimeAxis, 0);
}

/**
 * Templated function to append to time series on different NDArray data types.
 * \param[in] NDArray The pointer to the NDArray object
 * \return asynStatus
 */
template <typename epicsType>
asynStatus NDPluginTimeSeries::doAddToTimeSeriesT(NDArray *pArray)
{
  epicsType *pData         = (epicsType *)pArray->pData;
  epicsType *pIn; 
  epicsType *pTimeCircular = (epicsType *)pTimeCircular_->pData;
  int signal;
  int i;
  int numTimes = 1;
  epicsTimeStamp timeNow;
  double elapsedTime;
  
  if (pArray->ndims == 2) numTimes = (int)pArray->dims[1].size;
  
  for (i=0; i<numTimes; i++) {
    pIn = pData + i*numSignalsIn_;
    for (signal=0; signal<numSignals_; signal++) {
      averageStore_[signal] += (epicsFloat64)*pIn++;
    }
    numAveraged_++;
    if (numAveraged_ < numAverage_) continue;
    /* We have now collected the desired number of points to average */
    for (signal=0; signal<numSignals_; signal++) {
      pTimeCircular[signal * numTimePoints_ + currentTimePoint_] = (epicsType)averageStore_[signal]/numAveraged_;
      averageStore_[signal] = 0;
    }
    numAveraged_ = 0;
    timeStamp_[currentTimePoint_] = pArray->timeStamp;
    currentTimePoint_++;
    if (currentTimePoint_ >= numTimePoints_) {
      if (acquireMode_ == TSAcquireModeFixed) {
        setIntegerParam(P_TSAcquire, 0);
        doTimeSeriesCallbacks();
        break;
      }
      else { // Circular buffer mode
          currentTimePoint_ = 0;
      }
    }
  }  // for (i=0; ...)
  setIntegerParam(P_TSCurrentPoint, currentTimePoint_);     
  epicsTimeGetCurrent(&timeNow);
  elapsedTime = epicsTimeDiffInSeconds(&timeNow, &startTime_);
  setDoubleParam(P_TSElapsedTime, elapsedTime);
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

template <typename epicsType>
void NDPluginTimeSeries::doTimeSeriesCallbacksT()
{
  int signal;
  int timeOut;
  int timeIn=0;
  epicsType *timeCircular = (epicsType *)pTimeCircular_->pData;


  if (acquireMode_ == TSAcquireModeFixed) {
    for (signal=0; signal<numSignals_; signal++) {
      for (timeOut=0; timeOut<currentTimePoint_; timeOut++) {
        signalData_[timeOut] = timeCircular[signal*numTimePoints_ + timeOut];
      }
      doCallbacksFloat64Array(signalData_, currentTimePoint_, P_TSTimeSeries, signal);
    }
  }
  else {
    timeIn = currentTimePoint_;
    for (signal=0; signal<numSignals_; signal++) {
      for (timeOut=0; timeOut<numTimePoints_; timeOut++) {
        signalData_[timeOut] = timeCircular[signal*numTimePoints_ + timeIn];
        if (++timeIn >= numTimePoints_) timeIn = 0;
      }
      doCallbacksFloat64Array(signalData_, numTimePoints_, P_TSTimeSeries, signal);
    }
  }
}

/**
 * Call the templated doTimeSeriesCallbacks so we can cast correctly. 
 * \return asynStatus
 */
asynStatus NDPluginTimeSeries::doTimeSeriesCallbacks()
{
  int arrayCallbacks;
  epicsTimeStamp now;
  asynStatus status = asynSuccess;
  char *src, *dst;
  int signal;
  size_t dims[1]; 
  int numCopy;
  
  switch(dataType_) {
  case NDInt8:
    doTimeSeriesCallbacksT<epicsInt8>();
    break;
  case NDUInt8:
    doTimeSeriesCallbacksT<epicsUInt8>();
    break;
  case NDInt16:
    doTimeSeriesCallbacksT<epicsInt16>();
    break;
  case NDUInt16:
    doTimeSeriesCallbacksT<epicsUInt16>();
    break;
  case NDInt32:
    doTimeSeriesCallbacksT<epicsInt32>();
    break;
  case NDUInt32:
    doTimeSeriesCallbacksT<epicsUInt32>();
    break;
  case NDFloat32:
    doTimeSeriesCallbacksT<epicsFloat32>();
    break;
  case NDFloat64:
    doTimeSeriesCallbacksT<epicsFloat64>();
    break;
  default:
    return asynError;
    break;
  }

  getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
  if (arrayCallbacks) {
    NDArray *pArrayOut = this->pArrays[0];
    if (pArrayOut) pArrayOut->release();
    if (acquireMode_ == TSAcquireModeFixed) {
      pArrayOut = pNDArrayPool->copy(pTimeCircular_, NULL, 1);    
    }
    else {
      // Shift the data so the oldest time point is the first point in the array
      pArrayOut = pNDArrayPool->copy(pTimeCircular_, NULL, 0);
      for (signal=0; signal<numSignals_; signal++) {
        numCopy = numTimePoints_ - currentTimePoint_;
        src = (char *)pTimeCircular_->pData + ((signal * numTimePoints_) + currentTimePoint_)*dataSize_;
        dst = (char *)pArrayOut->pData      +  (signal * numTimePoints_)*dataSize_;
        memcpy(dst, src, numCopy*dataSize_);
        numCopy = currentTimePoint_;
        src = (char *)pTimeCircular_->pData +  (signal * numTimePoints_)*dataSize_;
        dst = (char *)pArrayOut->pData      + ((signal * numTimePoints_) + numTimePoints_ - currentTimePoint_)*dataSize_;
        memcpy(dst, src, numCopy*dataSize_);
      }
    }
    this->getAttributes(pArrayOut->pAttributeList);
    getTimeStamp(&pArrayOut->epicsTS);
    epicsTimeGetCurrent(&now);
    pArrayOut->timeStamp = now.secPastEpoch + now.nsec / 1.e9;
    pArrayOut->uniqueId = uniqueId_++;
    this->unlock();
    doCallbacksGenericPointer(pArrayOut, NDArrayData, numSignals_);
    this->lock();
    this->pArrays[0] = pArrayOut;
    // Now do NDArray callbacks on 1-D arrays for each signal
    numCopy = (int)pArrayOut->dims[0].size;
    dims[0] = numCopy;
    for (signal=0; signal<numSignals_; signal++) {
      NDArray *pArray = pNDArrayPool->alloc(1, dims, pArrayOut->dataType, 0, 0);
      src = (char *)pArrayOut->pData + (signal * numTimePoints_)*dataSize_;
      dst = (char *)pArray->pData;
      memcpy(dst, src, numCopy*dataSize_); 
      this->getAttributes(pArray->pAttributeList);
      pArray->epicsTS   = pArrayOut->epicsTS;
      pArray->timeStamp = pArrayOut->timeStamp;
      pArray->uniqueId  = pArrayOut->uniqueId;
      this->unlock();
      doCallbacksGenericPointer(pArray, NDArrayData, signal);
      this->lock();
      pArray->release();
    }
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
  int acquiring;
  NDArrayInfo_t arrayInfo;
  const char* functionName = "NDPluginTimeSeries::processCallbacks";

  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);

  // This plugin only works with 1-D or 2-D arrays
  if ((pArray->ndims < 1) || (pArray->ndims > 2)) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s: error, number of array dimensions must be 1 or 2\n",
      functionName);
    return;
  }

  // If the number of signals or the data type has changed from the last callback
  // then we need to allocate arrays
  if ((pArray->dataType          != dataType_) || 
      ((int)pArray->dims[0].size != numSignalsIn_)) {
    dataType_   = pArray->dataType;
    numSignalsIn_ = (int)pArray->dims[0].size;
    numSignals_ = numSignalsIn_;
    if (numSignals_ > maxSignals_) numSignals_ = maxSignals_;
    pArray->getInfo(&arrayInfo);
    dataSize_ = arrayInfo.bytesPerElement;
    allocateArrays();
  }
  
  getIntegerParam(P_TSAcquire, &acquiring);
  
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
  static const char* functionName = "NDPluginTimeSeries::writeInt32";

  status = getAddress(pasynUser, &signal); 
  if (status != asynSuccess) {
    return status;
  }

  /* Set parameter and readback in parameter library */
  stat = (setIntegerParam(signal, function, value) == asynSuccess) && stat;

  if (function == P_TSNumPoints) {
    allocateArrays();
  } else if (function == P_TSAcquireMode) {
    acquireMode_ = value;
    acquireReset();
    createAxisArray();
  } else if (function == P_TSAcquire) {
    if (value) {
      acquireReset();
    }
    else {
      doTimeSeriesCallbacks();
    }
  } else if (function == P_TSRead) {
    doTimeSeriesCallbacks();
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

asynStatus NDPluginTimeSeries::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  int function = pasynUser->reason;
  int signal;
  asynStatus status=asynSuccess;
  bool stat = true;
  static const char* functionName = "NDPluginTimeSeries::writeInt32";

  status = getAddress(pasynUser, &signal); 
  if (status != asynSuccess) {
    return status;
  }

  /* Set the parameter in the parameter library. */
  stat = (setDoubleParam(signal, function, value) == asynSuccess) && stat;
  if (function == P_TSTimePerPoint) {
    timePerPoint_ = value;
    computeNumAverage();
  } else if (function == P_TSAveragingTime) {
    averagingTimeRequested_ = value;
    computeNumAverage();
  } else if (function < FIRST_NDPLUGIN_TIME_SERIES_PARAM) {
    stat = (NDPluginDriver::writeFloat64(pasynUser, value) == asynSuccess) && stat;
  }

  /* Do callbacks so higher layers see any changes */
  stat = (callParamCallbacks(signal) == asynSuccess) && stat;
  stat = (callParamCallbacks() == asynSuccess) && stat;

  if (!stat) {
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
      "%s: status=%d, function=%d, value=%f",
      functionName, status, function, value);
    status = asynError;
  } else {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
      "%s: function=%d, signal=%d, value=%f\n",
      functionName, function, signal, value);
  }
  return(status);
}

/** Configuration command */
extern "C" int NDTimeSeriesConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr, 
                                     int maxSignals,
                                     int maxBuffers, size_t maxMemory,
                                     int priority, int stackSize)
{
    NDPluginTimeSeries *pPlugin = new NDPluginTimeSeries(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 
                                                         maxSignals, maxBuffers, maxMemory, priority, stackSize);
    return pPlugin->start();
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxSignals",iocshArgInt};
static const iocshArg initArg6 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg7 = { "maxMemory",iocshArgInt};
static const iocshArg initArg8 = { "priority",iocshArgInt};
static const iocshArg initArg9 = { "stackSize",iocshArgInt};
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
static const iocshFuncDef initFuncDef = {"NDTimeSeriesConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDTimeSeriesConfigure(args[0].sval, args[1].ival, args[2].ival,
                        args[3].sval, args[4].ival, args[5].ival,
                        args[6].ival, args[7].ival, args[8].ival, 
                        args[9].ival);
}

extern "C" void NDTimeSeriesRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDTimeSeriesRegister);
}
