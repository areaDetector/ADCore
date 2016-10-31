/* NDDriverStdArrays.cpp
 * 
 * This is a driver for converting standard EPICS arrays (waveform records)
 * into NDArrays.
 *
 * It allows any Channel Access client to inject NDArrays into an areaDetector IOC.
 * 
 * Author: David J. Vine
 * Berkeley National Lab
 *
 * Mark Rivers
 * University of Chicago
 * 
 * Created: September 28, 2016
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <epicsTime.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <iocsh.h>

#include "ADDriver.h"
#include <epicsExport.h>
#include "NDDriverStdArrays.h"

static const char *driverName = "NDDriverStdArrays";

/** Constructor for NDDriverStdArrays; most parameters are passed to ADDriver::ADDriver.
  * \param[in] portName The name of the asyn port to be created
  * \param[in] maxSizeX Maximum number of elements in x dimension
  * \param[in] maxSizeY Maximum number of elements in y dimension
  * \param[in] maxSizeZ Maximum number of elements in z dimension
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver
  *            is allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            is allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set
  *            in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in
  *            asynFlags.
  **/

NDDriverStdArrays::NDDriverStdArrays(const char *portName, int maxBuffers, size_t maxMemory,
                                     int priority, int stackSize)

    : ADDriver(portName, 1, NUM_NDSA_DETECTOR_PARAMS,
               maxBuffers, maxMemory,
               asynFloat64ArrayMask | asynDrvUserMask, 
               asynFloat64ArrayMask, 
               0, 1, /* ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=0, autoConnect=1 */
               priority, stackSize)
{

    int status = asynSuccess;
    const char *functionName = "NDDriverStdArrays";

    createParam(arrayModeString,                asynParamInt32,        &NDSAArrayMode_);
    createParam(partialArrayCallbacksString,    asynParamInt32,        &NDSAPartialArrayCallbacks_);
    createParam(numElementsString,              asynParamInt32,        &NDSANumElements_);
    createParam(currentPixelString,             asynParamInt32,        &NDSACurrentPixel_);

    status  = setStringParam (ADManufacturer, "NDDriverStdArrays");
    status |= setStringParam (ADModel, "Software Detector");
    status |= setIntegerParam(ADMaxSizeX, 256);
    status |= setIntegerParam(ADMaxSizeY, 256);
    status |= setIntegerParam(ADSizeX, 256);
    status |= setIntegerParam(ADSizeY, 256);
    status |= setIntegerParam(NDArraySizeX, 256);
    status |= setIntegerParam(NDArraySizeY, 256);
    status |= setIntegerParam(NDArraySize, 65536); 
    status |= setIntegerParam(ADImageMode, ADImageSingle);
    status |= setIntegerParam(ADNumImages, 100);
    status |= setIntegerParam(NDSAArrayMode_, 0);
    status |= setIntegerParam(NDSAPartialArrayCallbacks_, 1);
    status |= setIntegerParam(NDSANumElements_, 1);
    status |= setIntegerParam(NDSACurrentPixel_, 0);


    if (status) {
        printf("%s: Unable to set camera prameters.", functionName);
        return;
    }

}

template <typename epicsType> asynStatus NDDriverStdArrays::writeXXXArray(asynUser *pasynUser, void *pValue, size_t nElements)
{
    int acquire;
    asynStatus status = asynSuccess;
    NDDataType_t dataType;
    NDColorMode_t colorMode;
    int numDimensions;
    int itemp;
    int arrayMode; /* Overwrite (0) or append (1) */
    NDArray *pArray;
    static const char *functionName = "writeXXXArray";

    getIntegerParam(ADAcquire, &acquire);
    if (!acquire) return asynSuccess;

    /* Make sure parameters are consistent, fix them if they're not. */
    if (sizeX > maxSizeX) {
        sizeX = maxSizeX;
        status |= setIntegerParam(NDSAizeX, sizeX);
    }
    if (sizeY > maxSizeY) {
        sizeY = maxSizeY;
        status |= setIntegerParam(NDSAizeY, sizeY);
    }
    if (sizeZ > maxSizeZ) {
        sizeZ = maxSizeZ;
        status |= setIntegerParam(NDSASizeZ, sizeZ);
    }

    pImage = this->pArrays[0];
    getIntegerParam(NDDataType,   &itemp); dataType = (NDDataType_t) itemp;
    getIntegerParam(NDColorMode,  &itemp); colorMode = (NDColorMode_t) itemp;
    getIntegerParam(NDSAArrayMode_, &arrayMode);
    getIntegerParam(NDNDimensions, &numDimensions);

    if (arrayMode==0 || pNewData_==0){ /* Overwrite */
        if (this->pArrays[0]) this->pArrays[0]->release();

        /* Allocate the raw buffer we use to compute images. */
        this->pArrays[0] = this->pNDArrayPool->alloc(numDimensions, arrayDimensions_, dataType, 0, NULL);
        pArray = this->pArrays[0];

        if (!pArray) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s:%s: error allocating raw buffer\n",
                      driverName, functionName);
            return asynError;
        }
        pArray->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32, &colorMode);
    }

    switch (dataType){
        case NDInt8:
            copyBuffer<epicsType, epicsInt8>(pArray, pValue, nElements);
            break;
        case NDUInt8:
            copyBuffer<epicsType, epicsUInt8>(pArray, pValue, nElements);
            break;
        case NDInt16:
            copyBuffer<epicsType, epicsInt16>(pArray, pValue, nElements);
            break;
        case NDUInt16:
            copyBuffer<epicsType, epicsUInt16>(pArray, pValue, nElements);
            break;
        case NDInt32:
            copyBuffer<epicsType, epicsInt32>(pArray, pValue, nElements);
            break;
        case NDUInt32:
            copyBuffer<epicsType, epicsUInt32>(pArray, pValue, nElements);
            break;
        case NDFloat32:
            copyBuffer<epicsType, epicsFloat32>(pArray, pValue, nElements);
            break;
        case NDFloat64:
            copyBuffer<epicsType, epicsFloat64>(pArray, pValue, nElements);
            break;
    }
    
    pImage->getInfo(&arrayInfo);
    status = asynSuccess;
    status |= setIntegerParam(NDArraySize,  (int)arrayInfo.totalBytes);
    status |= setIntegerParam(NDArraySizeX, (int)pImage->dims[xDim].size);
    status |= setIntegerParam(NDArraySizeY, (int)pImage->dims[yDim].size);
    status |= setIntegerParam(NDArraySizeZ, (int)pImage->dims[zDim].size);
    callParamCallbacks();

        getIntegerParam(NDSAPartialArrayCallbacks, &partialArrayCallbacks);
        if ((partialArrayCallbacks==1) ||
            (arrayMode==0) || 
            ((arrayMode==1) && 
             (currentPixel>=sizeX*sizeY)))
        {
            pImage = this->pArrays[0];
            /* Get current parameters. */
            getIntegerParam(NDArrayCounter, &imageCounter);
            getIntegerParam(ADNumImages, &numImages);
            getIntegerParam(ADNumImagesCounter, &numImagesCounter);
            getIntegerParam(ADImageMode, &imageMode);
            getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
            imageCounter++;
            numImagesCounter++;
            setIntegerParam(NDArrayCounter, imageCounter);
            setIntegerParam(ADNumImagesCounter, numImagesCounter);

            /* put the frame number and timestamp into the current buffer. */
            pImage->uniqueId = imageCounter;
            pImage->timeStamp = startTime.secPastEpoch+startTime.nsec/1.e9;
            updateTimeStamp(&pImage->epicsTS);

            
            this->getAttributes(pImage->pAttributeList);
            if (arrayCallbacks) {
                this->unlock();
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                        "%s:%s: calling imageData callback\n", driverName, functionName);
                doCallbacksGenericPointer(pImage, NDArrayData, 0);
                this->lock();
            }
        }
        
        getIntegerParam(NDSAizeX, &sizeX);
        getIntegerParam(NDSAizeY, &sizeY);
        getIntegerParam(NDSAArrayMode, &arrayMode);
        getIntegerParam(NDSACurrentPixel_RBV, &currentPixel);
        /* Check if acquisition is done. */
        if ((imageMode==ADImageSingle) ||
            ((imageMode==ADImageMultiple) &&
            (numImagesCounter>=numImages))) 
        {
            /* Check if we are appending values to an array. If so
               we want to collect one whole array before acquisition is completed. */
            if ((arrayMode==0) || 
                ((arrayMode==1) && 
                (currentPixel>=sizeX*sizeY)))
            {
                setIntegerParam(ADAcquire, 0);
                setIntegerParam(NDSAtatus, NDSAtatusIdle);
                setIntegerParam(NDSACurrentPixel, 0);
                setIntegerParam(NDSACurrentPixel_RBV, 0);
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: acquisition completes\n", driverName, functionName);
            }
        }

    return status;
}



asynStatus NDDriverStdArrays::writeInt8Array(asynUser *pasynUser, epicsInt8 *value, size_t nElements)
{
    return writeXXXArray<epicsInt8>(pasynUser, (void *)value, nElements);
}

asynStatus NDDriverStdArrays::writeInt16Array(asynUser *pasynUser, epicsInt16 *value, size_t nElements)
{
    return writeXXXArray<epicsInt16>(pasynUser, (void *)value, nElements);
}

asynStatus NDDriverStdArrays::writeInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements)
{
    // This is called both for array data and for array dimensions 
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    
    switch (function) {
        case NDDimensions:
            for (int i=0; i<nElements && i<ND_ARRAY_MAX_DIMS; i++) {
                arrayDimensions_[i] = (size_t)value[i];
            } 
            break;

        case NDSAArrayData: 
            status = writeXXXArray<epicsInt32>(pasynUser, (void *)value, nElements);
            break;
    }
    return status;
}

asynStatus NDDriverStdArrays::writeFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements)
{
    return writeXXXArray<epicsFloat32>(pasynUser, (void *)value, nElements);
}

asynStatus NDDriverStdArrays::writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements)
{
    return writeXXXArray<epicsFloat64>(pasynUser, (void *)value, nElements);
}


extern "C" int NDDriverStdArraysConfig(const char *portName, int maxBuffers, int maxMemory, int priority, int stackSize)
{
    new NDDriverStdArrays(portName,
                    (maxBuffers < 0) ? 0 : maxBuffers,
                    (maxMemory < 0) ? 0 : maxMemory,
                    priority, stackSize);
    return(asynSuccess);
}

/** Code for iocsh registration */
static const iocshArg NDDriverStdArraysConfigArg0 = {"Port name", iocshArgString};
static const iocshArg NDDriverStdArraysConfigArg1 = {"maxBuffers", iocshArgInt};
static const iocshArg NDDriverStdArraysConfigArg2 = {"maxMemory", iocshArgInt};
static const iocshArg NDDriverStdArraysConfigArg3 = {"priority", iocshArgInt};
static const iocshArg NDDriverStdArraysConfigArg4 = {"stackSize", iocshArgInt};
static const iocshArg * const NDDriverStdArraysConfigArgs[] =  {&NDDriverStdArraysConfigArg0,
                                                                &NDDriverStdArraysConfigArg1,
                                                                &NDDriverStdArraysConfigArg2,
                                                                &NDDriverStdArraysConfigArg3,
                                                                &NDDriverStdArraysConfigArg4};
static const iocshFuncDef configNDDriverStdArrays = {"NDDriverStdArraysConfig", 5, NDDriverStdArraysConfigArgs};
static void configNDDriverStdArraysCallFunc(const iocshArgBuf *args)
{
    NDDriverStdArraysConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival, args[4].ival);
}

static void NDDriverStdArraysRegister(void)
{

    iocshRegister(&configNDDriverStdArrays, configNDDriverStdArraysCallFunc);
}

extern "C" {
epicsExportRegistrar(NDDriverStdArraysRegister);
}

