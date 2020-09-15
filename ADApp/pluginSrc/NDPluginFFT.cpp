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

#include <asynPortDriver.h>

#include <NDArray.h>

#include <epicsExport.h>

#include "NDPluginFFT.h"
#include "fft.h"

#define MIN(A,B) ((A <= B) ? A : B)

//static const char *driverName = "NDPluginFFT";

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
  * \param[in] maxThreads The maximum number of threads this driver is allowed to use. If 0 then 1 will be used.
  */
NDPluginFFT::NDPluginFFT(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize, int maxThreads)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
             NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             0, 1, priority, stackSize, maxThreads),
    numAverage_(0), uniqueId_(0), nTimeXIn_(0), nTimeYIn_(0), FFTAbsValue_(0), timePerPoint_(0), timeAxis_(0), freqAxis_(0)
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

void NDPluginFFT::allocateArrays(fftPvt_t *pPvt, bool sizeChanged)
{
  // Round dimensions up to next power of 2
  pPvt->nTimeX = nextPow2(pPvt->nTimeXIn);
  pPvt->nTimeY = nextPow2(pPvt->nTimeYIn);

  pPvt->nFreqX = pPvt->nTimeX / 2;
  pPvt->nFreqY = pPvt->nTimeY / 2;
  if (pPvt->nFreqY < 1) pPvt->nFreqY = 1;

  size_t timeSize = pPvt->nTimeX * pPvt->nTimeY;
  size_t freqSize = pPvt->nFreqX * pPvt->nFreqY;
  pPvt->timeSeries   = (double *)calloc(timeSize, sizeof(double));
  pPvt->FFTComplex   = (double *)calloc(timeSize, sizeof(double) * 2); // Complex data
  pPvt->FFTReal      = (double *)calloc(freqSize, sizeof(double));
  pPvt->FFTImaginary = (double *)calloc(freqSize, sizeof(double));
  pPvt->FFTAbsValue  = (double *)calloc(freqSize, sizeof(double));
  if (sizeChanged) {
    if (FFTAbsValue_) {
      free(FFTAbsValue_);
    }
    FFTAbsValue_ = (double *)calloc(freqSize, sizeof(double));
    nFreqX_ = pPvt->nFreqX;
    nFreqY_ = pPvt->nFreqY;
    createAxisArrays(pPvt);
  }
}

void NDPluginFFT::computeFFT_1D(fftPvt_t *pPvt)
{
  int j;

  for (j=0; j<pPvt->nTimeX; j++) {
    pPvt->FFTComplex[2*j] = pPvt->timeSeries[j];
    pPvt->FFTComplex[2*j+1] = 0.;
  }
  fft_1D(pPvt->FFTComplex, pPvt->nTimeX, 1);
  for (j=0; j<pPvt->nFreqX; j++) {
    pPvt->FFTReal     [j] = pPvt->FFTComplex[2*j];
    pPvt->FFTImaginary[j] = pPvt->FFTComplex[2*j+1];
    pPvt->FFTAbsValue [j] = sqrt((pPvt->FFTComplex[2*j]   * pPvt->FFTComplex[2*j] +
                            pPvt->FFTComplex[2*j+1] * pPvt->FFTComplex[2*j+1])) / pPvt->nTimeX;
  }
  if (pPvt->suppressDC) {
    pPvt->FFTReal      [0] = 0;
    pPvt->FFTImaginary [0] = 0;
    pPvt->FFTAbsValue  [0] = 0;
  }
}

void NDPluginFFT::computeFFT_2D(fftPvt_t *pPvt)
{
  int i,j, k;
  double *pIn;
  unsigned long dims[2];

  for (j=0; j<pPvt->nTimeX*pPvt->nTimeY; j++) {
    pPvt->FFTComplex[2*j] = pPvt->timeSeries[j];
    pPvt->FFTComplex[2*j+1] = 0.;
  }
  dims[0] = pPvt->nTimeX;
  dims[1] = pPvt->nTimeY;
  fft_ND(pPvt->FFTComplex, dims, 2, 1);
  for (i=0, k=0, pIn=pPvt->FFTComplex;
       i<pPvt->nFreqY;
       i++, pIn+=pPvt->nTimeX*2) {
    for (j=0; j<pPvt->nFreqX; j++, k++) {
      pPvt->FFTReal     [k] = pIn[j*2];
      pPvt->FFTImaginary[k] = pIn[j*2+1];
      pPvt->FFTAbsValue [k]= sqrt((pPvt->FFTReal[k] * pPvt->FFTReal[k]) + (pPvt->FFTImaginary[k] * pPvt->FFTImaginary[k])) / (pPvt->nTimeX * pPvt->nTimeY);
    }
  }
  if (pPvt->suppressDC) {
    pPvt->FFTReal      [0] = 0;
    pPvt->FFTImaginary [0] = 0;
    pPvt->FFTAbsValue  [0] = 0;
  }
}

void NDPluginFFT::doArrayCallbacks(fftPvt_t *pPvt)
{
  int j;
  size_t dims[2];
  epicsTimeStamp now;
  double *pOut;
  double oldFraction, newFraction;
  int numAverage;
  int numAveraged;
  int resetAverage;
  int freqSize;
  int arrayCallbacks;
  NDArray *pArrayOut;

  getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
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

  oldFraction = 1. - 1./numAveraged;
  newFraction = 1./numAveraged;
  if (numAveraged < numAverage) numAveraged++;
  setIntegerParam(P_FFTNumAveraged, numAveraged);
  // We need to use the smaller of FFTAbsValue_ and this array in case it changed when we were computing
  freqSize = MIN(pPvt->nFreqX * pPvt->nFreqY, nFreqX_ * nFreqY_);
  for (j=0; j < freqSize; j++) {
    FFTAbsValue_[j] = FFTAbsValue_[j] * oldFraction + pPvt->FFTAbsValue[j] * newFraction;
  }
  if (arrayCallbacks) {
    dims[0] = pPvt->nFreqX;
    dims[1] = pPvt->nFreqY;
    pArrayOut = pNDArrayPool->alloc(pPvt->rank, dims, NDFloat64, 0, 0);
    pOut=(double *)pArrayOut->pData;
    memcpy(pOut, FFTAbsValue_, freqSize * sizeof(double));
    this->getAttributes(pArrayOut->pAttributeList);
    getTimeStamp(&pArrayOut->epicsTS);
    epicsTimeGetCurrent(&now);
    pArrayOut->timeStamp = now.secPastEpoch + now.nsec / 1.e9;
    pArrayOut->uniqueId = uniqueId_++;
    NDPluginDriver::endProcessCallbacks(pArrayOut, false, false);
  }

  /* Do waveform callbacks.  This only does the first row for 2-D FFTs. */
  doCallbacksFloat64Array(pPvt->timeSeries,   pPvt->nTimeX, P_FFTTimeSeries, 0);
  doCallbacksFloat64Array(pPvt->FFTReal,      pPvt->nFreqX, P_FFTReal,       0);
  doCallbacksFloat64Array(pPvt->FFTImaginary, pPvt->nFreqX, P_FFTImaginary,  0);
  doCallbacksFloat64Array(FFTAbsValue_,       MIN(pPvt->nFreqX, nFreqX_), P_FFTAbsValue,   0);
  free(pPvt->timeSeries);
  free(pPvt->FFTComplex);
  free(pPvt->FFTReal);
  free(pPvt->FFTImaginary);
  free(pPvt->FFTAbsValue);
}

void NDPluginFFT::createAxisArrays(fftPvt_t *pPvt)
{
  double freqStep;
  int i;

  if (timeAxis_) free(timeAxis_);
  if (freqAxis_) free(freqAxis_);
  timeAxis_ = (double *)calloc(pPvt->nTimeX, sizeof(double));
  freqAxis_ = (double *)calloc(pPvt->nFreqX, sizeof(double));

  for (i=0; i<pPvt->nTimeX; i++) {
    timeAxis_[i] = i * timePerPoint_;
  }
  // Check this - are the frequencies correct, or off-by-one?
  freqStep = 0.5 / timePerPoint_ / (pPvt->nFreqX - 1);
  for (i=0; i<pPvt->nFreqX; i++) {
    freqAxis_[i] = i * freqStep;
  }
  doCallbacksFloat64Array(timeAxis_, pPvt->nTimeX, P_FFTTimeAxis, 0);
  doCallbacksFloat64Array(freqAxis_, pPvt->nFreqX, P_FFTFreqAxis, 0);
}

/**
 * Templated function to copy the data from the NDArray into double arrays with padding.
 * \param[in] pArray The pointer to the NDArray object
 * \param[in] pPvt Private pointer for FFT plugin
 */
template <typename epicsType>
void NDPluginFFT::convertToDoubleT(NDArray *pArray, fftPvt_t *pPvt)
{
  epicsType *pIn;
  double *pOut;
  int i, j;

  for (i=0, pIn=(epicsType *)pArray->pData, pOut=pPvt->timeSeries;
       i<pPvt->nTimeYIn;
       i++, pOut+=pPvt->nTimeX) {
    for (j=0; j<pPvt->nTimeXIn; j++) {
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

  double timePerPoint;
  fftPvt_t *pPvt = new fftPvt_t;
  bool sizeChanged = false;
  const char* functionName = "NDPluginFFT::processCallbacks";

  /* Call the base class method */
  NDPluginDriver::beginProcessCallbacks(pArray);

  // This plugin only works with 1-D or 2-D arrays
  switch (pArray->ndims) {
    case 1:
      pPvt->rank = 1;
      pPvt->nTimeXIn = (int)pArray->dims[0].size;
      pPvt->nTimeYIn = 1;
      break;
    case 2:
      pPvt->rank = 2;
      pPvt->nTimeXIn = (int)pArray->dims[0].size;
      pPvt->nTimeYIn = (int)pArray->dims[1].size;
      break;
    default:
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s: error, number of array dimensions must be 1 or 2\n",
        functionName);
      return;
      break;
  }

  if ((pPvt->nTimeXIn != nTimeXIn_) ||
      (pPvt->nTimeYIn != nTimeYIn_)) {
    sizeChanged = true;
    nTimeXIn_ = pPvt->nTimeXIn;
    nTimeYIn_ = pPvt->nTimeYIn;
  }

  getIntegerParam(P_FFTSuppressDC, &pPvt->suppressDC);

  allocateArrays(pPvt, sizeChanged);
  getDoubleParam(P_FFTTimePerPoint, &timePerPoint);
  if (timePerPoint != timePerPoint_) {
    timePerPoint_ = timePerPoint;
    createAxisArrays(pPvt);
  }

  // Release the lock, things below don't access shared memory
  this->unlock();
  switch(pArray->dataType) {
  case NDInt8:
    convertToDoubleT<epicsInt8>(pArray, pPvt);
    break;
  case NDUInt8:
    convertToDoubleT<epicsUInt8>(pArray, pPvt);
    break;
  case NDInt16:
    convertToDoubleT<epicsInt16>(pArray, pPvt);
    break;
  case NDUInt16:
    convertToDoubleT<epicsUInt16>(pArray, pPvt);
    break;
  case NDInt32:
    convertToDoubleT<epicsInt32>(pArray, pPvt);
    break;
  case NDUInt32:
    convertToDoubleT<epicsUInt32>(pArray, pPvt);
    break;
  case NDInt64:
    convertToDoubleT<epicsInt64>(pArray, pPvt);
    break;
  case NDUInt64:
    convertToDoubleT<epicsUInt64>(pArray, pPvt);
    break;
  case NDFloat32:
    convertToDoubleT<epicsFloat32>(pArray, pPvt);
    break;
  case NDFloat64:
    convertToDoubleT<epicsFloat64>(pArray, pPvt);
    break;
  default:
    break;
  }
  if (pPvt->rank == 1) computeFFT_1D(pPvt);
  if (pPvt->rank == 2) computeFFT_2D(pPvt);

  // Take the lock again
  this->lock();
  doArrayCallbacks(pPvt);
  delete pPvt;
  callParamCallbacks();
}

/** Configuration command */
extern "C" int NDFFTConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr,
                                     int maxBuffers, size_t maxMemory,
                                     int priority, int stackSize, int maxThreads)
{
    NDPluginFFT *pPlugin = new NDPluginFFT(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshFuncDef initFuncDef = {"NDFFTConfigure", 10, initArgs};
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
