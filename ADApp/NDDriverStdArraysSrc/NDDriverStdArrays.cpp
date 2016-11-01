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

    : ADDriver(portName, 1, NUM_NDSA_DRIVER_PARAMS,
               maxBuffers, maxMemory,
               asynFloat64ArrayMask | asynDrvUserMask, 
               asynFloat64ArrayMask, 
               0, 1, /* ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=0, autoConnect=1 */
               priority, stackSize)
{

    int status = asynSuccess;
    const char *functionName = "NDDriverStdArrays";

    createParam(NDSA_ArrayModeString,                asynParamInt32,        &NDSA_ArrayMode_);
    createParam(NDSA_PartialArrayCallbacksString,    asynParamInt32,        &NDSA_PartialArrayCallbacks_);
    createParam(NDSA_NumElementsString,              asynParamInt32,        &NDSA_NumElements_);
    createParam(NDSA_CurrentPixelString,             asynParamInt32,        &NDSA_CurrentPixel_);
    createParam(NDSA_NDimensionsString,              asynParamInt32,        &NDSA_NDimensions_);
    createParam(NDSA_DimensionsString,               asynParamInt32,        &NDSA_Dimensions_);
    createParam(NDSA_ArrayDataString,                asynParamInt32,        &NDSA_ArrayData_);

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
    status |= setIntegerParam(NDSA_ArrayMode_, 0);
    status |= setIntegerParam(NDSA_PartialArrayCallbacks_, 1);
    status |= setIntegerParam(NDSA_NumElements_, 1);
    status |= setIntegerParam(NDSA_CurrentPixel_, 0);


    if (status) {
        printf("%s: Unable to set camera prameters.", functionName);
        return;
    }

}

template <typename epicsType, typename NDArrayType> asynStatus NDDriverStdArrays::copyBuffer(NDArray *pArray, void *pValue, size_t nElements)
{
    epicsType *pIn = (epicsType *)pValue;
    NDArrayType *pOut = (NDArrayType *)pArray->pData;

    for (size_t i=0; i<nElements; i++) pOut[i] = (NDArrayType) pIn[i];
    return asynSuccess;
}

template <typename epicsType> asynStatus NDDriverStdArrays::writeXXXArray(asynUser *pasynUser, void *pValue, size_t nElements)
{
    int acquire;
    int i;
    asynStatus status = asynSuccess;
    NDDataType_t dataType;
    NDColorMode_t colorMode;
    NDArrayInfo arrayInfo;
    size_t maxElements=1;
    int numDimensions;
    int partialArrayCallbacks;
    int numImages;
    int imageMode;
    int numImagesCounter;
    int arrayCallbacks;
    int currentPixel;
    epicsTimeStamp startTime;
    int imageCounter;
    int itemp;
    int arrayMode; /* Overwrite (0) or append (1) */
    NDArray *pArray=0;
    static const char *functionName = "writeXXXArray";

    getIntegerParam(ADAcquire, &acquire);
    if (!acquire) return asynSuccess;


    getIntegerParam(NDDataType,   &itemp); dataType = (NDDataType_t) itemp;
    getIntegerParam(NDColorMode,  &itemp); colorMode = (NDColorMode_t) itemp;
    getIntegerParam(NDSA_ArrayMode_, &arrayMode);
    getIntegerParam(NDSA_NDimensions_, &numDimensions);


    if (arrayMode==0 || pNewData_==0) { /* Overwrite */
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

    /* Make sure size is valid */
    nElements = 1;
    for (i=0; i<numDimensions; i++) {
        nElements *= arrayDimensions_[i];
    }

    pArray->getInfo(&arrayInfo);
    setIntegerParam(NDArraySize,  (int)arrayInfo.totalBytes);
    setIntegerParam(NDArraySizeX, (int)pArray->dims[0].size);
    setIntegerParam(NDArraySizeY, (int)pArray->dims[1].size);
    setIntegerParam(NDArraySizeZ, (int)pArray->dims[2].size);

    if (nElements > arrayInfo.nElements) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s invalid dimensions, nElements=%d, arrayInfo.nElements=%d\n",
                  driverName, functionName, (int)nElements, (int)arrayInfo.nElements);
        return asynError;
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
    
    getIntegerParam(NDSA_PartialArrayCallbacks_, &partialArrayCallbacks);
    if ((partialArrayCallbacks==1) ||
        (arrayMode==0) || 
        ((arrayMode==1) && 
         (arrayInfo.nElements >=maxElements)))
    {
        pArray = this->pArrays[0];
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
        pArray->uniqueId = imageCounter;
        pArray->timeStamp = startTime.secPastEpoch+startTime.nsec/1.e9;
        updateTimeStamp(&pArray->epicsTS);

        this->getAttributes(pArray->pAttributeList);
        if (arrayCallbacks) {
            this->unlock();
            doCallbacksGenericPointer(pArray, NDArrayData, 0);
            this->lock();
        }
    }

    getIntegerParam(NDSA_ArrayMode_, &arrayMode);
    getIntegerParam(NDSA_CurrentPixel_, &currentPixel);
    /* Check if acquisition is done. */
    if ((imageMode==ADImageSingle) ||
        ((imageMode==ADImageMultiple) &&
        (numImagesCounter>=numImages))) 
    {
        /* Check if we are appending values to an array. If so
           we want to collect one whole array before acquisition is completed. */
        if ((arrayMode==0) || 
            ((arrayMode==1) && 
            (arrayInfo.nElements >=maxElements)))
        {
            setIntegerParam(ADAcquire, 0);
            setIntegerParam(ADStatus, ADStatusIdle);
            setIntegerParam(NDSA_CurrentPixel_, 0);
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s: acquisition completes\n", driverName, functionName);
        }
    }

    callParamCallbacks();

    return status;
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDDriverStdArrays::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int acquire;
    static const char *functionName = "writeInt32";

    getIntegerParam(ADAcquire, &acquire);
    
   /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    if (function == ADAcquire){
        if (value == 1){
            setIntegerParam(ADNumImagesCounter, 0);
        }
        else {
        }
    } else {
        // If this parameter belongs to a base class call its method
        if (function < FIRST_NDSA_DRIVER_PARAM)
            status = ADDriver::writeInt32(pasynUser, value);
    }
    
    // Do callbacks so higher layers see any changes
    status = (asynStatus) callParamCallbacks();
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
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
    
    if (function == NDSA_Dimensions_) {
        for (size_t i=0; i<nElements && i<ND_ARRAY_MAX_DIMS; i++) {
            arrayDimensions_[i] = (size_t)value[i];
        } 
    }
    else if (function == NDSA_ArrayData_) {
            status = writeXXXArray<epicsInt32>(pasynUser, (void *)value, nElements);
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

void NDDriverStdArrays::report (FILE *fp, int details)
{
    fprintf(fp, "NDDriverStdArrays %s\n", this->portName);
    if (details > 0) {
        int nDimensions;
        getIntegerParam(NDSA_NDimensions_, &nDimensions);
        fprintf(fp, "  nDimensions:      %d\n", nDimensions);
        fprintf(fp, "  array dimensions:  [");
        for (int i=0; i<nDimensions; i++) fprintf(fp, "%d ", (int)arrayDimensions_[i]);
        fprintf(fp, "]\n");
    }

    ADDriver::report(fp, details);
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

