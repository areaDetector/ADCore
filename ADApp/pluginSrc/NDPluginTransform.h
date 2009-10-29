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
	int index0;
	int index1;
} NDTransformIndex_t;

/* Enums to describe the types of transformations */
typedef enum {
	TransformNone,
	TransformRotateCW90,
	TransformRotateCCW90,
	TransformRotate180,
	TransformFlip0011,
	TransformFlip0110,
	TransformFlipX,
	TransformFlipY,
} NDPluginTransformType_t;

/** Enums to describe location of origin */
typedef enum {
	TransformOriginLL,
	TransformOriginUL,
	TransformOriginLR,
	TransformOriginUR,
} NDPluginTransformOrigin_t;

/** Enums for plugin-specific parameters. */
typedef enum {
    NDPluginTransformName                    /* (asynOctet,   r/w) Name of this Transform */
        = NDPluginTransformFirstTransformNParam,
	NDPluginTransform1Type,
	NDPluginTransform2Type,
	NDPluginTransform3Type,
	NDPluginTransform4Type,
	NDPluginTransformOrigin,
	NDPluginTransform1Dim0MaxSize,
	NDPluginTransform1Dim1MaxSize,
	NDPluginTransform2Dim0MaxSize,
	NDPluginTransform2Dim1MaxSize,
	NDPluginTransform3Dim0MaxSize,
	NDPluginTransform3Dim1MaxSize,
	NDPluginTransform4Dim0MaxSize,
	NDPluginTransform4Dim1MaxSize,

	NDPluginTransformArraySize0,
	NDPluginTransformArraySize1,

    NDPluginTransformLastTransformParam
} NDPluginTransformParam_t;

typedef NDTransformIndex_t (*transformFunctions_t) (NDTransformIndex_t, int, int) ;

#define NUM_Transform_PARAMS (sizeof(NDPluginTransformParamString)/sizeof(NDPluginTransformParamString[0]))

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
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                             const char **pptypeName, size_t *psize);

    /* These methods are unique to this class */

private:

    int userDims[ND_ARRAY_MAX_DIMS];
    int transformFlipsAxes(int);
    void setMaxSizes(int);
    int maxTransforms;
    NDTransform_t *pTransforms;    /* Array of NDTransform structures */
	int originLocation;
    epicsInt32 *totalArray;
    epicsInt32 *netArray;

	NDTransformIndex_t transformNone(NDTransformIndex_t indexIn, int originLocation, int transformNumber);
	NDTransformIndex_t transformRotateCW90(NDTransformIndex_t indexIn, int originLocation, int transformNumber);
	NDTransformIndex_t transformRotateCCW90(NDTransformIndex_t indexIn, int originLocation, int transformNumber);
	NDTransformIndex_t transformRotate180(NDTransformIndex_t indexIn, int originLocation, int transformNumber);
	NDTransformIndex_t transformFlip0011(NDTransformIndex_t indexIn, int originLocation, int transformNumber);
	NDTransformIndex_t transformFlip0110(NDTransformIndex_t indexIn, int originLocation, int transformNumber);
	NDTransformIndex_t transformFlipX(NDTransformIndex_t indexIn, int originLocation, int transformNumber);
	NDTransformIndex_t transformFlipY(NDTransformIndex_t indexIn, int originLocation, int transformNumber);
	void transformArray(NDArray *inArray, NDArray *outArray);

};

#endif
