/**
 * NDPluginFFT.h
 *
 * Computes the FFT of 1-D or 2-D data. 
 * 
 * @author Mark Rivers 
 * @date February 2016
 */

#ifndef NDPluginFFT_H
#define NDPluginFFT_H

#include <epicsTypes.h>
#include <epicsTime.h>

#include "NDPluginDriver.h"

#define FFTTimeAxisString        "FFT_TIME_AXIS"        /* (asynFloat64Array, r/o) Time axis array */
#define FFTFreqAxisString        "FFT_FREQ_AXIS"        /* (asynFloat64Array, r/o) Frequency axis array */
#define FFTTimePerPointString    "FFT_TIME_PER_POINT"   /* (asynFloat64,      r/o) Time per time point from driver */
#define FFTDirectionString       "FFT_DIRECTION"        /* (asynInt32,        r/w) FFT direction */
#define FFTSuppressDCString      "FFT_SUPPRESS_DC"      /* (asynInt32,        r/w) FFT DC offset suppression */
#define FFTNumAverageString      "FFT_NUM_AVERAGE"      /* (asynInt32,        r/w) # of FFTs to average */
#define FFTNumAveragedString     "FFT_NUM_AVERAGED"     /* (asynInt32,        r/o) # of FFTs averaged */
#define FFTResetAverageString    "FFT_RESET_AVERAGE"    /* (asynInt32,        r/w) Reset FFT average */
#define FFTTimeSeriesString      "FFT_TIME_SERIES"      /* (asynFloat64Array, r/o) Time series data */
#define FFTRealString            "FFT_REAL"             /* (asynFloat64Array, r/o) Real part of FFT */
#define FFTImaginaryString       "FFT_IMAGINARY"        /* (asynFloat64Array, r/o) Imaginary part of FFT */
#define FFTAbsValueString        "FFT_ABS_VALUE"        /* (asynFloat64Array, r/o) Absolute value of FFT */

/** Compute FFTs on signals */
class epicsShareClass NDPluginFFT : public NDPluginDriver {
public:
  NDPluginFFT(const char *portName, int queueSize, int blockingCallbacks, 
              const char *NDArrayPort, int NDArrayAddr, 
              int maxBuffers, size_t maxMemory,
              int priority, int stackSize);

  //These methods override the virtual methods in the base class
  void processCallbacks(NDArray *pArray);

protected:

  int P_FFTTimeAxis;
  #define FIRST_NDPLUGIN_FFT_PARAM P_FFTTimeAxis
  int P_FFTFreqAxis;
  int P_FFTTimePerPoint;
  int P_FFTDirection;
  int P_FFTSuppressDC;
  int P_FFTNumAverage;
  int P_FFTNumAveraged;
  int P_FFTResetAverage;

  int P_FFTTimeSeries;
  int P_FFTReal;
  int P_FFTImaginary;
  int P_FFTAbsValue;
  #define LAST_NDPLUGIN_FFT_PARAM P_FFTAbsValue
                                
private:
  template <typename epicsType> void convertToDoubleT(NDArray *pArray);
  void allocateArrays();
  void createAxisArrays();
  void computeFFT_1D();
  void computeFFT_2D();
  void doArrayCallbacks();
  int nextPow2(int v);

  int rank_;
  int uniqueId_;
  int nTimeXIn_;
  int nTimeYIn_;
  int nTimeX_;
  int nTimeY_;
  int nFreqX_;
  int nFreqY_;
  int suppressDC_;
  int numAverage_;
  double oldFraction_;
  double newFraction_;
  double timePerPoint_; /* Actual time between points in input arrays */
  double *timeAxis_;
  double *freqAxis_;
  double *timeSeries_;
  double *FFTComplex_;
  double *FFTReal_;
  double *FFTImaginary_;
  double *FFTAbsValue_;
};

#define NUM_NDPLUGIN_FFT_PARAMS (int)(&LAST_NDPLUGIN_FFT_PARAM - &FIRST_NDPLUGIN_FFT_PARAM + 1)
    
#endif //NDPluginFFT_H
