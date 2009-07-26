/*
 * NDPluginStdArrays.cpp
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
#include <iocsh.h>
#include <epicsExport.h>

#include "NDArray.h"
#include "NDPluginStdArrays.h"

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static asynParamString_t NDPluginStdArraysParamString[] = {
    {NDPluginStdArraysData,               "STD_ARRAY_DATA"}
};

#define NUM_ND_PLUGIN_STD_ARRAYS_PARAMS (sizeof(NDPluginStdArraysParamString)/sizeof(NDPluginStdArraysParamString[0]))

static const char *driverName="NDPluginStdArrays";

template <typename epicsType, typename interruptType>
void arrayInterruptCallback(NDArray *pArray, NDArrayPool *pNDArrayPool, 
                            void *interruptPvt, int *initialized, NDDataType_t signedType)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    int i;
    int status;
    epicsType *pData=NULL;
    NDArray *pOutput=NULL;
    NDArrayInfo_t arrayInfo;
    NDDimension_t outDims[ND_ARRAY_MAX_DIMS];

    pasynManager->interruptStart(interruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        interruptType *pInterrupt = (interruptType *)pnode->drvPvt;
        if (pInterrupt->pasynUser->reason == NDPluginStdArraysData) {
            if (!*initialized) {
                *initialized = 1;
                pArray->getInfo(&arrayInfo);
                for (i=0; i<pArray->ndims; i++)  {
                    pArray->initDimension(&outDims[i], pArray->dims[i].size);
                }
                status = pNDArrayPool->convert(pArray, &pOutput,
                                               signedType,
                                               outDims);
                if (status) {
                    asynPrint(pInterrupt->pasynUser, ASYN_TRACE_ERROR,
                              "%s::arrayInterruptCallback: error allocating array in convert()\n",
                               driverName);
                    break;
                }
                pData = (epicsType *)pOutput->pData;
            }
            pInterrupt->callback(pInterrupt->userPvt,
                                 pInterrupt->pasynUser,
                                 pData, arrayInfo.nElements);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(interruptPvt);
    if (pOutput) pOutput->release();
}

template <typename epicsType> 
asynStatus NDPluginStdArrays::readArray(asynUser *pasynUser, epicsType *value, size_t nElements, size_t *nIn, NDDataType_t outputType)
{
    int command = pasynUser->reason;
    asynStatus status = asynSuccess;
    NDArray *pOutput, *myArray;
    NDArrayInfo_t arrayInfo;
    NDDimension_t outDims[ND_ARRAY_MAX_DIMS];
    int i;

    myArray = this->pArrays[0];
    switch(command) {
        case NDPluginStdArraysData:
            /* If there is valid data available we already have a copy of it.
             * No need to call driver just copy the data from our buffer */
            if (!myArray || !myArray->pData) {
                status = asynError;
                break;
            }
            myArray->getInfo(&arrayInfo);
            if (arrayInfo.nElements > (int)nElements) {
                /* We have been requested fewer pixels than we have.
                 * Just pass the first nElements. */
                 arrayInfo.nElements = nElements;
            }
            /* Convert data from its actual data type.  */
            for (i=0; i<myArray->ndims; i++)  {
                myArray->initDimension(&outDims[i], myArray->dims[i].size);
            }
            status = (asynStatus)this->pNDArrayPool->convert(myArray,
                                                             &pOutput,
                                                             outputType,
                                                             outDims);
            if (status) {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                          "%s::readArray: error allocating array in convert()\n",
                           driverName);
                break;
            }
            /* Copy the data */
            *nIn = arrayInfo.nElements;
            memcpy(value, pOutput->pData, *nIn*sizeof(epicsType));
            pOutput->release();
            break;
        default:
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                      "%s::readArray, unknown command %d",
                      driverName, command);
            status = asynError;
    }
    return(status);
}



/** Callback function that is called by the NDArray driver with new NDArray data.
  * It does callbacks with the array data to any registered asyn clients on any
  * of the asynXXXArray interfaces.  It converts the array data to the type required for that
  * interface.
  * \param[in] pArray  The NDArray from the callback.
  */ 
void NDPluginStdArrays::processCallbacks(NDArray *pArray)
{
    /* This function calls back any registered clients on the standard asyn array interfaces with 
     * the data in our private buffer.
     * It is called with the mutex already locked.
     */
     
    int int8Initialized=0;
    int int16Initialized=0;
    int int32Initialized=0;
    int float32Initialized=0;
    int float64Initialized=0;
    NDArrayInfo_t arrayInfo;
    asynStandardInterfaces *pInterfaces = &this->asynStdInterfaces;
    /* const char* functionName = "NDStdArraysDoCallbacks"; */

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    pArray->getInfo(&arrayInfo);
 
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing pPvt */
    this->unlock();

    /* Pass interrupts for int8Array data*/
    arrayInterruptCallback<epicsInt8, asynInt8ArrayInterrupt>(pArray, this->pNDArrayPool, 
                             pInterfaces->int8ArrayInterruptPvt,
                             &int8Initialized, NDInt8);
    
    /* Pass interrupts for int16Array data*/
    arrayInterruptCallback<epicsInt16,  asynInt16ArrayInterrupt>(pArray, this->pNDArrayPool, 
                             pInterfaces->int16ArrayInterruptPvt,
                             &int16Initialized, NDInt16);
    
    /* Pass interrupts for int32Array data*/
    arrayInterruptCallback<epicsInt32, asynInt32ArrayInterrupt>(pArray, this->pNDArrayPool, 
                             pInterfaces->int32ArrayInterruptPvt,
                             &int32Initialized, NDInt32);
    
    /* Pass interrupts for float32Array data*/
    arrayInterruptCallback<epicsFloat32, asynFloat32ArrayInterrupt>(pArray, this->pNDArrayPool, 
                             pInterfaces->float32ArrayInterruptPvt,
                             &float32Initialized, NDFloat32);
    
    /* Pass interrupts for float64Array data*/
    arrayInterruptCallback<epicsFloat64, asynFloat64ArrayInterrupt>(pArray, this->pNDArrayPool, 
                             pInterfaces->float64ArrayInterruptPvt,
                             &float64Initialized, NDFloat64);

    /* We must exit with the mutex locked */
    this->lock();
    /* We always keep the last array so read() can use it.  
     * Release previous one, reserve new one */
    if (this->pArrays[0]) this->pArrays[0]->release();
    pArray->reserve();
    this->pArrays[0] = pArray;
    /* Update the parameters.  The counter should be updated after data are posted
     * because clients might use that to detect new data */
    callParamCallbacks();
    /* Release this array */
    pArray->release();
}


/** Called when asyn clients call pasynInt8Array->read().
  * Converts the last NDArray callback data to epicsInt8 (if necessary) and returns it.  
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readInt8Array(asynUser *pasynUser, epicsInt8 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt8>(pasynUser, value, nElements, nIn, NDInt8));
}

/** Called when asyn clients call pasynInt16Array->read().
  * Converts the last NDArray callback data to epicsInt16 (if necessary) and returns it.  
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readInt16Array(asynUser *pasynUser, epicsInt16 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt16>(pasynUser, value, nElements, nIn, NDInt16));
}

/** Called when asyn clients call pasynInt32Array->read().
  * Converts the last NDArray callback data to epicsInt32 (if necessary) and returns it.  
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
    asynStatus status;
    status = readArray<epicsInt32>(pasynUser, value, nElements, nIn, NDInt32);
    if (status != asynSuccess) 
        status = NDPluginDriver::readInt32Array(pasynUser, value, nElements, nIn);
    return(status);
    
}

/** Called when asyn clients call pasynFloat32Array->read().
  * Converts the last NDArray callback data to epicsFloat32 (if necessary) and returns it.  
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsFloat32>(pasynUser, value, nElements, nIn, NDFloat32));
}

/** Called when asyn clients call pasynFloat64Array->read().
  * Converts the last NDArray callback data to epicsFloat64 (if necessary) and returns it.  
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStdArrays::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
{
    return(readArray<epicsFloat64>(pasynUser, value, nElements, nIn, NDFloat64));
}


/* asynDrvUser interface methods */
/** Sets pasynUser->reason to one of the enum values for the parameters defined for
  * this class if the drvInfo field matches one the strings defined for it.
  * If the parameter is not recognized by this class then calls NDPluginDriver::drvUserCreate.
  * Uses asynPortDriver::drvUserCreateParam.
  * \param[in] pasynUser pasynUser structure that driver modifies
  * \param[in] drvInfo String containing information about what driver function is being referenced
  * \param[out] pptypeName Location in which driver puts a copy of drvInfo.
  * \param[out] psize Location where driver puts size of param 
  * \return Returns asynSuccess if a matching string was found, asynError if not found. */
asynStatus NDPluginStdArrays::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    asynStatus status;
    //const char *functionName = "drvUserCreate";
    
    status = this->drvUserCreateParam(pasynUser, drvInfo, pptypeName, psize, 
                                      NDPluginStdArraysParamString, NUM_ND_PLUGIN_STD_ARRAYS_PARAMS);

    /* If not, then call the base class method, see if it is known there */
    if (status) status = NDPluginDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return(status);
}


/** Constructor for NDPluginStdArrays; all parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * This plugin cannot block (ASYN_CANBLOCK=0) and is not multi-device (ASYN_MULTIDEVICE=0).
  * It allocates a maximum of 2 NDArray buffers for internal use.
  *  parameters defined in ADStdDriverParams.h.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginStdArrays::NDPluginStdArrays(const char *portName, int queueSize, int blockingCallbacks, 
                                     const char *NDArrayPort, int NDArrayAddr, size_t maxMemory,
                                     int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, 1, NDPluginStdArraysLastParam, 2, maxMemory,
                   
                   asynInt8ArrayMask | asynInt16ArrayMask | asynInt32ArrayMask | 
                   asynFloat32ArrayMask | asynFloat64ArrayMask,
                   
                   asynInt8ArrayMask | asynInt16ArrayMask | asynInt32ArrayMask | 
                   asynFloat32ArrayMask | asynFloat64ArrayMask,
                   
                   /* asynFlags is set to 0, because this plugin cannot block and is not multi-device.
                    * It does autoconnect */
                   0, 1, priority, stackSize)
{
    asynStatus status;
    //char *functionName = "NDPluginStdArrays";

    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDPluginStdArrays");

    /* Try to connect to the NDArray port */
    status = connectToArrayPort();
}

/* Configuration routine.  Called directly, or from the iocsh function */
extern "C" int NDStdArraysConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                                       const char *NDArrayPort, int NDArrayAddr, size_t maxMemory,
                                       int priority, int stackSize)
{
    new NDPluginStdArrays(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxMemory,
                          priority, stackSize);
    return(asynSuccess);
}


/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxMemory",iocshArgInt};
static const iocshArg initArg6 = { "priority",iocshArgInt};
static const iocshArg initArg7 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7};
static const iocshFuncDef initFuncDef = {"NDStdArraysConfigure",8,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDStdArraysConfigure(args[0].sval, args[1].ival, args[2].ival, 
                            args[3].sval, args[4].ival, args[5].ival,
                            args[6].ival, args[7].ival);
}

extern "C" void NDStdArraysRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDStdArraysRegister);
}
