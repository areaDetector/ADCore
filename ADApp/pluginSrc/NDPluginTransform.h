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

/** Map parameter enums to strings that will be used to set up EPICS databases
  */
#define NDPluginTransformNameString         "NAME"
#define NDPluginTransform1TypeString        "TYPE1"
#define NDPluginTransform2TypeString        "TYPE2"
#define NDPluginTransform3TypeString        "TYPE3"
#define NDPluginTransform4TypeString        "TYPE4"
#define NDPluginTransformOriginString       "ORIGIN"
#define NDPluginTransform1Dim0MaxSizeString "T1_DIM0_MAX_SIZE"
#define NDPluginTransform1Dim1MaxSizeString "T1_DIM1_MAX_SIZE"
#define NDPluginTransform1Dim2MaxSizeString "T1_DIM2_MAX_SIZE"
#define NDPluginTransform2Dim0MaxSizeString "T2_DIM0_MAX_SIZE"
#define NDPluginTransform2Dim1MaxSizeString "T2_DIM1_MAX_SIZE"
#define NDPluginTransform2Dim2MaxSizeString "T2_DIM2_MAX_SIZE"
#define NDPluginTransform3Dim0MaxSizeString "T3_DIM0_MAX_SIZE"
#define NDPluginTransform3Dim1MaxSizeString "T3_DIM1_MAX_SIZE"
#define NDPluginTransform3Dim2MaxSizeString "T3_DIM2_MAX_SIZE"
#define NDPluginTransform4Dim0MaxSizeString "T4_DIM0_MAX_SIZE"
#define NDPluginTransform4Dim1MaxSizeString "T4_DIM1_MAX_SIZE"
#define NDPluginTransform4Dim2MaxSizeString "T4_DIM2_MAX_SIZE"
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
    int NDPluginTransform1Type;
    int NDPluginTransform2Type;
    int NDPluginTransform3Type;
    int NDPluginTransform4Type;
    int NDPluginTransformOrigin;
    int NDPluginTransform1Dim0MaxSize;
    int NDPluginTransform1Dim1MaxSize;
    int NDPluginTransform1Dim2MaxSize;
    int NDPluginTransform2Dim0MaxSize;
    int NDPluginTransform2Dim1MaxSize;
    int NDPluginTransform2Dim2MaxSize;
    int NDPluginTransform3Dim0MaxSize;
    int NDPluginTransform3Dim1MaxSize;
    int NDPluginTransform3Dim2MaxSize;
    int NDPluginTransform4Dim0MaxSize;
    int NDPluginTransform4Dim1MaxSize;
    int NDPluginTransform4Dim2MaxSize;
    int NDPluginTransformArraySize0;
    int NDPluginTransformArraySize1;
    int NDPluginTransformArraySize2;
    #define LAST_TRANSFORM_PARAM NDPluginTransformArraySize2

private:

    size_t userDims[ND_ARRAY_MAX_DIMS];
    size_t realDims[ND_ARRAY_MAX_DIMS];
    int transformFlipsAxes(int);
    void setMaxSizes(int);
    int maxTransforms;
    NDTransform_t *pTransforms;    /* Array of NDTransform structures */
    size_t originLocation;
    epicsInt32 *totalArray;
    epicsInt32 *netArray;

    NDTransformIndex_t transformNone(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber);
    NDTransformIndex_t transformRotateCW90(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber);
    NDTransformIndex_t transformRotateCCW90(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber);
    NDTransformIndex_t transformRotate180(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber);
    NDTransformIndex_t transformFlip0011(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber);
    NDTransformIndex_t transformFlip0110(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber);
    NDTransformIndex_t transformFlipX(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber);
    NDTransformIndex_t transformFlipY(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber);
    NDTransformIndex_t transformPixel(NDTransformIndex_t indexIn, size_t originLocation);
    void transform2DArray(NDArray *inArray, NDArray *outArray);
    void transform3DArray(NDArray *inArray, NDArray *outArray);
    void moveStdPixel(NDArray *inArray, NDTransformIndex_t pixelIndexIn, NDArray *outArray, NDTransformIndex_t pixelIndexOut);
    void moveRGB1Pixel(NDArray *inArray, NDTransformIndex_t pixelIndexIn, NDArray *outArray, NDTransformIndex_t pixelIndexOut);
    void moveRGB2Pixel(NDArray *inArray, NDTransformIndex_t pixelIndexIn, NDArray *outArray, NDTransformIndex_t pixelIndexOut);

};
#define NUM_TRANSFORM_PARAMS ((int)(&LAST_TRANSFORM_PARAM - &FIRST_TRANSFORM_PARAM + 1))

#endif
