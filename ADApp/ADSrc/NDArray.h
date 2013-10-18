/** NDArray.h
 *
 * N-dimensional array definition
 *
 *
 * Mark Rivers
 * University of Chicago
 * May 10, 2008
 *
 */

#ifndef NDArray_H
#define NDArray_H

#include <epicsMutex.h>
#include <epicsTime.h>
#include <stdio.h>

#include "NDAttribute.h"
#include "NDAttributeList.h"

/** The maximum number of dimensions in an NDArray */
#define ND_ARRAY_MAX_DIMS 10

/** Enumeration of color modes for NDArray attribute "colorMode" */
typedef enum
{
    NDColorModeMono,    /**< Monochromatic image */
    NDColorModeBayer,   /**< Bayer pattern image, 1 value per pixel but with color filter on detector */
    NDColorModeRGB1,    /**< RGB image with pixel color interleave, data array is [3, NX, NY] */
    NDColorModeRGB2,    /**< RGB image with row color interleave, data array is [NX, 3, NY]  */
    NDColorModeRGB3,    /**< RGB image with plane color interleave, data array is [NX, NY, 3]  */
    NDColorModeYUV444,  /**< YUV image, 3 bytes encodes 1 RGB pixel */
    NDColorModeYUV422,  /**< YUV image, 4 bytes encodes 2 RGB pixel */
    NDColorModeYUV411   /**< YUV image, 6 bytes encodes 4 RGB pixels */
} NDColorMode_t;

/** Enumeration of Bayer patterns for NDArray attribute "bayerPattern".
  * This value is only meaningful if colorMode is NDColorModeBayer. 
  * This value is needed because the Bayer pattern will change when reading out a 
  * subset of the chip, for example if the X or Y offset values are not even numbers */
typedef enum
{
    NDBayerRGGB        = 0,    /**< First line RGRG, second line GBGB... */
    NDBayerGBRG        = 1,    /**< First line GBGB, second line RGRG... */
    NDBayerGRBG        = 2,    /**< First line GRGR, second line BGBG... */
    NDBayerBGGR        = 3     /**< First line BGBG, second line GRGR... */
} NDBayerPattern_t;

/** Structure defining a dimension of an NDArray */
typedef struct NDDimension {
    size_t size;    /**< The number of elements in this dimension of the array */
    size_t offset;  /**< The offset relative to the origin of the original data source (detector, for example).
                      * If a selected region of the detector is being read, then this value may be > 0. 
                      * The offset value is cumulative, so if a plugin such as NDPluginROI further selects
                      * a subregion, the offset is relative to the first element in the detector, and not 
                      * to the first element of the region passed to NDPluginROI. */
    int binning;    /**< The binning (pixel summation, 1=no binning) relative to original data source (detector, for example)
                      * The offset value is cumulative, so if a plugin such as NDPluginROI performs binning,
                      * the binning is expressed relative to the pixels in the detector and not to the possibly
                      * binned pixels passed to NDPluginROI.*/
    int reverse;    /**< The orientation (0=normal, 1=reversed) relative to the original data source (detector, for example)
                      * This value is cumulative, so if a plugin such as NDPluginROI reverses the data, the value must
                      * reflect the orientation relative to the original detector, and not to the possibly
                      * reversed data passed to NDPluginROI. */
} NDDimension_t;

/** Structure returned by NDArray::getInfo */
typedef struct NDArrayInfo {
    size_t nElements;       /**< The total number of elements in the array */
    int bytesPerElement;    /**< The number of bytes per element in the array */
    size_t totalBytes;      /**< The total number of bytes required to hold the array;
                              *  this may be less than NDArray::dataSize. */
                            /**< The following are mostly useful for color images (RGB1, RGB2, RGB3) */
    NDColorMode_t colorMode; /**< The color mode */
    int xDim;               /**< The array index which is the X dimension */
    int yDim;               /**< The array index which is the Y dimension */
    int colorDim;           /**< The array index which is the color dimension */
    size_t xSize;           /**< The X size of the array */
    size_t ySize;           /**< The Y size of the array */
    size_t colorSize;       /**< The color size of the array */
    size_t xStride;         /**< The number of array elements between X values */
    size_t yStride;         /**< The number of array elements between Y values */
    size_t colorStride;     /**< The number of array elements between color values */
} NDArrayInfo_t;

/** N-dimensional array class; each array has a set of dimensions, a data type, pointer to data, and optional attributes. 
  * An NDArray also has a uniqueId and timeStamp that to identify it. NDArray objects can be allocated
  * by an NDArrayPool object, which maintains a free list of NDArrays for efficient memory management. */
class epicsShareClass NDArray {
public:
    /* Methods */
    NDArray();
    ~NDArray();
    int          initDimension   (NDDimension_t *pDimension, size_t size);
    int          getInfo         (NDArrayInfo_t *pInfo);
    int          reserve();
    int          release();
    int          report(FILE *fp, int details);
    friend class NDArrayPool;
    
private:
    ELLNODE      node;              /**< This must come first because ELLNODE must have the same address as NDArray object */
    int          referenceCount;    /**< Reference count for this NDArray=number of clients who are using it */

public:
    class NDArrayPool *pNDArrayPool; /**< The NDArrayPool object that created this array */
    int           uniqueId;     /**< A number that must be unique for all NDArrays produced by a driver after is has started */
    double        timeStamp;    /**< The time stamp in seconds for this array; seconds since Epoch (00:00:00 UTC, January 1, 1970)
                                  * is recommended, but some drivers may use a different start time.*/
    epicsTimeStamp epicsTS;     /**< The epicsTimeStamp; this is set with pasynManager->updateTimeStamp(), 
                                  * and can come from a user-defined timestamp source. */
    int           ndims;        /**< The number of dimensions in this array; minimum=1. */
    NDDimension_t dims[ND_ARRAY_MAX_DIMS]; /**< Array of dimension sizes for this array; first ndims values are meaningful. */
    NDDataType_t  dataType;     /**< Data type for this array. */
    size_t        dataSize;     /**< Data size for this array; actual amount of memory allocated for *pData, may be more than
                                  * required to hold the array*/
    void          *pData;       /**< Pointer to the array data.
                                  * The data is assumed to be stored in the order of dims[0] changing fastest, and 
                                  * dims[ndims-1] changing slowest. */
    NDAttributeList *pAttributeList;  /**< Linked list of attributes */
};

/** The NDArrayPool class manages a free list (pool) of NDArray objects.
  * Drivers allocate NDArray objects from the pool, and pass these objects to plugins.
  * Plugins increase the reference count on the object when they place the object on
  * their queue, and decrease the reference count when they are done processing the
  * array. When the reference count reaches 0 again the NDArray object is placed back
  * on the free list. This mechanism minimizes the copying of array data in plugins.
  */
class epicsShareClass NDArrayPool {
public:
    NDArrayPool  (int maxBuffers, size_t maxMemory);
    NDArray*     alloc     (int ndims, size_t *dims, NDDataType_t dataType, size_t dataSize, void *pData);
    NDArray*     copy      (NDArray *pIn, NDArray *pOut, int copyData);

    int          reserve   (NDArray *pArray);
    int          release   (NDArray *pArray);
    int          convert   (NDArray *pIn,
                            NDArray **ppOut,
                            NDDataType_t dataTypeOut,
                            NDDimension_t *outDims);
    int          convert   (NDArray *pIn,
                            NDArray **ppOut,
                            NDDataType_t dataTypeOut);
    int          report     (FILE  *fp, int details);
    int          maxBuffers ();
    int          numBuffers ();
    size_t       maxMemory  ();
    size_t       memorySize ();
    int          numFree    ();
private:
    ELLLIST      freeList_;      /**< Linked list of free NDArray objects that form the pool */
    epicsMutexId listLock_;      /**< Mutex to protect the free list */
    int          maxBuffers_;    /**< Maximum number of buffers this object is allowed to allocate; -1=unlimited */
    int          numBuffers_;    /**< Number of buffers this object has currently allocated */
    size_t       maxMemory_;     /**< Maximum bytes of memory this object is allowed to allocate; -1=unlimited */
    size_t       memorySize_;    /**< Number of bytes of memory this object has currently allocated */
    int          numFree_;       /**< Number of NDArray objects in the free list */
};

#endif
