/*
 * NDPluginTransform.cpp
 *
 * Transform plugin
 * Author: John Hammonds
 *
 * Created Oct. 28, 2009
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>
#include <epicsExport.h>

#include "NDArray.h"
#include "NDPluginTransform.h"

/** Map parameter enums to strings that will be used to set up EPICS databases
  */
static asynParamString_t NDPluginTransformParamString[] = {
    {NDPluginTransformName, "NAME"},
	{NDPluginTransform1Type, "TYPE1"},
	{NDPluginTransform2Type, "TYPE2"},
	{NDPluginTransform3Type, "TYPE3"},
	{NDPluginTransform4Type, "TYPE4"},
	{NDPluginTransformOrigin, "ORIGIN"},
	{NDPluginTransform1Dim0MaxSize, "T1_DIM0_MAX_SIZE"},
	{NDPluginTransform1Dim1MaxSize, "T1_DIM1_MAX_SIZE"},
	{NDPluginTransform2Dim0MaxSize, "T2_DIM0_MAX_SIZE"},
	{NDPluginTransform2Dim1MaxSize, "T2_DIM1_MAX_SIZE"},
	{NDPluginTransform3Dim0MaxSize, "T3_DIM0_MAX_SIZE"},
	{NDPluginTransform3Dim1MaxSize, "T3_DIM1_MAX_SIZE"},
	{NDPluginTransform4Dim0MaxSize, "T4_DIM0_MAX_SIZE"},
	{NDPluginTransform4Dim1MaxSize, "T4_DIM1_MAX_SIZE"},
	{NDPluginTransformArraySize0, "ARRAY_SIZE_0"},
	{NDPluginTransformArraySize1, "ARRAY_SIZE_1"},

};


	/** Actually perform the move of the pixel from the input NDArray to it's transformed location in the output NDArray.
	* \param[in] The NDArray to be transformed
	* \param[in] A structure holding the indices of the pixel to be moved
	* \param[in] The NDArray that will hold the transformed data
	* \param[in] A structure to hold the indices of the transformed data (where to place the input pixel's data)
	*/
	template <typename epicsType>
	void movePixel(NDArray *inArray, NDTransformIndex_t pixelIndexIn,
									  NDArray *outArray, NDTransformIndex_t pixelIndexOut){
		epicsType *inData = (epicsType *)inArray->pData;
		epicsType *outData = (epicsType *)outArray->pData;


		outData[pixelIndexOut.index1 * outArray->dims[0].size + pixelIndexOut.index0 ] =
			inData[pixelIndexIn.index1 * inArray->dims[0].size + pixelIndexIn.index0];
	}

	/** Callback function that is called by the NDArray driver with new NDArray data.
	  * Grabs the current NDArray and applies the selected transforms to the data.  Apply the transforms in order.
	  * \param[in] pArray  The NDArray from the callback.
	  */
    void NDPluginTransform::processCallbacks(NDArray *pArray){
    	NDDimension_t dimsIn[ND_ARRAY_MAX_DIMS];
    	NDArray *transformedArray;
		int ii;
	    const char* functionName = "processCallbacks";

		/* Call the base class method */
		NDPluginDriver::processCallbacks(pArray);

		for (ii = 0; ii < ND_ARRAY_MAX_DIMS; ii++) {
			dimsIn[ii].size = pArray->dims[ii].size;
			if ( ii < 2 ) {
				setIntegerParam(NDPluginTransformArraySize0 + ii, dimsIn[ii].size);
			}
		}
		/* set max size params based on what the transform does to the image */
		this->setMaxSizes(0);

		transformedArray = this->pArrays[0];
		/* Previous version of the array was held in memory.  Release it and reserve a new one. */
		if (this->pArrays[0]) {
			this->pArrays[0]->release();
			this->pArrays[0] = NULL;
		}

		/* Copy the information from the current array */
			this->lock();
		this->pArrays[0] = this->pNDArrayPool->copy(pArray, NULL, 1);
		transformedArray = this->pArrays[0];
		if ( pArray->ndims == 2 ) {
			this->transformArray(pArray, transformedArray);
		}
		else if (pArray->ndims > 2) {

		}
		else {
			asynPrint( this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s, this method is meant to transform 2d images.  Need ndims >=2\n",
						pluginName, functionName);
		}
			this->unlock();

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
		int transformIndex;

		switch (function){
			case NDPluginTransform1Type:
			case NDPluginTransform2Type:
			case NDPluginTransform3Type:
			case NDPluginTransform4Type:
				transformIndex = function - NDPluginTransform1Type;
				this->pTransforms[transformIndex].type = value;
				setMaxSizes(transformIndex);
				break;
			case NDPluginTransformOrigin:
				this->originLocation = value;
				setIntegerParam(NDPluginTransformOrigin, value);
				break;
			default:
				/* This was not a parameter that this driver understands, try the base class */
				status = NDPluginDriver::writeInt32(pasynUser, value);
				break;
		}

		callParamCallbacks();
    	return status;
	}


	asynStatus NDPluginTransform::drvUserCreate(asynUser *pasynUser, const char *drvInfo,
								 const char **pptypeName, size_t *psize){
		asynStatus status;
		//const char *functionName = "drvUserCreate";

		status = this->drvUserCreateParam(pasynUser, drvInfo, pptypeName, psize,
										  NDPluginTransformParamString, NUM_Transform_PARAMS);

		/* If not, then call the base class method, see if it is known there */
		if (status) status = NDPluginDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
		return(status);
	}


	/** Look at the transform type.  Reports if transform interchanges size of x and y
	  * \param[in] The type of transform.
	  */
	int NDPluginTransform::transformFlipsAxes(int transformType) {
		if ((transformType == TransformRotateCW90) ||
	        (transformType == TransformRotateCCW90) ||
	        (transformType == TransformFlip0011) ||
	        (transformType == TransformFlip0110) ) {
			return true;
			 }
		return false;

	}


	/** set the maximum size for the allowed transform.  Checks to see if the transform interchanges axes.
	  * \param[in] index into array of transforms.
	  */
	void NDPluginTransform::setMaxSizes(int startingPoint) {
		int ii;
		int dim0MaxSize, dim1MaxSize;
		for (ii=startingPoint; ii< this->maxTransforms; ii++) {
			if ( ii == 0 ) {
				getIntegerParam( NDPluginTransformArraySize0, &dim0MaxSize);
				getIntegerParam( NDPluginTransformArraySize1, &dim1MaxSize);
				if ( transformFlipsAxes(this->pTransforms[ii].type )) {
					this->pTransforms[ii].dims[0].size = dim1MaxSize;
					this->pTransforms[ii].dims[1].size = dim0MaxSize;
				}
				else {
					this->pTransforms[ii].dims[0].size = dim0MaxSize;
					this->pTransforms[ii].dims[1].size = dim1MaxSize;
				}

			}
			else {
				getIntegerParam( (NDPluginTransform1Dim0MaxSize + 2*(ii-1)), &dim0MaxSize);
				getIntegerParam( (NDPluginTransform1Dim1MaxSize + 2*(ii-1)), &dim1MaxSize);
				if ( transformFlipsAxes(this->pTransforms[ii].type) ) {
					this->pTransforms[ii].dims[0].size = dim1MaxSize;
					this->pTransforms[ii].dims[1].size = dim0MaxSize;
				}
				else {
					this->pTransforms[ii].dims[0].size = dim0MaxSize;
					this->pTransforms[ii].dims[1].size = dim1MaxSize;
				}
			}
			setIntegerParam( (NDPluginTransform1Dim0MaxSize + 2*(ii)), this->pTransforms[ii].dims[0].size);
			setIntegerParam( (NDPluginTransform1Dim1MaxSize + 2*(ii)), this->pTransforms[ii].dims[1].size);

		}
		callParamCallbacks();
	}

	/** This transform basically does nothing.  It just returns the input indices.  A list of transforms is maintained.  If an
	  * entry in the list is not needed, then the transform is set to none and this routine is run.
	  * \param[in] indexIn A structure to hold the pixel location to be transformed.
	  * \param[in] originLocation Indicates the physical location of 0,0.
	  * \param[in] transformNumber Index into the list of transforms to be performed.
	  * \return the transformed pixel location
	*/
	NDTransformIndex_t NDPluginTransform::transformNone(NDTransformIndex_t indexIn, int originLocation, int transformNumber){
		NDTransformIndex_t indexOut;
		indexOut.index0 = indexIn.index0;
		indexOut.index1 = indexIn.index1;
		return indexOut;
	}

	/** This transform rotates pixels clockwise 90 degrees
	  * \param[in] indexIn A structure to hold the pixel location to be transformed.
	  * \param[in] originLocation Indicates the physical location of 0,0.
	  * \param[in] transformNumber Index into the list of transforms to be performed.
	  * \return the transformed pixel location
	*/
	NDTransformIndex_t NDPluginTransform::transformRotateCW90(NDTransformIndex_t indexIn, int originLocation, int transformNumber){
		NDTransformIndex_t indexOut;
		if ( (originLocation == 0 ) || (this->originLocation == 3)){
			indexOut.index0 = indexIn.index1;
			indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index0;
		}
		else if ( (originLocation == 1 ) || (this->originLocation == 2)){
			indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size -1) - indexIn.index1;
			indexOut.index1 = indexIn.index0;
		}
		return indexOut;
	}

	/** This transform rotates the pixels counter clockwise 90 degrees
	  * \param[in] indexIn A structure to hold the pixel location to be transformed.
	  * \param[in] originLocation Indicates the physical location of 0,0.
	  * \param[in] transformNumber Index into the list of transforms to be performed.
	  * \return the transformed pixel location
	*/
	NDTransformIndex_t NDPluginTransform::transformRotateCCW90(NDTransformIndex_t indexIn, int originLocation, int transformNumber){
		NDTransformIndex_t indexOut;
		if ( (originLocation == 0 ) || (this->originLocation == 3)){
			indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size - 1) - indexIn.index1;
			indexOut.index1 = indexIn.index0;
		}
		else if ( (originLocation == 1 ) || (this->originLocation == 2)){
			indexOut.index0 = indexIn.index1;
			indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index0;
		}

		return indexOut;
	}

	/** This transform rotates the pixels 180 degrees
	  * \param[in] indexIn A structure to hold the pixel location to be transformed.
	  * \param[in] originLocation Indicates the physical location of 0,0.
	  * \param[in] transformNumber Index into the list of transforms to be performed.
	  * \return the transformed pixel location
	*/
	NDTransformIndex_t NDPluginTransform::transformRotate180(NDTransformIndex_t indexIn, int originLocation, int transformNumber){
		NDTransformIndex_t indexOut;
		indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size - 1) - indexIn.index0;
		indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index1;

		return indexOut;
	}

	/** This transform flips pixels about the axis [0,0] [maxPixelsX-1, maxPixelsY-1]
	  * \param[in] indexIn A structure to hold the pixel location to be transformed.
	  * \param[in] originLocation Indicates the physical location of 0,0.
	  * \param[in] transformNumber Index into the list of transforms to be performed.
	  * \return the transformed pixel location
	*/
	NDTransformIndex_t NDPluginTransform::transformFlip0011(NDTransformIndex_t indexIn, int originLocation, int transformNumber){
		NDTransformIndex_t indexOut;
		indexOut.index0 = indexIn.index1;
		indexOut.index1 = indexIn.index0;

		return indexOut;
	}

	/** This transform flips pixels about the axis [0,maxPixelsY-1] [maxPixelsX-1,0]
	  * \param[in] indexIn A structure to hold the pixel location to be transformed.
	  * \param[in] originLocation Indicates the physical location of 0,0.
	  * \param[in] transformNumber Index into the list of transforms to be performed.
	  * \return the transformed pixel location
	*/
	NDTransformIndex_t NDPluginTransform::transformFlip0110(NDTransformIndex_t indexIn, int originLocation, int transformNumber){
		NDTransformIndex_t indexOut;
		indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size - 1) - indexIn.index1;
		indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index0;

		return indexOut;
	}

	/** This transform flips the pixels about a line running vertically throught the center of the image.
	  * \param[in] indexIn A structure to hold the pixel location to be transformed.
	  * \param[in] originLocation Indicates the physical location of 0,0.
	  * \param[in] transformNumber Index into the list of transforms to be performed.
	  * \return the transformed pixel location
	*/
	NDTransformIndex_t NDPluginTransform::transformFlipX(NDTransformIndex_t indexIn, int originLocation, int transformNumber){
		NDTransformIndex_t indexOut;
		indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size - 1) - indexIn.index0;
		indexOut.index1 = indexIn.index1;

		return indexOut;
	}

	/** This transform flips pixels about a horizontal line running through the center of the image.
	  * \param[in] indexIn A structure to hold the pixel location to be transformed.
	  * \param[in] originLocation Indicates the physical location of 0,0.
	  * \param[in] transformNumber Index into the list of transforms to be performed.
	  * \return the transformed pixel location
	*/
	NDTransformIndex_t NDPluginTransform::transformFlipY(NDTransformIndex_t indexIn, int originLocation, int transformNumber){
		NDTransformIndex_t indexOut;
		indexOut.index0 = indexIn.index0;
		indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index1;

		return indexOut;
	}


/** loop over the axes of the image to transform the axes
  * \param[in] inArray the input NDArray
  * \param[in] outArray the transformed Array
  */

void NDPluginTransform::transformArray(NDArray *inArray, NDArray *outArray) {
	int ii, jj;
	int transformIndex;
	int maxInSize0, maxInSize1, maxOutSize0, maxOutSize1;
	NDTransformIndex_t pixelIndexIn, pixelIndexOut, pixelIndexInit;


	maxInSize0 = inArray->dims[0].size;
	maxInSize1 = inArray->dims[1].size;
	maxOutSize0 = this->pTransforms[this->maxTransforms-1].dims[0].size;
	maxOutSize1 = this->pTransforms[this->maxTransforms-1].dims[1].size;
	outArray->dims[0].size = maxOutSize0;
	outArray->dims[1].size = maxOutSize1;


	for (ii = 0; ii<maxInSize1; ii++){
		for (jj = 0; jj<maxInSize0; jj++){
			pixelIndexIn.index0 = jj;
			pixelIndexIn.index1 = ii;
			pixelIndexInit.index0 = jj;
			pixelIndexInit.index1 = ii;
			for ( transformIndex = 0; transformIndex < this->maxTransforms; transformIndex++) {
				switch (this->pTransforms[transformIndex].type){
					case TransformNone:
						pixelIndexOut = transformNone(pixelIndexIn, this->originLocation, transformIndex);
						break;
					case TransformRotateCW90:
						pixelIndexOut = transformRotateCW90(pixelIndexIn, this->originLocation, transformIndex);
						break;
					case TransformRotateCCW90:
						pixelIndexOut = transformRotateCCW90(pixelIndexIn, this->originLocation, transformIndex);
						break;
					case TransformRotate180:
						pixelIndexOut = transformRotate180(pixelIndexIn, this->originLocation, transformIndex);
						break;
					case TransformFlip0011:
						pixelIndexOut = transformFlip0011(pixelIndexIn, this->originLocation, transformIndex);
						break;
					case TransformFlip0110:
						pixelIndexOut = transformFlip0110(pixelIndexIn, this->originLocation, transformIndex);
						break;
					case TransformFlipX:
						pixelIndexOut = transformFlipX(pixelIndexIn, this->originLocation, transformIndex);
						break;
					case TransformFlipY:
						pixelIndexOut = transformFlipY(pixelIndexIn, this->originLocation, transformIndex);
						break;
				}
				pixelIndexIn.index0 = pixelIndexOut.index0;
				pixelIndexIn.index1 = pixelIndexOut.index1;
			}
			switch (inArray->dataType) {
				case NDInt8:
					movePixel<epicsInt8>(inArray, pixelIndexInit, outArray, pixelIndexOut);
					break;
				case NDUInt8:
					movePixel<epicsUInt8>(inArray, pixelIndexInit, outArray, pixelIndexOut);
					break;
				case NDInt16:
					movePixel<epicsInt16>(inArray, pixelIndexInit, outArray, pixelIndexOut);
					break;
				case NDUInt16:
					movePixel<epicsUInt16>(inArray, pixelIndexInit, outArray, pixelIndexOut);
					break;
				case NDInt32:
					movePixel<epicsInt32>(inArray, pixelIndexInit, outArray, pixelIndexOut);
					break;
				case NDUInt32:
					movePixel<epicsUInt32>(inArray, pixelIndexInit, outArray, pixelIndexOut);
					break;
				case NDFloat32:
					movePixel<epicsFloat32>(inArray, pixelIndexInit, outArray, pixelIndexOut);
					break;
				case NDFloat64:
					movePixel<epicsFloat64>(inArray, pixelIndexInit, outArray, pixelIndexOut);
					break;
			}
		}
	}
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
                   NDArrayPort, NDArrayAddr, 1, NDPluginTransformLastTransformParam, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    asynStatus status;
    const char *functionName = "NDPluginTransform";
	int ii, jj;

    this->maxTransforms = 4;
    this->pTransforms = (NDTransform_t *)callocMustSucceed(this->maxTransforms, sizeof(*this->pTransforms), functionName);
	this-> originLocation = 0;
	for (ii = 0; ii < this->maxTransforms; ii++) {
		this->pTransforms[ii].type=0;
		for (jj = 0; jj < ND_ARRAY_MAX_DIMS; jj++) {
			this->pTransforms[ii].dims[jj].size=0;
		}
	}
	for (ii = 0; ii<ND_ARRAY_MAX_DIMS; jj++) {
		this->userDims[ii] = ii;
	}
    this->totalArray = (epicsInt32 *)callocMustSucceed(maxTransforms, sizeof(epicsInt32), functionName);
    this->netArray = (epicsInt32 *)callocMustSucceed(maxTransforms, sizeof(epicsInt32), functionName);
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginTransform");
	setStringParam(NDPluginTransformName, "");
	setIntegerParam(NDPluginTransform1Type, 0);
	setIntegerParam(NDPluginTransform2Type, 0);
	setIntegerParam(NDPluginTransform3Type, 0);
	setIntegerParam(NDPluginTransform4Type, 0);
	setIntegerParam(NDPluginTransformOrigin, 0);
	setIntegerParam(NDPluginTransform1Dim0MaxSize, 0);
	setIntegerParam(NDPluginTransform1Dim1MaxSize, 0);
	setIntegerParam(NDPluginTransform2Dim0MaxSize, 0);
	setIntegerParam(NDPluginTransform2Dim1MaxSize, 0);
	setIntegerParam(NDPluginTransform3Dim0MaxSize, 0);
	setIntegerParam(NDPluginTransform3Dim1MaxSize, 0);
	setIntegerParam(NDPluginTransform4Dim0MaxSize, 0);
	setIntegerParam(NDPluginTransform4Dim1MaxSize, 0);
	setIntegerParam(NDPluginTransformArraySize0, 0);
	setIntegerParam(NDPluginTransformArraySize1, 0);




    /* Try to connect to the array port */
    status = connectToArrayPort();
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
