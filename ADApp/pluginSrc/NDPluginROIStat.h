/**
 * See .cpp file for documentation.
 */

#ifndef NDPluginROIStat_H
#define NDPluginROIStat_H

#include <epicsTypes.h>

#include "NDPluginDriver.h"

/* ROI general parameters */
#define NDPluginROIStatFirstString              "ROISTAT_FIRST"
#define NDPluginROIStatLastString               "ROISTAT_LAST"
#define NDPluginROIStatNameString               "ROISTAT_NAME"              /* (asynOctet, r/w) Name of this ROI */
#define NDPluginROIStatResetAllString           "ROISTAT_RESETALL"          /* (asynInt32, r/w) Reset ROI data for all ROIs. */

/* ROI definition */
#define NDPluginROIStatUseString                "ROISTAT_USE"               /* (asynInt32, r/w) Use this ROI? */
#define NDPluginROIStatResetString              "ROISTAT_RESET"             /* (asynInt32, r/w) Reset ROI data. */
#define NDPluginROIStatBgdWidthString           "ROISTAT_BGD_WIDTH"         /* (asynInt32, r/w) Width of background region when computing net */
#define NDPluginROIStatDim0MinString            "ROISTAT_DIM0_MIN"          /* (asynInt32, r/w) Starting element of ROI in X dimension */
#define NDPluginROIStatDim0SizeString           "ROISTAT_DIM0_SIZE"         /* (asynInt32, r/w) Size of ROI in X dimension */
#define NDPluginROIStatDim0MaxSizeString        "ROISTAT_DIM0_MAX_SIZE"     /* (asynInt32, r/o) Maximum size of ROI in X dimension */
#define NDPluginROIStatDim1MinString            "ROISTAT_DIM1_MIN"          /* (asynInt32, r/w) Starting element of ROI in Y dimension */
#define NDPluginROIStatDim1SizeString           "ROISTAT_DIM1_SIZE"         /* (asynInt32, r/w) Size of ROI in Y dimension */
#define NDPluginROIStatDim1MaxSizeString        "ROISTAT_DIM1_MAX_SIZE"     /* (asynInt32, r/o) Maximum size of ROI in Y dimension */
#define NDPluginROIStatDim2MinString            "ROISTAT_DIM2_MIN"          /* (asynInt32, r/w) Starting element of ROI in Z dimension */
#define NDPluginROIStatDim2SizeString           "ROISTAT_DIM2_SIZE"         /* (asynInt32, r/w) Size of ROI in Z dimension */
#define NDPluginROIStatDim2MaxSizeString        "ROISTAT_DIM2_MAX_SIZE"     /* (asynInt32, r/o) Maximum size of ROI in Z dimension */

/* ROI statistics */
#define NDPluginROIStatMinValueString           "ROISTAT_MIN_VALUE"         /* (asynFloat64, r/o) Minimum counts in any element */
#define NDPluginROIStatMaxValueString           "ROISTAT_MAX_VALUE"         /* (asynFloat64, r/o) Maximum counts in any element */
#define NDPluginROIStatMeanValueString          "ROISTAT_MEAN_VALUE"        /* (asynFloat64, r/o) Mean counts of all elements */
#define NDPluginROIStatTotalString              "ROISTAT_TOTAL"             /* (asynFloat64, r/o) Sum of all elements */
#define NDPluginROIStatNetString                "ROISTAT_NET"               /* (asynFloat64, r/o) Sum of all elements minus background */

/* Time series of statistics */
#define NDPluginROIStatTSControlString          "ROISTAT_TS_CONTROL"        /* (asynInt32,        r/w) Erase/start, stop, start */
#define NDPluginROIStatTSNumPointsString        "ROISTAT_TS_NUM_POINTS"     /* (asynInt32,        r/w) Number of time series points to use */
#define NDPluginROIStatTSCurrentPointString     "ROISTAT_TS_CURRENT_POINT"  /* (asynInt32,        r/o) Current point in time series */
#define NDPluginROIStatTSAcquiringString        "ROISTAT_TS_ACQUIRING"      /* (asynInt32,        r/o) Acquiring time series */
#define NDPluginROIStatTSMinValueString         "ROISTAT_TS_MIN_VALUE"      /* (asynFloat64Array, r/o) Series of minimum counts */
#define NDPluginROIStatTSMaxValueString         "ROISTAT_TS_MAX_VALUE"      /* (asynFloat64Array, r/o) Series of maximum counts */
#define NDPluginROIStatTSMeanValueString        "ROISTAT_TS_MEAN_VALUE"     /* (asynFloat64Array, r/o) Series of mean counts */
#define NDPluginROIStatTSTotalString            "ROISTAT_TS_TOTAL"          /* (asynFloat64Array, r/o) Series of total */
#define NDPluginROIStatTSNetString              "ROISTAT_TS_NET"            /* (asynFloat64Array, r/o) Series of net */
#define NDPluginROIStatTSTimestampString        "ROISTAT_TS_TIMESTAMP"      /* (asynFloat64Array, r/o) Series of timestamps */

typedef enum {
    TSMinValue,
    TSMaxValue,
    TSMeanValue,
    TSTotal,
    TSNet,
    TSTimestamp,
    MAX_TIME_SERIES_TYPES
} NDPluginROIStatTSType;

typedef enum {
    TSEraseStart,
    TSStart,
    TSStop,
    TSRead
} NDPluginROIStatsTSControl_t;

/** Structure defining a Region-Of-Interest and Stats */
typedef struct NDROI {
    size_t offset[2];
    size_t size[2];
    size_t bgdWidth;
    double total;
    double mean;
    double min;
    double max;
    double net;
    size_t arraySize[2];
} NDROI_t;


/** Compute statistics on ROIs in an array */
class epicsShareClass NDPluginROIStat : public NDPluginDriver {
public:
    NDPluginROIStat(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxROIs, 
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    
    //These methods override the virtual methods in the base class
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:

    //ROI general parameters
    int NDPluginROIStatFirst;
    #define FIRST_NDPLUGIN_ROISTAT_PARAM NDPluginROIStatFirst
    int NDPluginROIStatName;
    int NDPluginROIStatUse;
    int NDPluginROIStatReset;
    int NDPluginROIStatBgdWidth;
    int NDPluginROIStatResetAll;

    //ROI definition
    int NDPluginROIStatDim0Min;
    int NDPluginROIStatDim0Size;
    int NDPluginROIStatDim0MaxSize;
    int NDPluginROIStatDim1Min;
    int NDPluginROIStatDim1Size;
    int NDPluginROIStatDim1MaxSize;
    int NDPluginROIStatDim2Min;
    int NDPluginROIStatDim2Size;
    int NDPluginROIStatDim2MaxSize;

    //ROI statistics
    int NDPluginROIStatMinValue;
    int NDPluginROIStatMaxValue;
    int NDPluginROIStatMeanValue;
    int NDPluginROIStatTotal;
    int NDPluginROIStatNet;

    // Time Series
    int NDPluginROIStatTSControl;
    int NDPluginROIStatTSNumPoints;
    int NDPluginROIStatTSCurrentPoint;
    int NDPluginROIStatTSAcquiring;
    int NDPluginROIStatTSMinValue;
    int NDPluginROIStatTSMaxValue;
    int NDPluginROIStatTSMeanValue;
    int NDPluginROIStatTSTotal;
    int NDPluginROIStatTSNet;
    int NDPluginROIStatTSTimestamp;
    
    int NDPluginROIStatLast;
    #define LAST_NDPLUGIN_ROISTAT_PARAM NDPluginROIStatLast
                                
private:

    template <typename epicsType> asynStatus doComputeStatisticsT(NDArray *pArray, NDROI_t *pROI);
    asynStatus doComputeStatistics(NDArray *pArray, NDROI_t *pStats);
    asynStatus clear(epicsUInt32 roi);
    void doTimeSeriesCallbacks();

    NDROI_t *pROIs_;    /* Array of NDROI structures */
    int maxROIs_;
    int numTSPoints_;
    int currentTSPoint_;
    double  *timeSeries_;
};

#define NUM_NDPLUGIN_ROISTAT_PARAMS (int)(&LAST_NDPLUGIN_ROISTAT_PARAM - &FIRST_NDPLUGIN_ROISTAT_PARAM + 1)
    
#endif //NDPluginROIStat_H
