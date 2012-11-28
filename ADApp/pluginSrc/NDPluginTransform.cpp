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

#include "NDArray.h"
#include "NDPluginTransform.h"
#include <epicsExport.h>


    /** Actually perform the move of the pixel from the input NDArray to it's transformed location in the output NDArray.
    * \param[in] inArray The NDArray to be transformed
    * \param[in] pixelIndexIn A structure holding the indices of the pixel to be moved
    * \param[in] outArray The NDArray that will hold the transformed data
    * \param[in] pixelIndexOut A structure to hold the indices of the transformed data (where to place the input pixel's data)
    */
    template <typename epicsType>
    void movePixelStd(NDArray *inArray, NDTransformIndex_t pixelIndexIn,
                                      NDArray *outArray, NDTransformIndex_t pixelIndexOut){
        epicsType *inData = (epicsType *)inArray->pData;
        epicsType *outData = (epicsType *)outArray->pData;


        outData[outArray->dims[0].size*outArray->dims[1].size * pixelIndexOut.index2 +
                pixelIndexOut.index1 * outArray->dims[0].size + pixelIndexOut.index0] =
            inData[inArray->dims[0].size*inArray->dims[1].size * pixelIndexIn.index2 +
                    pixelIndexIn.index1 * inArray->dims[0].size + pixelIndexIn.index0];
    }

    /** Actually perform the move of the pixel from the input NDArray to it's transformed location in the output NDArray.
    * \param[in] inArray The NDArray to be transformed
    * \param[in] pixelIndexIn A structure holding the indices of the pixel to be moved
    * \param[in] outArray The NDArray that will hold the transformed data
    * \param[in] pixelIndexOut A structure to hold the indices of the transformed data (where to place the input pixel's data)
    */
    template <typename epicsType>
    void movePixelRGB1(NDArray *inArray, NDTransformIndex_t pixelIndexIn,
                                      NDArray *outArray, NDTransformIndex_t pixelIndexOut){
        epicsType *inData = (epicsType *)inArray->pData;
        epicsType *outData = (epicsType *)outArray->pData;

        outData[outArray->dims[1].size*outArray->dims[0].size * pixelIndexOut.index1 +
                pixelIndexOut.index0 * outArray->dims[0].size + pixelIndexOut.index2] =
            inData[inArray->dims[1].size*inArray->dims[0].size * pixelIndexIn.index1 +
                    pixelIndexIn.index0 * inArray->dims[0].size + pixelIndexIn.index2];

    }

    /** Actually perform the move of the pixel from the input NDArray to it's transformed location in the output NDArray.
    * \param[in] inArray The NDArray to be transformed
    * \param[in] pixelIndexIn A structure holding the indices of the pixel to be moved
    * \param[in] outArray The NDArray that will hold the transformed data
    * \param[in] pixelIndexOut A structure to hold the indices of the transformed data (where to place the input pixel's data)
    */
    template <typename epicsType>
    void movePixelRGB2(NDArray *inArray, NDTransformIndex_t pixelIndexIn,
                                      NDArray *outArray, NDTransformIndex_t pixelIndexOut){
        epicsType *inData = (epicsType *)inArray->pData;
        epicsType *outData = (epicsType *)outArray->pData;


        outData[outArray->dims[1].size*outArray->dims[0].size * pixelIndexOut.index1 +
                pixelIndexOut.index2 * outArray->dims[0].size + pixelIndexOut.index0] =
            inData[inArray->dims[1].size*inArray->dims[0].size * pixelIndexIn.index1 +
                    pixelIndexIn.index2 * inArray->dims[0].size + pixelIndexIn.index0];
    }

    /** Callback function that is called by the NDArray driver with new NDArray data.
      * Grabs the current NDArray and applies the selected transforms to the data.  Apply the transforms in order.
      * \param[in] pArray  The NDArray from the callback.
      */
    void NDPluginTransform::processCallbacks(NDArray *pArray){
        NDDimension_t dimsIn[ND_ARRAY_MAX_DIMS];
        NDArray *transformedArray;
        int ii;
        int colorMode;
        const char* functionName = "processCallbacks";

        /* Call the base class method */
        NDPluginDriver::processCallbacks(pArray);

        colorMode = NDColorModeMono;

        /** Need to treat RGB modes diferently */
        getIntegerParam(NDColorMode, &colorMode);

        if ( colorMode == NDColorModeRGB1) {
            this->userDims[0] = 1;
            this->userDims[1] = 2;
            this->userDims[2] = 0;
            this->realDims[0] = 2;
            this->realDims[1] = 0;
            this->realDims[2] = 1;
        }
        else if (colorMode == NDColorModeRGB2) {
            this->userDims[0] = 0;
            this->userDims[1] = 2;
            this->userDims[2] = 1;
            this->realDims[0] = 0;
            this->realDims[1] = 2;
            this->realDims[2] = 1;
        }
        else {
            this->userDims[0] = 0;
            this->userDims[1] = 1;
            this->userDims[2] = 2;
            this->realDims[0] = 0;
            this->realDims[1] = 1;
            this->realDims[2] = 2;
        }

        for (ii = 0; ii < ND_ARRAY_MAX_DIMS; ii++) {
            dimsIn[userDims[ii]].size = pArray->dims[ii].size;
            if ( ii < 3 ) {
                setIntegerParam(NDPluginTransformArraySize0 + ii, (int)dimsIn[userDims[ii]].size);
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

        /* Release the lock; this is computationally intensive and does not access any shared data */
        this->unlock();
        /* Copy the information from the current array */
        this->pArrays[0] = this->pNDArrayPool->copy(pArray, NULL, 1);
        transformedArray = this->pArrays[0];
        if ( pArray->ndims == 2 ) {
            this->transform2DArray(pArray, transformedArray);
        }
        else if (pArray->ndims == 3) {
            this->transform3DArray(pArray, transformedArray);
        }
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
        int transformIndex;

        if ((function == NDPluginTransform1Type) ||
            (function == NDPluginTransform2Type) ||
            (function == NDPluginTransform3Type) ||
            (function == NDPluginTransform4Type)) {
            transformIndex = function - NDPluginTransform1Type;
            this->pTransforms[transformIndex].type = value;
            setMaxSizes(transformIndex);
        } else if (function == NDPluginTransformOrigin) {
            this->originLocation = value;
            setIntegerParam(NDPluginTransformOrigin, value);
        } else {
            /* This was not a parameter that this driver understands, try the base class */
            if (function < FIRST_TRANSFORM_PARAM) status = NDPluginDriver::writeInt32(pasynUser, value);
        }

        callParamCallbacks();
        return status;
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
        int dimMaxSize[3];
        for (ii=startingPoint; ii< this->maxTransforms; ii++) {
            if ( ii == 0 ) {
                getIntegerParam( NDPluginTransformArraySize0, &dimMaxSize[0]);
                getIntegerParam( NDPluginTransformArraySize1, &dimMaxSize[1]);
                getIntegerParam( NDPluginTransformArraySize2, &dimMaxSize[2]);
                if ( transformFlipsAxes(this->pTransforms[ii].type )) {
                    this->pTransforms[ii].dims[0].size = dimMaxSize[this->userDims[1]];
                    this->pTransforms[ii].dims[1].size = dimMaxSize[this->userDims[0]];
                    this->pTransforms[ii].dims[2].size = dimMaxSize[this->userDims[2]];
                }
                else {
                    this->pTransforms[ii].dims[0].size = dimMaxSize[this->userDims[0]];
                    this->pTransforms[ii].dims[1].size = dimMaxSize[this->userDims[1]];
                    this->pTransforms[ii].dims[2].size = dimMaxSize[this->userDims[2]];
                }

            }
            else {
                getIntegerParam( (NDPluginTransform1Dim0MaxSize + 3*(ii-1)), &dimMaxSize[this->userDims[0]]);
                getIntegerParam( (NDPluginTransform1Dim1MaxSize + 3*(ii-1)), &dimMaxSize[this->userDims[1]]);
                getIntegerParam( (NDPluginTransform1Dim2MaxSize + 3*(ii-1)), &dimMaxSize[this->userDims[2]]);
                if ( transformFlipsAxes(this->pTransforms[ii].type) ) {
                    this->pTransforms[ii].dims[0].size = dimMaxSize[this->userDims[1]];
                    this->pTransforms[ii].dims[1].size = dimMaxSize[this->userDims[0]];
                    this->pTransforms[ii].dims[2].size = dimMaxSize[this->userDims[2]];

                }
                else {
                    this->pTransforms[ii].dims[0].size = dimMaxSize[this->userDims[0]];
                    this->pTransforms[ii].dims[1].size = dimMaxSize[this->userDims[1]];
                    this->pTransforms[ii].dims[2].size = dimMaxSize[this->userDims[2]];
                }
            }
            setIntegerParam( (NDPluginTransform1Dim0MaxSize + 3*(ii)), (int)this->pTransforms[ii].dims[0].size);
            setIntegerParam( (NDPluginTransform1Dim1MaxSize + 3*(ii)), (int)this->pTransforms[ii].dims[1].size);
            setIntegerParam( (NDPluginTransform1Dim2MaxSize + 3*(ii)), (int)this->pTransforms[ii].dims[2].size);

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
    NDTransformIndex_t NDPluginTransform::transformNone(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber){
        NDTransformIndex_t indexOut;
        indexOut.index0 = indexIn.index0;
        indexOut.index1 = indexIn.index1;
        indexOut.index2 = indexIn.index2;
        return indexOut;
    }

    /** This transform rotates pixels clockwise 90 degrees
      * \param[in] indexIn A structure to hold the pixel location to be transformed.
      * \param[in] originLocation Indicates the physical location of 0,0.
      * \param[in] transformNumber Index into the list of transforms to be performed.
      * \return the transformed pixel location
    */
    NDTransformIndex_t NDPluginTransform::transformRotateCW90(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber){
        NDTransformIndex_t indexOut;
        if ( (originLocation == 0 ) || (this->originLocation == 3)){
            indexOut.index0 = indexIn.index1;
            indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index0;
            indexOut.index2 = indexIn.index2;
        }
        else if ( (originLocation == 1 ) || (this->originLocation == 2)){
            indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size -1) - indexIn.index1;
            indexOut.index1 = indexIn.index0;
            indexOut.index2 = indexIn.index2;
        }
        return indexOut;
    }

    /** This transform rotates the pixels counter clockwise 90 degrees
      * \param[in] indexIn A structure to hold the pixel location to be transformed.
      * \param[in] originLocation Indicates the physical location of 0,0.
      * \param[in] transformNumber Index into the list of transforms to be performed.
      * \return the transformed pixel location
    */
    NDTransformIndex_t NDPluginTransform::transformRotateCCW90(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber){
        NDTransformIndex_t indexOut;
        if ( (originLocation == 0 ) || (this->originLocation == 3)){
            indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size - 1) - indexIn.index1;
            indexOut.index1 = indexIn.index0;
            indexOut.index2 = indexIn.index2;
        }
        else if ( (originLocation == 1 ) || (this->originLocation == 2)){
            indexOut.index0 = indexIn.index1;
            indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index0;
            indexOut.index2 = indexIn.index2;
        }

        return indexOut;
    }

    /** This transform rotates the pixels 180 degrees
      * \param[in] indexIn A structure to hold the pixel location to be transformed.
      * \param[in] originLocation Indicates the physical location of 0,0.
      * \param[in] transformNumber Index into the list of transforms to be performed.
      * \return the transformed pixel location
    */
    NDTransformIndex_t NDPluginTransform::transformRotate180(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber){
        NDTransformIndex_t indexOut;
        indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size - 1) - indexIn.index0;
        indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index1;
        indexOut.index2 = indexIn.index2;

        return indexOut;
    }

    /** This transform flips pixels about the axis [0,0] [maxPixelsX-1, maxPixelsY-1]
      * \param[in] indexIn A structure to hold the pixel location to be transformed.
      * \param[in] originLocation Indicates the physical location of 0,0.
      * \param[in] transformNumber Index into the list of transforms to be performed.
      * \return the transformed pixel location
    */
    NDTransformIndex_t NDPluginTransform::transformFlip0011(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber){
        NDTransformIndex_t indexOut;
        indexOut.index0 = indexIn.index1;
        indexOut.index1 = indexIn.index0;
        indexOut.index2 = indexIn.index2;

        return indexOut;
    }

    /** This transform flips pixels about the axis [0,maxPixelsY-1] [maxPixelsX-1,0]
      * \param[in] indexIn A structure to hold the pixel location to be transformed.
      * \param[in] originLocation Indicates the physical location of 0,0.
      * \param[in] transformNumber Index into the list of transforms to be performed.
      * \return the transformed pixel location
    */
    NDTransformIndex_t NDPluginTransform::transformFlip0110(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber){
        NDTransformIndex_t indexOut;
        indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size - 1) - indexIn.index1;
        indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index0;
        indexOut.index2 = indexIn.index2;

        return indexOut;
    }

    /** This transform flips the pixels about a line running vertically throught the center of the image.
      * \param[in] indexIn A structure to hold the pixel location to be transformed.
      * \param[in] originLocation Indicates the physical location of 0,0.
      * \param[in] transformNumber Index into the list of transforms to be performed.
      * \return the transformed pixel location
    */
    NDTransformIndex_t NDPluginTransform::transformFlipX(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber){
        NDTransformIndex_t indexOut;
        indexOut.index0 = (this->pTransforms[transformNumber].dims[0].size - 1) - indexIn.index0;
        indexOut.index1 = indexIn.index1;
        indexOut.index2 = indexIn.index2;

        return indexOut;
    }

    /** This transform flips pixels about a horizontal line running through the center of the image.
      * \param[in] indexIn A structure to hold the pixel location to be transformed.
      * \param[in] originLocation Indicates the physical location of 0,0.
      * \param[in] transformNumber Index into the list of transforms to be performed.
      * \return the transformed pixel location
    */
    NDTransformIndex_t NDPluginTransform::transformFlipY(NDTransformIndex_t indexIn, size_t originLocation, int transformNumber){
        NDTransformIndex_t indexOut;
        indexOut.index0 = indexIn.index0;
        indexOut.index1 = (this->pTransforms[transformNumber].dims[1].size - 1) - indexIn.index1;
        indexOut.index2 = indexIn.index2;

        return indexOut;
    }


    /** Loop over available transforms to transform the pixel
      * \param[in] indexIn A structure to hold the pixel location to be transformed.
      * \param[in] originLocation Indicates the physical location of 0,0.
      * \return the transformed pixel location
    */
    NDTransformIndex_t NDPluginTransform::transformPixel(NDTransformIndex_t indexIn, size_t originLocation){
        int transformIndex;
        NDTransformIndex_t pixelIndexIn, pixelIndexOut;
        pixelIndexIn.index0 = indexIn.index0;
        pixelIndexIn.index1 = indexIn.index1;
        pixelIndexIn.index2 = indexIn.index2;

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
            pixelIndexIn.index2 = pixelIndexOut.index2;
        }
        return pixelIndexOut;
    }


/** Find move the pixels */
    void NDPluginTransform::moveRGB1Pixel(NDArray *inArray, NDTransformIndex_t pixelIndexIn,
                                      NDArray *outArray, NDTransformIndex_t pixelIndexOut){
        switch (inArray->dataType) {
            case NDInt8:
                movePixelRGB1<epicsInt8>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDUInt8:
                movePixelRGB1<epicsUInt8>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDInt16:
                movePixelRGB1<epicsInt16>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDUInt16:
                movePixelRGB1<epicsUInt16>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDInt32:
                movePixelRGB1<epicsInt32>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDUInt32:
                movePixelRGB1<epicsUInt32>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDFloat32:
                movePixelRGB1<epicsFloat32>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDFloat64:
                movePixelRGB1<epicsFloat64>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
        }
}
/** Find move the pixels */
    void NDPluginTransform::moveRGB2Pixel(NDArray *inArray, NDTransformIndex_t pixelIndexIn,
                                      NDArray *outArray , NDTransformIndex_t pixelIndexOut){
        switch (inArray->dataType) {
            case NDInt8:
                movePixelRGB2<epicsInt8>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDUInt8:
                movePixelRGB2<epicsUInt8>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDInt16:
                movePixelRGB2<epicsInt16>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDUInt16:
                movePixelRGB2<epicsUInt16>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDInt32:
                movePixelRGB2<epicsInt32>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDUInt32:
                movePixelRGB2<epicsUInt32>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDFloat32:
                movePixelRGB2<epicsFloat32>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDFloat64:
                movePixelRGB2<epicsFloat64>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
        }
}
/** Find move the pixels */
    void NDPluginTransform::moveStdPixel(NDArray *inArray, NDTransformIndex_t pixelIndexIn,
                                      NDArray *outArray, NDTransformIndex_t pixelIndexOut){
        switch (inArray->dataType) {
            case NDInt8:
                movePixelStd<epicsInt8>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDUInt8:
                movePixelStd<epicsUInt8>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDInt16:
                movePixelStd<epicsInt16>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDUInt16:
                movePixelStd<epicsUInt16>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDInt32:
                movePixelStd<epicsInt32>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDUInt32:
                movePixelStd<epicsUInt32>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDFloat32:
                movePixelStd<epicsFloat32>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
            case NDFloat64:
                movePixelStd<epicsFloat64>(inArray, pixelIndexIn, outArray, pixelIndexOut);
                break;
        }
    }
/** loop over the axes of the image to transform the axes
  * \param[in] inArray the input NDArray
  * \param[in] outArray the transformed Array
  */
void NDPluginTransform::transform3DArray(NDArray *inArray, NDArray *outArray) {
    size_t ii, jj, kk;
    size_t maxInSize[3], maxOutSize[3];
    NDTransformIndex_t pixelIndexIn, pixelIndexOut, pixelIndexInit;
    int colorMode;

    maxInSize[0] = inArray->dims[userDims[0]].size;
    maxInSize[1] = inArray->dims[userDims[1]].size;
    maxInSize[2] = inArray->dims[userDims[2]].size;
    maxOutSize[0] = this->pTransforms[this->maxTransforms-1].dims[0].size;
    maxOutSize[1] = this->pTransforms[this->maxTransforms-1].dims[1].size;
    maxOutSize[2] = this->pTransforms[this->maxTransforms-1].dims[2].size;
    outArray->dims[0].size = maxOutSize[this->realDims[0]];
    outArray->dims[1].size = maxOutSize[this->realDims[1]];
    outArray->dims[2].size = maxOutSize[this->realDims[2]];
    getIntegerParam(NDColorMode, &colorMode);

    for (kk = 0; kk<maxInSize[2]; kk++) {
        for (ii = 0; ii<maxInSize[1]; ii++){
            for (jj = 0; jj<maxInSize[0]; jj++){
                pixelIndexIn.index0 = jj;
                pixelIndexIn.index1 = ii;
                pixelIndexIn.index2 = kk;
                pixelIndexInit.index0 = jj;
                pixelIndexInit.index1 = ii;
                pixelIndexInit.index2 = kk;
                pixelIndexOut = this->transformPixel(pixelIndexIn, this->originLocation);
                if (colorMode == NDColorModeRGB1 ) {
                    this->moveRGB1Pixel(inArray, pixelIndexInit, outArray, pixelIndexOut);
                }
                else if (colorMode == NDColorModeRGB2) {
                    this->moveRGB2Pixel(inArray, pixelIndexInit, outArray, pixelIndexOut);
                }
                else {
                    this->moveStdPixel(inArray, pixelIndexInit, outArray, pixelIndexOut);
                }
            }
        }
    }
}

/** loop over the axes of the image to transform the axes
  * \param[in] inArray the input NDArray
  * \param[in] outArray the transformed Array
  */

void NDPluginTransform::transform2DArray(NDArray *inArray, NDArray *outArray) {
    size_t ii, jj;
    size_t maxInSize0, maxInSize1, maxOutSize0, maxOutSize1;
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
            pixelIndexIn.index2 = 0;
            pixelIndexInit.index0 = jj;
            pixelIndexInit.index1 = ii;
            pixelIndexInit.index2 = 0;
            pixelIndexOut = this->transformPixel(pixelIndexIn, this->originLocation);
            this->moveStdPixel(inArray, pixelIndexInit, outArray, pixelIndexOut);
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
                   NDArrayPort, NDArrayAddr, 1, NUM_TRANSFORM_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    const char *functionName = "NDPluginTransform";
    int ii, jj;

    createParam(NDPluginTransformNameString,         asynParamOctet, &NDPluginTransformName);
    createParam(NDPluginTransform1TypeString,        asynParamInt32, &NDPluginTransform1Type);
    createParam(NDPluginTransform2TypeString,        asynParamInt32, &NDPluginTransform2Type);
    createParam(NDPluginTransform3TypeString,        asynParamInt32, &NDPluginTransform3Type);
    createParam(NDPluginTransform4TypeString,        asynParamInt32, &NDPluginTransform4Type);
    createParam(NDPluginTransformOriginString,       asynParamInt32, &NDPluginTransformOrigin);
    createParam(NDPluginTransform1Dim0MaxSizeString, asynParamInt32, &NDPluginTransform1Dim0MaxSize);
    createParam(NDPluginTransform1Dim1MaxSizeString, asynParamInt32, &NDPluginTransform1Dim1MaxSize);
    createParam(NDPluginTransform1Dim2MaxSizeString, asynParamInt32, &NDPluginTransform1Dim2MaxSize);
    createParam(NDPluginTransform2Dim0MaxSizeString, asynParamInt32, &NDPluginTransform2Dim0MaxSize);
    createParam(NDPluginTransform2Dim1MaxSizeString, asynParamInt32, &NDPluginTransform2Dim1MaxSize);
    createParam(NDPluginTransform2Dim2MaxSizeString, asynParamInt32, &NDPluginTransform2Dim2MaxSize);
    createParam(NDPluginTransform3Dim0MaxSizeString, asynParamInt32, &NDPluginTransform3Dim0MaxSize);
    createParam(NDPluginTransform3Dim1MaxSizeString, asynParamInt32, &NDPluginTransform3Dim1MaxSize);
    createParam(NDPluginTransform3Dim2MaxSizeString, asynParamInt32, &NDPluginTransform3Dim2MaxSize);
    createParam(NDPluginTransform4Dim0MaxSizeString, asynParamInt32, &NDPluginTransform4Dim0MaxSize);
    createParam(NDPluginTransform4Dim1MaxSizeString, asynParamInt32, &NDPluginTransform4Dim1MaxSize);
    createParam(NDPluginTransform4Dim2MaxSizeString, asynParamInt32, &NDPluginTransform4Dim2MaxSize);
    createParam(NDPluginTransformArraySize0String,   asynParamInt32, &NDPluginTransformArraySize0);
    createParam(NDPluginTransformArraySize1String,   asynParamInt32, &NDPluginTransformArraySize1);
    createParam(NDPluginTransformArraySize2String,   asynParamInt32, &NDPluginTransformArraySize2);

    this->maxTransforms = 4;
    this->pTransforms = (NDTransform_t *)callocMustSucceed(this->maxTransforms, sizeof(*this->pTransforms), functionName);
    this-> originLocation = 0;
    for (ii = 0; ii < this->maxTransforms; ii++) {
        this->pTransforms[ii].type=0;
        for (jj = 0; jj < ND_ARRAY_MAX_DIMS; jj++) {
            this->pTransforms[ii].dims[jj].size=0;
        }
    }
    for (ii = 0; ii<ND_ARRAY_MAX_DIMS; ii++) {
        this->userDims[ii] = ii;
        this->realDims[ii] = ii;
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
    setIntegerParam(NDPluginTransform1Dim2MaxSize, 0);
    setIntegerParam(NDPluginTransform2Dim0MaxSize, 0);
    setIntegerParam(NDPluginTransform2Dim1MaxSize, 0);
    setIntegerParam(NDPluginTransform2Dim2MaxSize, 0);
    setIntegerParam(NDPluginTransform3Dim0MaxSize, 0);
    setIntegerParam(NDPluginTransform3Dim1MaxSize, 0);
    setIntegerParam(NDPluginTransform3Dim2MaxSize, 0);
    setIntegerParam(NDPluginTransform4Dim0MaxSize, 0);
    setIntegerParam(NDPluginTransform4Dim1MaxSize, 0);
    setIntegerParam(NDPluginTransform4Dim2MaxSize, 0);
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
