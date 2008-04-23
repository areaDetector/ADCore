/* NDArrayBuff.h
 *
 * N-dimensional array buffer allocation utility.
 * 
 *
 * Mark Rivers
 * University of Chicago
 * April 5, 2008
 *
 */
 
#ifndef ND_ARRAYBUFF_H
#define ND_ARRAYBUFF_H

#include <ellLib.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* An NDArray structure. */
typedef struct NDArray {
    /* The first 2 fields are used for the freelist */
    ELLNODE node;
    int referenceCount;
    int uniqueId;
    double timeStamp;
    int ndims;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    int dataType;
    int dataSize;
    void *pData;
} NDArray_t;

typedef struct {
    int         (*init)     (int maxBuffers, size_t maxMemory);
    NDArray_t*  (*alloc)    (int ndims, int *dims, int bytesPerElement, int dataSize, void *pData);
    int         (*reserve)  (NDArray_t *pArray); 
    int         (*release)  (NDArray_t *pArray);
    int         (*initDimension) (NDDimension_t *pDimension, int size);
    int         (*getInfo)  (NDArray_t *pArray, NDArrayInfo_t *pInfo);
    int         (*convert)  (NDArray_t *pIn, 
                             NDArray_t **ppOut,
                             int dataTypeOut,
                             NDDimension_t *outDims);
    int         (*copy)     (NDArray_t *pOut, NDArray_t *pIn);
    int         (*report)   (int details);     
} NDArrayBuffSupport;

extern NDArrayBuffSupport *NDArrayBuff;

#ifdef __cplusplus
}
#endif
#endif
