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

#include "ADInterface.h"
#include "NDArrayBuff.h"
#include "ADParamLib.h"
#include "ADUtils.h"
#include "NDPluginStdArrays.h"
#include "drvNDStdArrays.h"

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static asynParamString_t NDPluginStdArraysParamString[] = {
    {NDPluginStdArraysData,               "STD_ARRAY_DATA"}
};

#define NUM_ND_PLUGIN_STD_ARRAYS_PARAMS (sizeof(NDPluginStdArraysParamString)/sizeof(NDPluginStdArraysParamString[0]))

static char *driverName="NDPluginStdArrays";

template <typename epicsType, typename interruptType>
void arrayInterruptCallback(NDArray_t *pArray, void *interruptPvt, int *initialized, int signedType)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    int i;
    epicsType *pData=NULL;
    NDArray_t *pOutput=NULL;
    NDArrayInfo_t arrayInfo;
    NDDimension_t outDims[ND_ARRAY_MAX_DIMS];

    pasynManager->interruptStart(interruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        interruptType *pInterrupt = (interruptType *)pnode->drvPvt;
        if (pInterrupt->pasynUser->reason == NDPluginStdArraysData) {
            if (!*initialized) {
                *initialized = 1;
                NDArrayBuff->getInfo(pArray, &arrayInfo);
                for (i=0; i<pArray->ndims; i++)  {
                    NDArrayBuff->initDimension(&outDims[i], pArray->dims[i].size);
                }
                NDArrayBuff->convert(pArray, &pOutput,
                                     signedType,
                                     outDims);
                pData = (epicsType *)pOutput->pData;
            }
            pInterrupt->callback(pInterrupt->userPvt,
                                 pInterrupt->pasynUser,
                                 pData, arrayInfo.nElements);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(interruptPvt);
    if (pOutput) NDArrayBuff->release(pOutput);
}

template <typename epicsType> 
asynStatus NDPluginStdArrays::readArray(asynUser *pasynUser, epicsType *value, size_t nElements, size_t *nIn, int outputType)
{
    int command = pasynUser->reason;
    asynStatus status = asynSuccess;
    NDArray_t *pOutput, *myArray;
    NDArrayInfo_t arrayInfo;
    NDDimension_t outDims[ND_ARRAY_MAX_DIMS];
    int i;
    int addr=0;

    epicsMutexLock(this->mutexId);
    myArray = this->pArrays[addr];
    switch(command) {
        case NDPluginStdArraysData:
            NDArrayBuff->getInfo(myArray, &arrayInfo);
            if (arrayInfo.nElements > (int)nElements) {
                /* We have been requested fewer pixels than we have.
                 * Just pass the first nElements. */
                 arrayInfo.nElements = nElements;
            }
            /* We are guaranteed to have the most recent data in our buffer.  No need to call the driver,
             * just copy the data from our buffer */
            /* Convert data from its actual data type.  */
            if (!myArray || !myArray->pData) break;
            for (i=0; i<myArray->ndims; i++)  {
                NDArrayBuff->initDimension(&outDims[i], myArray->dims[i].size);
            }
            status = (asynStatus)NDArrayBuff->convert(myArray,
                                                      &pOutput,
                                                      outputType,
                                                      outDims);
            break;
        default:
            asynPrint(pasynUser, ASYN_TRACE_ERROR,
                      "%s::readArray, unknown command %d\n",
                      driverName, command);
            status = asynError;
    }
    epicsMutexUnlock(this->mutexId);
    return(asynSuccess);
}



void NDPluginStdArrays::processCallbacks(NDArray_t *pArray)
{
    /* This function calls back any registered clients on the standard asyn array interfaces with 
     * the data in our private buffer.
     * It is called with the mutex already locked.
     */
     
    int addr=0;
    int int8Initialized=0;
    int int16Initialized=0;
    int int32Initialized=0;
    int float32Initialized=0;
    int float64Initialized=0;
    NDArrayInfo_t arrayInfo;
    asynStandardInterfaces *pInterfaces = &this->asynStdInterfaces;
    /* const char* functionName = "NDStdArraysDoCallbacks"; */

    /* Call the base class method */
    NDPluginBase::processCallbacks(pArray);
    
    NDArrayBuff->getInfo(pArray, &arrayInfo);
 
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing pPvt */
    epicsMutexUnlock(this->mutexId);

    /* Pass interrupts for int8Array data*/
    arrayInterruptCallback<epicsInt8, asynInt8ArrayInterrupt>(pArray, pInterfaces->int8ArrayInterruptPvt,
                             &int8Initialized, NDInt8);
    
    /* Pass interrupts for int16Array data*/
    arrayInterruptCallback<epicsInt16,  asynInt16ArrayInterrupt>(pArray, pInterfaces->int16ArrayInterruptPvt,
                             &int16Initialized, NDInt16);
    
    /* Pass interrupts for int32Array data*/
    arrayInterruptCallback<epicsInt32, asynInt32ArrayInterrupt>(pArray, pInterfaces->int32ArrayInterruptPvt,
                             &int32Initialized, NDInt32);
    
    /* Pass interrupts for float32Array data*/
    arrayInterruptCallback<epicsFloat32, asynFloat32ArrayInterrupt>(pArray, pInterfaces->float32ArrayInterruptPvt,
                             &float32Initialized, NDFloat32);
    
    /* Pass interrupts for float64Array data*/
    arrayInterruptCallback<epicsFloat64, asynFloat64ArrayInterrupt>(pArray, pInterfaces->float64ArrayInterruptPvt,
                             &float64Initialized, NDFloat64);

    /* We must exit with the mutex locked */
    epicsMutexLock(this->mutexId);
    /* We always keep the last array so read() can use it.  
     * Release previous one, reserve new one */
    if (this->pArrays[addr]) NDArrayBuff->release(this->pArrays[addr]);
    NDArrayBuff->reserve(pArray);
    this->pArrays[addr] = pArray;
    /* Update the parameters.  The counter should be updated after data are posted
     * because clients might use that to detect new data */
    ADParam->callCallbacksAddr(this->params[addr], addr);
}


/* asynXXXArray interface methods */
asynStatus NDPluginStdArrays::readInt8Array(asynUser *pasynUser, epicsInt8 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt8>(pasynUser, value, nElements, nIn, NDInt8));
}

asynStatus NDPluginStdArrays::readInt16Array(asynUser *pasynUser, epicsInt16 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt16>(pasynUser, value, nElements, nIn, NDInt16));
}

asynStatus NDPluginStdArrays::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt32>(pasynUser, value, nElements, nIn, NDInt32));
}

asynStatus NDPluginStdArrays::readFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsFloat32>(pasynUser, value, nElements, nIn, NDFloat32));
}

asynStatus NDPluginStdArrays::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsFloat64>(pasynUser, value, nElements, nIn, NDFloat64));
}


/* asynDrvUser interface methods */
asynStatus NDPluginStdArrays::drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    asynStatus status;
    int param;
    static char *functionName = "drvUserCreate";

    /* First see if this is a parameter specific to this plugin */
    status = findParam(NDPluginStdArraysParamString, NUM_ND_PLUGIN_STD_ARRAYS_PARAMS, 
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
    }

    /* If not, then call the base class */
    status = NDPluginBase::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return(status);
}

    

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
                   NDArrayPort, NDArrayAddr, 1, NDPluginStdArraysLastParam)
{
    asynStatus status;
    char *functionName = "NDPluginStdArrays";

    /* Try to connect to the NDArray port */
    status = connectToArrayPort();
}

