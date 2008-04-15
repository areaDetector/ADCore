/* NDArrayBuff.c
 *
 * Array bufffer allocation utility.
 * 
 *
 * Mark Rivers
 * University of Chicago
 * April 5, 2008
 *
 */

#include <string.h>
#include <stdio.h>
#include <ellLib.h>

#include <epicsMutex.h>
#include <epicsTypes.h>
#include <ellLib.h>
#include <cantProceed.h>

#include "NDArrayBuff.h"

#define ND_SUCCESS 0
#define ND_ERROR -1


typedef struct {
    ELLLIST freeList;
    epicsMutexId listLock;
    int maxBuffers;
    int numBuffers;
    size_t maxMemory;
    size_t memorySize;
    int numFree;
} NDArrayBuffPvt_t;

static char *driverName = "NDArrayBuff";

NDArrayBuffPvt_t NDArrayBuffPvt;
int NDArrayBuffDebug = 0;


static int getInfo(NDArray_t *pArray, NDArrayInfo_t *pInfo)
{
    int i;
    
    switch(pArray->dataType) {
        case NDInt8:
            pInfo->bytesPerElement = sizeof(epicsInt8);
            break;
        case NDUInt8:
            pInfo->bytesPerElement = sizeof(epicsUInt8);
            break;
        case NDInt16:
            pInfo->bytesPerElement = sizeof(epicsInt16);
            break;
        case NDUInt16:
            pInfo->bytesPerElement = sizeof(epicsUInt16);
            break;
        case NDInt32:
            pInfo->bytesPerElement = sizeof(epicsInt32);
            break;
        case NDUInt32:
            pInfo->bytesPerElement = sizeof(epicsUInt32);
            break;
        case NDFloat32:
            pInfo->bytesPerElement = sizeof(epicsFloat32);
            break;
        case NDFloat64:
            pInfo->bytesPerElement = sizeof(epicsFloat64);
            break;
        default:
            return(ND_ERROR);
            break;
    }
    pInfo->nElements = 1;
    for (i=0; i<pArray->ndims; i++) pInfo->nElements *= pArray->dims[i].size;
    pInfo->totalBytes = pInfo->nElements * pInfo->bytesPerElement;
    return(ND_SUCCESS);
}

static int initDimension(NDDimension_t *pDimension, int size)
{
    pDimension->size=size;
    pDimension->binning = 1;
    pDimension->offset = 0;
    pDimension->invert = 0;
    return ND_SUCCESS;
}

static int init(int maxBuffers, size_t maxMemory)
{
    int status=ND_SUCCESS;
    NDArrayBuffPvt_t *pPvt = &NDArrayBuffPvt;

    ellInit(&pPvt->freeList);
    pPvt->maxBuffers = maxBuffers;
    pPvt->maxMemory = maxMemory;
    pPvt->numBuffers = 0;
    pPvt->memorySize = 0;
    pPvt->numFree = 0;
    pPvt->listLock = epicsMutexCreate();
    
    return(status);
}
    
static  NDArray_t* alloc(int ndims, int *dims, int dataType, int dataSize, void *pData)
{
    NDArray_t *pArray;
    NDArrayBuffPvt_t *pPvt = &NDArrayBuffPvt;
    NDArrayInfo_t arrayInfo;
    int i;
    const char* functionName = "NDArrayBuff::alloc:";
    
    if (!pPvt->listLock) {
        printf("%s: must call %s->init() first!\n", driverName, driverName);
        return NULL;
    }
    epicsMutexLock(pPvt->listLock);
    
    /* Find a free image */
    pArray = (NDArray_t *)ellFirst(&pPvt->freeList);
    
    if (!pArray) {
        /* We did not find a free image.  
         * Allocate a new one if we have not exceeded the limit */
        if (pPvt->numBuffers == pPvt->maxBuffers) {
            printf("%s: ERROR: reached limit of %d images\n", 
                   functionName, pPvt->maxBuffers);
        } else {
            pPvt->numBuffers++;
            pArray = callocMustSucceed(1, sizeof(NDArray_t),
                                       functionName);
            ellAdd(&pPvt->freeList, &pArray->node);
            pPvt->numFree++;
        }
    }
    
    if (pArray) {
        /* We have a frame */
        /* Initialize fields */
        pArray->uniqueId = 0;
        pArray->timeStamp = 0.;
        pArray->dataType = dataType;
        pArray->ndims = ndims;
        for (i=0; i<ndims && i<ND_ARRAY_MAX_DIMS; i++) {
            pArray->dims[i].size = dims[i];
            pArray->dims[i].offset = 0;
            pArray->dims[i].binning = 1;
            pArray->dims[i].invert = 0;
        }
        getInfo(pArray, &arrayInfo); 
        if (dataSize == 0) dataSize = arrayInfo.totalBytes;
        if (arrayInfo.totalBytes > dataSize) {
            printf("%s: ERROR: required size=%d passed size=%d is too small\n", 
            functionName, arrayInfo.totalBytes, dataSize);
            pArray=NULL;
        }
    }
 
    if (pArray) {
        /* If the caller passed a valid buffer use that, trust that its size is correct */
        if (pData) {
            pArray->pData = pData;
        } else {
            /* See if the current buffer is big enough */
            if (pArray->dataSize < dataSize) {
                /* No, we need to free the current buffer and allocate a new one */
                /* See if there is enough room */
                pPvt->memorySize -= pArray->dataSize;
                free(pArray->pData);
                if ((pPvt->memorySize + dataSize) > pPvt->maxMemory) {
                    printf("%s: ERROR: reached limit of %d memory\n", 
                           functionName, pPvt->maxMemory);
                    pArray = NULL;
                } else {
                    pArray->pData = callocMustSucceed(dataSize, 1,
                                                      functionName);
                    pArray->dataSize = dataSize;
                    pPvt->memorySize += dataSize;
                }
            }
        }
    }
    if (pArray) {
        if (NDArrayBuffDebug) {
            printf("%s:alloc allocated pArray=%p\n", 
                driverName, pArray);
        }
        /* Set the reference count to 1, remove from free list */
        pArray->referenceCount = 1;
        ellDelete(&pPvt->freeList, &pArray->node);
        pPvt->numFree--;
    }
    epicsMutexUnlock(pPvt->listLock);
    return (pArray);
}

static int reserve(NDArray_t *pArray)
{
    NDArrayBuffPvt_t *pPvt = &NDArrayBuffPvt;
    
    if (!pPvt->listLock) {
        printf("%s: must call %s->init() first!\n", driverName, driverName);
        return ND_ERROR;
    }
    epicsMutexLock(pPvt->listLock);
    pArray->referenceCount++;
    if (NDArrayBuffDebug) {
        printf("%s:reserve reserved image pArray=%p, referenceCount=%d\n", 
            driverName, pArray, pArray->referenceCount);
    }
    epicsMutexUnlock(pPvt->listLock);
    return ND_SUCCESS;
}

static int release(NDArray_t *pArray)
{   
    NDArrayBuffPvt_t *pPvt = &NDArrayBuffPvt;
    
    if (!pPvt->listLock) {
        printf("%s: must call %s->init() first!\n", driverName, driverName);
        return ND_ERROR;
    }
    epicsMutexLock(pPvt->listLock);
    pArray->referenceCount--;
    if (NDArrayBuffDebug) {
        printf("%s:release released image pArray=%p, referenceCount=%d\n", 
            driverName, pArray, pArray->referenceCount);
    }
    if (pArray->referenceCount == 0) {
        /* The last user has released this image, add it back to the free list */
        ellAdd(&pPvt->freeList, &pArray->node);
        pPvt->numFree++;
    }
    if (pArray->referenceCount < 0) {
        printf("%s:release ERROR, reference count < 0 pArray=%p\n",
            driverName, pArray);
    }
    epicsMutexUnlock(pPvt->listLock);
    return ND_SUCCESS;
}

/* The following macros save an enormous amount of code when converting data types */

/* This macro converts the data type of 2 images that have the same dimensions */

#define CONVERT_TYPE(DATA_TYPE_IN, DATA_TYPE_OUT) {  \
    int i;                                           \
    DATA_TYPE_IN *pDataIn = (DATA_TYPE_IN *)pIn->pData;     \
    DATA_TYPE_OUT *pDataOut = (DATA_TYPE_OUT *)pOut->pData; \
    for (i=0; i<arrayInfo.nElements; i++) {              \
        *pDataOut++ = (DATA_TYPE_OUT)(*pDataIn++);   \
    }                                                \
}

#define CONVERT_TYPE_SWITCH(DATA_TYPE_OUT) { \
    switch(pIn->dataType) {                        \
        case NDInt8:                              \
            CONVERT_TYPE(epicsInt8, DATA_TYPE_OUT);     \
            break;                                \
        case NDUInt8:                             \
            CONVERT_TYPE(epicsUInt8, DATA_TYPE_OUT);    \
            break;                                \
        case NDInt16:                             \
            CONVERT_TYPE(epicsInt16, DATA_TYPE_OUT);    \
            break;                                \
        case NDUInt16:                            \
            CONVERT_TYPE(epicsUInt16, DATA_TYPE_OUT);   \
            break;                                \
        case NDInt32:                             \
            CONVERT_TYPE(epicsInt32, DATA_TYPE_OUT);    \
            break;                                \
        case NDUInt32:                            \
            CONVERT_TYPE(epicsUInt32, DATA_TYPE_OUT);   \
            break;                                \
        case NDFloat32:                           \
            CONVERT_TYPE(epicsFloat32, DATA_TYPE_OUT);  \
            break;                                \
        case NDFloat64:                           \
            CONVERT_TYPE(epicsFloat64, DATA_TYPE_OUT);  \
            break;                                \
        default:                                  \
            status = ND_ERROR;         \
            break;                                \
    }                                             \
}

#define CONVERT_TYPE_ALL() { \
    switch(pOut->dataType) {                        \
        case NDInt8:                              \
            CONVERT_TYPE_SWITCH(epicsInt8);     \
            break;                                \
        case NDUInt8:                             \
            CONVERT_TYPE_SWITCH(epicsUInt8);    \
            break;                                \
        case NDInt16:                             \
            CONVERT_TYPE_SWITCH(epicsInt16);    \
            break;                                \
        case NDUInt16:                            \
            CONVERT_TYPE_SWITCH(epicsUInt16);   \
            break;                                \
        case NDInt32:                             \
            CONVERT_TYPE_SWITCH(epicsInt32);    \
            break;                                \
        case NDUInt32:                            \
            CONVERT_TYPE_SWITCH(epicsUInt32);   \
            break;                                \
        case NDFloat32:                           \
            CONVERT_TYPE_SWITCH(epicsFloat32);  \
            break;                                \
        case NDFloat64:                           \
            CONVERT_TYPE_SWITCH(epicsFloat64);  \
            break;                                \
        default:                                  \
            status = ND_ERROR;         \
            break;                                \
    }                                             \
}


/* This macro computes an output image from an input image, selecting a region of interest
 * with binning and data type conversion. */
#define CONVERT_DIMENSION(DATA_TYPE_IN, DATA_TYPE_OUT) {   \
    int xb, yb, inRow=startY, outRow, outCol;             \
    DATA_TYPE_OUT *pOut, *pOutTemp;                     \
    DATA_TYPE_IN *pIn;                                  \
    for (outRow=0; outRow<sizeYOut; outRow++) {         \
        pOut = (DATA_TYPE_OUT *)imageOut;               \
        pOut += outRow * sizeXOut;                      \
        for (yb=0; yb<binY; yb++) {                     \
            pOutTemp = pOut;                            \
            pIn = (DATA_TYPE_IN *)imageIn;              \
            pIn += inRow*sizeXIn + startX;              \
            for (out=0; out<pOut->dims[cdim]; out++) {  \
                for (bin=1; bin<binningOut[dim]; bin++) {      \
                    *pOut[out] += (DATA_TYPE_OUT)*pIn++; \
                } /* Next xbin */                       \
            } /* Next outCol */                         \
            inRow++;                                    \
        } /* Next ybin */                               \
    } /* Next outRow */                                 \
}

#define CONVERT_DIMENSION_SWITCH(DATA_TYPE_OUT) { \
    switch(pIn->dataType) {                        \
        case NDInt8:                              \
            CONVERT_DIMENSION(epicsInt8, DATA_TYPE_OUT);     \
            break;                                \
        case NDUInt8:                             \
            CONVERT_DIMENSION(epicsUInt8, DATA_TYPE_OUT);    \
            break;                                \
        case NDInt16:                             \
            CONVERT_DIMENSION(epicsInt16, DATA_TYPE_OUT);    \
            break;                                \
        case NDUInt16:                            \
            CONVERT_DIMENSION(epicsUInt16, DATA_TYPE_OUT);   \
            break;                                \
        case NDInt32:                             \
            CONVERT_DIMENSION(epicsInt32, DATA_TYPE_OUT);    \
            break;                                \
        case NDUInt32:                            \
            CONVERT_DIMENSION(epicsUInt32, DATA_TYPE_OUT);   \
            break;                                \
        case NDFloat32:                           \
            CONVERT_DIMENSION(epicsFloat32, DATA_TYPE_OUT);  \
            break;                                \
        case NDFloat64:                           \
            CONVERT_DIMENSION(epicsFloat64, DATA_TYPE_OUT);  \
            break;                                \
        default:                                  \
            status = ND_ERROR;         \
            break;                                \
    }                                             \
}

#define CONVERT_DIMENSION_ALL() { \
    switch(pOut->dataType) {                        \
        case NDInt8:                              \
            CONVERT_DIMENSION_SWITCH(epicsInt8);     \
            break;                                \
        case NDUInt8:                             \
            CONVERT_DIMENSION_SWITCH(epicsUInt8);    \
            break;                                \
        case NDInt16:                             \
            CONVERT_DIMENSION_SWITCH(epicsInt16);    \
            break;                                \
        case NDUInt16:                            \
            CONVERT_DIMENSION_SWITCH(epicsUInt16);   \
            break;                                \
        case NDInt32:                             \
            CONVERT_DIMENSION_SWITCH(epicsInt32);    \
            break;                                \
        case NDUInt32:                            \
            CONVERT_DIMENSION_SWITCH(epicsUInt32);   \
            break;                                \
        case NDFloat32:                           \
            CONVERT_DIMENSION_SWITCH(epicsFloat32);  \
            break;                                \
        case NDFloat64:                           \
            CONVERT_DIMENSION_SWITCH(epicsFloat64);  \
            break;                                \
        default:                                  \
            status = ND_ERROR;         \
            break;                                \
    }                                             \
}

static int convertDimension(NDArray_t *pIn, 
                            NDArray_t *pOut,
                            int dimension)
{
    int status = ND_SUCCESS;
    
    /* CONVERT_DIMENSION_ALL(); */
    
    return(status);
}

static int convert(NDArray_t *pIn, 
                   NDArray_t **ppOut,
                   int dataTypeOut,
                   NDDimension_t *dimsOut)
{
    int dimsUnchanged;
    int dimSizeOut[ND_ARRAY_MAX_DIMS];
    int i;
    int status = ND_SUCCESS;
    NDArray_t *pOut;
    NDArrayInfo_t arrayInfo;
    static char *functionName = "convertArray";
    
    /* Compute the dimensions of the output array */
    dimsUnchanged = 1;
    for (i=0; i<pIn->ndims; i++) {
        dimsOut[i].size = dimsOut[i].size/dimsOut[i].binning;
        dimSizeOut[i] = dimsOut[i].size;
        if ((pIn->dims[i].size  != dimsOut[i].size) ||
            (dimsOut[i].offset != 0) ||
            (dimsOut[i].binning != 1) ||
            (dimsOut[i].invert != 0)) dimsUnchanged = 0;
    }
    
    /* We now know the datatype and dimensions of the output array.
     * Allocate it */
    pOut = NDArrayBuff->alloc(pIn->ndims, dimSizeOut, dataTypeOut, 0, NULL);
    if (!pOut) {
        printf("%s:%s: ERROR, cannot allocate output array\n",
            driverName, functionName);
        return(ND_ERROR);
    }
    *ppOut = pOut;
    
    NDArrayBuff->getInfo(pOut, &arrayInfo);

    if (dimsUnchanged) {
        if (pIn->dataType == pOut->dataType) {
            /* The dimensions are the same and the data type is the same, 
             * then just copy the input image to the output image */
            memcpy(pOut->pData, pIn->pData, arrayInfo.totalBytes);
            return ND_SUCCESS;
        } else {
            /* We need to convert data types */
            CONVERT_TYPE_ALL();
        }
    } else {
        /* The input and output dimensions are not the same, so we are extracting a region
         * and/or binning */
        /* Clear entire output array */
        memset(pOut->pData, 0, arrayInfo.totalBytes);
        convertDimension(pIn, pOut, pIn->ndims-1);
    }
                    
    /* Set fields in the output array */
    for (i=0; i<pIn->ndims; i++) {
        pOut->dims[i].offset = pIn->dims[i].offset + dimsOut[i].offset;
        pOut->dims[i].binning = pIn->dims[i].binning * dimsOut[i].binning;
        if (pIn->dims[i].invert) pOut->dims[i].invert = !pOut->dims[i].invert;
    }
    pOut->timeStamp = pIn->timeStamp;
    pOut->uniqueId = pIn->uniqueId;
    
    return ND_SUCCESS;
}

static int report(int details)
{
    NDArrayBuffPvt_t *pPvt = &NDArrayBuffPvt;
    
    if (!pPvt->listLock) {
        printf("%s: must call %s->init() first!\n", driverName, driverName);
        return ND_ERROR;
    }
    printf("NDArrayBuff:\n");
    printf("  numBuffers=%d, maxBuffers=%d\n", 
        pPvt->numBuffers, pPvt->maxBuffers);
    printf("  memorySize=%d, maxMemory=%d\n", 
        pPvt->memorySize, pPvt->maxMemory);
    printf("  numFree=%d\n", 
        pPvt->numFree);
    return ND_SUCCESS;
}
   
static NDArrayBuffSupport support =
{
    init,
    alloc,
    reserve,
    release,
    initDimension,
    getInfo,
    convert,
    report,
};

NDArrayBuffSupport *NDArrayBuff = &support;

