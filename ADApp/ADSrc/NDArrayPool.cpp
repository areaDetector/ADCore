/** NDArrayPool.cpp
 *
 * Mark Rivers
 * University of Chicago
 * October 18, 2013
 *
 */

#include <stdlib.h>
#include <dbDefs.h>
#include <stdint.h>

#include <cantProceed.h>
#include <epicsExport.h>

#include "NDArray.h"

// How much larger an NDArray must be than the required size before it is considered "too large"
#define THRESHOLD_SIZE_RATIO 1.5

static const char *driverName = "NDArrayPool";


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
  * \param[in] pDriver Pointer to the asynNDArrayDriver that created this object.
  * \param[in] maxMemory Maxiumum number of bytes of memory the the pool is allowed to use, summed over
  * all of the NDArray objects; 0=unlimited.
  */
NDArrayPool::NDArrayPool(class asynNDArrayDriver *pDriver, size_t maxMemory)
  : numBuffers_(0), maxMemory_(maxMemory), memorySize_(0), pDriver_(pDriver)
{
  listLock_ = epicsMutexCreate();
}

/** Create new NDArray object. 
  * This method should be overriden by a pool class that manages objects 
  * that derive from NDArray class.
  */
NDArray* NDArrayPool::createArray() 
{
    return new NDArray;
}

/** Hook for pool classes that manage objects derived from NDArray class.
  * This hook is called after new array has been allocated.
  * \param[in] pArray Pointer to the allocated NDArray object
  */
void NDArrayPool::onAllocateArray(NDArray *pArray)
{
}

/** Hook for pool classes that manage objects derived from NDArray class.
  * This hook is called after array has been reserved.
  * \param[in] pArray Pointer to the reserved NDArray object
  */
void NDArrayPool::onReserveArray(NDArray *pArray)
{
}

/** Hook for pool classes that manage objects derived from NDArray class.
  * This hook is called after array has been released.
  * \param[in] pArray Pointer to the released NDArray object
  */
void NDArrayPool::onReleaseArray(NDArray *pArray)
{
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
  * allocate a new one and add it to the free list. If allocating the memory required for
  * this NDArray would cause the cumulative memory allocated for the pool to exceed
  * maxMemory then an error will be returned. alloc() sets the reference count for the
  * returned NDArray to 1.
  */
NDArray* NDArrayPool::alloc(int ndims, size_t *dims, NDDataType_t dataType, size_t dataSize, void *pData)
{
  NDArray *pArray=NULL;
  NDArrayInfo_t arrayInfo;
  const char* functionName = "NDArrayPool::alloc:";

  epicsMutexLock(listLock_);

  // Compute the required NDArray size
  NDArray::computeArrayInfo(ndims, dims, dataType, &arrayInfo);
  if (dataSize == 0) {
    dataSize = arrayInfo.totalBytes;
  } 
  else if (dataSize < arrayInfo.totalBytes) {
    asynPrint(pDriver_->pasynUserSelf, ASYN_TRACE_ERROR,
      "%s: ERROR: required size=%d passed size=%d is too small, using required size\n",
      functionName, (int)arrayInfo.totalBytes, (int)dataSize);
    dataSize = arrayInfo.totalBytes;
    // Since the passed dataSize was wrong we don't trust the passed pointer either
    if (pData != NULL) pData = NULL;
  }

  // Try to find an array in the free list which is big enough.
  freeListElement testElement(NULL, dataSize);
  std::multiset<freeListElement>::iterator pListElement = freeList_.lower_bound(testElement);
  if (pListElement == freeList_.end()) {
    /* We did not find a free image that is large enough, allocate a new one */
    numBuffers_++;
    pArray = this->createArray();
  } else {
    pArray = pListElement->pArray_;
    if (pListElement->dataSize_ > (dataSize * THRESHOLD_SIZE_RATIO)) {
      // We found an array but it is too large.  Set the size to 0 so it will be allocated below.
      memorySize_ -= pArray->dataSize;
      free(pArray->pData);
      pArray->pData = NULL;
    }
    freeList_.erase(pListElement);
  }
    
  /* Initialize fields */
  pArray->pNDArrayPool = this;
  pArray->referenceCount = 1;
  pArray->pDriver = pDriver_;
  pArray->dataType = dataType;
  pArray->ndims = ndims;
  memset(pArray->dims, 0, sizeof(pArray->dims));
  for (int i=0; i<ndims && i<ND_ARRAY_MAX_DIMS; i++) {
    pArray->dims[i].size = dims[i];
    pArray->dims[i].offset = 0;
    pArray->dims[i].binning = 1;
    pArray->dims[i].reverse = 0;
  }

  /* Erase the attributes if that global flag is set */
  if (eraseNDAttributes) pArray->pAttributeList->clear();

  /* At this point pArray exists, but pArray->pData may be NULL */
  /* If the caller passed a valid buffer use that */
  if (pData) {
    pArray->pData = pData;
  } else if (pArray->pData == NULL) {
    if ((maxMemory_ > 0) && ((memorySize_ + dataSize) > maxMemory_)) {
      // We don't have enough memory to allocate the array
      // See if we can get memory by deleting arrays
      // Delete the largest arrays first, i.e. work from the end of freeList_
      NDArray *freeArray;
      std::multiset<freeListElement>::iterator it;
      while (!freeList_.empty() && ((memorySize_ + dataSize) > maxMemory_)) {
        it = freeList_.end();
        it--;
        freeArray = it->pArray_;
        freeList_.erase(it);
        memorySize_ -= freeArray->dataSize;
        numBuffers_--;
        delete freeArray;
      }
    }
    if ((maxMemory_ > 0) && ((memorySize_ + dataSize) > maxMemory_)) {
      asynPrint(pDriver_->pasynUserSelf, ASYN_TRACE_ERROR, 
             "%s: error: reached limit of %ld memory (%d buffers)\n",
             functionName, (long)maxMemory_, numBuffers_);
    } else {
      pArray->pData = malloc(dataSize);
      if (pArray->pData) {
        pArray->dataSize = dataSize;
        memorySize_ += dataSize;
      }
    }
  }
  // If we don't have a valid memory buffer see pArray to NULL to indicate error
  if (pArray && (pArray->pData == NULL)) {
    delete pArray;
    numBuffers_--;
    pArray = NULL;
  }

  // Call allocation hook (for pools that manage objects derived from NDArray class)
  onAllocateArray(pArray);
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
    if(NULL==pOut) return NULL;
  }
  pOut->uniqueId = pIn->uniqueId;
  pOut->timeStamp = pIn->timeStamp;
  pOut->epicsTS = pIn->epicsTS;
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
    asynPrint(pDriver_->pasynUserSelf, ASYN_TRACE_ERROR, 
      "%s::%s: ERROR, not owner!  owner=%p, should be this=%p\n",
      driverName, functionName, pArray->pNDArrayPool, this);
    return(ND_ERROR);
  }
  //asynPrint(pDriver_->pasynUserSelf, ASYN_TRACE_FLOW,
  //  "NDArrayPool::reserve pArray=%p, count=%d\n", pArray, pArray->referenceCount);
  epicsMutexLock(listLock_);
  // If the reference count is less than 1 then something is wrong, this NDArray has been released.
  if (pArray->referenceCount < 1) {
    cantProceed("%s:reserve ERROR, reference count = %d, should be >= 1, pArray=%p\n",
           driverName, pArray->referenceCount, pArray);
  }
  pArray->referenceCount++;

  // Call reservation hook (for pools that manage objects derived from NDArray class)
  onReserveArray(pArray);
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
    asynPrint(pDriver_->pasynUserSelf, ASYN_TRACE_ERROR, 
      "%s::%s: ERROR, not owner!  owner=%p, should be this=%p\n",
      driverName, functionName, pArray->pNDArrayPool, this);
    return(ND_ERROR);
  }
  //asynPrint(pDriver_->pasynUserSelf, ASYN_TRACE_FLOW,
  //  "NDArrayPool::release pArray=%p, count=%d\n", pArray, pArray->referenceCount);
  epicsMutexLock(listLock_);
  pArray->referenceCount--;
  if (pArray->referenceCount == 0) {
    /* The last user has released this image, add it back to the free list */
    freeListElement listElement(pArray, pArray->dataSize);
    freeList_.insert(listElement);
  }
  if (pArray->referenceCount < 0) {
    cantProceed("%s:release ERROR, reference count < 0 pArray=%p\n",
           driverName, pArray);
  }

  // Call release hook (for pools that manage objects derived from NDArray class)
  onReleaseArray(pArray);
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
      asynPrint(pDriver_->pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s: ERROR, invalid output dimension, size=%d, binning=%d\n",
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
    asynPrint(pDriver_->pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: ERROR, cannot allocate output array\n",
      driverName, functionName);
    return(ND_ERROR);
  }
  /* Copy fields from input to output */
  pOut->timeStamp = pIn->timeStamp;
  pOut->epicsTS = pIn->epicsTS;
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
      pAttribute->setValue(&colorModeMono);
    else if ((colorMode == NDColorModeRGB2) && (pOut->dims[1].size != 3)) 
      pAttribute->setValue(&colorModeMono);
    else if ((colorMode == NDColorModeRGB3) && (pOut->dims[2].size != 3))
      pAttribute->setValue(&colorModeMono);
  }
  return ND_SUCCESS;
}

/** Returns number of buffers this object has currently allocated */
int NDArrayPool::getNumBuffers()
{  
  return numBuffers_;
}

/** Returns maximum bytes of memory this object is allowed to allocate; 0=unlimited */
size_t NDArrayPool::getMaxMemory()
{
  return maxMemory_;
}

/** Returns mumber of bytes of memory this object has currently allocated */
size_t NDArrayPool::getMemorySize()
{
  return memorySize_;
}

/** Returns number of NDArray objects in the free list */
int NDArrayPool::getNumFree()
{
  epicsMutexLock(listLock_);
  int size = (int)freeList_.size();
  epicsMutexUnlock(listLock_);
  return size;
}

/** Deletes all of the NDArrays in the free list */
void NDArrayPool::emptyFreeList()
{
  NDArray *freeArray;
  std::multiset<freeListElement>::iterator it;
  epicsMutexLock(listLock_);
  while (!freeList_.empty()) {
    it = freeList_.begin();
    freeArray = it->pArray_;
    freeList_.erase(it);
    memorySize_ -= freeArray->dataSize;
    numBuffers_--;
    delete freeArray;
  }
  epicsMutexUnlock(listLock_);
}

/** Reports on the free list size and other properties of the NDArrayPool
  * object.
  * \param[in] fp File pointer for the report output.
  * \param[in] details Level of report details desired; does nothing at present.
  */
int NDArrayPool::report(FILE *fp, int details)
{
  fprintf(fp, "\n");
  fprintf(fp, "NDArrayPool:\n");
  fprintf(fp, "  numBuffers=%d, numFree=%d\n",
         numBuffers_, this->getNumFree());
  fprintf(fp, "  memorySize=%ld, maxMemory=%ld\n",
        (long)memorySize_, (long)maxMemory_);
  if (details > 5) {
    int i;
    std::multiset<freeListElement>::iterator it;
    NDArray *freeArray;
    fprintf(fp, "  freeList: (index, dataSize, pArray)\n");
    epicsMutexLock(listLock_);
    for (it=freeList_.begin(),i=0; it!=freeList_.end(); ++it,i++) {
      fprintf(fp, "    %d %d %p\n", i, (int)it->dataSize_, it->pArray_);
    }
    if (details > 10) {
      for (it=freeList_.begin(); it!=freeList_.end(); ++it) {
        freeArray = it->pArray_;
        fprintf(fp, "    Array %d\n", i);
        freeArray->report(fp, details);
      }
    }
    epicsMutexUnlock(listLock_);
  }
  return ND_SUCCESS;
}
