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

#ifndef ND_ARRAY_H
#define ND_ARRAY_H

#include <ellLib.h>
#include <epicsMutex.h>
#include <epicsTypes.h>

/** The maximum number of dimensions in an NDArray */
#define ND_ARRAY_MAX_DIMS 10
/** Success return code  */
#define ND_SUCCESS 0
/** Failure return code  */
#define ND_ERROR -1

#define MAX_ATTRIBUTE_STRING_SIZE 256

/** Enumeration of NDArray data types */
typedef enum
{
    NDInt8,     /**< Signed 8-bit integer */
    NDUInt8,    /**< Unsigned 8-bit integer */
    NDInt16,    /**< Signed 16-bit integer */
    NDUInt16,   /**< Unsigned 16-bit integer */
    NDInt32,    /**< Signed 32-bit integer */
    NDUInt32,   /**< Unsigned 32-bit integer */
    NDFloat32,  /**< 32-bit float */
    NDFloat64   /**< 64-bit float */
} NDDataType_t;

/** Enumeration of NDAttribute attribute data types */
typedef enum
{
    NDAttrInt8    = NDInt8,     /**< Signed 8-bit integer */
    NDAttrUInt8   = NDUInt8,    /**< Unsigned 8-bit integer */
    NDAttrInt16   = NDInt16,    /**< Signed 16-bit integer */
    NDAttrUInt16  = NDUInt16,   /**< Unsigned 16-bit integer */
    NDAttrInt32   = NDInt32,    /**< Signed 32-bit integer */
    NDAttrUInt32  = NDUInt32,   /**< Unsigned 32-bit integer */
    NDAttrFloat32 = NDFloat32,  /**< 32-bit float */
    NDAttrFloat64 = NDFloat64,  /**< 64-bit float */
    NDAttrString,               /**< Dynamic length string */
    NDAttrUndefined             /**< Undefined data type */
} NDAttrDataType_t;

/** Enumeration of NDAttibute source types */
typedef enum
{
    NDAttrSourceDriver,  /**< Attribute is obtained directly from driver */
    NDAttrSourceParam,   /**< Attribute is obtained from parameter library */
    NDAttrSourceEPICSPV  /**< Attribute is obtained from an EPICS PV */
} NDAttrSource_t;

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

/** Structure used by the EPICS ellLib library for linked lists of C++ objects.
  * This is needed for ellLists of C++ objects, for which making the first data element the ELLNODE 
  * does not work if the class has virtual functions or derived classes. */
typedef struct NDAttributeListNode {
    ELLNODE node;
    class NDAttribute *pNDAttribute;
} NDAttributeListNode;

/** Union defining the values in an NDAttribute object */
typedef union {
    epicsInt8    i8;    /**< Signed 8-bit integer */
    epicsUInt8   ui8;   /**< Unsigned 8-bit integer */
    epicsInt16   i16;   /**< Signed 16-bit integer */
    epicsUInt16  ui16;  /**< Unsigned 16-bit integer */
    epicsInt32   i32;   /**< Signed 32-bit integer */
    epicsUInt32  ui32;  /**< Unsigned 32-bit integer */
    epicsFloat32 f32;   /**< 32-bit float */
    epicsFloat64 f64;   /**< 64-bit float */
} NDAttrValue;

/** NDAttribute class; an attribute has a name, description, source type, source string,
  * data type, and value.
  */
class epicsShareFunc NDAttribute {
public:
    /* Methods */
    NDAttribute(const char *pName, const char *pDescription="", 
                NDAttrDataType_t dataType=NDAttrUndefined, void *pValue=NULL);
    virtual ~NDAttribute();
    virtual NDAttribute* copy(NDAttribute *pAttribute);
    virtual int setDescription(const char *pDescription);
    virtual int setSource(const char *pSource);
    virtual int getValueInfo(NDAttrDataType_t *pDataType, size_t *pDataSize);
    virtual int getValue(NDAttrDataType_t dataType, void *pValue, size_t dataSize=0);
    virtual int setValue(NDAttrDataType_t dataType, void *pValue);
    virtual int updateValue();
    virtual int report(int details);
    char *pName;                /**< Name string */
    char *pDescription;         /**< Description string */
    char *pSource;              /**< Source string - EPICS PV name or DRV_INFO string */
    NDAttrSource_t sourceType;  /**< Source type; driver hardcoded, EPICS PV or driver/plugin parameter */
    NDAttrDataType_t dataType;  /**< Data type of attribute */
    friend class NDArray;
    friend class NDAttributeList;

protected:
    NDAttrValue value;             /**< Value of attribute unless it is a string */
    char *pString;                 /**< Value when attribute type is NDAttrString; dynamic length string */
    NDAttributeListNode listNode;  /**< Used for NDAttributeList */
};

/** NDAttributeList class; this is a linked list of attributes.
  */
class epicsShareFunc NDAttributeList {
public:
    NDAttributeList();
    ~NDAttributeList();
    int          add(NDAttribute *pAttribute);
    NDAttribute* add(const char *pName, const char *pDescription="", 
                     NDAttrDataType_t dataType=NDAttrUndefined, void *pValue=NULL);
    NDAttribute* find(const char *pName);
    NDAttribute* next(NDAttribute *pAttribute);
    int          count();
    int          remove(const char *pName);
    int          clear();
    int          copy(NDAttributeList *pOut);
    int          updateValues();
    int          report(int details);
    
private:
    ELLLIST      list;   /**< The EPICS ELLLIST  */
    epicsMutexId lock;  /**< Mutex to protect the ELLLIST */
};

/** N-dimensional array class; each array has a set of dimensions, a data type, pointer to data, and optional attributes. 
  * An NDArray also has a uniqueId and timeStamp that to identify it. NDArray objects can be allocated
  * by an NDArrayPool object, which maintains a free list of NDArrays for efficient memory management. */
class epicsShareFunc NDArray {
public:
    /* Methods */
    NDArray();
    ~NDArray();
    int          initDimension   (NDDimension_t *pDimension, size_t size);
    int          getInfo         (NDArrayInfo_t *pInfo);
    int          reserve();
    int          release();
    int          report(int details);
    friend class NDArrayPool;
    
private:
    ELLNODE      node;              /**< This must come first because ELLNODE must have the same address as NDArray object */
    int          referenceCount;    /**< Reference count for this NDArray=number of clients who are using it */

public:
    class NDArrayPool *pNDArrayPool; /**< The NDArrayPool object that created this array */
    int           uniqueId;     /**< A number that must be unique for all NDArrays produced by a driver after is has started */
    double        timeStamp;    /**< The time stamp in seconds for this array; seconds since Epoch (00:00:00 UTC, January 1, 1970)
                                  * is recommended, but some drivers may use a different start time.*/
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
class epicsShareFunc NDArrayPool {
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
    int          report     (int details);
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
