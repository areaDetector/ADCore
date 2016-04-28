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

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsTime.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <NDArray.h>

#include <epicsExport.h>

#include "NDPluginFFT.h"
#include "fft.h"

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
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginFFT::NDPluginFFT(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, 
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
             NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_FFT_PARAMS, maxBuffers, maxMemory,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             0, 1, priority, stackSize),
    uniqueId_(0), timePerPoint_(0), timeAxis_(0), freqAxis_(0), timeSeries_(0), 
    FFTReal_(0), FFTImaginary_(0), FFTAbsValue_(0)
{
  //const char *functionName = "NDPluginFFT::NDPluginFFT";

  createParam(FFTTimeAxisString,         asynParamFloat64Array, &P_FFTTimeAxis);
  createParam(FFTFreqAxisString,         asynParamFloat64Array, &P_FFTFreqAxis);
  createParam(FFTTimePerPointString,          asynParamFloat64, &P_FFTTimePerPoint);
  createParam(FFTDirectionString,               asynParamInt32, &P_FFTDirection);
  createParam(FFTSuppressDCString,              asynParamInt32, &P_FFTSuppressDC);
  createParam(FFTNumAverageString,              asynParamInt32, &P_FFTNumAverage);
  createParam(FFTNumAveragedString,             asynParamInt32, &P_FFTNumAveraged);
  createParam(FFTResetAverageString,            asynParamInt32, &P_FFTResetAverage);
  
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

int NDPluginFFT::nextPow2(int v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

void NDPluginFFT::allocateArrays()
{
  if (timeSeries_)    free(timeSeries_);
  if (FFTReal_)       free(FFTReal_);
  if (FFTImaginary_)  free(FFTImaginary_);
  if (FFTAbsValue_)   free(FFTAbsValue_);

  // Round dimensions up to next power of 2
  nTimeX_ = nextPow2(nTimeXIn_);
  nTimeY_ = nextPow2(nTimeYIn_);

  nFreqX_ = nTimeX_ / 2;
  nFreqY_ = nTimeY_ / 2;
  if (nFreqY_ < 1) nFreqY_ = 1;

  size_t timeSize = nTimeX_ * nTimeY_;
  size_t freqSize = nFreqX_ * nFreqY_;
  timeSeries_   = (double *)calloc(timeSize, sizeof(double));
  FFTComplex_   = (double *)calloc(timeSize, sizeof(double) * 2); // Complex data
  FFTReal_      = (double *)calloc(freqSize, sizeof(double));
  FFTImaginary_ = (double *)calloc(freqSize, sizeof(double));
  FFTAbsValue_  = (double *)calloc(freqSize, sizeof(double));
  createAxisArrays();
}

void NDPluginFFT::computeFFT_1D()
{
  int j;
  double newAbsValue;

  for (j=0; j<nTimeX_; j++) {
    FFTComplex_[2*j] = timeSeries_[j];
    FFTComplex_[2*j+1] = 0.;
  }
  fft_1D(FFTComplex_, nTimeX_, 1);
  for (j=0; j<nFreqX_; j++) {
    FFTReal_     [j] = FFTComplex_[2*j]; 
    FFTImaginary_[j] = FFTComplex_[2*j+1]; 
    newAbsValue      = sqrt((FFTComplex_[2*j]   * FFTComplex_[2*j] + 
                             FFTComplex_[2*j+1] * FFTComplex_[2*j+1])) / nTimeX_;
    FFTAbsValue_ [j] = FFTAbsValue_[j] * oldFraction_ + newAbsValue * newFraction_;
  }
  if (suppressDC_) {
    FFTReal_      [0] = 0;
    FFTImaginary_ [0] = 0;
    FFTAbsValue_  [0] = 0;
  }
  doArrayCallbacks();
}         

void NDPluginFFT::computeFFT_2D()
{
  int i,j, k;
  double *pIn;
  unsigned long dims[2];
  double newAbsValue;
 
  for (j=0; j<nTimeX_*nTimeY_; j++) {
    FFTComplex_[2*j] = timeSeries_[j];
    FFTComplex_[2*j+1] = 0.;
  }
  dims[0] = nTimeX_;
  dims[1] = nTimeY_;
  unlock();
  fft_ND(FFTComplex_, dims, 2, 1);
  lock();
  for (i=0, k=0, pIn=FFTComplex_; 
       i<nFreqY_; 
       i++, pIn+=nTimeX_*2) {
    for (j=0; j<nFreqX_; j++, k++) {
      FFTReal_     [k] = pIn[j*2]; 
      FFTImaginary_[k] = pIn[j*2+1]; 
      newAbsValue      = sqrt((FFTReal_[k] * FFTReal_[k]) + (FFTImaginary_[k] * FFTImaginary_[k])) / (nTimeX_ * nTimeY_);
      FFTAbsValue_ [k] = FFTAbsValue_[k] * oldFraction_ + newAbsValue * newFraction_;
    }
  }
  if (suppressDC_) {
    FFTReal_      [0] = 0;
    FFTImaginary_ [0] = 0;
    FFTAbsValue_  [0] = 0;
  }
  doArrayCallbacks();
}         

void NDPluginFFT::doArrayCallbacks()
{
  int j; 
  size_t dims[2];
  epicsTimeStamp now;
  double *pIn, *pOut;
  int arrayCallbacks;
  NDArray *pArrayOut = this->pArrays[0];

  getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
  if (arrayCallbacks) {
    if (pArrayOut) pArrayOut->release();
    dims[0] = nFreqX_;
    dims[1] = nFreqY_;
    pArrayOut = pNDArrayPool->alloc(rank_, dims, NDFloat64, 0, 0);
    for (j=0, pIn=FFTAbsValue_, pOut=(double *)pArrayOut->pData; 
         j<nFreqY_; 
         j++, pIn+=nFreqX_, pOut+=nFreqX_) {
      memcpy(pOut, pIn, nFreqX_*sizeof(double));
    }
    this->getAttributes(pArrayOut->pAttributeList);
    getTimeStamp(&pArrayOut->epicsTS);
    epicsTimeGetCurrent(&now);
    pArrayOut->timeStamp = now.secPastEpoch + now.nsec / 1.e9;
    pArrayOut->uniqueId = uniqueId_++;
    this->unlock();
    doCallbacksGenericPointer(pArrayOut, NDArrayData, 0);
    this->lock();
    this->pArrays[0] = pArrayOut;
  }

  /* Do waveform callbacks.  This only does the first row for 2-D FFTs. */
  doCallbacksFloat64Array(timeSeries_,   nTimeX_, P_FFTTimeSeries, 0);
  doCallbacksFloat64Array(FFTReal_,      nFreqX_, P_FFTReal,       0);
  doCallbacksFloat64Array(FFTImaginary_, nFreqX_, P_FFTImaginary,  0);
  doCallbacksFloat64Array(FFTAbsValue_,  nFreqX_, P_FFTAbsValue,   0);
}

void NDPluginFFT::createAxisArrays()
{
  double freqStep;
  int i;
  
  if (timeAxis_) free(timeAxis_);
  if (freqAxis_) free(freqAxis_);
  timeAxis_ = (double *)calloc(nTimeX_, sizeof(double));
  freqAxis_ = (double *)calloc(nFreqX_, sizeof(double));

  for (i=0; i<nTimeX_; i++) {
    timeAxis_[i] = i * timePerPoint_;
  }
  // Check this - are the frequencies correct, or off-by-one?
  freqStep = 0.5 / timePerPoint_ / (nFreqX_ - 1);
  for (i=0; i<nFreqX_; i++) {
    freqAxis_[i] = i * freqStep;
  }
  doCallbacksFloat64Array(timeAxis_, nTimeX_, P_FFTTimeAxis, 0);
  doCallbacksFloat64Array(freqAxis_, nFreqX_, P_FFTFreqAxis, 0);
}

/**
 * Templated function to copy the data from the NDArray into double arrays with padding.
 * \param[in] NDArray The pointer to the NDArray object
 */
template <typename epicsType>
void NDPluginFFT::convertToDoubleT(NDArray *pArray)
{
  epicsType *pIn;
  double *pOut;
  int i, j;
    
  for (i=0, pIn=(epicsType *)pArray->pData, pOut=timeSeries_;
       i<nTimeYIn_; 
       i++, pOut+=nTimeX_) {
    for (j=0; j<nTimeXIn_; j++) {
      pOut[j] = (epicsFloat64)*pIn++;
    }
  }
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

  int nTimeX, nTimeY;
  int numAverage;
  int numAveraged;
  int resetAverage;
  double timePerPoint;  
  const char* functionName = "NDPluginFFT::processCallbacks";

  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);

  // This plugin only works with 1-D or 2-D arrays
  switch (pArray->ndims) {
    case 1:
      rank_ = 1;
      nTimeX = (int)pArray->dims[0].size;
      nTimeY = 1;
      break;
    case 2:
      rank_ = 2;
      nTimeX = (int)pArray->dims[0].size;
      nTimeY = (int)pArray->dims[1].size;
      break;
    default:
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s: error, number of array dimensions must be 1 or 2\n",
        functionName);
      return;
      break;
  }
  if ((nTimeX != nTimeXIn_) ||
      (nTimeY != nTimeYIn_)) {
    nTimeXIn_ = nTimeX;
    nTimeYIn_ = nTimeY;
    allocateArrays();
  }

  getDoubleParam(P_FFTTimePerPoint, &timePerPoint);
  if (timePerPoint != timePerPoint_) {
    timePerPoint_ = timePerPoint;
    createAxisArrays();
  }

  getIntegerParam(P_FFTSuppressDC,   &suppressDC_);
  getIntegerParam(P_FFTResetAverage, &resetAverage);
  getIntegerParam(P_FFTNumAverage,   &numAverage);
  getIntegerParam(P_FFTNumAveraged,  &numAveraged);
  if (resetAverage) {
    setIntegerParam(P_FFTResetAverage, 0);
    numAveraged = 1;
  }
  if (numAverage != numAverage_) {
    numAverage_ = numAverage;
    numAveraged = 1;
  }
  
  oldFraction_ = 1. - 1./numAveraged;
  newFraction_ = 1./numAveraged;
  if (numAveraged < numAverage) numAveraged++;
  setIntegerParam(P_FFTNumAveraged, numAveraged);

  switch(pArray->dataType) {
  case NDInt8:
    convertToDoubleT<epicsInt8>(pArray);
    break;
  case NDUInt8:
    convertToDoubleT<epicsUInt8>(pArray);
    break;
  case NDInt16:
    convertToDoubleT<epicsInt16>(pArray);
    break;
  case NDUInt16:
    convertToDoubleT<epicsUInt16>(pArray);
    break;
  case NDInt32:
    convertToDoubleT<epicsInt32>(pArray);
    break;
  case NDUInt32:
    convertToDoubleT<epicsUInt32>(pArray);
    break;
  case NDFloat32:
    convertToDoubleT<epicsFloat32>(pArray);
    break;
  case NDFloat64:
    convertToDoubleT<epicsFloat64>(pArray);
    break;
  default:
    break;
  }
  if (rank_ == 1) computeFFT_1D();
  if (rank_ == 2) computeFFT_2D();

  callParamCallbacks();
}

/** Configuration command */
extern "C" int NDFFTConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr, 
                                     int maxBuffers, size_t maxMemory,
                                     int priority, int stackSize)
{
    NDPluginFFT *pPlugin = new NDPluginFFT(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 
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
static const iocshFuncDef initFuncDef = {"NDFFTConfigure", 9, initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDFFTConfigure(args[0].sval, args[1].ival, args[2].ival,
                        args[3].sval, args[4].ival, args[5].ival,
                        args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDFFTRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFFTRegister);
}
