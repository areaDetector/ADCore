#ifndef NDPluginStats_H
#define NDPluginStats_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

typedef struct NDStats {
    int     nElements;
    double  total;
    double  net;
    double  mean;
    double  sigma;
    double  min;
    double  max;
} NDStats_t;

typedef enum {
   profAverage,
   profThreshold,
   profCentroid,
   profCursor
} NDStatProfileType;

#define MAX_PROFILE_TYPES profCursor+1

/* Statistics */
#define NDPluginStatsComputeStatisticsString  "COMPUTE_STATISTICS"  /* (asynInt32,        r/w) Compute statistics? */
#define NDPluginStatsBgdWidthString           "BGD_WIDTH"           /* (asynInt32,        r/w) Width of background region when computing net */
#define NDPluginStatsMinValueString           "MIN_VALUE"           /* (asynFloat64,      r/o) Minimum counts in any element */
#define NDPluginStatsMaxValueString           "MAX_VALUE"           /* (asynFloat64,      r/o) Maximum counts in any element */
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
#define NDPluginStatsTotalArrayString         "TOTAL_ARRAY"         /* (asynInt32Array,   r/o) Total counts array */
#define NDPluginStatsNetArrayString           "NET_ARRAY"           /* (asynInt32Array,   r/o) Net counts array */

/** Does image statistics.  These include
  * Min, max, mean, sigma
  * X and Y centroid and sigma
  * Histogram
  */
class NDPluginStats : public NDPluginDriver {
public:
    NDPluginStats(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    
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
    int NDPluginStatsMaxValue;
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

    /* Arrays of total and net counts for MCA or waveform record */   
    int NDPluginStatsCallbackPeriod;
    int NDPluginStatsTotalArray;
    int NDPluginStatsNetArray;

    #define LAST_NDPLUGIN_STATS_PARAM NDPluginStatsNetArray
                                
private:
    double  centroidThreshold;
    double  centroidX;
    double  centroidY;
    double  sigmaX;
    double  sigmaY;
    double  sigmaXY;
    double  *profileX[MAX_PROFILE_TYPES];
    double  *profileY[MAX_PROFILE_TYPES];
    int profileSizeX;
    int profileSizeY;
    int cursorX;
    int cursorY;
    epicsInt32 *totalArray;
    epicsInt32 *netArray;
    int histogramSize;
    int histSizeNew;
    double *histogram;
    double histMin;
    double histMax;
    double histEntropy;
};
#define NUM_NDPLUGIN_STATS_PARAMS (&LAST_NDPLUGIN_STATS_PARAM - &FIRST_NDPLUGIN_STATS_PARAM + 1)
    
#endif
