#ifndef NDPluginROI_H
#define NDPluginROI_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/** Structure defining a Region-Of-Interest (ROI) */
typedef struct NDROI {
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    double *profiles[ND_ARRAY_MAX_DIMS];
    int profileSize[ND_ARRAY_MAX_DIMS];
    int nElements;
    int bgdWidth;
    double total;
    double net;
    double mean;
    double min;
    double max;
    double histMin;
    double histMax;
    int histSize;
    double *histogram;
} NDROI_t;

#define NDPluginROINameString               "NAME"               /* (asynOctet,   r/w) Name of this ROI */
#define NDPluginROIUseString                "USE"                /* (asynInt32,   r/w) Use this ROI? */
#define NDPluginROIComputeStatisticsString  "COMPUTE_STATISTICS" /* (asynInt32,   r/w) Compute statistics for this ROI? */
#define NDPluginROIComputeHistogramString   "COMPUTE_HISTOGRAM" /* (asynInt32,   r/w) Compute histogram for this ROI? */
#define NDPluginROIComputeProfilesString    "COMPUTE_PROFILES"  /* (asynInt32,   r/w) Compute profiles for this ROI? */
#define NDPluginROIHighlightString          "HIGHLIGHT"         /* (asynInt32,   r/w) Highlight ROIs? */
    
    /* ROI definition */
#define NDPluginROIDim0MinString            "DIM0_MIN"          /* (asynInt32,   r/w) Starting element of ROI in each dimension */
#define NDPluginROIDim0SizeString           "DIM0_SIZE"         /* (asynInt32,   r/w) Size of ROI in each dimension */
#define NDPluginROIDim0MaxSizeString        "DIM0_MAX_SIZE"     /* (asynInt32,   r/o) Maximum size of ROI in each dimension */
#define NDPluginROIDim0BinString            "DIM0_BIN"          /* (asynInt32,   r/w) Binning of ROI in each dimension */
#define NDPluginROIDim0ReverseString        "DIM0_REVERSE"      /* (asynInt32,   r/w) Reversal of ROI in each dimension */
#define NDPluginROIDim1MinString            "DIM1_MIN"          /* (asynInt32,   r/w) Starting element of ROI in each dimension */
#define NDPluginROIDim1SizeString           "DIM1_SIZE"         /* (asynInt32,   r/w) Size of ROI in each dimension */
#define NDPluginROIDim1MaxSizeString        "DIM1_MAX_SIZE"     /* (asynInt32,   r/o) Maximum size of ROI in each dimension */
#define NDPluginROIDim1BinString            "DIM1_BIN"          /* (asynInt32,   r/w) Binning of ROI in each dimension */
#define NDPluginROIDim1ReverseString        "DIM1_REVERSE"      /* (asynInt32,   r/w) Reversal of ROI in each dimension */
#define NDPluginROIDim2MinString            "DIM2_MIN"          /* (asynInt32,   r/w) Starting element of ROI in each dimension */
#define NDPluginROIDim2SizeString           "DIM2_SIZE"         /* (asynInt32,   r/w) Size of ROI in each dimension */
#define NDPluginROIDim2MaxSizeString        "DIM2_MAX_SIZE"     /* (asynInt32,   r/o) Maximum size of ROI in each dimension */
#define NDPluginROIDim2BinString            "DIM2_BIN"          /* (asynInt32,   r/w) Binning of ROI in each dimension */
#define NDPluginROIDim2ReverseString        "DIM2_REVERSE"      /* (asynInt32,   r/w) Reversal of ROI in each dimension */
#define NDPluginROIDataTypeString           "ROI_DATA_TYPE"     /* (asynInt32,   r/w) Data type for ROI.  -1 means automatic. */
    
    /* ROI statistics */
#define NDPluginROIBgdWidthString           "BGD_WIDTH"         /* (asynInt32,   r/w) Width of background region when computing net */
#define NDPluginROIMinValueString           "MIN_VALUE"         /* (asynFloat64, r/o) Minimum counts in any element */
#define NDPluginROIMaxValueString           "MAX_VALUE"         /* (asynFloat64, r/o) Maximum counts in any element */
#define NDPluginROIMeanValueString          "MEAN_VALUE"        /* (asynFloat64, r/o) Mean counts of all elements */
#define NDPluginROITotalString              "TOTAL"             /* (asynFloat64, r/o) Sum of all elements */
#define NDPluginROINetString                "NET"               /* (asynFloat64, r/o) Sum of all elements minus background */
    
    /* ROI histogram */
#define NDPluginROIHistSizeString           "HIST_SIZE"         /* (asynInt32,   r/w) Number of elements in histogram */
#define NDPluginROIHistMinString            "HIST_MIN"          /* (asynFloat64, r/w) Minimum value for histogram */
#define NDPluginROIHistMaxString            "HIST_MAX"          /* (asynFloat64, r/w) Maximum value for histogram */
#define NDPluginROIHistEntropyString        "HIST_ENTROPY"      /* (asynFloat64, r/o) Image entropy calculcated from histogram */
#define NDPluginROIHistArrayString          "HIST_ARRAY"        /* (asynFloat64Array, r/o) Histogram array */

#define NDPluginROITotalArrayString         "TOTAL_ARRAY"       /* (asynInt32Array, r/o) Total counts array */
#define NDPluginROINetArrayString           "NET_ARRAY"         /* (asynInt32Array, r/o) Net counts array */


/** Extract Regions-Of-Interest (ROI) from NDArray data; the plugin can be a source of NDArray callbacks for
  * other plugins, passing these sub-arrays. 
  * The plugin also optionally computes a statistics on the ROI. */
class NDPluginROI : public NDPluginDriver {
public:
    NDPluginROI(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxROIs, 
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

    /* These methods are unique to this class */
    asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, 
                                size_t nelements, size_t *nIn);
protected:
    int NDPluginROIName;
    #define FIRST_NDPLUGIN_ROI_PARAM NDPluginROIName
    int NDPluginROIUse;
    int NDPluginROIComputeStatistics;
    int NDPluginROIComputeHistogram;
    int NDPluginROIComputeProfiles;
    int NDPluginROIHighlight;
    int NDPluginROIDim0Min;
    int NDPluginROIDim0Size;
    int NDPluginROIDim0MaxSize;
    int NDPluginROIDim0Bin;
    int NDPluginROIDim0Reverse;
    int NDPluginROIDim1Min;
    int NDPluginROIDim1Size;
    int NDPluginROIDim1MaxSize;
    int NDPluginROIDim1Bin;
    int NDPluginROIDim1Reverse;
    int NDPluginROIDim2Min;
    int NDPluginROIDim2Size;
    int NDPluginROIDim2MaxSize;
    int NDPluginROIDim2Bin;
    int NDPluginROIDim2Reverse;
    int NDPluginROIDataType;
    int NDPluginROIBgdWidth;
    int NDPluginROIMinValue;
    int NDPluginROIMaxValue;
    int NDPluginROIMeanValue;
    int NDPluginROITotal;
    int NDPluginROINet;
    int NDPluginROITotalArray;
    int NDPluginROINetArray;
    int NDPluginROIHistSize;
    int NDPluginROIHistMin;
    int NDPluginROIHistMax;
    int NDPluginROIHistEntropy;
    int NDPluginROIHistArray;
    #define LAST_NDPLUGIN_ROI_PARAM NDPluginROIHistArray
                                
private:
    int maxROIs;
    NDROI_t *pROIs;    /* Array of NDROI structures */
    epicsInt32 *totalArray;
    epicsInt32 *netArray;
};
#define NUM_NDPLUGIN_ROI_PARAMS (&LAST_NDPLUGIN_ROI_PARAM - &FIRST_NDPLUGIN_ROI_PARAM + 1)
    
#endif
