/* NDArray.h
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

#define ND_ARRAY_MAX_DIMS 10
#define ND_SUCCESS 0
#define ND_ERROR -1

/* Enumeration of image data types
 * This list will grow when color image models are supported */
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

class NDArray {
public:
    /* Data: NOTE this must come first because ELLNODE must be first, i.e. same address as object */
    /* The first 2 fields are used for the freelist */
    ELLNODE node;
    int referenceCount;
    /* The NDArrayPool object that created this array */
    void *owner;
    int uniqueId;
    double timeStamp;
    int ndims;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    int dataType;
    int dataSize;
    void *pData;

    /* Methods */
    NDArray();
    int          initDimension (NDDimension_t *pDimension, int size);
    int          getInfo       (NDArrayInfo_t *pInfo);
    int          convertDimension(NDArray *pOut,
                                  void *pDataIn,
                                  void *pDataOut,
                                  int dim);
    int          copy          (NDArray *pOut);
    int          reserve(); 
    int          release();
};


class NDArrayPool {
public:
                 NDArrayPool   (int maxBuffers, size_t maxMemory);
    NDArray*     alloc         (int ndims, int *dims, int bytesPerElement, int dataSize, void *pData);
    int          reserve       (NDArray *pArray); 
    int          release       (NDArray *pArray);
    int          convert       (NDArray *pIn, 
                                NDArray **ppOut,
                                int dataTypeOut,
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
