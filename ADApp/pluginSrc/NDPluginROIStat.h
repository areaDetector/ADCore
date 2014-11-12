/**
 * See .cpp file for documentation.
 */

#ifndef NDPluginROIStat_H
#define NDPluginROIStat_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/* ROI general parameters */
#define NDPluginROIStatFirstString              "FIRST"
#define NDPluginROIStatLastString               "LAST"
#define NDPluginROIStatNameString               "NAME"                /* (asynOctet,   r/w) Name of this ROI */
#define NDPluginROIStatResetAllString           "RESETALL"            /* (asynInt32,   r/w) Reset ROI data for all ROIs. */
#define NDPluginROIStatNDArrayCallbacksString   "NDARRAY_CALLBACKS"   /* (asynInt32,   r/w) Do NDArray callbacks. */

/* ROI definition */
#define NDPluginROIStatUseString                "USE"               /* (asynInt32,   r/w) Use this ROI? */
#define NDPluginROIStatResetString              "RESET"             /* (asynInt32,   r/w) Reset ROI data. */
#define NDPluginROIStatDim0MinString            "DIM0_MIN"          /* (asynInt32,   r/w) Starting element of ROI in X dimension */
#define NDPluginROIStatDim0SizeString           "DIM0_SIZE"         /* (asynInt32,   r/w) Size of ROI in X dimension */
#define NDPluginROIStatDim0MaxSizeString        "DIM0_MAX_SIZE"     /* (asynInt32,   r/o) Maximum size of ROI in X dimension */
#define NDPluginROIStatDim1MinString            "DIM1_MIN"          /* (asynInt32,   r/w) Starting element of ROI in Y dimension */
#define NDPluginROIStatDim1SizeString           "DIM1_SIZE"         /* (asynInt32,   r/w) Size of ROI in Y dimension */
#define NDPluginROIStatDim1MaxSizeString        "DIM1_MAX_SIZE"     /* (asynInt32,   r/o) Maximum size of ROI in Y dimension */
#define NDPluginROIStatDim2MinString            "DIM2_MIN"          /* (asynInt32,   r/w) Starting element of ROI in Z dimension */
#define NDPluginROIStatDim2SizeString           "DIM2_SIZE"         /* (asynInt32,   r/w) Size of ROI in Z dimension */
#define NDPluginROIStatDim2MaxSizeString        "DIM2_MAX_SIZE"     /* (asynInt32,   r/o) Maximum size of ROI in Z dimension */

/* ROI statistics */
#define NDPluginROIStatMinValueString           "MIN_VALUE"         /* (asynFloat64, r/o) Minimum counts in any element */
#define NDPluginROIStatMaxValueString           "MAX_VALUE"         /* (asynFloat64, r/o) Maximum counts in any element */
#define NDPluginROIStatMeanValueString          "MEAN_VALUE"        /* (asynFloat64, r/o) Mean counts of all elements */
#define NDPluginROIStatTotalString              "TOTAL"             /* (asynFloat64, r/o) Sum of all elements */

/** Structure defining a Region-Of-Interest and Stats */
typedef struct NDROI {
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    int nElements;
    double total;
    double mean;
    double min;
    double max;
  int arraySizeX;
  int arraySizeY;
} NDROI_t;


/** Extract Regions-Of-Interest (ROI) from NDArray data; the plugin can be a source of NDArray callbacks for
  * other plugins, passing these sub-arrays. 
  * The plugin also optionally computes a statistics on the ROI. */
class NDPluginROIStat : public NDPluginDriver {
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
    int NDPluginROIStatResetAll;
    int NDPluginROIStatNDArrayCallbacks;

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

    int NDPluginROIStatLast;
    #define LAST_NDPLUGIN_ROISTAT_PARAM NDPluginROIStatLast
                                
private:

    template <typename epicsType> asynStatus doComputeStatisticsT(NDArray *pArray, NDROI_t *pROI);
    asynStatus doComputeStatistics(NDArray *pArray, NDROI_t *pStats);
    asynStatus clear(epicsUInt32 roi);

    int maxROIs;
    NDROI_t *pROIs;    /* Array of NDROI structures */
};

#define NUM_NDPLUGIN_ROISTAT_PARAMS (&LAST_NDPLUGIN_ROISTAT_PARAM - &FIRST_NDPLUGIN_ROISTAT_PARAM + 1)
    
#endif //NDPluginROIStat_H
