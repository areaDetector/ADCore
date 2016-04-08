/**
 * NDPluginTimeSeries.h
 *
 * Plugin that creates time-series arrays from callback data.
 * 
 * @author Mark Rivers 
 * @date February 2016
 */

#ifndef NDPluginTimeSeries_H
#define NDPluginTimeSeries_H

#include <epicsTypes.h>
#include <epicsTime.h>

#include "NDPluginDriver.h"


/* Per-plugin parameters */
#define TSAcquireString         "TS_ACQUIRE"          /* (asynInt32,        r/w) Acquire on/off */
#define TSReadString            "TS_READ"             /* (asynInt32,        r/w) Read data */
#define TSNumPointsString       "TS_NUM_POINTS"       /* (asynInt32,        r/w) Number of time series points to use */
#define TSCurrentPointString    "TS_CURRENT_POINT"    /* (asynInt32,        r/o) Current point in time series */
#define TSTimePerPointString    "TS_TIME_PER_POINT"   /* (asynFloat64,      r/o) Time per time point from driver */
#define TSAveragingTimeString   "TS_AVERAGING_TIME"   /* (asynFloat64,      r/o) Averaging time in plugin */
#define TSNumAverageString      "TS_NUM_AVERAGE"      /* (asynInt32,        r/o) Time points to average */
#define TSElapsedTimeString     "TS_ELAPSED_TIME"     /* (asynFloat64,      r/o) Elapsed acquisition time */
#define TSAcquireModeString     "TS_ACQUIRE_MODE"     /* (asynInt32,        r/w) Acquire mode */
#define TSTimeAxisString        "TS_TIME_AXIS"        /* (asynFloat64Array, r/o) Time axis array */
#define TSTimestampString       "TS_TIMESTAMP"        /* (asynFloat64Array, r/o) Series of timestamps */

/* Per-signal parameters */
#define TSTimeSeriesString      "TS_TIME_SERIES"      /* (asynFloat64Array, r/o) Time series array */


/** Compute time series on signals */
class epicsShareClass NDPluginTimeSeries : public NDPluginDriver {
public:
  NDPluginTimeSeries(const char *portName, int queueSize, int blockingCallbacks, 
                     const char *NDArrayPort, int NDArrayAddr, 
                     int maxSignals, int maxBuffers, size_t maxMemory,
                     int priority, int stackSize);

  //These methods override the virtual methods in the base class
  void processCallbacks(NDArray *pArray);
  asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

protected:

  // Per-plugin parameters
  int P_TSAcquire;
  #define FIRST_NDPLUGIN_TIME_SERIES_PARAM P_TSAcquire
  int P_TSRead;
  int P_TSNumPoints;
  int P_TSCurrentPoint;
  int P_TSTimePerPoint;
  int P_TSAveragingTime;
  int P_TSNumAverage;
  int P_TSElapsedTime;
  int P_TSAcquireMode;
  int P_TSTimeAxis;
  int P_TSTimestamp;

  // Per-signal parameters
  int P_TSTimeSeries;
  #define LAST_NDPLUGIN_TIME_SERIES_PARAM P_TSTimeSeries
                                
private:
  template <typename epicsType> asynStatus doAddToTimeSeriesT(NDArray *pArray);
  asynStatus addToTimeSeries(NDArray *pArray);
  asynStatus clear(epicsUInt32 roi);
  template <typename epicsType> void doTimeSeriesCallbacksT();
  asynStatus doTimeSeriesCallbacks();
  void allocateArrays();
  void acquireReset();
  void createAxisArray();
  void computeNumAverage();

  int maxSignals_;
  int numSignals_;
  int numSignalsIn_;
  NDDataType_t dataType_;
  int dataSize_;
  int numTimePoints_;
  int currentTimePoint_;
  int uniqueId_;
  int numAverage_;
  int numAveraged_;
  int acquireMode_;
  double averagingTimeRequested_;
  double averagingTimeActual_;
  double timePerPoint_; /* Actual time between points in input arrays */
  epicsTimeStamp startTime_;
  double *averageStore_;
  double *signalData_;
  double *timeAxis_;
  double *timeStamp_;
  NDArray *pTimeCircular_;
};

#define NUM_NDPLUGIN_TIME_SERIES_PARAMS (int)(&LAST_NDPLUGIN_TIME_SERIES_PARAM - &FIRST_NDPLUGIN_TIME_SERIES_PARAM + 1)
    
#endif //NDPluginTimeSeries_H
