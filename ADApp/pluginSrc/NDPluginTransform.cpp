/*
 * NDPluginTransform.cpp
 *
 * Transform plugin
 * Author: John Hammonds, Chris Roehrig, Mark Rivers
 *
 * Created Oct. 28, 2009
 *
 * Change Log:
 *
 * 27 April 2014 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginTransform.h"

/* Enums to describe the types of transformations */
typedef enum {
  TransformNone,
  TransformRotate90,
  TransformRotate180,
  TransformRotate270,
  TransformMirror,
  TransformRotate90Mirror,
  TransformRotate180Mirror,
  TransformRotate270Mirror,
} NDPluginTransformType_t;

/** Perform the move of the pixels to the new orientation. */
template <typename epicsType>
void transformNDArray(NDArray *inArray, NDArray *outArray, int transformType, int colorMode, NDArrayInfo_t *arrayInfo)
{
  epicsType *inData = (epicsType *)inArray->pData;
  epicsType *outData = (epicsType *)outArray->pData;
  int x, y, color;
  int xSize, ySize, colorSize;
  int xStride, yStride, colorStride;
  int elementSize;

  xSize = (int)arrayInfo->xSize;
  ySize = (int)arrayInfo->ySize;
  if (arrayInfo->colorSize > 0)
    colorSize = (int)arrayInfo->colorSize;
  else
    colorSize = 1;

  xStride = (int)arrayInfo->xStride;
  yStride = (int)arrayInfo->yStride;
  colorStride = (int)arrayInfo->colorStride;
  elementSize = arrayInfo->bytesPerElement;

  // Assume output array is same dimensions as input.  Handle rotation cases below.
  outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->xDim].size;
  outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->yDim].size;
  if (inArray->ndims > 2) outArray->dims[arrayInfo->colorDim].size = inArray->dims[arrayInfo->colorDim].size;

  switch (transformType)
  {
    case (TransformNone):

      // Nothing to do, since the input array has already been copied to the output array
      break;

    case (TransformRotate90):

      outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->yDim].size;
      outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->xDim].size;

      if (colorMode == NDColorModeMono)
      {      
        for (x = 0; x < xSize; x++)
        {
          for (y = (ySize - 1); y >= 0; y--)
          {
            outData[(((ySize-1) - y) * xStride) + (x * ySize)] = inData[(y * yStride) + (x * xStride)];
          }
        }          
      }

      if (colorMode == NDColorModeRGB3)
      {      
        for (x = 0; x < xSize; x++)
        {
          for (y = (ySize - 1); y >= 0; y--)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(((ySize-1) - y) * xStride) + (x * ySize)] = inData[(y * yStride) + (x * xStride)];
            outData[(((ySize-1) - y) * xStride) + (x * ySize) + colorStride] = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(((ySize-1) - y) * xStride) + (x * ySize) + (2 * colorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }          
      }

      if (colorMode == NDColorModeRGB2)
      {
        /** The values of colorStride and yStride need to change
          because the dimensions of the image may have changed.
        */
        int newColorStride = ySize;
        int newYStride = newColorStride * colorSize;

        for (y = (ySize - 1); y >= 0; y--)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(((ySize - 1) - y) * xStride) + (x * newYStride)] = inData[(y * yStride) + (x * xStride)];
            outData[(((ySize - 1) - y) * xStride) + (x * newYStride) + newColorStride] = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(((ySize - 1) - y) * xStride) + (x * newYStride) + (2 * newColorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB1)
      {
        int source_offset;
        int target_offset;

        /** Calculate a new value for the Y stride. */
        int newStride = colorSize * ySize;

        for (y = (ySize - 1); y >= 0; y--)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            source_offset = (x * xStride) + (y * yStride);
            target_offset = (((ySize - 1) - y) * xStride) + (x * (newStride));
            outData[target_offset + 0] = inData[source_offset + 0];;
            outData[target_offset + 1] = inData[source_offset + 1];;
            outData[target_offset + 2] = inData[source_offset + 2];;
          }
        }
      }

      break;

    case (TransformRotate180):

      if (colorMode == NDColorModeMono)
      {
        for (y = (ySize - 1); y >= 0; y--)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            outData[(((xSize - 1) - x) * xStride) + (((ySize - 1) - y) * yStride)] = inData[(y * yStride) + (x * xStride)];
          }
        }
      }
      else
      {
        for (y = (ySize - 1); y >= 0; y--)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(((xSize - 1) - x) * xStride) + (((ySize - 1) - y) * yStride)] = inData[(y * yStride) + (x * xStride)];
            outData[(((xSize - 1) - x) * xStride) + (((ySize - 1) - y) * yStride) + colorStride] = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(((xSize - 1) - x) * xStride) + (((ySize - 1) - y) * yStride) + (2 * colorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }
      }  
      break;

    case (TransformRotate270):

      outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->yDim].size;
      outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->xDim].size;

      if (colorMode == NDColorModeMono)
      {
        for (x = (xSize - 1); x >= 0; x--)
        {
          for (y = 0; y < ySize; y++)
          {
            outData[(y * xStride) + (((xSize - 1) - x) * ySize)] = inData[(y * yStride) + (x * xStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB3)
      {
        for (x = (xSize - 1); x >= 0; x--)
        {
          for (y = 0; y < ySize; y++)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(y * xStride) + (((xSize - 1) - x) * ySize)]           = inData[(y * yStride) + (x * xStride)];
            outData[(y * xStride) + (((xSize - 1) - x) * ySize) + colorStride]     = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(y * xStride) + (((xSize - 1) - x) * ySize) + (2 * colorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB2)
      {
        /** The values of colorStride and yStride need to change
          because the dimensions of the image may have changed.
        */
        int newColorStride = ySize;
        int newYStride = newColorStride * colorSize;

        for (y = 0; y < ySize; y++)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(y * xStride) + (((xSize - 1) - x) * newYStride)]            = inData[(y * yStride) + (x * xStride)];
            outData[(y * xStride) + (((xSize - 1) - x) * newYStride) + newColorStride]     = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(y * xStride) + (((xSize - 1) - x) * newYStride) + (2 * newColorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB1)
      {
        int source_offset;
        int target_offset;

        /** Calculate a new value for the Y stride. */
        int newStride = colorSize * ySize;

        for (y = 0; y < ySize; y++)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            source_offset = (x * xStride) + (y * yStride);
            target_offset = (y * xStride) + (((xSize-1) - x) * (newStride));
            outData[target_offset + 0] = inData[source_offset + 0];;
            outData[target_offset + 1] = inData[source_offset + 1];;
            outData[target_offset + 2] = inData[source_offset + 2];;
          }
        }
      }

      break;

    case (TransformRotate90Mirror):

      outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->yDim].size;
      outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->xDim].size;

      if (colorMode == NDColorModeMono)
      {
        for (x = 0; x < xSize; x++)
        {
          for (y = 0; y < ySize; y++)
          {
            outData[(y * xStride) + (x * ySize)] = inData[(y * yStride) + (x * xStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB3)
      {
        for (x = 0; x < xSize; x++)
        {
          for (y = 0; y < ySize; y++)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(y * xStride) + (x * ySize)] = inData[(y * yStride) + (x * xStride)];
            outData[(y * xStride) + (x * ySize) + colorStride] = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(y * xStride) + (x * ySize) + (2 * colorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB2)
      {
        /** The values of colorStride and yStride need to change
          because the dimensions of the image may have changed.
        */
        int newColorStride = ySize;
        int newYStride = newColorStride * colorSize;

        for (y = 0; y < ySize; y++)
        {
          for (x = 0; x < xSize; x++)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(y * xStride) + (x * newYStride)] = inData[(y * yStride) + (x * xStride)];
            outData[(y * xStride) + (x * newYStride) + newColorStride] = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(y * xStride) + (x * newYStride) + (2 * newColorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB1)
      {
        int source_offset;
        int target_offset;

        /** Calculate a new value for the Y stride. */
        int newStride = colorSize * ySize;

        for (y = 0; y < ySize; y++)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            source_offset = (x * xStride) + (y * yStride);
            target_offset = (y * xStride) + (x * (newStride));
            outData[target_offset + 0] = inData[source_offset + 0];;
            outData[target_offset + 1] = inData[source_offset + 1];;
            outData[target_offset + 2] = inData[source_offset + 2];;
          }
        }
      }

      break;

    case (TransformRotate270Mirror):

      outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->yDim].size;
      outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->xDim].size;

      if (colorMode == NDColorModeMono)
      {
        for (x = (xSize - 1); x >= 0; x--)
        {
          for (y = (ySize - 1); y >= 0; y--)
          {
            outData[(((ySize - 1) - y) * xStride) + (((xSize - 1) - x) * ySize)] = inData[(y * yStride) + (x * xStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB3)
      {
        for (x = (xSize - 1); x >= 0; x--)
        {
          for (y = (ySize - 1); y >= 0; y--)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(((ySize - 1) - y) * xStride) + (((xSize - 1) - x) * ySize)] = inData[(y * yStride) + (x * xStride)];
            outData[(((ySize - 1) - y) * xStride) + (((xSize - 1) - x) * ySize) + colorStride] = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(((ySize - 1) - y) * xStride) + (((xSize - 1) - x) * ySize) + (2 * colorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB2)
      {
        /** The values of colorStride and yStride need to change
          because the dimensions of the image may have changed.
        */
        int newColorStride = ySize;
        int newYStride = newColorStride * colorSize;

        for (y = (ySize - 1); y >= 0; y--)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(((ySize - 1) - y) * xStride) + (((xSize - 1) - x) * newYStride)] = inData[(y * yStride) + (x * xStride)];
            outData[(((ySize - 1) - y) * xStride) + (((xSize - 1) - x) * newYStride) + newColorStride] = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(((ySize - 1) - y) * xStride) + (((xSize - 1) - x) * newYStride) + (2 * newColorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }
      }

      if (colorMode == NDColorModeRGB1)
      {
        int source_offset;
        int target_offset;

        /** Calculate a new value for the Y stride. */
        int newStride = colorSize * ySize;

        for (y = (ySize - 1); y >= 0; y--)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            source_offset = (((xSize - 1) - x) * xStride) + (((ySize - 1) - y) * yStride);
            target_offset = (y * xStride) + (x * (newStride));
            outData[target_offset + 0] = inData[source_offset + 0];;
            outData[target_offset + 1] = inData[source_offset + 1];;
            outData[target_offset + 2] = inData[source_offset + 2];;
          }
        }
      }

      break;

    case (TransformMirror):

      if (colorMode == NDColorModeMono)
      {
        for (y = 0; y < ySize; y++)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            outData[(((xSize - 1) - x) * xStride) + (y * yStride)] = inData[(y * yStride) + (x * xStride)];
          }
        }
      }
      else
      {
        for (y = 0; y < ySize; y++)
        {
          for (x = (xSize - 1); x >= 0; x--)
          {
            /** Copy three values for each iteration of the inner loop.  This moves the red, green, and blue information. */
            outData[(((xSize - 1) - x) * xStride) + (y * yStride)] = inData[(y * yStride) + (x * xStride)];
            outData[(((xSize - 1) - x) * xStride) + (y * yStride) + colorStride] = inData[(y * yStride) + (x * xStride) + colorStride];
            outData[(((xSize - 1) - x) * xStride) + (y * yStride) + (2 * colorStride)] = inData[(y * yStride) + (x * xStride) + (2 * colorStride)];
          }
        }
      }
      break;

    case (TransformRotate180Mirror):

      int source_offset;
      int target_offset;

      /** In this mode, we are moving an entire row at once.  First all of the red rows are moved.
        Next, all of the green rows are moved.  Lastly, all of the blue rows are moved.  In mono
        mode, the outer loop only executes once.*/
      if (colorMode == NDColorModeMono || colorMode == NDColorModeRGB3)
      {
        for (color = 0; color < colorSize; color++)
        {          
          for (y = 0; y < ySize; y++)
          {
            source_offset = (y * yStride) + (color * colorStride);
            target_offset = (((ySize-1) - y) * yStride) + (color * colorStride);

            memcpy(outData + target_offset, inData + source_offset, (yStride * elementSize));
          }

        }
      }

      /** In this mode, we are moving three rows at a time. All the red, green, and blue
        information for each for are moved at once. */
      if (colorMode == NDColorModeRGB2)
      {
        for (y = 0; y < ySize; y++)
        {
          source_offset = (y * yStride);
          target_offset = (((ySize-1) - y) * yStride);

          memcpy(outData + target_offset, inData + source_offset, (yStride * elementSize));
        }
      }

      if (colorMode == NDColorModeRGB1)
      {
        for (y = 0; y < ySize; y++)
        {
          for (x = 0; x < xSize; x++)
          {
            source_offset = (x * xStride) + (y * yStride);
            target_offset = (x * xStride) + (((ySize-1) - y) * yStride);
            outData[target_offset + 0] = inData[source_offset + 0];;
            outData[target_offset + 1] = inData[source_offset + 1];;
            outData[target_offset + 2] = inData[source_offset + 2];;
          }
        }
      }

      break;

    default:
      break;
  }

  return;
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Grabs the current NDArray and applies the selected transforms to the data.  Apply the transforms in order.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginTransform::processCallbacks(NDArray *pArray){
  NDArray *transformedArray;
  NDArrayInfo_t arrayInfo;
  static const char* functionName = "processCallbacks";

  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);

  /** Create a pointer to a structure of type NDArrayInfo_t and use it to get information about
    the input array.
  */
  pArray->getInfo(&arrayInfo);

  this->userDims_[0] = arrayInfo.xDim;
  this->userDims_[1] = arrayInfo.yDim;
  this->userDims_[2] = arrayInfo.colorDim;

  /* Previous version of the array was held in memory.  Release it and reserve a new one. */
  if (this->pArrays[0]) {
    this->pArrays[0]->release();
    this->pArrays[0] = NULL;
  }

  /* Release the lock; this is computationally intensive and does not access any shared data */
  this->unlock();
  /* Copy the information from the current array */
  this->pArrays[0] = this->pNDArrayPool->copy(pArray, NULL, 1);
  transformedArray = this->pArrays[0];

  if ( pArray->ndims <=3 )
    this->transformImage(pArray, transformedArray, &arrayInfo);
  else {
    asynPrint( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s, this method is meant to transform 2Dimages when the number of dimensions is <= 3\n",
          pluginName, functionName);
  }
  this->lock();

  this->getAttributes(transformedArray->pAttributeList);
  doCallbacksGenericPointer(transformedArray, NDArrayData,0);
  callParamCallbacks();
}


/** Transform the image according to the selected choice.*/  
void NDPluginTransform::transformImage(NDArray *inArray, NDArray *outArray, NDArrayInfo_t *arrayInfo)
{
  //static const char *functionName = "transformNDArray";
  int colorMode;
  int transformType;

  colorMode = NDColorModeMono;
  getIntegerParam(NDColorMode, &colorMode);
  getIntegerParam(NDPluginTransformType_, &transformType);

  switch (inArray->dataType) {
    case NDInt8:
      transformNDArray<epicsInt8>(inArray, outArray, transformType, colorMode, arrayInfo);
      break;
    case NDUInt8:
      transformNDArray<epicsUInt8>(inArray, outArray, transformType, colorMode, arrayInfo);
      break;
    case NDInt16:
      transformNDArray<epicsInt16>(inArray, outArray, transformType, colorMode, arrayInfo);
      break;
    case NDUInt16:
      transformNDArray<epicsUInt16>(inArray, outArray, transformType, colorMode, arrayInfo);
      break;
    case NDInt32:
      transformNDArray<epicsInt32>(inArray, outArray, transformType, colorMode, arrayInfo);
      break;
    case NDUInt32:
      transformNDArray<epicsUInt32>(inArray, outArray, transformType, colorMode, arrayInfo);
      break;
    case NDFloat32:
      transformNDArray<epicsFloat32>(inArray, outArray, transformType, colorMode, arrayInfo);
      break;
    case NDFloat64:
      transformNDArray<epicsFloat64>(inArray, outArray, transformType, colorMode, arrayInfo);
      break;
  }

  return;
}


/** Constructor for NDPluginTransform; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * Transform parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *      NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *      at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *      0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *      of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *      allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *      allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginTransform::NDPluginTransform(const char *portName, int queueSize, int blockingCallbacks,
             const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
             int priority, int stackSize)
  /* Invoke the base class constructor */
  : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_TRANSFORM_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
  //static const char *functionName = "NDPluginTransform";
  int i;

  createParam(NDPluginTransformTypeString, asynParamInt32, &NDPluginTransformType_);

  for (i = 0; i < ND_ARRAY_MAX_DIMS; i++) {
    this->userDims_[i] = i;
  }
  
  /* Set the plugin type string */
  setStringParam(NDPluginDriverPluginType, "NDPluginTransform");
  setIntegerParam(NDPluginTransformType_, TransformNone);

  // Enable ArrayCallbacks.  
  // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
  setIntegerParam(NDArrayCallbacks, 1);

  /* Try to connect to the array port */
  connectToArrayPort();
}

/** Configuration command */
extern "C" int NDTransformConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                    const char *NDArrayPort, int NDArrayAddr,
                                    int maxBuffers, size_t maxMemory,
                                    int priority, int stackSize)
{
  new NDPluginTransform(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
              maxBuffers, maxMemory, priority, stackSize);
  return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDTransformConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDTransformConfigure(args[0].sval, args[1].ival, args[2].ival,
                       args[3].sval, args[4].ival, args[5].ival,
                       args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDTransformRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDTransformRegister);
}
