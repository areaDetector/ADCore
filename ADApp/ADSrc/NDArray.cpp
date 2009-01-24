/* NDArray.cpp
 *
 * NDArray classes
 * 
 *
 * Mark Rivers
 * University of Chicago
 * May 11 2008
 *
 */

#include <string.h>
#include <stdio.h>
#include <ellLib.h>

#include <epicsMutex.h>
#include <epicsTypes.h>
#include <ellLib.h>
#include <cantProceed.h>

#include "NDArray.h"

static const char *driverName = "NDArray";

NDArrayPool::NDArrayPool(int maxBuffers, size_t maxMemory)
    : maxBuffers(maxBuffers),numBuffers(0),  maxMemory(maxMemory), memorySize(0), numFree(0)
{
    ellInit(&this->freeList);
    this->listLock = epicsMutexCreate();
}
    
NDArray* NDArrayPool::alloc(int ndims, int *dims, NDDataType_t dataType, int dataSize, void *pData)
{
    NDArray *pArray;
    NDArrayInfo_t arrayInfo;
    int i;
    const char* functionName = "NDArrayPool::alloc:";
    
    epicsMutexLock(this->listLock);
    
    /* Find a free image */
    pArray = (NDArray *)ellFirst(&this->freeList);
    
    if (!pArray) {
        /* We did not find a free image.  
         * Allocate a new one if we have not exceeded the limit */
        if ((this->maxBuffers > 0) && (this->numBuffers >= this->maxBuffers)) {
            printf("%s: error: reached limit of %d buffers (memory use=%d/%d bytes)\n", 
                   functionName, this->maxBuffers, this->memorySize, this->maxMemory);
        } else {
            this->numBuffers++;
            pArray = new NDArray;
            ellAdd(&this->freeList, &pArray->node);
            this->numFree++;
        }
    }
    
    if (pArray) {
        /* We have a frame */
        /* Initialize fields */
        pArray->owner = this;
        pArray->dataType = dataType;
        pArray->colorMode = NDColorModeMono;
        pArray->bayerPattern = NDBayerRGGB;
        pArray->ndims = ndims;
        memset(pArray->dims, 0, sizeof(pArray->dims));
        for (i=0; i<ndims && i<ND_ARRAY_MAX_DIMS; i++) {
            pArray->dims[i].size = dims[i];
            pArray->dims[i].offset = 0;
            pArray->dims[i].binning = 1;
            pArray->dims[i].reverse = 0;
        }
        pArray->getInfo(&arrayInfo); 
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
                this->memorySize -= pArray->dataSize;
                if (pArray->pData) {
                    free(pArray->pData);
                    pArray->pData = NULL;
                }
                if ((this->maxMemory > 0) && ((this->memorySize + dataSize) > this->maxMemory)) {
                    printf("%s: error: reached limit of %d memory (%d/%d buffers)\n", 
                           functionName, this->maxMemory, this->numBuffers, this->maxBuffers);
                    pArray = NULL;
                } else {
                    pArray->pData = callocMustSucceed(dataSize, 1,
                                                      functionName);
                    pArray->dataSize = dataSize;
                    this->memorySize += dataSize;
                }
            }
        }
    }
    if (pArray) {
        /* Set the reference count to 1, remove from free list */
        pArray->referenceCount = 1;
        ellDelete(&this->freeList, &pArray->node);
        this->numFree--;
    }
    epicsMutexUnlock(this->listLock);
    return (pArray);
}

NDArray* NDArrayPool::copy(NDArray *pIn, NDArray *pOut, int copyData)
{
    //const char *functionName = "copy";
    int dimSizeOut[ND_ARRAY_MAX_DIMS];
    int i;
    int numCopy;
    NDArrayInfo arrayInfo;
    
    /* If the output array does not exist then create it */
    if (!pOut) {
        for (i=0; i<pIn->ndims; i++) dimSizeOut[i] = pIn->dims[i].size;
        pOut = this->alloc(pIn->ndims, dimSizeOut, pIn->dataType, 0, NULL);
    }
    pOut->colorMode = pIn->colorMode;
    pOut->bayerPattern = pIn->bayerPattern;
    pOut->uniqueId = pIn->uniqueId;
    pOut->timeStamp = pIn->timeStamp;
    pOut->ndims = pIn->ndims;
    memcpy(pOut->dims, pIn->dims, sizeof(pIn->dims));
    pOut->dataType = pIn->dataType;
    if (copyData) {
        pIn->getInfo(&arrayInfo);
        numCopy = arrayInfo.totalBytes;
        if (pOut->dataSize < numCopy) numCopy = pOut->dataSize;
        memcpy(pOut->pData, pIn->pData, numCopy);
    }
    return(pOut);
}

int NDArrayPool::reserve(NDArray *pArray)
{
    const char *functionName = "reserve";
    
    /* Make sure we own this array */
    if (pArray->owner != this) {
        printf("%s:%s: ERROR, not owner!  owner=%p, should be this=%p\n",
               driverName, functionName, pArray->owner, this);
        return(ND_ERROR);
    }
    epicsMutexLock(this->listLock);
    pArray->referenceCount++;
    epicsMutexUnlock(this->listLock);
    return ND_SUCCESS;
}

int NDArrayPool::release(NDArray *pArray)
{   
    const char *functionName = "release";
    
    /* Make sure we own this array */
    if (pArray->owner != this) {
        printf("%s:%s: ERROR, not owner!  owner=%p, should be this=%p\n",
               driverName, functionName, pArray->owner, this);
        return(ND_ERROR);
    }
    epicsMutexLock(this->listLock);
    pArray->referenceCount--;
    if (pArray->referenceCount == 0) {
        /* The last user has released this image, add it back to the free list */
        ellAdd(&this->freeList, &pArray->node);
        this->numFree++;
    }
    if (pArray->referenceCount < 0) {
        printf("%s:release ERROR, reference count < 0 pArray=%p\n",
            driverName, pArray);
    }
    epicsMutexUnlock(this->listLock);
    return ND_SUCCESS;
}

template <typename dataTypeIn, typename dataTypeOut> void convertType(NDArray *pIn, NDArray *pOut) 
{   
    int i;
    dataTypeIn *pDataIn = (dataTypeIn *)pIn->pData;
    dataTypeOut *pDataOut = (dataTypeOut *)pOut->pData;
    NDArrayInfo_t arrayInfo;
    
    pOut->getInfo(&arrayInfo);
    for (i=0; i<arrayInfo.nElements; i++) {
        *pDataOut++ = (dataTypeOut)(*pDataIn++);
    }
}

template <typename dataTypeOut> int convertTypeSwitch (NDArray *pIn, NDArray *pOut)
{
    int status = ND_SUCCESS;
    
    switch(pIn->dataType) {
        case NDInt8:
            convertType<epicsInt8, dataTypeOut> (pIn, pOut);
            break;
        case NDUInt8:
            convertType<epicsUInt8, dataTypeOut> (pIn, pOut);
            break;
        case NDInt16:
            convertType<epicsInt16, dataTypeOut> (pIn, pOut);
            break;
        case NDUInt16:
            convertType<epicsUInt16, dataTypeOut> (pIn, pOut);
            break;
        case NDInt32:
            convertType<epicsInt32, dataTypeOut> (pIn, pOut);
            break;
        case NDUInt32:
            convertType<epicsUInt32, dataTypeOut> (pIn, pOut);
            break;
        case NDFloat32:
            convertType<epicsFloat32, dataTypeOut> (pIn, pOut);
            break;
        case NDFloat64:
            convertType<epicsFloat64, dataTypeOut> (pIn, pOut);
            break;
        default:
            status = ND_ERROR;
            break;
    }
    return(status);
}


template <typename dataTypeIn, typename dataTypeOut> void convertDim(NDArray *pIn, NDArray *pOut, 
                                                                void *pDataIn, void *pDataOut, int dim)
{
    dataTypeOut *pDOut = (dataTypeOut *)pDataOut;
    dataTypeIn *pDIn = (dataTypeIn *)pDataIn;
    NDDimension_t *pOutDims = pOut->dims;
    NDDimension_t *pInDims = pIn->dims;
    int inStep, outStep, inOffset, inDir;
    int i, inc, in, out, bin;

    inStep = 1;
    outStep = 1;
    inDir = 1;
    inOffset = pOutDims[dim].offset;
    for (i=0; i<dim; i++) {
        inStep  *= pInDims[i].size;
        outStep *= pOutDims[i].size;
    }
    if (pOutDims[dim].reverse) {
        inOffset += pOutDims[dim].size * pOutDims[dim].binning - 1;
        inDir = -1;
    }
    inc = inDir * inStep;
    pDIn += inOffset*inStep;
    for (in=0, out=0; out<pOutDims[dim].size; out++, in++) {
        for (bin=0; bin<pOutDims[dim].binning; bin++) {
            if (dim > 0) {
                convertDim <dataTypeIn, dataTypeOut> (pIn, pOut, pDIn, pDOut, dim-1);
            } else {
                *pDOut += (dataTypeOut)*pDIn;
            }
            pDIn += inc;
        }
        pDOut += outStep;
    }
}

template <typename dataTypeOut> int convertDimensionSwitch(NDArray *pIn, NDArray *pOut, 
                                                            void *pDataIn, void *pDataOut, int dim)
{
    int status = ND_SUCCESS;
    
    switch(pIn->dataType) {
        case NDInt8:
            convertDim <epicsInt8, dataTypeOut> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDUInt8:
            convertDim <epicsUInt8, dataTypeOut> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDInt16:
            convertDim <epicsInt16, dataTypeOut> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDUInt16:
            convertDim <epicsUInt16, dataTypeOut> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDInt32:
            convertDim <epicsInt32, dataTypeOut> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDUInt32:
            convertDim <epicsUInt32, dataTypeOut> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDFloat32:
            convertDim <epicsFloat32, dataTypeOut> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDFloat64:
            convertDim <epicsFloat64, dataTypeOut> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        default:
            status = ND_ERROR;
            break;
    }
    return(status);
}

static int convertDimension(NDArray *pIn, 
                                  NDArray *pOut,
                                  void *pDataIn,
                                  void *pDataOut,
                                  int dim)
{
    int status = ND_SUCCESS;
    /* This routine is passed:
     * A pointer to the start of the input data
     * A pointer to the start of the output data 
     * An array of dimensions
     * A dimension index */
    switch(pOut->dataType) {
        case NDInt8:
            convertDimensionSwitch <epicsInt8>(pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDUInt8:
            convertDimensionSwitch <epicsUInt8> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDInt16:
            convertDimensionSwitch <epicsInt16> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDUInt16:
            convertDimensionSwitch <epicsUInt16> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDInt32:
            convertDimensionSwitch <epicsInt32> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDUInt32:
            convertDimensionSwitch <epicsUInt32> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDFloat32:
            convertDimensionSwitch <epicsFloat32> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        case NDFloat64:
            convertDimensionSwitch <epicsFloat64> (pIn, pOut, pDataIn, pDataOut, dim);
            break;
        default:
            status = ND_ERROR;
            break;
    }
    return(status);
}

int NDArrayPool::convert(NDArray *pIn, 
                         NDArray **ppOut,
                         NDDataType_t dataTypeOut,
                         NDDimension_t *dimsOut)
{
    int dimsUnchanged;
    int dimSizeOut[ND_ARRAY_MAX_DIMS];
    NDDimension_t dimsOutCopy[ND_ARRAY_MAX_DIMS];
    int i;
    int status = ND_SUCCESS;
    NDArray *pOut;
    NDArrayInfo_t arrayInfo;
    const char *functionName = "convert";
    
    /* Initialize failure */
    *ppOut = NULL;
    
    /* Copy the input dimension array because we need to modify it
     * but don't want to affect caller */
    memcpy(dimsOutCopy, dimsOut, pIn->ndims*sizeof(NDDimension_t));
    /* Compute the dimensions of the output array */
    dimsUnchanged = 1;
    for (i=0; i<pIn->ndims; i++) {
        dimsOutCopy[i].size = dimsOutCopy[i].size/dimsOutCopy[i].binning;
        if (dimsOutCopy[i].size <= 0) {
            printf("%s:%s: ERROR, invalid output dimension, size=%d, binning=%d\n",
                driverName, functionName, dimsOut[i].size, dimsOut[i].binning);
            return(ND_ERROR);
        }
        dimSizeOut[i] = dimsOutCopy[i].size;
        if ((pIn->dims[i].size  != dimsOutCopy[i].size) ||
            (dimsOutCopy[i].offset != 0) ||
            (dimsOutCopy[i].binning != 1) ||
            (dimsOutCopy[i].reverse != 0)) dimsUnchanged = 0;
    }
    
    /* We now know the datatype and dimensions of the output array.
     * Allocate it */
    pOut = alloc(pIn->ndims, dimSizeOut, dataTypeOut, 0, NULL);
    *ppOut = pOut;
    if (!pOut) {
        printf("%s:%s: ERROR, cannot allocate output array\n",
            driverName, functionName);
        return(ND_ERROR);
    }
    /* Copy fields from input to output */
    pOut->colorMode = pIn->colorMode;
    pOut->bayerPattern = pIn->bayerPattern;
    pOut->timeStamp = pIn->timeStamp;
    pOut->uniqueId = pIn->uniqueId;
    /* Replace the dimensions with those passed to this function */
    memcpy(pOut->dims, dimsOutCopy, pIn->ndims*sizeof(NDDimension_t));
    
    pOut->getInfo(&arrayInfo);

    if (dimsUnchanged) {
        if (pIn->dataType == pOut->dataType) {
            /* The dimensions are the same and the data type is the same, 
             * then just copy the input image to the output image */
            memcpy(pOut->pData, pIn->pData, arrayInfo.totalBytes);
            return ND_SUCCESS;
        } else {
            /* We need to convert data types */
            switch(pOut->dataType) {
                case NDInt8:
                    convertTypeSwitch <epicsInt8> (pIn, pOut);
                    break;
                case NDUInt8:
                    convertTypeSwitch <epicsUInt8> (pIn, pOut);
                    break;
                case NDInt16:
                    convertTypeSwitch <epicsInt16> (pIn, pOut);
                    break;
                case NDUInt16:
                    convertTypeSwitch <epicsUInt16> (pIn, pOut);
                    break;
                case NDInt32:
                    convertTypeSwitch <epicsInt32> (pIn, pOut);
                    break;
                case NDUInt32:
                    convertTypeSwitch <epicsUInt32> (pIn, pOut);
                    break;
                case NDFloat32:
                    convertTypeSwitch <epicsFloat32> (pIn, pOut);
                    break;
                case NDFloat64:
                    convertTypeSwitch <epicsFloat64> (pIn, pOut);
                    break;
                default:
                    status = ND_ERROR;
                    break;
            }
        }
    } else {
        /* The input and output dimensions are not the same, so we are extracting a region
         * and/or binning */
        /* Clear entire output array */
        memset(pOut->pData, 0, arrayInfo.totalBytes);
        status = convertDimension(pIn, pOut, pIn->pData, pOut->pData, pIn->ndims-1);
    }
                    
    /* Set fields in the output array */
    for (i=0; i<pIn->ndims; i++) {
        pOut->dims[i].offset = pIn->dims[i].offset + dimsOutCopy[i].offset;
        pOut->dims[i].binning = pIn->dims[i].binning * dimsOutCopy[i].binning;
        if (pIn->dims[i].reverse) pOut->dims[i].reverse = !pOut->dims[i].reverse;
    }
    
    /* If the frame is an RGBx frame and we have collapsed that dimension then change the colorMode */
    if      ((pOut->colorMode == NDColorModeRGB1) && (pOut->dims[0].size != 3)) pOut->colorMode= NDColorModeMono;
    else if ((pOut->colorMode == NDColorModeRGB2) && (pOut->dims[1].size != 3)) pOut->colorMode= NDColorModeMono;
    else if ((pOut->colorMode == NDColorModeRGB3) && (pOut->dims[2].size != 3)) pOut->colorMode= NDColorModeMono;
    return ND_SUCCESS;
}


int NDArrayPool::report(int details)
{
    printf("NDArrayPool:\n");
    printf("  numBuffers=%d, maxBuffers=%d\n", 
        this->numBuffers, this->maxBuffers);
    printf("  memorySize=%d, maxMemory=%d\n", 
        this->memorySize, this->maxMemory);
    printf("  numFree=%d\n", 
        this->numFree);
    return ND_SUCCESS;
}

NDArray::NDArray()
    : referenceCount(0), owner(NULL), 
      uniqueId(0), timeStamp(0.0), ndims(0), dataType(NDInt8), colorMode(NDColorModeMono), 
      bayerPattern(NDBayerRGGB), dataSize(0), pData(NULL)
{
    memset(this->dims, 0, sizeof(this->dims));
    memset(&this->node, 0, sizeof(this->node));
}
   
int NDArray::getInfo(NDArrayInfo_t *pInfo)
{
    int i;
    
    switch(this->dataType) {
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
    for (i=0; i<this->ndims; i++) pInfo->nElements *= this->dims[i].size;
    pInfo->totalBytes = pInfo->nElements * pInfo->bytesPerElement;
    return(ND_SUCCESS);
}

int NDArray::initDimension(NDDimension_t *pDimension, int size)
{
    pDimension->size=size;
    pDimension->binning = 1;
    pDimension->offset = 0;
    pDimension->reverse = 0;
    return ND_SUCCESS;
}

int NDArray::reserve()
{
    const char *functionName = "NDArray::reserve";
    
    NDArrayPool *pNDArrayPool = (NDArrayPool *)this->owner;
    
    if (!pNDArrayPool) {
        printf("%s: ERROR, no owner\n", functionName);
        return(ND_ERROR);
    }
    return(pNDArrayPool->reserve(this));
}

int NDArray::release()
{
    const char *functionName = "NDArray::release";
    
    NDArrayPool *pNDArrayPool = (NDArrayPool *)this->owner;
    
    if (!pNDArrayPool) {
        printf("%s: ERROR, no owner\n", functionName);
        return(ND_ERROR);
    }
    return(pNDArrayPool->release(this));
}

