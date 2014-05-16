#ifndef NDPluginTransform_H
#define NDPluginTransform_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"


/* The following enum is for each of the Transforms */
#define NDPluginTransformFirstTransformNParam NDPluginDriverLastParam

/** Structure that defines the elements of a transformation */
typedef struct NDTransform {
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    int type;
} NDTransform_t;

/** Structure to define a pair of indices */
typedef struct {
    size_t index0;
    size_t index1;
    size_t index2;
} NDTransformIndex_t;

/* Enums to describe the types of transformations */
typedef enum {
    TransformNone,
    TransformRotateCW90,
    TransformRotateCCW90,
    TransformRotate180,
    TransformTranspose,
    TransformTransposeRotate180,
    TransformFlipHoriz,
    TransformFlipVert,
} NDPluginTransformType_t;

/** Map parameter enums to strings that will be used to set up EPICS databases
  */
#define NDPluginTransformNameString         "NAME"
#define NDPluginTransformTypeString         "TYPE"
#define NDPluginTransformDim0MaxSizeString  "DIM0_MAX_SIZE"
#define NDPluginTransformDim1MaxSizeString  "DIM1_MAX_SIZE"
#define NDPluginTransformDim2MaxSizeString  "DIM2_MAX_SIZE"
#define NDPluginTransformArraySize0String   "ARRAY_SIZE_0"
#define NDPluginTransformArraySize1String   "ARRAY_SIZE_1"
#define NDPluginTransformArraySize2String   "ARRAY_SIZE_2"

typedef NDTransformIndex_t (*transformFunctions_t) (NDTransformIndex_t, int, int) ;

static const char* pluginName = "NDPluginTransform";


/** Perform transformations (rotations, flips) on NDArrays.   */
class NDPluginTransform : public NDPluginDriver {
public:
    NDPluginTransform(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

    /* These methods are unique to this class */

protected:
    int NDPluginTransformName;           /* (asynOctet;   r/w) Name of this Transform */
    #define FIRST_TRANSFORM_PARAM NDPluginTransformName
    int NDPluginTransformType;
    int NDPluginTransformDim0MaxSize;
    int NDPluginTransformDim1MaxSize;
    int NDPluginTransformDim2MaxSize;
    int NDPluginTransformArraySize0;
    int NDPluginTransformArraySize1;
    int NDPluginTransformArraySize2;
    #define LAST_TRANSFORM_PARAM NDPluginTransformArraySize2

private:
    size_t userDims[ND_ARRAY_MAX_DIMS];
    NDTransform_t transformStruct;
    void transformImage(NDArray *inArray, NDArray *outArray, NDArrayInfo_t *arrayInfo);
};
#define NUM_TRANSFORM_PARAMS ((int)(&LAST_TRANSFORM_PARAM - &FIRST_TRANSFORM_PARAM + 1))

#endif
