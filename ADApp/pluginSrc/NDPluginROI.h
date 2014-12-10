#ifndef NDPluginROI_H
#define NDPluginROI_H

#include "NDPluginDriver.h"

/* ROI general parameters */
#define NDPluginROINameString               "NAME"                /* (asynOctet,   r/w) Name of this ROI */

/* ROI definition */
#define NDPluginROIDim0MinString            "DIM0_MIN"          /* (asynInt32,   r/w) Starting element of ROI in each dimension */
#define NDPluginROIDim1MinString            "DIM1_MIN"          /* (asynInt32,   r/w) Starting element of ROI in each dimension */
#define NDPluginROIDim2MinString            "DIM2_MIN"          /* (asynInt32,   r/w) Starting element of ROI in each dimension */
#define NDPluginROIDim0SizeString           "DIM0_SIZE"         /* (asynInt32,   r/w) Size of ROI in each dimension */
#define NDPluginROIDim1SizeString           "DIM1_SIZE"         /* (asynInt32,   r/w) Size of ROI in each dimension */
#define NDPluginROIDim2SizeString           "DIM2_SIZE"         /* (asynInt32,   r/w) Size of ROI in each dimension */
#define NDPluginROIDim0MaxSizeString        "DIM0_MAX_SIZE"     /* (asynInt32,   r/o) Maximum size of ROI in each dimension */
#define NDPluginROIDim1MaxSizeString        "DIM1_MAX_SIZE"     /* (asynInt32,   r/o) Maximum size of ROI in each dimension */
#define NDPluginROIDim2MaxSizeString        "DIM2_MAX_SIZE"     /* (asynInt32,   r/o) Maximum size of ROI in each dimension */
#define NDPluginROIDim0BinString            "DIM0_BIN"          /* (asynInt32,   r/w) Binning of ROI in each dimension */
#define NDPluginROIDim1BinString            "DIM1_BIN"          /* (asynInt32,   r/w) Binning of ROI in each dimension */
#define NDPluginROIDim2BinString            "DIM2_BIN"          /* (asynInt32,   r/w) Binning of ROI in each dimension */
#define NDPluginROIDim0ReverseString        "DIM0_REVERSE"      /* (asynInt32,   r/w) Reversal of ROI in each dimension */
#define NDPluginROIDim1ReverseString        "DIM1_REVERSE"      /* (asynInt32,   r/w) Reversal of ROI in each dimension */
#define NDPluginROIDim2ReverseString        "DIM2_REVERSE"      /* (asynInt32,   r/w) Reversal of ROI in each dimension */
#define NDPluginROIDim0EnableString         "DIM0_ENABLE"       /* (asynInt32,   r/w) If set then do ROI in this dimension */
#define NDPluginROIDim1EnableString         "DIM1_ENABLE"       /* (asynInt32,   r/w) If set then do ROI in this dimension */
#define NDPluginROIDim2EnableString         "DIM2_ENABLE"       /* (asynInt32,   r/w) If set then do ROI in this dimension */
#define NDPluginROIDim0AutoSizeString       "DIM0_AUTO_SIZE"    /* (asynInt32,   r/w) Automatically set size to max */
#define NDPluginROIDim1AutoSizeString       "DIM1_AUTO_SIZE"    /* (asynInt32,   r/w) Automatically  set size to max */
#define NDPluginROIDim2AutoSizeString       "DIM2_AUTO_SIZE"    /* (asynInt32,   r/w) Automatically  set size to max */
#define NDPluginROIDataTypeString           "ROI_DATA_TYPE"     /* (asynInt32,   r/w) Data type for ROI.  -1 means automatic. */
#define NDPluginROIEnableScaleString        "ENABLE_SCALE"      /* (asynInt32,   r/w) Disable/Enable scaling */
#define NDPluginROIScaleString              "SCALE_VALUE"       /* (asynFloat64, r/w) Scaling value, used as divisor */

/** Extract Regions-Of-Interest (ROI) from NDArray data; the plugin can be a source of NDArray callbacks for
  * other plugins, passing these sub-arrays. 
  * The plugin also optionally computes a statistics on the ROI. */
class epicsShareClass NDPluginROI : public NDPluginDriver {
public:
    NDPluginROI(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
    /* ROI general parameters */
    int NDPluginROIName;
    #define FIRST_NDPLUGIN_ROI_PARAM NDPluginROIName

    /* ROI definition */
    int NDPluginROIDim0Min;
    int NDPluginROIDim1Min;
    int NDPluginROIDim2Min;
    int NDPluginROIDim0Size;
    int NDPluginROIDim1Size;
    int NDPluginROIDim2Size;
    int NDPluginROIDim0MaxSize;
    int NDPluginROIDim1MaxSize;
    int NDPluginROIDim2MaxSize;
    int NDPluginROIDim0Bin;
    int NDPluginROIDim1Bin;
    int NDPluginROIDim2Bin;
    int NDPluginROIDim0Reverse;
    int NDPluginROIDim1Reverse;
    int NDPluginROIDim2Reverse;
    int NDPluginROIDim0Enable;
    int NDPluginROIDim1Enable;    
    int NDPluginROIDim2Enable;    
    int NDPluginROIDim0AutoSize;    
    int NDPluginROIDim1AutoSize;    
    int NDPluginROIDim2AutoSize;    
    int NDPluginROIDataType;
    int NDPluginROIEnableScale;
    int NDPluginROIScale;

    #define LAST_NDPLUGIN_ROI_PARAM NDPluginROIScale
                                
private:
    int requestedSize_[3];
    int requestedOffset_[3];
};
#define NUM_NDPLUGIN_ROI_PARAMS ((int)(&LAST_NDPLUGIN_ROI_PARAM - &FIRST_NDPLUGIN_ROI_PARAM + 1))
    
#endif
