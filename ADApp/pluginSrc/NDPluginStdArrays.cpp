/*
 * NDPluginStdArrays.c
 * 
 * Asyn driver for callbacks to standard asyn array interfaces for NDArray drivers.
 * This is commonly used for EPICS waveform records.
 *
 * Author: Mark Rivers
 *
 * Created April 25, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsMessageQueue.h>
#include <cantProceed.h>

#include <asynStandardInterfaces.h>

#define DEFINE_STANDARD_PARAM_STRINGS 1
#include "ADInterface.h"
#include "NDArrayBuff.h"
#include "ADParamLib.h"
#include "ADUtils.h"
#include "NDPluginStdArrays.h"
#include "drvNDStdArrays.h"

#define driverName "NDPluginStdArrays"


void NDPluginStdArrays::processCallbacks(NDArray_t *pArray)
{
    /* This function calls back any registered clients on the standard asyn array interfaces with 
     * the data in our private buffer.
     * It is called with the mutex already locked.
     */
     
    ELLLIST *pclientList;
    interruptNode *pnode;
    int arrayCounter;
    int int8Initialized=0;
    int int16Initialized=0;
    int int32Initialized=0;
    int float32Initialized=0;
    int float64Initialized=0;
    NDArrayInfo_t arrayInfo;
    int i;
    asynStandardInterfaces *pInterfaces = &this->asynStdInterfaces;
    /* const char* functionName = "NDStdArraysDoCallbacks"; */

    NDArrayBuff->getInfo(pArray, &arrayInfo);
 
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing pPvt */
    epicsMutexUnlock(this->mutexId);

    /* This macro saves a lot of code, since there are 5 types of array interrupts that we support */
    #define ARRAY_INTERRUPT_CALLBACK(INTERRUPT_PVT, INTERRUPT_TYPE, INITIALIZED, \
                                     SIGNED_TYPE, UNSIGNED_TYPE, EPICS_TYPE) { \
        EPICS_TYPE *pData=NULL;\
        NDArray_t *pOutput=NULL; \
        NDDimension_t outDims[ND_ARRAY_MAX_DIMS]; \
         \
        pasynManager->interruptStart(pInterfaces->INTERRUPT_PVT, &pclientList); \
        pnode = (interruptNode *)ellFirst(pclientList); \
        while (pnode) { \
            INTERRUPT_TYPE *pInterrupt = (INTERRUPT_TYPE *)pnode->drvPvt; \
            if (pInterrupt->pasynUser->reason == NDPluginStdArraysData) { \
                if (!INITIALIZED) { \
                    INITIALIZED = 1; \
                    for (i=0; i<pArray->ndims; i++)  {\
                        NDArrayBuff->initDimension(&outDims[i], pArray->dims[i].size); \
                    } \
                    NDArrayBuff->convert(pArray, &pOutput, \
                                         SIGNED_TYPE, \
                                         outDims); \
                    pData = (EPICS_TYPE *)pOutput->pData; \
                 } \
                pInterrupt->callback(pInterrupt->userPvt, \
                                     pInterrupt->pasynUser, \
                                     pData, arrayInfo.nElements); \
            } \
            pnode = (interruptNode *)ellNext(&pnode->node); \
        } \
        pasynManager->interruptEnd(pInterfaces->INTERRUPT_PVT); \
        if (pOutput) NDArrayBuff->release(pOutput); \
    }

    /* Pass interrupts for int8Array data*/
    ARRAY_INTERRUPT_CALLBACK(int8ArrayInterruptPvt, asynInt8ArrayInterrupt,
                             int8Initialized, NDInt8, NDUInt8, epicsInt8);
    
    /* Pass interrupts for int16Array data*/
    ARRAY_INTERRUPT_CALLBACK(int16ArrayInterruptPvt, asynInt16ArrayInterrupt,
                             int16Initialized, NDInt16, NDUInt16, epicsInt16);
    
    /* Pass interrupts for int32Array data*/
    ARRAY_INTERRUPT_CALLBACK(int32ArrayInterruptPvt, asynInt32ArrayInterrupt,
                             int32Initialized, NDInt32, NDUInt32, epicsInt32);
    
    /* Pass interrupts for float32Array data*/
    ARRAY_INTERRUPT_CALLBACK(float32ArrayInterruptPvt, asynFloat32ArrayInterrupt,
                             float32Initialized, NDFloat32, NDFloat32, epicsFloat32);
    
    /* Pass interrupts for float64Array data*/
    ARRAY_INTERRUPT_CALLBACK(float64ArrayInterruptPvt, asynFloat64ArrayInterrupt,
                             float64Initialized, NDFloat64, NDFloat64, epicsFloat64);

    /* See if the array dimensions have changed.  If so then do callbacks on them. */
    ADUtils->dimensionCallback(pInterfaces->int32ArrayInterruptPvt, this->dimsPrev, 
                               pArray, NDPluginStdArraysDimensions);

    /* We must exit with the mutex locked */
    epicsMutexLock(this->mutexId);
    /* We always keep the last array so read() can use it.  Release it now */
    if (this->pArray) NDArrayBuff->release(this->pArray);
    this->pArray = pArray;
    /* Update the parameters.  The counter should be updated after data are posted
     * because clients might use that to detect new data */
    ADParam->setInteger(this->params, NDPluginStdArraysNDimensions, pArray->ndims);
    ADParam->setInteger(this->params, ADDataType, pArray->dataType);
    ADParam->getInteger(this->params, NDPluginBaseArrayCounter, &arrayCounter);    
    arrayCounter++;
    ADParam->setInteger(this->params, NDPluginBaseArrayCounter, arrayCounter);    
    ADParam->callCallbacks(this->params);
}

asynStatus NDPluginStdArrays::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(this->mutexId);

    /* Set the parameter in the parameter library. */
    status = (asynStatus)ADParam->setInteger(this->params, function, value);

    switch(function) {
        default:
            /* This was not a parameter that this driver understands, try the base class */
            epicsMutexUnlock(this->mutexId);
            status = NDPluginBase::writeInt32(pasynUser, value);
            epicsMutexLock(this->mutexId);
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacks(this->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeInt32 error, status=%d function=%d, value=%d\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeInt32: function=%d, value=%d\n", 
              driverName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}

asynStatus NDPluginStdArrays::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char* functionName = "writeFloat64";

    epicsMutexLock(this->mutexId);

    /* Set the parameter in the parameter library. */
    status = (asynStatus)ADParam->setDouble(this->params, function, value);

    switch(function) {
        /* We don't currently need to do anything special when these functions are received */
        default:
            /* This was not a parameter that this driver understands, try the base class */
            epicsMutexUnlock(this->mutexId);
            status = NDPluginBase::writeFloat64(pasynUser, value);
            epicsMutexLock(this->mutexId);
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacks(this->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}


asynStatus NDPluginStdArrays::writeOctet(asynUser *pasynUser,
                             const char *value, size_t nChars, size_t *nActual)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(this->mutexId);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)ADParam->setString(this->params, function, (char *)value);

    switch(function) {
        default:
            /* This was not a parameter that this driver understands, try the base class */
            epicsMutexUnlock(this->mutexId);
            status = NDPluginBase::writeOctet(pasynUser, value, nChars, nActual);
            epicsMutexLock(this->mutexId);
            break;
    }
    
     /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacks(this->params);

    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeOctet error, status=%d function=%d, value=%s\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeOctet: function=%d, value=%s\n", 
              driverName, function, value);
    *nActual = nChars;
    epicsMutexUnlock(this->mutexId);
    return status;
}


/* The following macros save a lot of code, since we have 5 array types to support */
#define DEFINE_READ_ARRAY(FUNCTION_NAME, EPICS_TYPE, AD_TYPE) \
static asynStatus FUNCTION_NAME(void *drvPvt, asynUser *pasynUser, \
                                EPICS_TYPE *value, size_t nElements, size_t *nIn) \
{ \
    NDPluginStdArrays *pPvt = (NDPluginStdArrays *)drvPvt; \
    return(pPvt->FUNCTION_NAME(pasynUser, value, nElements, nIn)); \
}\
\
asynStatus NDPluginStdArrays::FUNCTION_NAME(asynUser *pasynUser, \
                                EPICS_TYPE *value, size_t nElements, size_t *nIn) \
{ \
    int command = pasynUser->reason; \
    asynStatus status = asynSuccess; \
    NDArray_t *pOutput; \
    NDArrayInfo_t arrayInfo; \
    NDDimension_t outDims[ND_ARRAY_MAX_DIMS]; \
    int i; \
    size_t ncopy; \
    int dataType; \
    \
    epicsMutexLock(this->mutexId); \
    switch(command) { \
        case NDPluginStdArraysData: \
            dataType = this->pArray->dataType; \
            NDArrayBuff->getInfo(this->pArray, &arrayInfo); \
            if (arrayInfo.nElements > (int)nElements) { \
                /* We have been requested fewer pixels than we have.  \
                 * Just pass the first nElements. */ \
                 arrayInfo.nElements = nElements; \
            } \
            /* We are guaranteed to have the most recent data in our buffer.  No need to call the driver, \
             * just copy the data from our buffer */ \
            /* Convert data from its actual data type.  */ \
            if (!this->pArray || !this->pArray->pData) break; \
            for (i=0; i<this->pArray->ndims; i++)  {\
                NDArrayBuff->initDimension(&outDims[i], this->pArray->dims[i].size); \
            } \
            status = (asynStatus)NDArrayBuff->convert(this->pArray, \
                                                      &pOutput, \
                                                      AD_TYPE, \
                                                      outDims); \
            break; \
        case NDPluginStdArraysDimensions: \
            ncopy = ND_ARRAY_MAX_DIMS; \
            if (nElements < ncopy) ncopy = nElements; \
            memcpy(value, this->dimsPrev, ncopy*sizeof(*this->dimsPrev)); \
            *nIn = ncopy;\
            break;\
        default: \
            asynPrint(pasynUser, ASYN_TRACE_ERROR, \
                      "%s::readArray, unknown command %d\n" \
                      driverName, command); \
            status = asynError; \
    } \
    epicsMutexUnlock(this->mutexId); \
    return(asynSuccess); \
}

#define DEFINE_WRITE_ARRAY(FUNCTION_NAME, EPICS_TYPE, AD_TYPE) \
static asynStatus FUNCTION_NAME(void *drvPvt, asynUser *pasynUser, \
                                EPICS_TYPE *value, size_t nElements) \
{ \
    /* Note yet implemented */ \
    asynPrint(pasynUser, ASYN_TRACE_ERROR, \
              "%s::WRITE_ARRAY not yet implemented\n", driverName); \
    return(asynError); \
}

/* asynInt8Array interface methods */
DEFINE_READ_ARRAY(readInt8Array, epicsInt8, NDInt8)
DEFINE_WRITE_ARRAY(writeInt8Array, epicsInt8, NDInt8)
    
/* asynInt16Array interface methods */
DEFINE_READ_ARRAY(readInt16Array, epicsInt16, NDInt16)
DEFINE_WRITE_ARRAY(writeInt16Array, epicsInt16, NDInt16)
    
/* asynInt32Array interface methods */
DEFINE_READ_ARRAY(readInt32Array, epicsInt32, NDInt32)
DEFINE_WRITE_ARRAY(writeInt32Array, epicsInt32, NDInt32)
    
/* asynFloat32Array interface methods */
DEFINE_READ_ARRAY(readFloat32Array, epicsFloat32, NDFloat32)
DEFINE_WRITE_ARRAY(writeFloat32Array, epicsFloat32, NDFloat32)
    
/* asynFloat64Array interface methods */
DEFINE_READ_ARRAY(readFloat64Array, epicsFloat64, NDFloat64)
DEFINE_WRITE_ARRAY(writeFloat64Array, epicsFloat64, NDFloat64)
    

/* asynDrvUser interface methods */
asynStatus NDPluginStdArrays::drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    int status;
    int param;
    static char *functionName = "drvUserCreate";

    /* First see if this is a parameter specific to this plugin */
        status = ADUtils->findParam(NDPluginStdArraysParamString, NUM_ND_PLUGIN_STD_ARRAYS_PARAMS, 
                                    drvInfo, &param);
                                    
    /* If not, then see if this is a base plugin parameter */
    if (status != asynSuccess) 
        status = ADUtils->findParam(NDPluginBaseParamString, NUM_ND_PLUGIN_BASE_PARAMS, 
                                    drvInfo, &param);

    if (status == asynSuccess) {
        pasynUser->reason = param;
        if (pptypeName) {
            *pptypeName = epicsStrDup(drvInfo);
        }
        if (psize) {
            *psize = sizeof(param);
        }
        asynPrint(pasynUser, ASYN_TRACE_FLOW,
                  "%s:%s:, drvInfo=%s, param=%d\n", 
                  driverName, functionName, drvInfo, param);
        return(asynSuccess);
    } else {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "%s:%s:, unknown drvInfo=%s", 
                     driverName, functionName, drvInfo);
        return(asynError);
    }
}

    
static asynInt8Array ifaceInt8Array = {
    writeInt8Array,
    readInt8Array,
};

static asynInt16Array ifaceInt16Array = {
    writeInt16Array,
    readInt16Array,
};

static asynInt32Array ifaceInt32Array = {
    writeInt32Array,
    readInt32Array,
};

static asynFloat32Array ifaceFloat32Array = {
    writeFloat32Array,
    readFloat32Array,
};

static asynFloat64Array ifaceFloat64Array = {
    writeFloat64Array,
    readFloat64Array,
};


/* Configuration routine.  Called directly, or from the iocsh function in drvNDStdArraysEpics */

extern "C" int drvNDStdArraysConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                                       const char *NDArrayPort, int NDArrayAddr)
{
    new NDPluginStdArrays(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr);
    return(asynSuccess);
}

NDPluginStdArrays::NDPluginStdArrays(const char *portName, int queueSize, int blockingCallbacks, 
                                     const char *NDArrayPort, int NDArrayAddr)
    /* Invoke the base class constructor */
    : NDPluginBase(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, NDPluginStdArraysLastParam)
{
    asynStatus status;
    char *functionName = "NDPluginStdArrays";
    asynStandardInterfaces *pInterfaces;

    /* Set addresses of asyn interfaces */
    pInterfaces = &this->asynStdInterfaces;
    
    pInterfaces->int8Array.pinterface     = (void *)&ifaceInt8Array;
    pInterfaces->int16Array.pinterface    = (void *)&ifaceInt16Array;
    pInterfaces->int32Array.pinterface    = (void *)&ifaceInt32Array;
    pInterfaces->float32Array.pinterface  = (void *)&ifaceFloat32Array;
    pInterfaces->float64Array.pinterface  = (void *)&ifaceFloat64Array;

    /* Define which interfaces can generate interrupts */
    pInterfaces->int8ArrayCanInterrupt    = 1;
    pInterfaces->int16ArrayCanInterrupt   = 1;
    pInterfaces->int32ArrayCanInterrupt   = 1;
    pInterfaces->float32ArrayCanInterrupt = 1;
    pInterfaces->float64ArrayCanInterrupt = 1;

    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     this->pasynUser, this);
    if (status != asynSuccess) {
        printf("%s:%s ERROR: Can't register interfaces: %s.\n",
                driverName, functionName, this->pasynUser->errorMessage);
        return;
    }

    /* Connect to our device for asynTrace */
    status = pasynManager->connectDevice(this->pasynUser, portName, 0);
    if (status != asynSuccess) {
        printf("%s:%s:, connectDevice failed\n", driverName, functionName);
        return;
    }
   
    /* Try to connect to the NDrray port */
    status = connectToArrayPort();
}

