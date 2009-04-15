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

#define ND_ARRAY_MAX_DIMS 10
#define ND_SUCCESS 0
#define ND_ERROR -1
#define ND_MAX_ATTR_NAME_SIZE 40
#define ND_MAX_ATTR_STRING_SIZE 40

/** Enumeration of array data types */
typedef enum
{
    NDInt8,
    NDUInt8,
    NDInt16,
    NDUInt16,
    NDInt32,
    NDUInt32,
    NDFloat32,
    NDFloat64
} NDDataType_t;

/** Enumeration of attribute data types */
typedef enum
{
    NDAttrInt8    = NDInt8,
    NDAttrUInt8   = NDUInt8,
    NDAttrInt16   = NDInt16,
    NDAttrUInt16  = NDUInt16,
    NDAttrInt32   = NDInt32,
    NDAttrUInt32  = NDUInt32,
    NDAttrFloat32 = NDFloat32,
    NDAttrFloat64 = NDFloat64,
    NDAttrString,
    NDAttrUndefined    
} NDAttrDataType_t;

/** Enumeration of color modes */
typedef enum
{
    NDColorModeMono,
    NDColorModeBayer,
    NDColorModeRGB1,
    NDColorModeRGB2,
    NDColorModeRGB3,
    NDColorModeYUV444,
    NDColorModeYUV422,
    NDColorModeYUV421
} NDColorMode_t;

typedef enum
{
    NDBayerRGGB        = 0,    /**< First line RGRG, second line GBGB... */
    NDBayerGBRG        = 1,    /**< First line GBGB, second line RGRG... */
    NDBayerGRBG        = 2,    /**< First line GRGR, second line BGBG... */
    NDBayerBGGR        = 3     /**< First line BGBG, second line GRGR... */
} NDBayerPattern_t;

typedef struct NDDimension {
    int size;
    int offset;
    int binning;
    int reverse;
} NDDimension_t;

typedef struct NDArrayInfo {
    int nElements;
    int bytesPerElement;
    int totalBytes;
} NDArrayInfo_t;

typedef union {
    epicsInt8    i8;
    epicsUInt8   ui8;
    epicsInt16   i16;
    epicsUInt16  ui16;
    epicsInt32   i32;
    epicsUInt32  ui32;
    epicsFloat32 f32;
    epicsFloat64 f64;
    char         string[ND_MAX_ATTR_STRING_SIZE];
} NDAttrValue;

class NDAttribute {
public:
    /* Methods */
    int getNameInfo(size_t *nameSize);
    int getName(char *name, size_t nameSize=0);
    int getValueInfo(NDAttrDataType_t *dataType, size_t *dataSize);
    int setValue(NDAttrDataType_t dataType, void *value);
    int getValue(NDAttrDataType_t dataType, void *value, size_t dataSize=0);
    friend class NDArray;

private:
    /* Data: NOTE this must come first because ELLNODE must be first, i.e. same address as object */
    /* The first 2 fields are used for the freelist */
    NDAttribute(const char *name);
    ~NDAttribute();
    ELLNODE node;
    char name[ND_MAX_ATTR_NAME_SIZE];
    NDAttrDataType_t dataType;
    NDAttrValue value;
};

class NDArray {
public:
    /* Methods */
    NDArray();
    int          initDimension   (NDDimension_t *pDimension, int size);
    int          getInfo         (NDArrayInfo_t *pInfo);
    int          reserve();
    int          release();
    NDAttribute* addAttribute(const char *name);
    NDAttribute* addAttribute(const char *name, NDAttrDataType_t dataType, void *value);
    NDAttribute* findAttribute(const char *name);
    NDAttribute* nextAttribute(NDAttribute* pAttribute);
    int          numAttributes();
    int          deleteAttribute(const char *name);
    int          clearAttributes();
    int          copyAttributes(NDArray *pOut);
    friend class NDArrayPool;
    
private:
    /* Data: NOTE this must come first because ELLNODE must be first, i.e. same address as object */
    /* The first 2 fields are used for the freelist */
    ELLNODE node;
    int          referenceCount;
    void         *owner;  /* The NDArrayPool object that created this array */
    ELLLIST      attributeList;
    epicsMutexId listLock;

public:
    /* Public data */
    int           uniqueId;
    double        timeStamp;
    int           ndims;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    NDDataType_t  dataType;
    int           dataSize;
    void          *pData;
};



/** The NDArrayPool class manages a free list (pool) of NDArray objects (described above).
  * Drivers allocate NDArray objects from the pool, and pass these objects to plugins.
  * Plugins increase the reference count on the object when they place the object on
  * their queue, and decrease the reference count when they are done processing the
  * array. When the reference count reaches 0 again the NDArray object is placed back
  * on the free list. This mechanism minimizes the copying of array data in plugins.
  */
class NDArrayPool {
public:
                 NDArrayPool   (int maxBuffers, size_t maxMemory);
    NDArray*     alloc         (int ndims, int *dims, NDDataType_t dataType, int dataSize, void *pData);
    NDArray*     copy          (NDArray *pIn, NDArray *pOut, int copyData);

    int          reserve       (NDArray *pArray);
    int          release       (NDArray *pArray);
    int          convert       (NDArray *pIn,
                                NDArray **ppOut,
                                NDDataType_t dataTypeOut,
                                NDDimension_t *outDims);
    int          report        (int details);
private:
    ELLLIST      freeList;
    epicsMutexId listLock;
    int          maxBuffers;
    int          numBuffers;
    size_t       maxMemory;
    size_t       memorySize;
    int          numFree;
};


#endif
