#ifndef NDPluginStats_H
#define NDPluginStats_H

#include "NDPluginDriver.h"

typedef enum {
    profAverage,
    profThreshold,
    profCentroid,
    profCursor,
    MAX_PROFILE_TYPES
} NDStatProfileType;

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
    TSCentroidTotal,
    TSCentroidX,
    TSCentroidY,
    TSSigmaX,
    TSSigmaY,
    TSSigmaXY,
    TSSkewX,
    TSSkewY,
    TSKurtosisX,
    TSKurtosisY,
    TSEccentricity,
    TSOrientation,
    TSTimestamp,
    MAX_TIME_SERIES_TYPES
} NDStatTSType;

typedef enum {
    TSEraseStart,
    TSStart,
    TSStop,
    TSRead
} NDStatsTSControl_t;

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
    double  centroidThreshold;
    double  centroidTotal;
    double  centroidX;
    double  centroidY;
    double  sigmaX;
    double  sigmaY;
    double  sigmaXY;
    double  skewX;
    double  skewY;
    double  kurtosisX;
    double  kurtosisY;
    double  eccentricity;
    double  orientation;
    double  *profileX[MAX_PROFILE_TYPES];
    double  *profileY[MAX_PROFILE_TYPES];
    size_t profileSizeX;
    size_t profileSizeY;
    size_t cursorX;
    size_t cursorY;
    double  cursorValue;
    epicsInt32 *totalArray;
    epicsInt32 *netArray;
    int histSize;
    double *histogram;
    double histMin;
    double histMax;
    epicsInt32 histBelow;
    epicsInt32 histAbove;
    double histEntropy;
} NDStats_t;

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
#define NDPluginStatsCentroidTotalString      "CENTROID_TOTAL"      /* (asynFloat64,      r/o) Total centroid */
#define NDPluginStatsCentroidXString          "CENTROIDX_VALUE"     /* (asynFloat64,      r/o) X centroid */
#define NDPluginStatsCentroidYString          "CENTROIDY_VALUE"     /* (asynFloat64,      r/o) Y centroid */
#define NDPluginStatsSigmaXString             "SIGMAX_VALUE"        /* (asynFloat64,      r/o) Sigma X */
#define NDPluginStatsSigmaYString             "SIGMAY_VALUE"        /* (asynFloat64,      r/o) Sigma Y */
#define NDPluginStatsSigmaXYString            "SIGMAXY_VALUE"       /* (asynFloat64,      r/o) Sigma XY */
#define NDPluginStatsSkewXString              "SKEWX_VALUE"         /* (asynFloat64,      r/o) Skew X */
#define NDPluginStatsSkewYString              "SKEWY_VALUE"         /* (asynFloat64,      r/o) Skew Y */
#define NDPluginStatsKurtosisXString          "KURTOSISX_VALUE"     /* (asynFloat64,      r/o) Kurtosis X */
#define NDPluginStatsKurtosisYString          "KURTOSISY_VALUE"     /* (asynFloat64,      r/o) Kurtosis Y */
#define NDPluginStatsEccentricityString       "ECCENTRICITY_VALUE"  /* (asynFloat64,      r/o) Eccentricity */
#define NDPluginStatsOrientationString        "ORIENTATION_VALUE"   /* (asynFloat64,      r/o) Orientation */

/* Profiles*/
#define NDPluginStatsComputeProfilesString    "COMPUTE_PROFILES"    /* (asynInt32,        r/w) Compute profiles? */
#define NDPluginStatsProfileSizeXString       "PROFILE_SIZE_X"      /* (asynInt32,        r/o) X profile size */
#define NDPluginStatsProfileSizeYString       "PROFILE_SIZE_Y"      /* (asynInt32,        r/o) Y profile size */
#define NDPluginStatsCursorXString            "CURSOR_X"            /* (asynInt32,        r/w) X cursor position */
#define NDPluginStatsCursorYString            "CURSOR_Y"            /* (asynInt32,        r/w) Y cursor position */
#define NDPluginStatsCursorValString          "CURSOR_VAL"          /* (asynFloat64,      r/o) value at cursor position */
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
#define NDPluginStatsHistBelowString          "HIST_BELOW"          /* (asynInt32,        r/o) Number of pixels below minimum */
#define NDPluginStatsHistAboveString          "HIST_ABOVE"          /* (asynInt32,        r/o) Number of pixels above maximum */
#define NDPluginStatsHistEntropyString        "HIST_ENTROPY"        /* (asynFloat64,      r/o) Image entropy calculcated from histogram */
#define NDPluginStatsHistArrayString          "HIST_ARRAY"          /* (asynFloat64Array, r/o) Histogram array */
#define NDPluginStatsHistXArrayString         "HIST_X_ARRAY"        /* (asynFloat64Array, r/o) Histogram X axis array */


/* Arrays of total and net counts for MCA or waveform record */
#define NDPluginStatsCallbackPeriodString     "CALLBACK_PERIOD"     /* (asynFloat64,      r/w) Callback period */

/** Does image statistics.  These include
  * Min, max, mean, sigma
  * X and Y centroid and sigma
  * Histogram
  */
class NDPLUGIN_API NDPluginStats : public NDPluginDriver {
public:
    NDPluginStats(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize, int maxThreads=1);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

    template <typename epicsType> void doComputeStatisticsT(NDArray *pArray, NDStats_t *pStats);
    int doComputeStatistics(NDArray *pArray, NDStats_t *pStats);
    template <typename epicsType> asynStatus doComputeCentroidT(NDArray *pArray, NDStats_t *pStats);
    asynStatus doComputeCentroid(NDArray *pArray, NDStats_t *pStats);
    template <typename epicsType> asynStatus doComputeProfilesT(NDArray *pArray, NDStats_t *pStats);
    asynStatus doComputeProfiles(NDArray *pArray, NDStats_t *pStats);
    template <typename epicsType> asynStatus doComputeHistogramT(NDArray *pArray, NDStats_t *pStats);
    asynStatus doComputeHistogram(NDArray *pArray, NDStats_t *pStats);

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
    int NDPluginStatsCentroidTotal;
    int NDPluginStatsCentroidX;
    int NDPluginStatsCentroidY;
    int NDPluginStatsSigmaX;
    int NDPluginStatsSigmaY;
    int NDPluginStatsSigmaXY;
    int NDPluginStatsSkewX;
    int NDPluginStatsSkewY;
    int NDPluginStatsKurtosisX;
    int NDPluginStatsKurtosisY;
    int NDPluginStatsEccentricity;
    int NDPluginStatsOrientation;

    /* Profiles */
    int NDPluginStatsComputeProfiles;
    int NDPluginStatsProfileSizeX;
    int NDPluginStatsProfileSizeY;
    int NDPluginStatsCursorX;
    int NDPluginStatsCursorY;
    int NDPluginStatsCursorVal;
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
    int NDPluginStatsHistBelow;
    int NDPluginStatsHistAbove;
    int NDPluginStatsHistEntropy;
    int NDPluginStatsHistArray;
    int NDPluginStatsHistXArray;

private:
    asynStatus computeHistX();
};

#endif
