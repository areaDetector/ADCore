#ifndef NDPluginTransform_H
#define NDPluginTransform_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/** Map parameter enums to strings that will be used to set up EPICS databases
  */
#define NDPluginTransformTypeString  "TRANSFORM_TYPE"

static const char* pluginName = "NDPluginTransform";

/** Perform transformations (rotations, flips) on NDArrays.   */
class epicsShareClass NDPluginTransform : public NDPluginDriver {
public:
    NDPluginTransform(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

protected:
    int NDPluginTransformType_;
    #define FIRST_TRANSFORM_PARAM NDPluginTransformType_
    #define LAST_TRANSFORM_PARAM NDPluginTransformType_

private:
    size_t userDims_[ND_ARRAY_MAX_DIMS];
    void transformImage(NDArray *inArray, NDArray *outArray, NDArrayInfo_t *arrayInfo);
};
#define NUM_TRANSFORM_PARAMS ((int)(&LAST_TRANSFORM_PARAM - &FIRST_TRANSFORM_PARAM + 1))

#endif
