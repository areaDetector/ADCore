/**
 * NDPluginFFT.cpp
 *
 * Plugin that computes 1-D and 2-D FFTs
 * 
 * @author Mark Rivers 
 * @date February 2016
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <NDArray.h>

#include <epicsExport.h>

#include "NDPluginFFT.h"
#include "fft_1d.h"

/** Constructor for NDPluginFFT; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
NDPluginFFT::NDPluginFFT(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, 
                         int maxSignals, int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
             NDArrayPort, NDArrayAddr, maxSignals, NUM_NDPLUGIN_FFT_PARAMS, maxBuffers, maxMemory,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             ASYN_MULTIDEVICE, 1, priority, stackSize),
    timePerPoint_(0), timeAxis_(0), freqAxis_(0), timeSeries_(0), FFTReal_(0), FFTImaginary_(0), FFTAbsValue_(0)
{
  //const char *functionName = "NDPluginFFT::NDPluginFFT";

  if (maxSignals < 1) {
    maxSignals = 1;
  }
  maxSignals_ = maxSignals;
  
  /* Per-plugin parameters */
  createParam(FFTReadString,                    asynParamInt32, &P_FFTRead);
  createParam(FFTTimePerPointString,          asynParamFloat64, &P_FFTTimePerPoint);
  createParam(FFTTimeAxisString,         asynParamFloat64Array, &P_FFTTimeAxis);
  createParam(FFTFreqAxisString,         asynParamFloat64Array, &P_FFTFreqAxis);
  
  /* Per-signal parameters */
  createParam(FFTTimeSeriesString,       asynParamFloat64Array, &P_FFTTimeSeries);
  createParam(FFTRealString,             asynParamFloat64Array, &P_FFTReal);
  createParam(FFTImaginaryString,        asynParamFloat64Array, &P_FFTImaginary);
  createParam(FFTAbsValueString,         asynParamFloat64Array, &P_FFTAbsValue);
 
  /* Set the plugin type string */
  setStringParam(NDPluginDriverPluginType, "NDPluginFFT");
  
  /* Try to connect to the array port */
  connectToArrayPort();

  callParamCallbacks();
  
}

void NDPluginFFT::allocateArrays()
{

  if (timeSeries_)    free(timeSeries_);
  if (FFTReal_)       free(FFTReal_);
  if (FFTImaginary_)  free(FFTImaginary_);
  if (FFTAbsValue_)   free(FFTAbsValue_);

  numFreqPoints_ = numTimePoints_ / 2;

  timeSeries_   = (double *)calloc(maxSignals_*numTimePoints_, sizeof(double));
  FFTComplex_   = (double *)calloc(maxSignals_*numFreqPoints_, sizeof(double)*2);
  FFTReal_      = (double *)calloc(maxSignals_*numFreqPoints_, sizeof(double));
  FFTImaginary_ = (double *)calloc(maxSignals_*numFreqPoints_, sizeof(double));
  FFTAbsValue_  = (double *)calloc(maxSignals_*numFreqPoints_, sizeof(double));
  createAxisArrays();
}

void NDPluginFFT::zeroArrays()
{
  size_t freqSize = maxSignals_ * numFreqPoints_ * sizeof(double);

  memset(FFTComplex_,   0, freqSize * 2); // Complex data
  memset(FFTReal_,      0, freqSize);
  memset(FFTImaginary_, 0, freqSize);
  memset(FFTAbsValue_,  0, freqSize);
}

void NDPluginFFT::computeFFT_1D()
{
  int i, j;
  
  /* Compute the FFTs of each array */
  for (i=0; i<maxSignals_; i++) {
    for (j=0; j<numTimePoints_; j++) {
      FFTComplex_[2*j] = timeSeries_[i*numTimePoints_ + j];
      FFTComplex_[2*j+1] = 0.;
    }
    fft_1d(FFTComplex_-1, numTimePoints_, 1);
    /* Start at 1 so we don't copy DC offset which messes up scaling of plots */
    for (j=1; j<numFreqPoints_; j++) {
      FFTReal_     [i*numFreqPoints_ + j] = FFTComplex_[2*j]; 
      FFTImaginary_[i*numFreqPoints_ + j] = FFTComplex_[2*j+1]; 
      FFTAbsValue_ [i*numFreqPoints_ + j] = sqrt((FFTComplex_[2*j]   * FFTComplex_[2*j] + 
                                                  FFTComplex_[2*j+1] * FFTComplex_[2*j+1])
                                                 / (numTimePoints_ * numTimePoints_));
    }
  }
}         

void NDPluginFFT::createAxisArrays()
{
  double freqStep;
  int i;
  
  if (timeAxis_) free(timeAxis_);
  if (freqAxis_) free(freqAxis_);
  timeAxis_ = (double *)calloc(numTimePoints_, sizeof(double));
  freqAxis_ = (double *)calloc(numFreqPoints_, sizeof(double));

  for (i=0; i<numTimePoints_; i++) {
    timeAxis_[i] = i * timePerPoint_;
  }
  // Check this - are the frequencies correct, or off-by-one?
  freqStep = 0.5 / timePerPoint_ / (numFreqPoints_ - 2);
  for (i=0; i<numFreqPoints_; i++) {
    freqAxis_[i] = i * freqStep;
  }
  doCallbacksFloat64Array(timeAxis_, numTimePoints_, P_FFTTimeAxis, 0);
  doCallbacksFloat64Array(freqAxis_, numFreqPoints_, P_FFTFreqAxis, 0);
}

/**
 * Templated function to copy the data from the NDArray into double arrays.
 * \param[in] NDArray The pointer to the NDArray object
 * \return asynStatus
 */
template <typename epicsType>
asynStatus NDPluginFFT::convertToDoubleT(NDArray *pArray)
{
  epicsType *pData = (epicsType *)pArray->pData;
  int signal;
  int i;
    
  for (signal=0; signal<maxSignals_; signal++) {
    for (i=0; i<numTimePoints_; i++) {
      timeSeries_[signal*numTimePoints_ + i] = (epicsFloat64)*pData++;
    }
  }
  return asynSuccess;
}

     
/**
 * Call the templated convertToDouble so we can cast correctly. 
 * \param[in] NDArray The pointer to the NDArray object
 * \return asynStatus
 */
asynStatus NDPluginFFT::computeFFTs(NDArray *pArray)
{
  asynStatus status = asynSuccess;
  int signal;

  
  
  switch(pArray->dataType) {
  case NDInt8:
    status = convertToDoubleT<epicsInt8>(pArray);
    break;
  case NDUInt8:
    status = convertToDoubleT<epicsUInt8>(pArray);
    break;
  case NDInt16:
    status = convertToDoubleT<epicsInt16>(pArray);
    break;
  case NDUInt16:
    status = convertToDoubleT<epicsUInt16>(pArray);
    break;
  case NDInt32:
    status = convertToDoubleT<epicsInt32>(pArray);
    break;
  case NDUInt32:
    status = convertToDoubleT<epicsUInt32>(pArray);
    break;
  case NDFloat32:
    status = convertToDoubleT<epicsFloat32>(pArray);
    break;
  case NDFloat64:
    status = convertToDoubleT<epicsFloat64>(pArray);
    break;
  default:
    return asynError;
    break;
  }
  computeFFT_1D();

  /* Do array callbacks */
  for (signal=0; signal<maxSignals_; signal++) {
    doCallbacksFloat64Array(timeSeries_   + signal*numTimePoints_, numTimePoints_, P_FFTTimeSeries, signal);
    doCallbacksFloat64Array(FFTReal_      + signal*numFreqPoints_, numFreqPoints_, P_FFTReal,       signal);
    doCallbacksFloat64Array(FFTImaginary_ + signal*numFreqPoints_, numFreqPoints_, P_FFTImaginary,  signal);
    doCallbacksFloat64Array(FFTAbsValue_  + signal*numFreqPoints_, numFreqPoints_, P_FFTAbsValue,   signal);
  }

  return status;
}


/** 
 * Callback function that is called by the NDArray driver with new NDArray data.
 * Appends the new array data to the time series if possible.
 * \param[in] pArray The NDArray from the callback.
 */
void NDPluginFFT::processCallbacks(NDArray *pArray)
{
  //This function is called with the mutex already locked.  
  //It unlocks it during long calculations when private structures don't need to be protected.

  int numTimePoints;
  int numSignals;
  double timePerPoint;  
  const char* functionName = "NDPluginFFT::processCallbacks";

  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);

  // This plugin only works with 1-D or 2-D arrays
  if ((pArray->ndims < 1) || (pArray->ndims > 2)) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s: error, number of array dimensions must be 1 or 2\n",
      functionName);
    return;
  }

  numTimePoints = pArray->dims[0].size;
  if (pArray->ndims == 1) 
    numSignals = 1;
  else
    numSignals=pArray->dims[1].size;

  if (numSignals > maxSignals_) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s: warning, input array has %d signals, greater than maxSignals=%d\n",
      functionName, numSignals, maxSignals_);
    numSignals = maxSignals_;
  }
  
  if ((numTimePoints != numTimePoints_) ||
      (numSignals    != numSignals_)) {
    numTimePoints_ = numTimePoints;
    numSignals_ = numSignals;
    allocateArrays();
  }

  getDoubleParam(P_FFTTimePerPoint, &timePerPoint);
  if (timePerPoint != timePerPoint_) {
    timePerPoint_ = timePerPoint;
    createAxisArrays();
  }

  computeFFTs(pArray);

  callParamCallbacks();
}

/** Configuration command */
extern "C" int NDFFTConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr, 
                                     int maxSignals,
                                     int maxBuffers, size_t maxMemory,
                                     int priority, int stackSize)
{
    new NDPluginFFT(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 
                           maxSignals, maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
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
static const iocshFuncDef initFuncDef = {"NDFFTConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDFFTConfigure(args[0].sval, args[1].ival, args[2].ival,
                        args[3].sval, args[4].ival, args[5].ival,
                        args[6].ival, args[7].ival, args[8].ival, 
                        args[9].ival);
}

extern "C" void NDFFTRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFFTRegister);
}
