/**
 * NDPluginTimeSeries.h
 *
 * Plugin that creates time-series arrays from callback data.
 * Optionally computes the FFT of the time-series data. 
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
#define TSTimePerPointString    "TS_TIME_PER_POINT"   /* (asynFloat64,      r/o) Time per time point */
#define TSNumAverageString      "TS_NUM_AVERAGE"      /* (asynInt32,        r/o) Time points to average */
#define TSElapsedTimeString     "TS_ELAPSED_TIME"     /* (asynFloat64,      r/o) Elapsed acquisition time */
#define TSAcquiringString       "TS_ACQUIRING"        /* (asynInt32,        r/o) Acquiring time series */
#define TSAcquireModeString     "TS_ACQUIRE_MODE"     /* (asynInt32,        r/w) Acquire mode */
#define TSComputeFFTString      "TS_COMPUTE_FFT"      /* (asynInt32,        r/w) Compute FFTs */
#define TSTimeAxisString        "TS_TIME_AXIS"        /* (asynFloat64Array, r/o) Time axis array */
#define TSFreqAxisString        "TS_FREQ_AXIS"        /* (asynFloat64Array, r/o) Frequency axis array */
#define TSTimestampString       "TS_TIMESTAMP"        /* (asynFloat64Array, r/o) Series of timestamps */

/* Per-signal parameters */
#define TSSignalNameString      "TS_SIGNAL_NAME"      /* (asynOctet,        r/w) Name of this signal */
#define TSSignalUseString       "TS_SIGNAL_USE"       /* (asynInt32,        r/w) Use this signal? */
#define TSTimeSeriesString      "TS_TIME_SERIES"      /* (asynFloat64Array, r/o) Time series array */
#define TSFFTRealString         "TS_FFT_REAL"         /* (asynFloat64Array, r/o) Real part of FFT */
#define TSFFTImaginaryString    "TS_FFT_IMAGINARY"    /* (asynFloat64Array, r/o) Imaginary part of FFT */
#define TSFFTAbsValueString     "TS_FFT_ABS_VALUE"    /* (asynFloat64Array, r/o) Absolute value of FFT */

enum {
    TSEraseStart,
    TSStart,
    TSStop,
    TSRead
};

enum {
    TSAcquireModeFixed,
    TSAcquireModeCircular
};



/** Compute time series and FFTs on signals */
class epicsShareClass NDPluginTimeSeries : public NDPluginDriver {
public:
    NDPluginTimeSeries(const char *portName, int queueSize, int blockingCallbacks, 
                       const char *NDArrayPort, int NDArrayAddr, 
                       int maxSignals, const char *drvInfoTimePerPoint,
                       int maxBuffers, size_t maxMemory,
                       int priority, int stackSize);
    
    //These methods override the virtual methods in the base class
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    // Need to be public because called from C
    void timePerPointCallback(double seconds);

protected:

    // Per-plugin parameters
    int P_TSAcquire;
    #define FIRST_NDPLUGIN_TIME_SERIES_PARAM P_TSAcquire
    int P_TSRead;
    int P_TSNumPoints;
    int P_TSCurrentPoint;
    int P_TSTimePerPoint;
    int P_TSNumAverage;
    int P_TSElapsedTime;
    int P_TSAcquiring;
    int P_TSAcquireMode;
    int P_TSComputeFFT;
    int P_TSTimeAxis;
    int P_TSFreqAxis;
    int P_TSTimestamp;

    // Per-signal parameters
    int P_TSSignalName;
    int P_TSSignalUse;
    int P_TSTimeSeries;
    int P_TSFFTReal;
    int P_TSFFTImaginary;
    int P_TSFFTAbsValue;
    #define LAST_NDPLUGIN_TIME_SERIES_PARAM P_TSFFTAbsValue
                                
private:
    template <typename epicsType> asynStatus doAddToTimeSeriesT(NDArray *pArray);
    asynStatus addToTimeSeries(NDArray *pArray);
    asynStatus clear(epicsUInt32 roi);
    void doTimeSeriesCallbacks();
    void allocateArrays();
    void zeroArrays();
    void createAxisArrays();
    void computeFFTs();
    void computeNumAverage();

    asynUser *pasynUserInput_;
    asynUser *pasynUserFloat64SyncIO_;
    int maxSignals_;
    int numTimePoints_;
    int numFreqPoints_;
    int currentTimePoint_;
    int numAverage_;
    int numAveraged_;
    int acquireMode_;
    double timePerPointRequested_;
    double timePerPointActual_;
    double timePerPointInput_; /* Actual time between points in callbacks */
    epicsTimeStamp startTime_;
    double *averageStore_;
    double *timeAxis_;
    double *freqAxis_;
    double *timeStamp_;
    double *timeSeries_;
    double *timeCircular_;
    double *FFTComplex_;
    double *FFTReal_;
    double *FFTImaginary_;
    double *FFTAbsValue_;
    void *float64RegistrarPvt_;
};

#define NUM_NDPLUGIN_TIME_SERIES_PARAMS (int)(&LAST_NDPLUGIN_TIME_SERIES_PARAM - &FIRST_NDPLUGIN_TIME_SERIES_PARAM + 1)
    
#endif //NDPluginTimeSeries_H
