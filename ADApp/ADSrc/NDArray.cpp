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

#include <epicsExport.h>

#include "NDArray.h"

/** NDArray constructor, no parameters.
  * Initializes all fields to 0.  Creates the attribute linked list and linked list mutex. */
NDArray::NDArray()
  : referenceCount(0), pNDArrayPool(NULL),  
    uniqueId(0), timeStamp(0.0), ndims(0), dataType(NDInt8),
    dataSize(0),  pData(NULL)
{
  this->epicsTS.secPastEpoch = 0;
  this->epicsTS.nsec = 0;
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
        // This is a 3-D array, but is not RGB
        pInfo->xDim    = 0;
        pInfo->yDim    = 1;
        pInfo->colorDim  = 2;
        pInfo->xStride   = 1;
        pInfo->yStride   = this->dims[0].size;
        pInfo->colorStride = this->dims[0].size * this->dims[1].size;
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
    printf("%s: WARNING, no owner\n", functionName);
    return(ND_ERROR);
  }
  return(pNDArrayPool->reserve(this));
}

/** Calls NDArrayPool::release() for this object; decreases the reference count for this array. */
int NDArray::release()
{
  const char *functionName = "NDArray::release";

  if (!pNDArrayPool) {
    printf("%s: WARNING, no owner\n", functionName);
    return(ND_ERROR);
  }
  return(pNDArrayPool->release(this));
}

/** Reports on the properties of the array.
  * \param[in] fp File pointer for the report output.
  * \param[in] details Level of report details desired; if >5 calls NDAttributeList::report().
  */
int NDArray::report(FILE *fp, int details)
{
  int dim;
  
  fprintf(fp, "\n");
  fprintf(fp, "NDArray  Array address=%p:\n", this);
  fprintf(fp, "  ndims=%d dims=[",
    this->ndims);
  for (dim=0; dim<this->ndims; dim++) fprintf(fp, "%d ", (int)this->dims[dim].size);
  fprintf(fp, "]\n");
  fprintf(fp, "  dataType=%d, dataSize=%d, pData=%p\n",
        this->dataType, (int)this->dataSize, this->pData);
  fprintf(fp, "  uniqueId=%d, timeStamp=%f\n",
        this->uniqueId, this->timeStamp);
  fprintf(fp, "  number of attributes=%d\n", this->pAttributeList->count());
  if (details > 5) {
    this->pAttributeList->report(fp, details);
  }
  return ND_SUCCESS;
}

