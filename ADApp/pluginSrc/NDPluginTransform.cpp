/*
 * NDPluginTransform.cpp
 *
 * Transform plugin
 * Author: John Hammonds, Chris Roehrig
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

#include "NDArray.h"
#include "NDPluginTransform.h"
#include <epicsExport.h>
	
	/** Perform the move of the pixels to the new orientation. */
	template <typename epicsType>
	void transformNDArray(NDArray *inArray, NDArray *outArray, int transformType, int colorMode, NDArrayInfo_t *arrayInfo)
	{
		epicsType *inData = (epicsType *)inArray->pData;
		epicsType *outData = (epicsType *)outArray->pData;
		int x, y, color;
		int xSize, ySize, colorSize;
		size_t xStride, yStride, colorStride;
		int elementSize;
				
		xSize = arrayInfo->xSize;
		ySize = arrayInfo->ySize;
		if (arrayInfo->colorSize > 0)
			colorSize = arrayInfo->colorSize;
		else
			colorSize = 1;
		
		xStride = arrayInfo->xStride;
		yStride = arrayInfo->yStride;
		colorStride = arrayInfo->colorStride;
		elementSize = arrayInfo->bytesPerElement;
		
		switch (transformType)
		{
			case (TransformNone):
			
				outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->xDim].size;
				outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->yDim].size;
				outArray->dims[arrayInfo->colorDim].size = inArray->dims[arrayInfo->colorDim].size;
				
				memcpy(outData, inData, arrayInfo->totalBytes);
				break;
				
			case (TransformRotateCW90):
			
				outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->yDim].size;
				outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->xDim].size;
				outArray->dims[arrayInfo->colorDim].size = inArray->dims[arrayInfo->colorDim].size;

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
				
				/** For this mode, since the red, green, and blue information is contiguous in memory,
				    we can use memcpy to move them all at once for each pixel
				*/
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
							memcpy(outData + target_offset, inData + source_offset, (xStride * elementSize));
						}
					}
				}
				
				break;
			
			case (TransformRotate180):
			
				outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->xDim].size;
				outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->yDim].size;
				outArray->dims[arrayInfo->colorDim].size = inArray->dims[arrayInfo->colorDim].size;
				
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
				
			case (TransformRotateCCW90):
			
				outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->yDim].size;
				outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->xDim].size;
				outArray->dims[arrayInfo->colorDim].size = inArray->dims[arrayInfo->colorDim].size;
				
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
							outData[(y * xStride) + (((xSize - 1) - x) * ySize)] = inData[(y * yStride) + (x * xStride)];
							outData[(y * xStride) + (((xSize - 1) - x) * ySize) + colorStride] = inData[(y * yStride) + (x * xStride) + colorStride];
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
							color = 0;
							outData[(y * xStride) + (((xSize - 1) - x) * newYStride) + (color * newColorStride)] = inData[(y * yStride) + (x * xStride) + (color * colorStride)];
							color = 1;
							outData[(y * xStride) + (((xSize - 1) - x) * newYStride) + (color * newColorStride)] = inData[(y * yStride) + (x * xStride) + (color * colorStride)];
							color = 2;
							outData[(y * xStride) + (((xSize - 1) - x) * newYStride) + (color * newColorStride)] = inData[(y * yStride) + (x * xStride) + (color * colorStride)];
							
							/** These lines don't work.  The red is moved correctly, but not the green or blue.  I don't understand
							    how the above works and not this.
							*/
							//outData[(y * xStride) + (((xSize - 1) - x) * newYStride)] = inData[(y * yStride) + (x * xStride)];
							//outData[(y * xStride) + (((xSize - 1) - x) * newYStride) + newColorStride] = inData[(y * yStride) + (x * xStride) + newColorStride];
							//outData[(y * xStride) + (((xSize - 1) - x) * newYStride) + (2 * newColorStride)] = inData[(y * yStride) + (x * xStride) + (2 * newColorStride)];
						}
					}
				}
				
				/** For this mode, since the red, green, and blue information is contiguous in memory,
				    we can use memcpy to move them all at once for each pixel
				*/
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
							memcpy(outData + target_offset, inData + source_offset, (xStride * elementSize));
						}
					}
				}
				
				break;
				
			case (TransformTranspose):
			
				outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->yDim].size;
				outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->xDim].size;
				outArray->dims[arrayInfo->colorDim].size = inArray->dims[arrayInfo->colorDim].size;
			
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
				
				/** For this mode, since the red, green, and blue information is contiguous in memory,
				    we can use memcpy to move them all at once for each pixel
				*/
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
							memcpy(outData + target_offset, inData + source_offset, (xStride * elementSize));
						}
					}
				}
			
				break;
				
			case (TransformTransposeRotate180):
			
				outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->yDim].size;
				outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->xDim].size;
				outArray->dims[arrayInfo->colorDim].size = inArray->dims[arrayInfo->colorDim].size;
				
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
				
				/** For this mode, since the red, green, and blue information is contiguous in memory,
				    we can use memcpy to move them all at once for each pixel
				*/
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
							memcpy(outData + target_offset, inData + source_offset, (xStride * elementSize));
						}
					}
				}
				
				break;
				
			case (TransformFlipHoriz):
			
				outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->xDim].size;
				outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->yDim].size;
				outArray->dims[arrayInfo->colorDim].size = inArray->dims[arrayInfo->colorDim].size;
			
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
				
			case (TransformFlipVert):
			
				int source_offset;
				int target_offset;
				
				outArray->dims[arrayInfo->xDim].size = inArray->dims[arrayInfo->xDim].size;
				outArray->dims[arrayInfo->yDim].size = inArray->dims[arrayInfo->yDim].size;
				outArray->dims[arrayInfo->colorDim].size = inArray->dims[arrayInfo->colorDim].size;
				
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
				
				/** For this mode, since the red, green, and blue information is contiguous in memory,
				    we can use memcpy to move them all at once for each pixel
				*/
				if (colorMode == NDColorModeRGB1)
				{
					for (y = 0; y < ySize; y++)
					{
						for (x = 0; x < xSize; x++)
						{
							source_offset = (x * xStride) + (y * yStride);
							target_offset = (x * xStride) + (((ySize-1) - y) * yStride);
							memcpy(outData + target_offset, inData + source_offset, (xStride * elementSize));
						}
					}
				}
				
				break;
				
			default:
				break;
		}
				
		free(arrayInfo);
		arrayInfo = NULL;
		
		return;
	}

    /** Callback function that is called by the NDArray driver with new NDArray data.
      * Grabs the current NDArray and applies the selected transforms to the data.  Apply the transforms in order.
      * \param[in] pArray  The NDArray from the callback.
      */
    void NDPluginTransform::processCallbacks(NDArray *pArray){
        NDDimension_t dimsIn[ND_ARRAY_MAX_DIMS];
        NDArray *transformedArray;
		NDArrayInfo_t *arrayInfo;
        int ii;
        const char* functionName = "processCallbacks";

        /* Call the base class method */
        NDPluginDriver::processCallbacks(pArray);
		
		/** Create a pointer to a structure of type NDArrayInfo_t and use it to get information about
		    the input array.
		*/
		arrayInfo = (NDArrayInfo_t *) callocMustSucceed (1, sizeof(NDArrayInfo_t), functionName);
		pArray->getInfo(arrayInfo);
		
		this->userDims[0] = arrayInfo->xDim;
        this->userDims[1] = arrayInfo->yDim;
        this->userDims[2] = arrayInfo->colorDim;

        for (ii = 0; ii < ND_ARRAY_MAX_DIMS; ii++) {
            dimsIn[userDims[ii]].size = pArray->dims[ii].size;
            if ( ii < 3 ) {
                setIntegerParam(NDPluginTransformArraySize0 + ii, (int)dimsIn[userDims[ii]].size);
            }
        }
		
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
			this->transformImage(pArray, transformedArray, arrayInfo);
        else if (pArray->ndims > 3) {
            asynPrint( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s, this method is meant to transform 2Dimages when the number of dimensions is <= 3\n",
                        pluginName, functionName);
        }
        else {
            asynPrint( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s, this method is meant to transform 2d images.  Need ndims >=2\n",
                        pluginName, functionName);
        }
        this->lock();

        doCallbacksGenericPointer(transformedArray, NDArrayData,0);
        callParamCallbacks();
    }


    /** Called when asyn clients call pasynInt32->write().
      * This function performs actions for some parameters, including transform type and origin.
      * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter.
      * For all parameters it sets the value in the parameter library and calls any registered callbacks..
      * \param[in] pasynUser pasynUser structure that encodes the reason and address.
      * \param[in] value Value to write. */

    asynStatus NDPluginTransform::writeInt32(asynUser *pasynUser, epicsInt32 value){
        int function = pasynUser->reason;
        asynStatus status = asynSuccess;

		if (function == NDPluginTransformType) {
			this->transformStruct.type = value;
        } else {
            /* This was not a parameter that this driver understands, try the base class */
            if (function < FIRST_TRANSFORM_PARAM) status = NDPluginDriver::writeInt32(pasynUser, value);
        }

        callParamCallbacks();
        return status;
    }

/** Transform the image according to the selected choice.*/	
	void NDPluginTransform::transformImage(NDArray *inArray, NDArray *outArray, NDArrayInfo_t *arrayInfo)
	{
		const char *functionName = "transformNDArray";
		int colorMode;
		
		colorMode = NDColorModeMono;
		getIntegerParam(NDColorMode, &colorMode);
				
		switch (inArray->dataType) {
            case NDInt8:
				transformNDArray<epicsInt8>(inArray, outArray, this->transformStruct.type, colorMode, arrayInfo);
                break;
            case NDUInt8:
				transformNDArray<epicsUInt8>(inArray, outArray, this->transformStruct.type, colorMode, arrayInfo);
                break;
            case NDInt16:
				transformNDArray<epicsInt16>(inArray, outArray, this->transformStruct.type, colorMode, arrayInfo);
                break;
            case NDUInt16:
				transformNDArray<epicsUInt16>(inArray, outArray, this->transformStruct.type, colorMode, arrayInfo);
                break;
            case NDInt32:
				transformNDArray<epicsInt32>(inArray, outArray, this->transformStruct.type, colorMode, arrayInfo);
                break;
            case NDUInt32:
				transformNDArray<epicsUInt32>(inArray, outArray, this->transformStruct.type, colorMode, arrayInfo);
                break;
            case NDFloat32:
				transformNDArray<epicsFloat32>(inArray, outArray, this->transformStruct.type, colorMode, arrayInfo);
                break;
            case NDFloat64:
				transformNDArray<epicsFloat64>(inArray, outArray, this->transformStruct.type, colorMode, arrayInfo);
                break;
        }
		
		setIntegerParam( (NDPluginTransformDim0MaxSize), (int)outArray->dims[arrayInfo->xDim].size);
        setIntegerParam( (NDPluginTransformDim1MaxSize), (int)outArray->dims[arrayInfo->yDim].size);
        setIntegerParam( (NDPluginTransformDim2MaxSize), (int)outArray->dims[arrayInfo->colorDim].size);
				
		return;
	}


/** Constructor for NDPluginTransform; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * Transform parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
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
    const char *functionName = "NDPluginTransform";
    int ii, jj;

    createParam(NDPluginTransformNameString,         asynParamOctet, &NDPluginTransformName);
	createParam(NDPluginTransformTypeString,        asynParamInt32, &NDPluginTransformType);
	createParam(NDPluginTransformDim0MaxSizeString, asynParamInt32, &NDPluginTransformDim0MaxSize);
    createParam(NDPluginTransformDim1MaxSizeString, asynParamInt32, &NDPluginTransformDim1MaxSize);
    createParam(NDPluginTransformDim2MaxSizeString, asynParamInt32, &NDPluginTransformDim2MaxSize);
    createParam(NDPluginTransformArraySize0String,   asynParamInt32, &NDPluginTransformArraySize0);
    createParam(NDPluginTransformArraySize1String,   asynParamInt32, &NDPluginTransformArraySize1);
    createParam(NDPluginTransformArraySize2String,   asynParamInt32, &NDPluginTransformArraySize2);

	this->transformStruct.type=TransformNone;
    for (jj = 0; jj < ND_ARRAY_MAX_DIMS; jj++) {
        this->transformStruct.dims[jj].size=0;
		this->userDims[jj] = jj;
    }
    
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginTransform");
    setStringParam(NDPluginTransformName, "");
	setIntegerParam(NDPluginTransformType, 0);
	setIntegerParam(NDPluginTransformDim0MaxSize, 0);
    setIntegerParam(NDPluginTransformDim1MaxSize, 0);
    setIntegerParam(NDPluginTransformDim2MaxSize, 0);
    setIntegerParam(NDPluginTransformArraySize0, 0);
    setIntegerParam(NDPluginTransformArraySize1, 0);
    setIntegerParam(NDPluginTransformArraySize2, 0);

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
