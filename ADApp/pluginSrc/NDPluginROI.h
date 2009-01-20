#ifndef NDPluginROI_H
#define NDPluginROI_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

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

/* The following enum is for each of the ROIs */
#define NDPluginROIFirstROINParam NDPluginDriverLastParam

typedef enum {
    NDPluginROIName                    /* (asynOctet,   r/w) Name of this ROI */
        = NDPluginROIFirstROINParam,
    NDPluginROIUse,                    /* (asynInt32,   r/w) Use this ROI? */
    NDPluginROIComputeStatistics,      /* (asynInt32,   r/w) Compute statistics for this ROI? */
    NDPluginROIComputeHistogram,       /* (asynInt32,   r/w) Compute histogram for this ROI? */
    NDPluginROIComputeProfiles,        /* (asynInt32,   r/w) Compute profiles for this ROI? */
    NDPluginROIHighlight,              /* (asynInt32,   r/w) Highlight ROIs? */
    
    /* ROI definition */
    NDPluginROIDim0Min,                /* (asynInt32,   r/w) Starting element of ROI in each dimension */
    NDPluginROIDim0Size,               /* (asynInt32,   r/w) Size of ROI in each dimension */
    NDPluginROIDim0MaxSize,            /* (asynInt32,   r/o) Maximum size of ROI in each dimension */
    NDPluginROIDim0Bin,                /* (asynInt32,   r/w) Binning of ROI in each dimension */
    NDPluginROIDim0Reverse,            /* (asynInt32,   r/w) Reversal of ROI in each dimension */
    NDPluginROIDim1Min,                /* (asynInt32,   r/w) Starting element of ROI in each dimension */
    NDPluginROIDim1Size,               /* (asynInt32,   r/w) Size of ROI in each dimension */
    NDPluginROIDim1MaxSize,            /* (asynInt32,   r/o) Maximum size of ROI in each dimension */
    NDPluginROIDim1Bin,                /* (asynInt32,   r/w) Binning of ROI in each dimension */
    NDPluginROIDim1Reverse,            /* (asynInt32,   r/w) Reversal of ROI in each dimension */
    NDPluginROIDim2Min,                /* (asynInt32,   r/w) Starting element of ROI in each dimension */
    NDPluginROIDim2Size,               /* (asynInt32,   r/w) Size of ROI in each dimension */
    NDPluginROIDim2MaxSize,            /* (asynInt32,   r/o) Maximum size of ROI in each dimension */
    NDPluginROIDim2Bin,                /* (asynInt32,   r/w) Binning of ROI in each dimension */
    NDPluginROIDim2Reverse,            /* (asynInt32,   r/w) Reversal of ROI in each dimension */
    NDPluginROIDataType,               /* (asynInt32,   r/w) Data type for ROI.  -1 means automatic. */
    
    /* ROI statistics */
    NDPluginROIBgdWidth,               /* (asynInt32,   r/w) Width of background region when computing net */
    NDPluginROIMinValue,               /* (asynFloat64, r/o) Minimum counts in any element */
    NDPluginROIMaxValue,               /* (asynFloat64, r/o) Maximum counts in any element */
    NDPluginROIMeanValue,              /* (asynFloat64, r/o) Mean counts of all elements */
    NDPluginROITotal,                  /* (asynFloat64, r/o) Sum of all elements */
    NDPluginROINet,                    /* (asynFloat64, r/o) Sum of all elements minus background */
    
    /* ROI histogram */
    NDPluginROIHistSize,               /* (asynInt32,   r/w) Number of elements in histogram */
    NDPluginROIHistMin,                /* (asynFloat64, r/w) Minimum value for histogram */
    NDPluginROIHistMax,                /* (asynFloat64, r/w) Maximum value for histogram */
    NDPluginROIHistEntropy,            /* (asynFloat64, r/o) Image entropy calculcated from histogram */
    NDPluginROIHistArray,              /* (asynFloat64Array, r/o) Histogram array */

    NDPluginROITotalArray,             /* (asynInt32Array, r/o) Total counts array */
    NDPluginROINetArray,               /* (asynInt32Array, r/o) Net counts array */

    NDPluginROILastROINParam
} NDPluginROINParam_t;


#define NUM_ROIN_PARAMS (sizeof(NDPluginROINParamString)/sizeof(NDPluginROINParamString[0]))

class NDPluginROI : public NDPluginDriver {
public:
    NDPluginROI(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxROIs, 
                 int maxBuffers, size_t maxMemory);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                             const char **pptypeName, size_t *psize);

    /* These methods are unique to this class */
    asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, 
                                size_t nelements, size_t *nIn);
                                
private:
    int maxROIs;
    NDROI_t *pROIs;    /* Array of NDROI structures */
    epicsInt32 *totalArray;
    epicsInt32 *netArray;
};
    
#endif
