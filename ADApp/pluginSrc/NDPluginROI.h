#ifndef NDPluginROI_H
#define NDPluginROI_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "ADInterface.h"
#include "NDPluginBase.h"

typedef struct NDROI {
    PARAMS params;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    NDArray_t *pArray;
    double *histogram;
    int histogramSize;
    double *profiles[ND_ARRAY_MAX_DIMS];
    int profileSize[ND_ARRAY_MAX_DIMS];
} NDROI_t;

/* The following enum is for each of the ROIs */
#define NDPluginROIFirstROINParam NDPluginBaseLastParam

typedef enum {
    NDPluginROIName = NDPluginROIFirstROINParam,
                             /* (asynOctet,   r/w) Name of this ROI */
    NDPluginROIUse,                /* (asynInt32,   r/w) Use this ROI? */
    NDPluginROIComputeStatistics,  /* (asynInt32,   r/w) Compute statistics for this ROI? */
    NDPluginROIComputeHistogram,   /* (asynInt32,   r/w) Compute histogram for this ROI? */
    NDPluginROIComputeProfiles,    /* (asynInt32,   r/w) Compute profiles for this ROI? */
    NDPluginROIHighlight,          /* (asynInt32,   r/w) Highlight other ROIs in this ROI? */
    
    /* ROI definition */
    NDPluginROIDim0Min,            /* (asynInt32,   r/w) Starting element of ROI in each dimension */
    NDPluginROIDim0Size,           /* (asynInt32,   r/w) Size of ROI in each dimension */
    NDPluginROIDim0Bin,            /* (asynInt32,   r/w) Binning of ROI in each dimension */
    NDPluginROIDim0Reverse,        /* (asynInt32,   r/w) Reversal of ROI in each dimension */
    NDPluginROIDim1Min,            /* (asynInt32,   r/w) Starting element of ROI in each dimension */
    NDPluginROIDim1Size,           /* (asynInt32,   r/w) Size of ROI in each dimension */
    NDPluginROIDim1Bin,            /* (asynInt32,   r/w) Binning of ROI in each dimension */
    NDPluginROIDim1Reverse,        /* (asynInt32,   r/w) Reversal of ROI in each dimension */
    NDPluginROIDataType,           /* (asynInt32,   r/w) Data type for ROI.  -1 means automatic. */
    
    /* ROI statistics */
    NDPluginROIBgdWidth,           /* (asynFloat64, r/w) Width of background region when computing net */
    NDPluginROIMinValue,           /* (asynFloat64, r/o) Minimum counts in any element */
    NDPluginROIMaxValue,           /* (asynFloat64, r/o) Maximum counts in any element */
    NDPluginROIMeanValue,          /* (asynFloat64, r/o) Mean counts of all elements */
    NDPluginROITotal,              /* (asynFloat64, r/o) Sum of all elements */
    NDPluginROINet,                /* (asynFloat64, r/o) Sum of all elements minus background */
    
    /* ROI histogram */
    NDPluginROIHistSize,           /* (asynInt32,   r/w) Number of elements in histogram */
    NDPluginROIHistMin,            /* (asynFloat64, r/w) Minimum value for histogram */
    NDPluginROIHistMax,            /* (asynFloat64, r/w) Maximum value for histogram */
    NDPluginROIHistEntropy,        /* (asynFloat64, r/o) Image entropy calculcated from histogram */
    NDPluginROIHistArray,          /* (asynFloat64Array, r/o) Histogram array */

    NDPluginROILastROINParam
} NDPluginROINParam_t;

static ADParamString_t NDPluginROINParamString[] = {
    {NDPluginROIName,               "NAME"},
    {NDPluginROIUse,                "USE"},
    {NDPluginROIComputeStatistics,  "COMPUTE_STATISTICS"},
    {NDPluginROIComputeHistogram,   "COMPUTE_HISTOGRAM"},
    {NDPluginROIComputeProfiles,    "COMPUTE_PROFILES"},
    {NDPluginROIHighlight,          "HIGHLIGHT"},

    {NDPluginROIDim0Min,            "DIM0_MIN"},
    {NDPluginROIDim0Size,           "DIM0_SIZE"},
    {NDPluginROIDim0Bin,            "DIM0_BIN"},
    {NDPluginROIDim0Reverse,        "DIM0_REVERSE"},
    {NDPluginROIDim1Min,            "DIM1_MIN"},
    {NDPluginROIDim1Size,           "DIM1_SIZE"},
    {NDPluginROIDim1Bin,            "DIM1_BIN"},
    {NDPluginROIDim1Reverse,        "DIM1_REVERSE"},
    {NDPluginROIDataType,           "DATA_TYPE"},

    {NDPluginROIBgdWidth,           "BGD_WIDTH"},
    {NDPluginROIMinValue,           "MIN_VALUE"},
    {NDPluginROIMaxValue,           "MAX_VALUE"},
    {NDPluginROIMeanValue,          "MEAN_VALUE"},
    {NDPluginROITotal,              "TOTAL"},
    {NDPluginROINet,                "NET"},

    {NDPluginROIHistSize,           "HIST_SIZE"},
    {NDPluginROIHistMin,            "HIST_MIN"},
    {NDPluginROIHistMax,            "HIST_MAX"},
    {NDPluginROIHistEntropy,        "HIST_ENTROPY"},
    {NDPluginROIHistArray,          "HIST_ARRAY"},
};

#define NUM_ROIN_PARAMS (sizeof(NDPluginROINParamString)/sizeof(NDPluginROINParamString[0]))

class NDPluginROI : public NDPluginBase {
public:
    NDPluginROI(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxROIs);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray_t *pArray);
    asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars,
                          size_t *nActual, int *eomReason);
    asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    asynStatus readNDArray(asynUser *pasynUser, void *handle);
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                             const char **pptypeName, size_t *psize);
    void report(FILE *fp, int details);

    /* These methods are unique to this class */
    asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, 
                                size_t nelements, size_t *nIn);
    asynStatus writeFloat64Array(asynUser *pasynUser, 
                                 epicsFloat64 *value, size_t nelements);
    void float64ArrayCallback(epicsFloat64 *pData, int len, int reason, int addr);

private:
    int maxROIs;
    NDROI_t *pROIs;    /* Array of drvNDROI structures */
};
    
#endif
