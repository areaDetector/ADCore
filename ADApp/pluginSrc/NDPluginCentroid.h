#ifndef NDPluginCentroid_H
#define NDPluginCentroid_H

#include "NDPluginDriver.h"

typedef struct NDCentroid {
    double  centroidThreshold;
    double  centroidX;
    double  centroidY;
} NDCentroid_t;

/* Centroid */
#define NDPluginCentroidComputeString    "COMPUTE_CENTROID"    /* (asynInt32,        r/w) Compute centroid? */
#define NDPluginCentroidThresholdString  "CENTROID_THRESHOLD"  /* (asynFloat64,      r/w) Threshold when computing centroids */
#define NDPluginCentroidXString          "CENTROIDX_VALUE"     /* (asynFloat64,      r/o) X centroid */
#define NDPluginCentroidYString          "CENTROIDY_VALUE"     /* (asynFloat64,      r/o) Y centroid */

#define NDPluginCentroidCallbackPeriodString     "CALLBACK_PERIOD"     /* (asynFloat64,      r/w) Callback period */

/** Does image 
  * X and Y centroid 
  */
class NDPLUGIN_API NDPluginCentroid : public NDPluginDriver {
public:
    NDPluginCentroid(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize, int maxThreads=1);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

    template <typename epicsType> asynStatus doComputeCentroidT(NDArray *pArray, NDCentroid_t *pCentroid);
    asynStatus doComputeCentroid(NDArray *pArray, NDCentroid_t *pCentroid);

protected:
    /* Centroid */
    int NDPluginCentroidCompute;
    #define FIRST_NDPLUGIN_CENTROID_PARAM NDPluginCentroidCompute
   
    int NDPluginCentroidThreshold;
    int NDPluginCentroidX;
    int NDPluginCentroidY;

};

#endif
