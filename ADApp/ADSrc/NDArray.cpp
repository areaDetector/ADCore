/* NDArray.cpp
 *
 * NDArray classes
 *
 *
 * \author Mark Rivers
 *
 * \author University of Chicago
 *
 * \date May 11 2008
 *
 */

#include <string.h>
#include <stdio.h>
#include <ellLib.h>

#include <epicsMutex.h>
#include <epicsTypes.h>
#include <epicsString.h>
#include <ellLib.h>
#include <cantProceed.h>

#define epicsExportSharedSymbols
#include <shareLib.h>
#include "NDArray.h"
#include <epicsExport.h>

static const char *driverName = "NDArray";

/** eraseNDAttributes is a global flag the controls whether NDArray::clearAttributes() is called
  * each time a new array is allocated with NDArrayPool->alloc().
  * The default value is 0, meaning that clearAttributes() is not called.  This mode is efficient
  * because it saves lots of allocation/deallocation, and it is fine when the attributes for a driver
  * are set once and not changed.  If driver attributes are deleted however, the allocated arrays
  * will still have the old attributes if this flag is 0.  Set this flag to force attributes to be
  * removed each time an NDArray is allocated.
  */

volatile int eraseNDAttributes=0;
extern "C" {epicsExportAddress(int, eraseNDAttributes);}

/** NDArrayPool constructor
  * \param[in] maxBuffers Maximum number of NDArray objects that the pool is allowed to contain; 0=unlimited.
  * \param[in] maxMemory Maxiumum number of bytes of memory the the pool is allowed to use, summed over
  * all of the NDArray objects; 0=unlimited.
  */
NDArrayPool::NDArrayPool(int maxBuffers, size_t maxMemory)
  : maxBuffers_(maxBuffers), numBuffers_(0), maxMemory_(maxMemory), memorySize_(0), numFree_(0)
{
  ellInit(&freeList_);
  listLock_ = epicsMutexCreate();
}

/** Allocates a new NDArray object; the first 3 arguments are required.
  * \param[in] ndims The number of dimensions in the NDArray. 
  * \param[in] dims Array of dimensions, whose size must be at least ndims.
  * \param[in] dataType Data type of the NDArray data.
  * \param[in] dataSize Number of bytes to allocate for the array data; if 0 then
  * alloc() will compute the size required from ndims, dims, and dataType.
  * \param[in] pData Pointer to a data buffer; if NULL then alloc will allocate a new
  * array buffer; if not NULL then it is assumed to point to a valid buffer.
  * 
  * If pData is not NULL then dataSize must contain the actual number of bytes in the existing
  * array, and this array must be large enough to hold the array data. 
  * alloc() searches
  * its free list to find a free NDArray buffer. If is cannot find one then it will
  * allocate a new one and add it to the free list. If doing so would exceed maxBuffers
  * then alloc() will return an error. Similarly if allocating the memory required for
  * this NDArray would cause the cumulative memory allocated for the pool to exceed
  * maxMemory then an error will be returned. alloc() sets the reference count for the
  * returned NDArray to 1.
  */
NDArray* NDArrayPool::alloc(int ndims, size_t *dims, NDDataType_t dataType, size_t dataSize, void *pData)
{
  NDArray *pArray;
  NDArrayInfo_t arrayInfo;
  int i;
  const char* functionName = "NDArrayPool::alloc:";

  epicsMutexLock(listLock_);

  /* Find a free image */
  pArray = (NDArray *)ellFirst(&freeList_);

  if (!pArray) {
    /* We did not find a free image.
     * Allocate a new one if we have not exceeded the limit */
    if ((maxBuffers_ > 0) && (numBuffers_ >= maxBuffers_)) {
      printf("%s: error: reached limit of %d buffers (memory use=%ld/%ld bytes)\n",
             functionName, maxBuffers_, (long)memorySize_, (long)maxMemory_);
    } else {
      numBuffers_++;
      pArray = new NDArray;
      ellAdd(&freeList_, &pArray->node);
      numFree_++;
    }
  }

  if (pArray) {
    /* We have a frame */
    /* Initialize fields */
    pArray->pNDArrayPool = this;
    pArray->dataType = dataType;
    pArray->ndims = ndims;
    memset(pArray->dims, 0, sizeof(pArray->dims));
    for (i=0; i<ndims && i<ND_ARRAY_MAX_DIMS; i++) {
      pArray->dims[i].size = dims[i];
      pArray->dims[i].offset = 0;
      pArray->dims[i].binning = 1;
      pArray->dims[i].reverse = 0;
    }
    /* Erase the attributes if that global flag is set */
    if (eraseNDAttributes) pArray->pAttributeList->clear();
    pArray->getInfo(&arrayInfo);
    if (dataSize == 0) dataSize = arrayInfo.totalBytes;
    if (arrayInfo.totalBytes > dataSize) {
      printf("%s: ERROR: required size=%d passed size=%d is too small\n",
      functionName, (int)arrayInfo.totalBytes, (int)dataSize);
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
        if (pArray->pData) {
          memorySize_ -= pArray->dataSize;
          free(pArray->pData);
          pArray->pData = NULL;
          pArray->dataSize = 0;
        }
        if ((maxMemory_ > 0) && ((memorySize_ + dataSize) > maxMemory_)) {
          // We don't have enough memory to allocate the array
          // See if we can get memory by deleting arrays
          NDArray *freeArray = (NDArray *)ellFirst(&freeList_);
          while (freeArray && ((memorySize_ + dataSize) > maxMemory_)) {
            if (freeArray->pData) {
              memorySize_ -= freeArray->dataSize;
              free(freeArray->pData);
              freeArray->pData = NULL;
              freeArray->dataSize = 0;
            }
            // Next array
            freeArray = (NDArray *)ellNext(&freeArray->node);
          }
        }
        if ((maxMemory_ > 0) && ((memorySize_ + dataSize) > maxMemory_)) {
          printf("%s: error: reached limit of %ld memory (%d/%d buffers)\n",
                 functionName, (long)maxMemory_, numBuffers_, maxBuffers_);
          pArray = NULL;
        } else {
          pArray->pData = malloc(dataSize);
          if (pArray->pData) {
            pArray->dataSize = dataSize;
            memorySize_ += dataSize;
          } else {
            pArray = NULL;
          }
        }
      }
    }
    // If we don't have a valid memory buffer see pArray to NULL to indicate error
    if (pArray && (pArray->pData == NULL)) pArray = NULL;
  }
  if (pArray) {
    /* Set the reference count to 1, remove from free list */
    pArray->referenceCount = 1;
    ellDelete(&freeList_, &pArray->node);
    numFree_--;
  }
  epicsMutexUnlock(listLock_);
  return (pArray);
}

/** This method makes a copy of an NDArray object.
  * \param[in] pIn The input array to be copied.
  * \param[in] pOut The output array that will be copied to.
  * \param[in] copyData If this flag is 1 then everything including the array data is copied;
  * if 0 then everything except the data (including attributes) is copied.
  * \return Returns a pointer to the output array.
  *
  * If pOut is NULL then it is first allocated. If the output array
  * object already exists (pOut!=NULL) then it must have sufficient memory allocated to
  * it to hold the data.
  */
NDArray* NDArrayPool::copy(NDArray *pIn, NDArray *pOut, int copyData)
{
  //const char *functionName = "copy";
  size_t dimSizeOut[ND_ARRAY_MAX_DIMS];
  int i;
  size_t numCopy;
  NDArrayInfo arrayInfo;

  /* If the output array does not exist then create it */
  if (!pOut) {
    for (i=0; i<pIn->ndims; i++) dimSizeOut[i] = pIn->dims[i].size;
    pOut = this->alloc(pIn->ndims, dimSizeOut, pIn->dataType, 0, NULL);
  }
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
  pOut->pAttributeList->clear();
  pIn->pAttributeList->copy(pOut->pAttributeList);
  return(pOut);
}

/** This method increases the reference count for the NDArray object.
  * \param[in] pArray The array on which to increase the reference count.
  *
  * Plugins must call reserve() when an NDArray is placed on a queue for later
  * processing.
  */
int NDArrayPool::reserve(NDArray *pArray)
{
  const char *functionName = "reserve";

  /* Make sure we own this array */
  if (pArray->pNDArrayPool != this) {
    printf("%s:%s: ERROR, not owner!  owner=%p, should be this=%p\n",
         driverName, functionName, pArray->pNDArrayPool, this);
    return(ND_ERROR);
  }
  epicsMutexLock(listLock_);
  pArray->referenceCount++;
  epicsMutexUnlock(listLock_);
  return ND_SUCCESS;
}

/** This method decreases the reference count for the NDArray object.
  * \param[in] pArray The array on which to decrease the reference count.
  *
  * When the reference count reaches 0 the NDArray is placed back in the free list.
  * Plugins must call release() when an NDArray is removed from the queue and
  * processing on it is complete. Drivers must call release() after calling all
  * plugins.
  */
int NDArrayPool::release(NDArray *pArray)
{
  const char *functionName = "release";

  /* Make sure we own this array */
  if (pArray->pNDArrayPool != this) {
    printf("%s:%s: ERROR, not owner!  owner=%p, should be this=%p\n",
           driverName, functionName, pArray->pNDArrayPool, this);
     return(ND_ERROR);
  }
  epicsMutexLock(listLock_);
  pArray->referenceCount--;
  if (pArray->referenceCount == 0) {
    /* The last user has released this image, add it back to the free list */
    ellAdd(&freeList_, &pArray->node);
    numFree_++;
  }
  if (pArray->referenceCount < 0) {
    printf("%s:release ERROR, reference count < 0 pArray=%p\n",
           driverName, pArray);
  }
  epicsMutexUnlock(listLock_);
  return ND_SUCCESS;
}

template <typename dataTypeIn, typename dataTypeOut> void convertType(NDArray *pIn, NDArray *pOut)
{
  size_t i;
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
  size_t inStep, outStep, inOffset;
  int inDir;
  int i, bin;
  size_t inc, in, out;

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

/** Creates a new output NDArray from an input NDArray, performing
  * conversion operations.
  * This form of the function is for changing the data type only, not the dimensions,
  * which are preserved. 
  * \param[in] pIn The input array, source of the conversion.
  * \param[out] ppOut The output array, result of the conversion.
  * \param[in] dataTypeOut The data type of the output array.
  */
int NDArrayPool::convert(NDArray *pIn,
                         NDArray **ppOut,
                         NDDataType_t dataTypeOut)
                         
{
  NDDimension_t dims[ND_ARRAY_MAX_DIMS];
  int i;
  
  for (i=0; i<pIn->ndims; i++) {
    dims[i].size  = pIn->dims[i].size;
    dims[i].offset  = 0;
    dims[i].binning = 1;
    dims[i].reverse = 0;
  }
  return this->convert(pIn, ppOut, dataTypeOut, dims);
}             

/** Creates a new output NDArray from an input NDArray, performing
  * conversion operations.
  * The conversion can change the data type if dataTypeOut is different from
  * pIn->dataType. It can also change the dimensions. outDims may have different
  * values of size, binning, offset and reverse for each of its dimensions from input
  * array dimensions (pIn->dims).
  * \param[in] pIn The input array, source of the conversion.
  * \param[out] ppOut The output array, result of the conversion.
  * \param[in] dataTypeOut The data type of the output array.
  * \param[in] dimsOut The dimensions of the output array.
  */
int NDArrayPool::convert(NDArray *pIn,
                         NDArray **ppOut,
                         NDDataType_t dataTypeOut,
                         NDDimension_t *dimsOut)
{
  int dimsUnchanged;
  size_t dimSizeOut[ND_ARRAY_MAX_DIMS];
  NDDimension_t dimsOutCopy[ND_ARRAY_MAX_DIMS];
  int i;
  NDArray *pOut;
  NDArrayInfo_t arrayInfo;
  NDAttribute *pAttribute;
  int colorMode, colorModeMono = NDColorModeMono;
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
             driverName, functionName, (int)dimsOut[i].size, dimsOut[i].binning);
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
  pOut->timeStamp = pIn->timeStamp;
  pOut->uniqueId = pIn->uniqueId;
  /* Replace the dimensions with those passed to this function */
  memcpy(pOut->dims, dimsOutCopy, pIn->ndims*sizeof(NDDimension_t));
  pIn->pAttributeList->copy(pOut->pAttributeList);

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
          //status = ND_ERROR;
          break;
      }
    }
  } else {
    /* The input and output dimensions are not the same, so we are extracting a region
     * and/or binning */
    /* Clear entire output array */
    memset(pOut->pData, 0, arrayInfo.totalBytes);
    convertDimension(pIn, pOut, pIn->pData, pOut->pData, pIn->ndims-1);
  }

  /* Set fields in the output array */
  for (i=0; i<pIn->ndims; i++) {
    pOut->dims[i].offset = pIn->dims[i].offset + dimsOutCopy[i].offset;
    pOut->dims[i].binning = pIn->dims[i].binning * dimsOutCopy[i].binning;
    if (pIn->dims[i].reverse) pOut->dims[i].reverse = !pOut->dims[i].reverse;
  }

  /* If the frame is an RGBx frame and we have collapsed that dimension then change the colorMode */
  pAttribute = pOut->pAttributeList->find("ColorMode");
  if (pAttribute && pAttribute->getValue(NDAttrInt32, &colorMode)) {
    if      ((colorMode == NDColorModeRGB1) && (pOut->dims[0].size != 3)) 
      pAttribute->setValue(NDAttrInt32, (void *)&colorModeMono);
    else if ((colorMode == NDColorModeRGB2) && (pOut->dims[1].size != 3)) 
      pAttribute->setValue(NDAttrInt32, (void *)&colorModeMono);
    else if ((colorMode == NDColorModeRGB3) && (pOut->dims[2].size != 3))
      pAttribute->setValue(NDAttrInt32, (void *)&colorModeMono);
  }
  return ND_SUCCESS;
}

/** Returns maximum number of buffers this object is allowed to allocate; 0=unlimited */
int NDArrayPool::maxBuffers()
{  
return maxBuffers_;
}

/** Returns number of buffers this object has currently allocated */
int NDArrayPool::numBuffers()
{  
return numBuffers_;
}

/** Returns maximum bytes of memory this object is allowed to allocate; 0=unlimited */
size_t NDArrayPool::maxMemory()
{  
return maxMemory_;
}

/** Returns mumber of bytes of memory this object has currently allocated */
size_t NDArrayPool::memorySize()
{
  return memorySize_;
}

/** Returns number of NDArray objects in the free list */
int NDArrayPool::numFree()
{
  return numFree_;
}

/** Reports on the free list size and other properties of the NDArrayPool
  * object.
  * \param[in] details Level of report details desired; does nothing at present.
  */
int NDArrayPool::report(int details)
{
  printf("NDArrayPool:\n");
  printf("  numBuffers=%d, maxBuffers=%d\n",
         numBuffers_, maxBuffers_);
  printf("  memorySize=%ld, maxMemory=%ld\n",
        (long)memorySize_, (long)maxMemory_);
  printf("  numFree=%d\n",
         numFree_);
      
  return ND_SUCCESS;
}

/** NDArray constructor, no parameters.
  * Initializes all fields to 0.  Creates the attribute linked list and linked list mutex. */
NDArray::NDArray()
  : referenceCount(0), pNDArrayPool(NULL),  
    uniqueId(0), timeStamp(0.0), ndims(0), dataType(NDInt8),
    dataSize(0),  pData(NULL)
{
  memset(this->dims, 0, sizeof(this->dims));
  memset(&this->node, 0, sizeof(this->node));
  this->pAttributeList = new NDAttributeList();
}

/** NDArray destructor 
  * Frees the data array, deletes all attributes, frees the attribute list and destroys the mutex. */
NDArray::~NDArray()
{
  if (this->pData) free(this->pData);
  delete this->pAttributeList;
}

/** Convenience method returns information about an NDArray, including the total number of elements, 
  * the number of bytes per element, and the total number of bytes in the array.
  \param[out] pInfo Pointer to an NDArrayInfo_t structure, must have been allocated by caller. */
int NDArray::getInfo(NDArrayInfo_t *pInfo)
{
  int i;
  NDAttribute *pAttribute;

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
  pInfo->colorMode = NDColorModeMono;
  pAttribute = this->pAttributeList->find("ColorMode");
  if (pAttribute) pAttribute->getValue(NDAttrInt32, &pInfo->colorMode);
  pInfo->xDim        = 0;
  pInfo->yDim        = 0;
  pInfo->colorDim    = 0;
  pInfo->xSize       = 0;
  pInfo->ySize       = 0;
  pInfo->colorSize   = 0;
  pInfo->xStride     = 0;
  pInfo->yStride     = 0;
  pInfo->colorStride = 0;
  if (this->ndims > 0) {
    pInfo->xStride = 1;
    pInfo->xSize   = this->dims[0].size;
  }
  if (this->ndims > 1) {
    pInfo->yDim  = 1;
    pInfo->yStride = pInfo->xSize;
    pInfo->ySize   = this->dims[1].size;
  }
  if (this->ndims == 3) {
    switch (pInfo->colorMode) {
      case NDColorModeRGB1:
        pInfo->xDim    = 1;
        pInfo->yDim    = 2;
        pInfo->colorDim  = 0;
        pInfo->xStride   = this->dims[0].size;
        pInfo->yStride   = this->dims[0].size * this->dims[1].size;
        pInfo->colorStride = 1;
        break;
      case NDColorModeRGB2:
        pInfo->xDim    = 0;
        pInfo->yDim    = 2;
        pInfo->colorDim  = 1;
        pInfo->xStride   = 1;
        pInfo->yStride   = this->dims[0].size * this->dims[1].size;
        pInfo->colorStride = this->dims[0].size;
        break;
      case NDColorModeRGB3:
        pInfo->xDim    = 0;
        pInfo->yDim    = 1;
        pInfo->colorDim  = 2;
        pInfo->xStride   = 1;
        pInfo->yStride   = this->dims[0].size;
        pInfo->colorStride = this->dims[0].size * this->dims[1].size;
        break;
      default:
        break;
    }
    pInfo->xSize     = this->dims[pInfo->xDim].size;
    pInfo->ySize     = this->dims[pInfo->yDim].size;
    pInfo->colorSize = this->dims[pInfo->colorDim].size;
  }
  return(ND_SUCCESS);
}

/** Initializes the dimension structure to size=size, binning=1, reverse=0, offset=0.
  * \param[in] pDimension Pointer to an NDDimension_t structure, must have been allocated by caller.
  * \param[in] size The size of this dimension. */
int NDArray::initDimension(NDDimension_t *pDimension, size_t size)
{
  pDimension->size=size;
  pDimension->binning = 1;
  pDimension->offset = 0;
  pDimension->reverse = 0;
  return ND_SUCCESS;
}

/** Calls NDArrayPool::reserve() for this NDArray object; increases the reference count for this array. */
int NDArray::reserve()
{
  const char *functionName = "NDArray::reserve";

  if (!pNDArrayPool) {
    printf("%s: ERROR, no owner\n", functionName);
    return(ND_ERROR);
  }
  return(pNDArrayPool->reserve(this));
}

/** Calls NDArrayPool::release() for this object; decreases the reference count for this array. */
int NDArray::release()
{
  const char *functionName = "NDArray::release";

  if (!pNDArrayPool) {
    printf("%s: ERROR, no owner\n", functionName);
    return(ND_ERROR);
  }
  return(pNDArrayPool->release(this));
}

/** Reports on the properties of the array.
  * \param[in] details Level of report details desired; if >5 calls NDAttributeList::report().
  */
int NDArray::report(int details)
{
  int dim;
  
  printf("\n");
  printf("NDArray  Array address=%p:\n", this);
  printf("  ndims=%d dims=[",
    this->ndims);
  for (dim=0; dim<this->ndims; dim++) printf("%d ", (int)this->dims[dim].size);
  printf("]\n");
  printf("  dataType=%d, dataSize=%d, pData=%p\n",
        this->dataType, (int)this->dataSize, this->pData);
  printf("  uniqueId=%d, timeStamp=%f\n",
        this->uniqueId, this->timeStamp);
  printf("  number of attributes=%d\n", this->pAttributeList->count());
  if (details > 5) {
    this->pAttributeList->report(details);
  }
  return ND_SUCCESS;
}

/** NDAttributeList constructor
  */
NDAttributeList::NDAttributeList()
{
  ellInit(&this->list);
  this->lock = epicsMutexCreate();
}

/** NDAttributeList destructor
  */
NDAttributeList::~NDAttributeList()
{
  this->clear();
  ellFree(&this->list);
  epicsMutexDestroy(this->lock);
}

/** Adds an attribute to the list.
  * If an attribute of the same name already exists then
  * the existing attribute is deleted and replaced with the new one.
  * \param[in] pAttribute A pointer to the attribute to add.
  */
int NDAttributeList::add(NDAttribute *pAttribute)
{
  //const char *functionName = "NDAttributeList::add";

  epicsMutexLock(this->lock);
  /* Remove any existing attribute with this name */
  this->remove(pAttribute->pName);
  ellAdd(&this->list, &pAttribute->listNode.node);
  epicsMutexUnlock(this->lock);
  return(ND_SUCCESS);
}

/** Adds an attribute to the list.
  * This is a convenience function for adding attributes to a list.  
  * It first searches the list to see if there is an existing attribute
  * with the same name.  If there is it just changes the properties of the
  * existing attribute.  If not, it creates a new attribute with the
  * specified properties. 
  * IMPORTANT: This method is only capable of creating attributes
  * of the NDAttribute base class type, not derived class attributes.
  * To add attributes of a derived class to a list the NDAttributeList::add(NDAttribute*)
  * method must be used.
  * \param[in] pName The name of the attribute to be added. 
  * \param[in] pDescription The description of the attribute.
  * \param[in] dataType The data type of the attribute.
  * \param[in] pValue A pointer to the value for this attribute.
  *
  */
NDAttribute* NDAttributeList::add(const char *pName, const char *pDescription, NDAttrDataType_t dataType, void *pValue)
{
  //const char *functionName = "NDAttributeList::add";
  NDAttribute *pAttribute;

  epicsMutexLock(this->lock);
  pAttribute = this->find(pName);
  if (pAttribute) {
    pAttribute->setDescription(pDescription);
    pAttribute->setValue(dataType, pValue);
  } else {
    pAttribute = new NDAttribute(pName, pDescription, dataType, pValue);
    ellAdd(&this->list, &pAttribute->listNode.node);
  }
  epicsMutexUnlock(this->lock);
  return(pAttribute);
}



/** Finds an attribute by name; the search is case-insensitive.
  * \param[in] pName The name of the attribute to be found.
  * \return Returns a pointer to the attribute if found, NULL if not found. 
  */
NDAttribute* NDAttributeList::find(const char *pName)
{
  NDAttribute *pAttribute;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::find";

  epicsMutexLock(this->lock);
  pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  while (pListNode) {
    pAttribute = pListNode->pNDAttribute;
    if (epicsStrCaseCmp(pAttribute->pName, pName) == 0) goto done;
    pListNode = (NDAttributeListNode *)ellNext(&pListNode->node);
  }
  pAttribute = NULL;

  done:
  epicsMutexUnlock(this->lock);
  return(pAttribute);
}

/** Finds the next attribute in the linked list of attributes.
  * \param[in] pAttributeIn A pointer to the previous attribute in the list; 
  * if NULL the first attribute in the list is returned.
  * \return Returns a pointer to the next attribute if there is one, 
  * NULL if there are no more attributes in the list. */
NDAttribute* NDAttributeList::next(NDAttribute *pAttributeIn)
{
  NDAttribute *pAttribute=NULL;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::next";

  epicsMutexLock(this->lock);
  if (!pAttributeIn) {
    pListNode = (NDAttributeListNode *)ellFirst(&this->list);
   }
  else {
    pListNode = (NDAttributeListNode *)ellNext(&pAttributeIn->listNode.node);
  }
  if (pListNode) pAttribute = pListNode->pNDAttribute;
  epicsMutexUnlock(this->lock);
  return(pAttribute);
}

/** Returns the total number of attributes in the list of attributes.
  * \return Returns the number of attributes. */
int NDAttributeList::count()
{
  //const char *functionName = "NDAttributeList::count";

  return ellCount(&this->list);
}

/** Removes an attribute from the list.
  * \param[in] pName The name of the attribute to be deleted.
  * \return Returns ND_SUCCESS if the attribute was found and deleted, ND_ERROR if the
  * attribute was not found. */
int NDAttributeList::remove(const char *pName)
{
  NDAttribute *pAttribute;
  int status = ND_ERROR;
  //const char *functionName = "NDAttributeList::remove";

  epicsMutexLock(this->lock);
  pAttribute = this->find(pName);
  if (!pAttribute) goto done;
  ellDelete(&this->list, &pAttribute->listNode.node);
  delete pAttribute;
  status = ND_SUCCESS;

  done:
  epicsMutexUnlock(this->lock);
  return(status);
}

/** Deletes all attributes from the list. */
int NDAttributeList::clear()
{
  NDAttribute *pAttribute;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::clear";

  epicsMutexLock(this->lock);
  pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  while (pListNode) {
    pAttribute = pListNode->pNDAttribute;
    ellDelete(&this->list, &pListNode->node);
    delete pAttribute;
    pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  }
  epicsMutexUnlock(this->lock);
  return(ND_SUCCESS);
}

/** Copies all attributes from one attribute list to another.
  * It is efficient so that if the attribute already exists in the output
  * list it just copies the properties, and memory allocation is minimized.
  * The attributes are added to any existing attributes already present in the output list.
  * \param[out] pListOut A pointer to the output attribute list to copy to.
  */
int NDAttributeList::copy(NDAttributeList *pListOut)
{
  NDAttribute *pAttrIn, *pAttrOut, *pFound;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::copy";

  epicsMutexLock(this->lock);
  pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  while (pListNode) {
    pAttrIn = pListNode->pNDAttribute;
    /* See if there is already an attribute of this name in the output list */
    pFound = pListOut->find(pAttrIn->pName);
    /* The copy function will copy the properties, and will create the attribute if pFound is NULL */
    pAttrOut = pAttrIn->copy(pFound);
    /* If pFound is NULL, then a copy created a new attribute, need to add it to the list */
    if (!pFound) pListOut->add(pAttrOut);
    pListNode = (NDAttributeListNode *)ellNext(&pListNode->node);
  }
  epicsMutexUnlock(this->lock);
  return(ND_SUCCESS);
}

/** Updates all attribute values in the list; calls NDAttribute::updateValue() for each attribute in the list.
  */
int NDAttributeList::updateValues()
{
  NDAttribute *pAttribute;
  NDAttributeListNode *pListNode;
  //const char *functionName = "NDAttributeList::updateValues";

  epicsMutexLock(this->lock);
  pListNode = (NDAttributeListNode *)ellFirst(&this->list);
  while (pListNode) {
    pAttribute = pListNode->pNDAttribute;
    pAttribute->updateValue();
    pListNode = (NDAttributeListNode *)ellNext(&pListNode->node);
  }
  epicsMutexUnlock(this->lock);
  return(ND_SUCCESS);
}

/** Reports on the properties of the attribute list.
  * \param[in] details Level of report details desired; if >10 calls NDAttribute::report() for each attribute.
  */
int NDAttributeList::report(int details)
{
  NDAttribute *pAttribute;
  NDAttributeListNode *pListNode;
  
  epicsMutexLock(this->lock);
  printf("\n");
  printf("NDAttributeList: address=%p:\n", this);
  printf("  number of attributes=%d\n", this->count());
  if (details > 10) {
    pListNode = (NDAttributeListNode *) ellFirst(&this->list);
    while (pListNode) {
      pAttribute = (NDAttribute *)pListNode->pNDAttribute;
      pAttribute->report(details);
      pListNode = (NDAttributeListNode *) ellNext(&pListNode->node);
    }
  }
  epicsMutexUnlock(this->lock);
  return ND_SUCCESS;
}


/** NDAttribute constructor
  * \param[in] pName The name of the attribute to be created. 
  * \param[in] pDescription The description of the attribute.
  * \param[in] dataType The data type of the attribute.
  * \param[in] pValue A pointer to the value for this attribute.
  */
NDAttribute::NDAttribute(const char *pName, const char *pDescription, NDAttrDataType_t dataType, void *pValue)
{

  this->pName = epicsStrDup(pName);
  this->pDescription = epicsStrDup(pDescription);
  this->pSource = epicsStrDup("");
  this->sourceType = NDAttrSourceDriver;
  this->pString = NULL;
  if (pValue) this->setValue(dataType, pValue);
  this->listNode.pNDAttribute = this;
}


/** NDAttribute destructor 
  * Frees the strings for the name, and if they exist, the description and pString. */
NDAttribute::~NDAttribute()
{
  if (this->pName) free(this->pName);
  if (this->pDescription) free(this->pDescription);
  if (this->pSource) free(this->pSource);
  if (this->pString) free(this->pString);
}

/** Copies properties from <b>this</b> to pOut.
  * \param[in] pOut A pointer to the output attribute
  *         If NULL the output attribute will be created.
  * \return  Returns a pointer to the copy
  */
NDAttribute* NDAttribute::copy(NDAttribute *pOut)
{
  void *pValue;
  
  if (!pOut) pOut = new NDAttribute(this->pName);
  pOut->setDescription(this->pDescription);
  pOut->setSource(this->pSource);
  pOut->sourceType = this->sourceType;
  pOut->dataType = this->dataType;
  if (this->dataType == NDAttrString) pValue = this->pString;
  else pValue = &this->value;
  pOut->setValue(this->dataType, pValue);
  return(pOut);
}

/** Sets the description string for this attribute.
  * This method must be used to set the description string; pDescription must not be directly modified. 
  * \param[in] pDescription String with the desciption. */
int NDAttribute::setDescription(const char *pDescription) {

  if (this->pDescription) {
    /* If the new description is the same as the old one return, 
     * saves freeing and allocating memory */
    if (strcmp(this->pDescription, pDescription) == 0) return(ND_SUCCESS);
    free(this->pDescription);
  }
  if (pDescription) this->pDescription = epicsStrDup(pDescription);
  else this->pDescription = NULL;
  return(ND_SUCCESS);
}

/** Sets the source string for this attribute. 
  * This method must be used to set the source string; pSource must not be directly modified. 
  * \param[in] pSource String with the source. */
int NDAttribute::setSource(const char *pSource) {

  if (this->pSource) {
    /* If the new srouce is the same as the old one return, 
     * saves freeing and allocating memory */
    if (strcmp(this->pSource, pSource) == 0) return(ND_SUCCESS);
    free(this->pSource);
  }
  if (pSource) this->pSource = epicsStrDup(pSource);
  else this->pSource = NULL;
  return(ND_SUCCESS);
}

/** Sets the value for this attribute. 
  * \param[in] dataType Data type of the value.
  * \param[in] pValue Pointer to the value. */
int NDAttribute::setValue(NDAttrDataType_t dataType, void *pValue)
{
  NDAttrDataType_t prevDataType = this->dataType;
    
  this->dataType = dataType;

  /* If any data type but undefined then pointer must be valid */
  if ((dataType != NDAttrUndefined) && !pValue) return(ND_ERROR);

  /* Treat strings specially */
  if (dataType == NDAttrString) {
    /* If the previous value was the same string don't do anything, 
     * saves freeing and allocating memory.  
     * If not the same free the old string and copy new one. */
    if ((prevDataType == NDAttrString) && this->pString) {
      if (strcmp(this->pString, (char *)pValue) == 0) return(ND_SUCCESS);
      free(this->pString);
    }
    this->pString = epicsStrDup((char *)pValue);
    return(ND_SUCCESS);
  }
  if (this->pString) {
    free(this->pString);
    this->pString = NULL;
  }
  switch (dataType) {
    case NDAttrInt8:
      this->value.i8 = *(epicsInt8 *)pValue;
      break;
    case NDAttrUInt8:
      this->value.ui8 = *(epicsUInt8 *)pValue;
      break;
    case NDAttrInt16:
      this->value.i16 = *(epicsInt16 *)pValue;
      break;
    case NDAttrUInt16:
      this->value.ui16 = *(epicsUInt16 *)pValue;
      break;
    case NDAttrInt32:
      this->value.i32 = *(epicsInt32*)pValue;
      break;
    case NDAttrUInt32:
      this->value.ui32 = *(epicsUInt32 *)pValue;
      break;
    case NDAttrFloat32:
      this->value.f32 = *(epicsFloat32 *)pValue;
      break;
    case NDAttrFloat64:
      this->value.f64 = *(epicsFloat64 *)pValue;
      break;
    case NDAttrUndefined:
      break;
    default:
      return(ND_ERROR);
      break;
  }
  return(ND_SUCCESS);
}

/** Returns the data type and size of this attribute.
  * \param[out] pDataType Pointer to location to return the data type.
  * \param[out] pSize Pointer to location to return the data size; this is the
  *  data type size for all data types except NDAttrString, in which case it is the length of the
  * string including 0 terminator. */
int NDAttribute::getValueInfo(NDAttrDataType_t *pDataType, size_t *pSize)
{
  *pDataType = this->dataType;
  switch (this->dataType) {
    case NDAttrInt8:
      *pSize = sizeof(this->value.i8);
      break;
    case NDAttrUInt8:
      *pSize = sizeof(this->value.ui8);
      break;
    case NDAttrInt16:
      *pSize = sizeof(this->value.i16);
      break;
    case NDAttrUInt16:
      *pSize = sizeof(this->value.ui16);
      break;
    case NDAttrInt32:
      *pSize = sizeof(this->value.i32);
      break;
    case NDAttrUInt32:
      *pSize = sizeof(this->value.ui32);
      break;
    case NDAttrFloat32:
      *pSize = sizeof(this->value.f32);
      break;
    case NDAttrFloat64:
      *pSize = sizeof(this->value.f64);
      break;
    case NDAttrString:
      if (this->pString) *pSize = strlen(this->pString)+1;
      else *pSize = 0;
      break;
    case NDAttrUndefined:
      *pSize = 0;
      break;
    default:
      return(ND_ERROR);
      break;
  }
  return(ND_SUCCESS);
}

/** Returns the value of this attribute.
  * \param[in] dataType Data type for the value.
  * \param[out] pValue Pointer to location to return the value.
  * \param[in] dataSize Size of the input data location; only used when dataType is NDAttrString.
  *
  * Currently the dataType parameter is only used to check that it matches the actual data type,
  * and ND_ERROR is returned if it does not.  In the future data type conversion may be added. */
int NDAttribute::getValue(NDAttrDataType_t dataType, void *pValue, size_t dataSize)
{
  if (dataType != this->dataType) return(ND_ERROR);
  switch (this->dataType) {
    case NDAttrInt8:
      *(epicsInt8 *)pValue = this->value.i8;
      break;
    case NDAttrUInt8:
       *(epicsUInt8 *)pValue = this->value.ui8;
      break;
    case NDAttrInt16:
      *(epicsInt16 *)pValue = this->value.i16;
      break;
    case NDAttrUInt16:
      *(epicsUInt16 *)pValue = this->value.ui16;
      break;
    case NDAttrInt32:
      *(epicsInt32*)pValue = this->value.i32;
      break;
    case NDAttrUInt32:
      *(epicsUInt32 *)pValue = this->value.ui32;
      break;
    case NDAttrFloat32:
      *(epicsFloat32 *)pValue = this->value.f32;
      break;
    case NDAttrFloat64:
      *(epicsFloat64 *)pValue = this->value.f64;
      break;
    case NDAttrString:
      if (!this->pString) return (ND_ERROR);
      if (dataSize == 0) dataSize = strlen(this->pString)+1;
      strncpy((char *)pValue, this->pString, dataSize);
      break;
    case NDAttrUndefined:
    default:
      return(ND_ERROR);
      break;
  }
  return(ND_SUCCESS);
}

/** Updates the current value of this attribute.
  * The base class does nothing, but derived classes may fetch the current value of the attribute,
  * for example from an EPICS PV or driver parameter library.
 */
int NDAttribute::updateValue()
{
  return(ND_SUCCESS);
}

/** Reports on the properties of the attribute.
 * \param[in] details Level of report details desired; currently does nothing
  */
int NDAttribute::report(int details)
{
  
  printf("NDAttribute, address=%p:\n", this);
  printf("  name=%s\n", this->pName);
  printf("  description=%s\n", this->pDescription);
  printf("  source type=%d\n", this->sourceType);
  printf("  source=%s\n", this->pSource);
  switch (this->dataType) {
    case NDAttrInt8:
      printf("  dataType=NDAttrInt8, value=%d\n", this->value.i8);
      break;
    case NDAttrUInt8:
      printf("  dataType=NDAttrUInt8, value=%u\n", this->value.ui8);
      break;
    case NDAttrInt16:
      printf("  dataType=NDAttrInt16, value=%d\n", this->value.i16);
      break;
    case NDAttrUInt16:
      printf("  dataType=NDAttrUInt16, value=%d\n", this->value.ui16);
      break;
    case NDAttrInt32:
      printf("  dataType=NDAttrInt32, value=%d\n", this->value.i32);
      break;
    case NDAttrUInt32:
      printf("  dataType=NDAttrUInt32, value=%d\n", this->value.ui32);
      break;
    case NDAttrFloat32:
      printf("  dataType=NDAttrFloat32, value=%f\n", this->value.f32);
      break;
    case NDAttrFloat64:
      printf("  dataType=NDAttrFloat64, value=%f\n", this->value.f64);
      break;
    case NDAttrString:
      printf("  dataType=NDAttrString, value=%s\n", this->pString);
      break;
    case NDAttrUndefined:
      printf("  dataType=NDAttrUndefined\n");
      break;
    default:
      printf("  dataType=UNKNOWN\n");
      return(ND_ERROR);
      break;
  }
  return ND_SUCCESS;
}


