#ifndef NDPluginStats_H
#define NDPluginStats_H

#include <epicsTypes.h>

#include "NDPluginDriver.h"

typedef struct NDStats {
    size_t  nElements;
    double  total;
    double  net;
    double  mean;
    double  sigma;
    double  min;
    size_t  minX;
    size_t  minY;    
    double  max;
    size_t  maxX;
    size_t  maxY;
} NDStats_t;

typedef enum {
    profAverage,
    profThreshold,
    profCentroid,
    profCursor
} NDStatProfileType;
#define MAX_PROFILE_TYPES profCursor+1

typedef enum {
    TSMinValue,
    TSMinX,
    TSMinY,    
    TSMaxValue,
    TSMaxX,
    TSMaxY,
    TSMeanValue,
    TSSigmaValue,
    TSTotal,
    TSNet,
    TSCentroidX,
    TSCentroidY,
    TSSigmaX,
    TSSigmaY,
    TSSigmaXY,
    TSTimestamp
} NDStatTSType;
#define MAX_TIME_SERIES_TYPES TSTimestamp+1

typedef enum {
    TSEraseStart,
    TSStart,
    TSStop,
    TSRead
} NDStatsTSControl_t;

/* Statistics */
#define NDPluginStatsComputeStatisticsString  "COMPUTE_STATISTICS"  /* (asynInt32,        r/w) Compute statistics? */
#define NDPluginStatsBgdWidthString           "BGD_WIDTH"           /* (asynInt32,        r/w) Width of background region when computing net */
#define NDPluginStatsMinValueString           "MIN_VALUE"           /* (asynFloat64,      r/o) Minimum counts in any element */
#define NDPluginStatsMinXString               "MIN_X"               /* (asynFloat64,      r/o) X position of minimum counts */
#define NDPluginStatsMinYString               "MIN_Y"               /* (asynFloat64,      r/o) Y position of minimum counts */
#define NDPluginStatsMaxValueString           "MAX_VALUE"           /* (asynFloat64,      r/o) Maximum counts in any element */
#define NDPluginStatsMaxXString               "MAX_X"               /* (asynFloat64,      r/o) X position of maximum counts */
#define NDPluginStatsMaxYString               "MAX_Y"               /* (asynFloat64,      r/o) Y position of maximum counts */
#define NDPluginStatsMeanValueString          "MEAN_VALUE"          /* (asynFloat64,      r/o) Mean counts of all elements */
#define NDPluginStatsSigmaValueString         "SIGMA_VALUE"         /* (asynFloat64,      r/o) Sigma of all elements */
#define NDPluginStatsTotalString              "TOTAL"               /* (asynFloat64,      r/o) Sum of all elements */
#define NDPluginStatsNetString                "NET"                 /* (asynFloat64,      r/o) Sum of all elements minus background */

/* Centroid */
#define NDPluginStatsComputeCentroidString    "COMPUTE_CENTROID"    /* (asynInt32,        r/w) Compute centroid? */
#define NDPluginStatsCentroidThresholdString  "CENTROID_THRESHOLD"  /* (asynFloat64,      r/w) Threshold when computing centroids */
#define NDPluginStatsCentroidXString          "CENTROIDX_VALUE"     /* (asynFloat64,      r/o) X centroid */
#define NDPluginStatsCentroidYString          "CENTROIDY_VALUE"     /* (asynFloat64,      r/o) Y centroid */
#define NDPluginStatsSigmaXString             "SIGMAX_VALUE"        /* (asynFloat64,      r/o) Sigma X */
#define NDPluginStatsSigmaYString             "SIGMAY_VALUE"        /* (asynFloat64,      r/o) Sigma Y */
#define NDPluginStatsSigmaXYString            "SIGMAXY_VALUE"       /* (asynFloat64,      r/o) Sigma XY */
    
/* Time series of basic statistics and centroid statistics */
#define NDPluginStatsTSControlString          "TS_CONTROL"          /* (asynInt32,        r/w) Erase/start, stop, start */
#define NDPluginStatsTSNumPointsString        "TS_NUM_POINTS"       /* (asynInt32,        r/w) Number of time series points to use */
#define NDPluginStatsTSCurrentPointString     "TS_CURRENT_POINT"    /* (asynInt32,        r/o) Current point in time series */
#define NDPluginStatsTSAcquiringString        "TS_ACQUIRING"        /* (asynInt32,        r/o) Acquiring time series */
#define NDPluginStatsTSMinValueString         "TS_MIN_VALUE"        /* (asynFloat64Array, r/o) Series of minimum counts */
#define NDPluginStatsTSMinXString             "TS_MIN_X"            /* (asynFloat64Array, r/o) Series of X position of minimum counts */
#define NDPluginStatsTSMinYString             "TS_MIN_Y"            /* (asynFloat64Array, r/o) Series of Y position of minimum counts */
#define NDPluginStatsTSMaxValueString         "TS_MAX_VALUE"        /* (asynFloat64Array, r/o) Series of maximum counts */
#define NDPluginStatsTSMaxXString             "TS_MAX_X"            /* (asynFloat64Array, r/o) Series of X position of maximum counts */
#define NDPluginStatsTSMaxYString             "TS_MAX_Y"            /* (asynFloat64Array, r/o) Series of Y position of maximum counts */
#define NDPluginStatsTSMeanValueString        "TS_MEAN_VALUE"       /* (asynFloat64Array, r/o) Series of mean counts */
#define NDPluginStatsTSSigmaValueString       "TS_SIGMA_VALUE"      /* (asynFloat64Array, r/o) Series of sigma */
#define NDPluginStatsTSTotalString            "TS_TOTAL"            /* (asynFloat64Array, r/o) Series of total */
#define NDPluginStatsTSNetString              "TS_NET"              /* (asynFloat64Array, r/o) Series of net */
#define NDPluginStatsTSSeriesMaxString        "TS_MAX_SUM"          /* (asynFloat64Array, r/o) Series of max elements sum */
#define NDPluginStatsTSCentroidXString        "TS_CENTROIDX_VALUE"  /* (asynFloat64Array, r/o) Series of X centroid */
#define NDPluginStatsTSCentroidYString        "TS_CENTROIDY_VALUE"  /* (asynFloat64Array, r/o) Series of Y centroid */
#define NDPluginStatsTSSigmaXString           "TS_SIGMAX_VALUE"     /* (asynFloat64Array, r/o) Series of sigma X */
#define NDPluginStatsTSSigmaYString           "TS_SIGMAY_VALUE"     /* (asynFloat64Array, r/o) Series of sigma Y */
#define NDPluginStatsTSSigmaXYString          "TS_SIGMAXY_VALUE"    /* (asynFloat64Array, r/o) Series of sigma XY */
#define NDPluginStatsTSTimestampString        "TS_TIMESTAMP_VALUE"  /* (asynFloat64Array, r/o) Series of timestamps */

/* Profiles*/   
#define NDPluginStatsComputeProfilesString    "COMPUTE_PROFILES"    /* (asynInt32,        r/w) Compute profiles? */
#define NDPluginStatsProfileSizeXString       "PROFILE_SIZE_X"      /* (asynInt32,        r/o) X profile size */
#define NDPluginStatsProfileSizeYString       "PROFILE_SIZE_Y"      /* (asynInt32,        r/o) Y profile size */
#define NDPluginStatsCursorXString            "CURSOR_X"            /* (asynInt32,        r/w) X cursor position */
#define NDPluginStatsCursorYString            "CURSOR_Y"            /* (asynInt32,        r/w) Y cursor position */
#define NDPluginStatsProfileAverageXString    "PROFILE_AVERAGE_X"   /* (asynFloat64Array, r/o) X average profile array */
#define NDPluginStatsProfileAverageYString    "PROFILE_AVERAGE_Y"   /* (asynFloat64Array, r/o) Y average profile array */
#define NDPluginStatsProfileThresholdXString  "PROFILE_THRESHOLD_X" /* (asynFloat64Array, r/o) X average profile array after threshold */
#define NDPluginStatsProfileThresholdYString  "PROFILE_THRESHOLD_Y" /* (asynFloat64Array, r/o) Y average profile array after threshold */
#define NDPluginStatsProfileCentroidXString   "PROFILE_CENTROID_X"  /* (asynFloat64Array, r/o) X centroid profile array */
#define NDPluginStatsProfileCentroidYString   "PROFILE_CENTROID_Y"  /* (asynFloat64Array, r/o) Y centroid profile array */
#define NDPluginStatsProfileCursorXString     "PROFILE_CURSOR_X"    /* (asynFloat64Array, r/o) X cursor profile array */
#define NDPluginStatsProfileCursorYString     "PROFILE_CURSOR_Y"    /* (asynFloat64Array, r/o) Y cursor profile array */

/* Histogram */
#define NDPluginStatsComputeHistogramString   "COMPUTE_HISTOGRAM"   /* (asynInt32,        r/w) Compute histogram? */
#define NDPluginStatsHistSizeString           "HIST_SIZE"           /* (asynInt32,        r/w) Number of elements in histogram */
#define NDPluginStatsHistMinString            "HIST_MIN"            /* (asynFloat64,      r/w) Minimum value for histogram */
#define NDPluginStatsHistMaxString            "HIST_MAX"            /* (asynFloat64,      r/w) Maximum value for histogram */
#define NDPluginStatsHistEntropyString        "HIST_ENTROPY"        /* (asynFloat64,      r/o) Image entropy calculcated from histogram */
#define NDPluginStatsHistArrayString          "HIST_ARRAY"          /* (asynFloat64Array, r/o) Histogram array */


/* Arrays of total and net counts for MCA or waveform record */   
#define NDPluginStatsCallbackPeriodString     "CALLBACK_PERIOD"     /* (asynFloat64,      r/w) Callback period */

/** Does image statistics.  These include
  * Min, max, mean, sigma
  * X and Y centroid and sigma
  * Histogram
  */
class epicsShareClass NDPluginStats : public NDPluginDriver {
public:
    NDPluginStats(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    
    template <typename epicsType> void doComputeStatisticsT(NDArray *pArray, NDStats_t *pStats);
    int doComputeStatistics(NDArray *pArray, NDStats_t *pStats);
    template <typename epicsType> asynStatus doComputeCentroidT(NDArray *pArray);
    asynStatus doComputeCentroid(NDArray *pArray);
    template <typename epicsType> asynStatus doComputeProfilesT(NDArray *pArray);
    asynStatus doComputeProfiles(NDArray *pArray);
    template <typename epicsType> asynStatus doComputeHistogramT(NDArray *pArray);
    asynStatus doComputeHistogram(NDArray *pArray);
   
protected:
    int NDPluginStatsComputeStatistics;
    #define FIRST_NDPLUGIN_STATS_PARAM NDPluginStatsComputeStatistics
    /* Statistics */
    int NDPluginStatsBgdWidth;
    int NDPluginStatsMinValue;
    int NDPluginStatsMinX;
    int NDPluginStatsMinY;            
    int NDPluginStatsMaxValue;
    int NDPluginStatsMaxX;
    int NDPluginStatsMaxY;        
    int NDPluginStatsMeanValue;
    int NDPluginStatsSigmaValue;
    int NDPluginStatsTotal;
    int NDPluginStatsNet;

    /* Centroid */
    int NDPluginStatsComputeCentroid;
    int NDPluginStatsCentroidThreshold;
    int NDPluginStatsCentroidX;
    int NDPluginStatsCentroidY;
    int NDPluginStatsSigmaX;
    int NDPluginStatsSigmaY;
    int NDPluginStatsSigmaXY;

    /* Time Series */
    int NDPluginStatsTSControl;
    int NDPluginStatsTSNumPoints;
    int NDPluginStatsTSCurrentPoint;
    int NDPluginStatsTSAcquiring;
    int NDPluginStatsTSMinValue;
    int NDPluginStatsTSMinX;
    int NDPluginStatsTSMinY;                
    int NDPluginStatsTSMaxValue;
    int NDPluginStatsTSMaxX;
    int NDPluginStatsTSMaxY;            
    int NDPluginStatsTSMeanValue;
    int NDPluginStatsTSSigmaValue;
    int NDPluginStatsTSTotal;
    int NDPluginStatsTSNet;
    int NDPluginStatsTSCentroidX;
    int NDPluginStatsTSCentroidY;
    int NDPluginStatsTSSigmaX;
    int NDPluginStatsTSSigmaY;
    int NDPluginStatsTSSigmaXY;
    int NDPluginStatsTSTimestamp;
    
    /* Profiles */
    int NDPluginStatsComputeProfiles;
    int NDPluginStatsProfileSizeX;
    int NDPluginStatsProfileSizeY;
    int NDPluginStatsCursorX;
    int NDPluginStatsCursorY;
    int NDPluginStatsProfileAverageX;
    int NDPluginStatsProfileAverageY;
    int NDPluginStatsProfileThresholdX;
    int NDPluginStatsProfileThresholdY;
    int NDPluginStatsProfileCentroidX;
    int NDPluginStatsProfileCentroidY;
    int NDPluginStatsProfileCursorX;
    int NDPluginStatsProfileCursorY;

    /* Histogram */
    int NDPluginStatsComputeHistogram;
    int NDPluginStatsHistSize;
    int NDPluginStatsHistMin;
    int NDPluginStatsHistMax;
    int NDPluginStatsHistEntropy;
    int NDPluginStatsHistArray;

    #define LAST_NDPLUGIN_STATS_PARAM NDPluginStatsHistArray
                                
private:
    double  centroidThreshold;
    double  centroidX;
    double  centroidY;
    double  sigmaX;
    double  sigmaY;
    double  sigmaXY;
    double  *profileX[MAX_PROFILE_TYPES];
    double  *profileY[MAX_PROFILE_TYPES];
    double  *timeSeries[MAX_TIME_SERIES_TYPES];
    size_t profileSizeX;
    size_t profileSizeY;
    size_t cursorX;
    size_t cursorY;
    epicsInt32 *totalArray;
    epicsInt32 *netArray;
    size_t histogramSize;
    size_t histSizeNew;
    double *histogram;
    double histMin;
    double histMax;
    double histEntropy;
    void doTimeSeriesCallbacks();
};
#define NUM_NDPLUGIN_STATS_PARAMS ((int)(&LAST_NDPLUGIN_STATS_PARAM - &FIRST_NDPLUGIN_STATS_PARAM + 1))
    
#endif
