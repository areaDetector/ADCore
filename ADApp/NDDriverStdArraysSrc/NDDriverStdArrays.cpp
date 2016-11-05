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
               asynFloat64ArrayMask | asynFloat32ArrayMask | asynInt32ArrayMask | asynInt16ArrayMask | asynInt8ArrayMask, 
               0, 
               0, 1, /* ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=0, autoConnect=1 */
               priority, stackSize)
{

    int status = asynSuccess;
    const char *functionName = "NDDriverStdArrays";

    createParam(NDSA_CallbackModeString,             asynParamInt32,        &NDSA_CallbackMode_);
    createParam(NDSA_DoCallbacksString,              asynParamInt32,        &NDSA_DoCallbacks_);
    createParam(NDSA_AppendModeString,               asynParamInt32,        &NDSA_AppendMode_);
    createParam(NDSA_NumElementsString,              asynParamInt32,        &NDSA_NumElements_);
    createParam(NDSA_NextElementString,              asynParamInt32,        &NDSA_NextElement_);
    createParam(NDSA_NewArrayString,                 asynParamInt32,        &NDSA_NewArray_);
    createParam(NDSA_ArrayCompleteString,            asynParamInt32,        &NDSA_ArrayComplete_);
    createParam(NDSA_ArrayDataString,                asynParamInt32,        &NDSA_ArrayData_);

    status  = setStringParam (ADManufacturer, "NDDriverStdArrays");
    status |= setStringParam (ADModel, "Software Detector");
    status |= setIntegerParam(ADImageMode, ADImageSingle);
    status |= setIntegerParam(ADNumImages, 100);
    status |= setIntegerParam(NDSA_CallbackMode_, (int)NDSA_OnUpdate);
    status |= setIntegerParam(NDSA_NumElements_, 0);
    status |= setIntegerParam(NDSA_NextElement_, 0);
    status |= setIntegerParam(NDSA_NewArray_, 1);
    status |= setIntegerParam(NDSA_ArrayComplete_, 0);

    if (status) {
        printf("%s: Unable to set camera prameters.", functionName);
        return;
    }
}

template <typename epicsType, typename NDArrayType> void NDDriverStdArrays::copyBuffer(size_t nextElement, void *pValue, size_t nElements)
{
    epicsType *pIn = (epicsType *)pValue;
    NDArrayType *pOut = (NDArrayType *)pArrays[0]->pData + nextElement;

    for (size_t i=0; i<nElements; i++) pOut[i] = (NDArrayType) pIn[i];
}

template <typename epicsType> asynStatus NDDriverStdArrays::writeXXXArray(asynUser *pasynUser, void *pValue, size_t nElements)
{
    int acquire;
    asynStatus status = asynSuccess;
    int dataType;
    int colorMode;
    NDArrayInfo arrayInfo;
    int callbackMode;
    int appendMode;
    int numElements;
    int nextElement;
    int numDimensions;
    int arrayCallbacks;
    int i;
    int itemp;
    int newArray;
    epicsInt32 currentIndex[ND_ARRAY_MAX_DIMS];
    size_t dimProd[ND_ARRAY_MAX_DIMS];
    NDArray *pArray;
    static const char *functionName = "writeXXXArray";

    getIntegerParam(ADAcquire, &acquire);
    if (!acquire) return asynSuccess;

    getIntegerParam(NDDataType,         &dataType);
    getIntegerParam(NDColorMode,        &colorMode);
    getIntegerParam(NDSA_CallbackMode_, &callbackMode);
    getIntegerParam(NDSA_AppendMode_,   &appendMode);
    getIntegerParam(NDNDimensions,      &numDimensions);
    getIntegerParam(NDSA_NumElements_,  &numElements);
    getIntegerParam(NDSA_NewArray_,     &newArray);
    getIntegerParam(NDArrayCallbacks,   &arrayCallbacks);

    if ((appendMode == 0) || ((appendMode == 1) && newArray)) {
        if (this->pArrays[0]) this->pArrays[0]->release();
        setIntegerParam(NDSA_NewArray_, 0);

        /* Allocate the raw buffer we use to compute images. */
        this->pArrays[0] = this->pNDArrayPool->alloc(numDimensions, arrayDimensions_, (NDDataType_t)dataType, 0, NULL);
        pArray = this->pArrays[0];

        if (!pArray) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s:%s: error allocating raw buffer\n",
                      driverName, functionName);
            return asynError;
        }
        pArray->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32, &colorMode);
        pArray->getInfo(&arrayInfo);
        setIntegerParam(NDArraySize,  (int)arrayInfo.totalBytes);
        setIntegerParam(NDSA_NumElements_, arrayInfo.nElements);
        setIntegerParam(ADMaxSizeX,   arrayDimensions_[0]);
        setIntegerParam(ADMaxSizeY,   arrayDimensions_[1]);
        setIntegerParam(NDArraySizeX, 0);
        setIntegerParam(NDArraySizeY, 0);
        setIntegerParam(NDArraySizeZ, 0);

        memset(currentIndex, 0, ND_ARRAY_MAX_DIMS*sizeof(currentIndex[0]));
        memset(dimProd, 0, ND_ARRAY_MAX_DIMS*sizeof(dimProd[0]));
        dimProd[0] = arrayDimensions_[0];
        for (i=1; i<numDimensions; i++) {
            dimProd[i] = arrayDimensions_[i] * dimProd[i-1];
        }

        // In append mode zero-fill the array
        if (appendMode == 1 || 
          ((appendMode == 0) && (arrayInfo.nElements < nElements))) {
            memset(pArray->pData, 0, arrayInfo.totalBytes);
        }
        if (appendMode == 0) {
            setIntegerParam(NDSA_NextElement_, 0);
        }
    }

    getIntegerParam(NDSA_NextElement_, &nextElement);
    if ((nextElement + nElements) >= arrayInfo.nElements) {
        nElements = arrayInfo.nElements - nextElement;
    }
    
    switch (dataType){
        case NDInt8:
            copyBuffer<epicsType, epicsInt8>(nextElement, pValue, nElements);
            break;
        case NDUInt8:
            copyBuffer<epicsType, epicsUInt8>(nextElement, pValue, nElements);
            break;
        case NDInt16:
            copyBuffer<epicsType, epicsInt16>(nextElement, pValue, nElements);
            break;
        case NDUInt16:
            copyBuffer<epicsType, epicsUInt16>(nextElement, pValue, nElements);
            break;
        case NDInt32:
            copyBuffer<epicsType, epicsInt32>(nextElement, pValue, nElements);
            break;
        case NDUInt32:
            copyBuffer<epicsType, epicsUInt32>(nextElement, pValue, nElements);
            break;
        case NDFloat32:
            copyBuffer<epicsType, epicsFloat32>(nextElement, pValue, nElements);
            break;
        case NDFloat64:
            copyBuffer<epicsType, epicsFloat64>(nextElement, pValue, nElements);
            break;
    }
    
    nextElement += nElements;
    setIntegerParam(NDSA_NextElement_, nextElement);

    //  Convert nextElement into multi-array dimensions which is more user-friendly
    itemp = nextElement-1;
    for (i=numDimensions-1; i>0; i--) {
        if (i < (numDimensions-1)) {
            itemp %= dimProd[i];
        }
        currentIndex[i] = 1 + (itemp / dimProd[i-1]);
    }
    currentIndex[0] = 1 + (itemp % arrayDimensions_[0]);
    doCallbacksInt32Array(currentIndex, ND_ARRAY_MAX_DIMS, NDDimensions, 0);

    if (appendMode == 0) {
        setArrayComplete();
    }

    if (arrayCallbacks && (appendMode == 1) && (callbackMode == NDSA_OnUpdate)) {
        doCallbacks();
    }

    callParamCallbacks();

    return status;
}

void NDDriverStdArrays::setArrayComplete()
{
    int imageMode;
    int numImagesCounter;
    int numImages;
    int callbackMode;
    int arrayCallbacks;

    getIntegerParam(ADNumImages,        &numImages);
    getIntegerParam(ADNumImagesCounter, &numImagesCounter);
    getIntegerParam(NDSA_CallbackMode_, &callbackMode);
    getIntegerParam(ADImageMode,        &imageMode);
    getIntegerParam(NDArrayCallbacks,   &arrayCallbacks);

    numImagesCounter++;
    setIntegerParam(ADNumImagesCounter, numImagesCounter);

    if (arrayCallbacks && (callbackMode != NDSA_OnCommand)) {
        doCallbacks();
    }

    /* Check if acquisition is done. */
    if ((imageMode==ADImageSingle) ||
        ((imageMode==ADImageMultiple) && (numImagesCounter>=numImages))) {
        setIntegerParam(ADAcquire, 0);
        setIntegerParam(ADStatus, ADStatusIdle);
    }
}

void NDDriverStdArrays::doCallbacks()
{
    NDArray *pArray = this->pArrays[0];
    epicsTimeStamp startTime;
    int imageCounter;

    if (!pArray) return;

    getIntegerParam(NDArrayCounter, &imageCounter);
    imageCounter++;
    setIntegerParam(NDArrayCounter, imageCounter);

    /* Put the frame number and timestamp into the NDArray */
    pArray->uniqueId = imageCounter;
    epicsTimeGetCurrent(&startTime);
    pArray->timeStamp = startTime.secPastEpoch+startTime.nsec/1.e9;
    updateTimeStamp(&pArray->epicsTS);
    this->getAttributes(pArray->pAttributeList);
    this->unlock();
    doCallbacksGenericPointer(pArray, NDArrayData, 0);
    this->lock();
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
    } 
    else if (function == NDSA_DoCallbacks_) {
        doCallbacks();
    }
    else if (function == NDSA_NewArray_) {
        setIntegerParam(NDSA_NextElement_, 0);
    }
    else if (function == NDSA_ArrayComplete_) {
        int appendMode;
        getIntegerParam(NDSA_AppendMode_, &appendMode);
        if (appendMode) setArrayComplete();
    }
    else {
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
    
    if (function == NDDimensions) {
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
        getIntegerParam(NDNDimensions, &nDimensions);
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

