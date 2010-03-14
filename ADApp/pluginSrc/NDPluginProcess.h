#ifndef NDPluginProcess_H
#define NDPluginProcess_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/* Background array subtraction */
#define NDPluginProcessSaveBackgroundString     "SAVE_BACKGROUND"     /* (asynInt32,   r/w) Save the current frame as background */
#define NDPluginProcessEnableBackgroundString   "ENABLE_BACKGROUND"   /* (asynInt32,   r/w) Enable background subtraction? */
#define NDPluginProcessValidBackgroundString    "VALID_BACKGROUND"    /* (asynInt32,   r/o) Is there a valid background */

/* Flat field normalization */
#define NDPluginProcessSaveFlatFieldString      "SAVE_FLAT_FIELD"     /* (asynInt32,   r/w) Save the current frame as flat field */
#define NDPluginProcessEnableFlatFieldString    "ENABLE_FLAT_FIELD"   /* (asynInt32,   r/w) Enable flat field normalization? */
#define NDPluginProcessValidFlatFieldString     "VALID_FLAT_FIELD"    /* (asynInt32,   r/o) Is there a valid flat field */
#define NDPluginProcessScaleFlatFieldString     "SCALE_FLAT_FIELD"    /* (asynInt32,   r/o) Scale factor after dividing by flat field */

/* High and low clipping */
#define NDPluginProcessLowClipString            "LOW_CLIP"          /* (asynFloat64, r/w) Low clip value */
#define NDPluginProcessEnableLowClipString      "ENABLE_LOW_CLIP"   /* (asynInt32,   r/w) Enable low clipping? */
#define NDPluginProcessHighClipString           "HIGH_CLIP"         /* (asynFloat64, r/w) High clip value */
#define NDPluginProcessEnableHighClipString     "ENABLE_HIGH_CLIP"  /* (asynInt32,   r/w) Enable high clipping? */
    
/* Frame averaging */
#define NDPluginProcessEnableAverageString      "ENABLE_AVERAGE"      /* (asynInt32,   r/w) Enable frame averaging? */
#define NDPluginProcessNumAverageString         "NUM_AVERAGE"         /* (asynInt32,   r/o) Number of frames to average */
#define NDPluginProcessNumAveragedString        "NUM_AVERAGED"        /* (asynInt32,   r/o) Number of frames in current average */

/* Output data type */
#define NDPluginProcessDataTypeString           "PROCESS_DATA_TYPE"   /* (asynInt32,   r/w) Output type.  -1 means automatic. */
   

/** Does image processing operations.  These include
  * Background subtraction
  * Flat field normalization
  * Low clipping
  * High clipping
  * Frame averaging */
class NDPluginProcess : public NDPluginDriver {
public:
    NDPluginProcess(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    
protected:
    /* Background array subtraction */
    int NDPluginProcessSaveBackground;
    #define FIRST_NDPLUGIN_PROCESS_PARAM NDPluginProcessSaveBackground
    int NDPluginProcessEnableBackground;
    int NDPluginProcessValidBackground;

    /* Flat field normalization */
    int NDPluginProcessSaveFlatField;
    int NDPluginProcessEnableFlatField;
    int NDPluginProcessValidFlatField;
    int NDPluginProcessScaleFlatField;

    /* High and low clipping */
    int NDPluginProcessLowClip;
    int NDPluginProcessEnableLowClip;
    int NDPluginProcessHighClip;
    int NDPluginProcessEnableHighClip;

    /* Frame averaging */
    int NDPluginProcessEnableAverage;
    int NDPluginProcessNumAverage;
    int NDPluginProcessNumAveraged;
    
    /* Output data type */
    int NDPluginProcessDataType;

    #define LAST_NDPLUGIN_PROCESS_PARAM NDPluginProcessDataType
                                
private:
    NDArray *pBackground;
    int     nBackgroundElements;
    NDArray *pFlatField;
    int     nFlatFieldElements;
    NDArray *pAverage;
    int     numAveraged;
};
#define NUM_NDPLUGIN_PROCESS_PARAMS (&LAST_NDPLUGIN_PROCESS_PARAM - &FIRST_NDPLUGIN_PROCESS_PARAM + 1)
    
#endif
