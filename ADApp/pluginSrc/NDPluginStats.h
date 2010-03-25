#ifndef NDPluginStats_H
#define NDPluginStats_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/* ROI statistics */
#define NDPluginStatsComputeStatisticsString  "COMPUTE_STATISTICS"  /* (asynInt32,   r/w) Compute statistics for this ROI? */
#define NDPluginStatsBgdWidthString           "BGD_WIDTH"           /* (asynInt32,   r/w) Width of background region when computing net */
#define NDPluginStatsMinValueString           "MIN_VALUE"           /* (asynFloat64, r/o) Minimum counts in any element */
#define NDPluginStatsMaxValueString           "MAX_VALUE"           /* (asynFloat64, r/o) Maximum counts in any element */
#define NDPluginStatsMeanValueString          "MEAN_VALUE"          /* (asynFloat64, r/o) Mean counts of all elements */
#define NDPluginStatsSigmaValueString         "SIGMA_VALUE"         /* (asynFloat64, r/o) Sigma of all elements */
#define NDPluginStatsTotalString              "TOTAL"               /* (asynFloat64, r/o) Sum of all elements */
#define NDPluginStatsNetString                "NET"                 /* (asynFloat64, r/o) Sum of all elements minus background */

/* ROI centroid */
#define NDPluginStatsComputeCentroidString    "COMPUTE_CENTROID"    /* (asynInt32,   r/w) Compute centroid for this ROI? */
#define NDPluginStatsCentroidThresholdString  "CENTROID_THRESHOLD"  /* (asynFloat64,   r/w) Threshold when computing centroids */
#define NDPluginStatsCentroidXString          "CENTROIDX_VALUE"     /* (asynFloat64, r/o) X centroid */
#define NDPluginStatsCentroidYString          "CENTROIDY_VALUE"     /* (asynFloat64, r/o) Y centroid */
#define NDPluginStatsSigmaXString             "SIGMAX_VALUE"        /* (asynFloat64, r/o) X sigma */
#define NDPluginStatsSigmaYString             "SIGMAY_VALUE"        /* (asynFloat64, r/o) Y sigma */
    
/* ROI histogram */
#define NDPluginStatsComputeHistogramString   "COMPUTE_HISTOGRAM"   /* (asynInt32,   r/w) Compute histogram for this ROI? */
#define NDPluginStatsHistSizeString           "HIST_SIZE"           /* (asynInt32,   r/w) Number of elements in histogram */
#define NDPluginStatsHistMinString            "HIST_MIN"            /* (asynFloat64, r/w) Minimum value for histogram */
#define NDPluginStatsHistMaxString            "HIST_MAX"            /* (asynFloat64, r/w) Maximum value for histogram */
#define NDPluginStatsHistEntropyString        "HIST_ENTROPY"        /* (asynFloat64, r/o) Image entropy calculcated from histogram */
#define NDPluginStatsHistArrayString          "HIST_ARRAY"          /* (asynFloat64Array, r/o) Histogram array */

/* ROI profiles - not yet implemented */
#define NDPluginStatsComputeProfilesString    "COMPUTE_PROFILES"    /* (asynInt32,   r/w) Compute profiles for this ROI? */

/* Arrays of total and net counts for MCA or waveform record */   
#define NDPluginStatsTotalArrayString         "TOTAL_ARRAY"       /* (asynInt32Array, r/o) Total counts array */
#define NDPluginStatsNetArrayString           "NET_ARRAY"         /* (asynInt32Array, r/o) Net counts array */


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
    
    template <typename epicsType> void doComputeHistogramT(NDArray *pArray);
    int doComputeHistogram(NDArray *pArray);
    
protected:
    /* Background array subtraction */
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

    /* Histogram */
    int NDPluginStatsComputeHistogram;
    int NDPluginStatsHistSize;
    int NDPluginStatsHistMin;
    int NDPluginStatsHistMax;
    int NDPluginStatsHistEntropy;
    int NDPluginStatsHistArray;

    /* Profiles - not yet implemented */
    int NDPluginStatsComputeProfiles;

    /* Arrays of total and net counts for MCA or waveform record */   
    int NDPluginStatsTotalArray;
    int NDPluginStatsNetArray;

    #define LAST_NDPLUGIN_STATS_PARAM NDPluginStatsNetArray
                                
private:
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
