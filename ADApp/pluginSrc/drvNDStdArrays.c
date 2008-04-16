/*
 * drvNDStdArrays.c
 * 
 * Asyn driver for callbacks to standard asyn array interfaces for NDArray drivers.
 * This is commonly used for EPICS waveform records.
 *
 * Author: Mark Rivers
 *
 * Created April 2, 2008
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
#include "asynHandle.h"
#include "drvNDStdArrays.h"

#define driverName "drvNDStdArrays"


typedef enum
{
    NDStdArrayPort=ADFirstDriverParam, /* (asynOctet,    r/w) The port for the NDArray driver */
    NDStdArrayAddr,           /* (asynInt32,    r/w) The address on the port */
    NDStdUpdateTime,          /* (asynFloat64,  r/w) Minimum time between array updates */
    NDStdDroppedArrays,       /* (asynInt32,    r/w) Number of dropped arrays */
    NDStdArrayCounter,        /* (asynInt32,    r/w) Number of arrays processed */
    NDStdPostArrays,          /* (asynInt32,    r/w) Post arrays (1=Yes, 0=No) */
    NDStdBlockingCallbacks,   /* (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
    NDStdNDimensions,         /* (asynInt32,    r/o) Number of dimensions in array */
    NDStdDimensions,          /* (asynInt32Array, r/o) Array dimensions */
    NDStdData,                /* (asynXXXArray, r/w) Array data waveform */
    NDStdLastDriverParam
} NDStdArraysParam_t;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t NDStdArraysParamString[] = {
    {NDStdArrayPort,          "NDARRAY_PORT"},
    {NDStdArrayAddr,          "NDARRAY_ADDR"},
    {NDStdUpdateTime,         "ARRAY_UPDATE_TIME"},
    {NDStdDroppedArrays,      "DROPPED_ARRAYS"},
    {NDStdArrayCounter,       "ARRAY_COUNTER"},
    {NDStdPostArrays,         "POST_ARRAYS"},
    {NDStdBlockingCallbacks,  "BLOCKING_CALLBACKS"},
    {NDStdNDimensions,        "ARRAY_NDIMENSIONS"},
    {NDStdDimensions,         "ARRAY_DIMENSIONS"},
    {NDStdData,               "ARRAY_DATA"}
};

#define NUM_STD_ARRAY_PARAMS (sizeof(NDStdArraysParamString)/sizeof(NDStdArraysParamString[0]))

typedef struct drvADPvt {
    /* These fields will be needed by most asyn plug-in drivers */
    char *portName;
    epicsMutexId mutexId;
    epicsMessageQueueId msgQId;
    PARAMS params;
    NDArray_t *pArray;
    
    /* The asyn interfaces this driver implements */
    asynStandardInterfaces asynStdInterfaces;
    
    /* The asyn interfaces we access as a client */
    asynHandle *pasynHandle;
    void *asynHandlePvt;
    void *asynHandleInterruptPvt;
    asynUser *pasynUserHandle;
    
    /* asynUser connected to ourselves for asynTrace */
    asynUser *pasynUser;
    
    /* These fields are specific to the NDStdArrays driver */
    epicsTimeStamp lastArrayPostTime;
    int dimsPrev[ND_ARRAY_MAX_DIMS];
} drvADPvt;


/* Local functions, not in any interface */


static void NDStdArraysDoCallbacks(drvADPvt *pPvt, NDArray_t *pArray)
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
    asynStandardInterfaces *pInterfaces = &pPvt->asynStdInterfaces;
    /* const char* functionName = "NDStdArraysDoCallbacks"; */

    NDArrayBuff->getInfo(pArray, &arrayInfo);
 
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing pPvt */
    epicsMutexUnlock(pPvt->mutexId);

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
            INTERRUPT_TYPE *pInterrupt = pnode->drvPvt; \
            if (pInterrupt->pasynUser->reason == NDStdData) { \
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
    ADUtils->dimensionCallback(pInterfaces->int32ArrayInterruptPvt, pPvt->dimsPrev, 
                               pArray, NDStdDimensions);

    /* We must exit with the mutex locked */
    epicsMutexLock(pPvt->mutexId);
    /* We always keep the last array so read() can use it.  Release it now */
    if (pPvt->pArray) NDArrayBuff->release(pPvt->pArray);
    pPvt->pArray = pArray;
    /* Update the parameters.  The counter should be updated after data are posted
     * because clients might use that to detect new data */
    ADParam->setInteger(pPvt->params, NDStdNDimensions, pArray->ndims);
    ADParam->setInteger(pPvt->params, ADDataType, pArray->dataType);
    ADParam->getInteger(pPvt->params, NDStdArrayCounter, &arrayCounter);    
    arrayCounter++;
    ADParam->setInteger(pPvt->params, NDStdArrayCounter, arrayCounter);    
    ADParam->callCallbacks(pPvt->params);
}

static void NDStdArraysCallback(void *drvPvt, asynUser *pasynUser, void *handle)
{
    /* This callback function is called from the detector driver when a new array arrives.
     * It calls back registered clients on the standard asynXXXArray interfaces.
     * It can either do the callbacks directly (if BlockingCallbacks=1) or by queueing
     * the arrays to be processed by a background task (if BlockingCallbacks=0).
     * In the latter case arrays can be dropped if the queue is full.
     */
     
    drvADPvt *pPvt = drvPvt;
    NDArray_t *pArray = handle;
    epicsTimeStamp tNow;
    double minArrayUpdateTime, deltaTime;
    int status;
    int blockingCallbacks;
    int arrayCounter, droppedArrays;
    char *functionName = "NDStdArraysCallback";

    epicsMutexLock(pPvt->mutexId);

    status |= ADParam->getDouble(pPvt->params, NDStdUpdateTime, &minArrayUpdateTime);
    status |= ADParam->getInteger(pPvt->params, NDStdArrayCounter, &arrayCounter);
    status |= ADParam->getInteger(pPvt->params, NDStdDroppedArrays, &droppedArrays);
    status |= ADParam->getInteger(pPvt->params, NDStdBlockingCallbacks, &blockingCallbacks);
    
    epicsTimeGetCurrent(&tNow);
    deltaTime = epicsTimeDiffInSeconds(&tNow, &pPvt->lastArrayPostTime);

    if (deltaTime > minArrayUpdateTime) {  
        /* Time to post the next array */
        
        /* The callbacks can operate in 2 modes: blocking or non-blocking.
         * If blocking we call NDStdArraysDoCallbacks directly, executing them
         * in the detector callback thread.
         * If non-blocking we put the array on the queue and it executes
         * in our background thread. */
        /* Reserve (increase reference count) on new array */
        NDArrayBuff->reserve(pArray);
        /* Update the time we last posted an array */
        epicsTimeGetCurrent(&tNow);
        memcpy(&pPvt->lastArrayPostTime, &tNow, sizeof(tNow));
        if (blockingCallbacks) {
            NDStdArraysDoCallbacks(pPvt, pArray);
        } else {
            /* Increase the reference count again on this array
             * It will be released in the background task when processing is done */
            NDArrayBuff->reserve(pArray);
            /* Try to put this array on the message queue.  If there is no room then return
             * immediately. */
            status = epicsMessageQueueTrySend(pPvt->msgQId, &pArray, sizeof(&pArray));
            if (status) {
                asynPrint(pasynUser, ASYN_TRACE_FLOW, 
                    "%s:%s message queue full, dropped array %d\n",
                    driverName, functionName, arrayCounter);
                droppedArrays++;
                status |= ADParam->setInteger(pPvt->params, NDStdDroppedArrays, droppedArrays);
                 /* This buffer needs to be released twice, because it never made it onto the queue where
                 * it would be released later */
                NDArrayBuff->release(pArray);
                NDArrayBuff->release(pArray);
            }
        }
    }
    ADParam->callCallbacks(pPvt->params);
    epicsMutexUnlock(pPvt->mutexId);
}


static void NDStdArraysTask(drvADPvt *pPvt)
{
    /* This thread does the callbacks to the clients when a new array arrives */

    /* Loop forever */
    NDArray_t *pArray;
    
    while (1) {
        /* Wait for an array to arrive from the queue */    
        epicsMessageQueueReceive(pPvt->msgQId, &pArray, sizeof(&pArray));
        
        /* Take the lock.  The function we are calling must release the lock
         * during time-consuming operations when it does not need it. */
        epicsMutexLock(pPvt->mutexId);
        /* Call the function that does the callbacks to standard asyn interfaces. */
        NDStdArraysDoCallbacks(pPvt, pArray);
        epicsMutexUnlock(pPvt->mutexId); 
        
        /* We are done with this array buffer */       
        NDArrayBuff->release(pArray);
    }
}

static int setArrayInterrupt(drvADPvt *pPvt, int connect)
{
    int status = asynSuccess;
    const char *functionName = "setArrayInterrupt";
    
    /* Lock the port.  May not be necessary to do this. */
    status = pasynManager->lockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock array port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        return(status);
    }
    if (connect) {
        status = pPvt->pasynHandle->registerInterruptUser(
                    pPvt->asynHandlePvt, pPvt->pasynUserHandle,
                    NDStdArraysCallback, pPvt, &pPvt->asynHandleInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't register for interrupt callbacks on detector port: %s\n",
                driverName, functionName, pPvt->pasynUserHandle->errorMessage);
            return(status);
        }
    } else {
        status = pPvt->pasynHandle->cancelInterruptUser(pPvt->asynHandlePvt, 
                        pPvt->pasynUserHandle, pPvt->asynHandleInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't unregister for interrupt callbacks on detector port: %s\n",
                driverName, functionName, pPvt->pasynUserHandle->errorMessage);
            return(status);
        }
    }
    /* Unlock the port.  May not be necessary to do this. */
    status = pasynManager->unlockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock array port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        return(status);
    }
    return(asynSuccess);
}

static int connectToArrayPort(drvADPvt *pPvt)
{
    asynStatus status;
    asynInterface *pasynInterface;
    int currentlyPosting;
    NDArray_t array;
    int isConnected;
    char arrayPort[20];
    int arrayAddr;
    const char *functionName = "connectToArrayPort";

    ADParam->getString(pPvt->params, NDStdArrayPort, sizeof(arrayPort), arrayPort);
    ADParam->getInteger(pPvt->params, NDStdArrayAddr, &arrayAddr);
    status = ADParam->getInteger(pPvt->params, NDStdPostArrays, &currentlyPosting);
    if (status) currentlyPosting = 0;
    status = pasynManager->isConnected(pPvt->pasynUserHandle, &isConnected);
    if (status) isConnected=0;

    /* If we are currently connected and there is a callback registered, cancel it */    
    if (isConnected && currentlyPosting) {
        status = setArrayInterrupt(pPvt, 0);
    }
    
    /* Disconnect the array port from our asynUser.  Ignore error if there is no device
     * currently connected. */
    pasynManager->exceptionCallbackRemove(pPvt->pasynUserHandle);
    pasynManager->disconnect(pPvt->pasynUserHandle);

    /* Connect to the array port driver */
    status = pasynManager->connectDevice(pPvt->pasynUserHandle, arrayPort, arrayAddr);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::%s ERROR: Can't connect to array port %s address %d: %s\n",
                  driverName, functionName, arrayPort, arrayAddr, pPvt->pasynUserHandle->errorMessage);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return (status);
    }

    /* Find the asynHandle interface in that driver */
    pasynInterface = pasynManager->findInterface(pPvt->pasynUserHandle, asynHandleType, 1);
    if (!pasynInterface) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::connectToPort ERROR: Can't find asynHandle interface on array port %s address %d\n",
                  driverName, arrayPort, arrayAddr);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return(asynError);
    }
    pPvt->pasynHandle = pasynInterface->pinterface;
    pPvt->asynHandlePvt = pasynInterface->drvPvt;
    pasynManager->exceptionConnect(pPvt->pasynUser);

    /* Read the current array parameters from the array driver */
    /* Lock the port. Defintitely necessary to do this. */
    status = pasynManager->lockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock array port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
    }
    /* Read the current array, but only request 0 bytes so no data are actually transferred */
    array.dataSize = 0;
    status = pPvt->pasynHandle->read(pPvt->asynHandlePvt,pPvt->pasynUserHandle, &array);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: reading array data:%s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        status = asynError;
    } else {
        ADParam->setInteger(pPvt->params, NDStdNDimensions, array.ndims);
        ADUtils->dimensionCallback(pPvt->asynStdInterfaces.int32ArrayInterruptPvt, 
                                   pPvt->dimsPrev, &array, NDStdDimensions);
        ADParam->setInteger(pPvt->params, ADDataType, array.dataType);
        ADParam->callCallbacks(pPvt->params);
    }
    /* Unlock the port.  Definitely necessary to do this. */
    status = pasynManager->unlockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock array port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
    }
    
    /* If we are posting enable interrupt callbacks */
    if (currentlyPosting) {
        status = setArrayInterrupt(pPvt, 1);
    }

    return(status);
}   



/* asynInt32 interface methods */
static asynStatus readInt32(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 *value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(pPvt->mutexId);
    
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getInteger(pPvt->params, function, value);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:readInt32 error, status=%d function=%d, value=%d\n", 
                  driverName, status, function, *value);
    else        
        asynPrint(pPvt->pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readInt32: function=%d, value=%d\n", 
              driverName, function, *value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeInt32(void *drvPvt, asynUser *pasynUser, 
                             epicsInt32 value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int isConnected;
    int currentlyPosting;

    epicsMutexLock(pPvt->mutexId);

    /* See if we are connected */
    status = pasynManager->isConnected(pPvt->pasynUserHandle, &isConnected);
    if (status) {isConnected=0; status=asynSuccess;}

    /* Get the current value of NDStdPost arrays, so we don't add more than 1 callback request */
    status = ADParam->getInteger(pPvt->params, NDStdPostArrays, &currentlyPosting);
    if (status) {currentlyPosting = 0; status=asynSuccess;}

    /* Set the parameter in the parameter library. */
    status |= ADParam->setInteger(pPvt->params, function, value);

    switch(function) {
        case NDStdPostArrays:
            if (value) {  
                if (isConnected && !currentlyPosting) {
                    /* We need to register to be called with interrupts from the detector driver on 
                     * the asynHandle interface. */
                    status |= setArrayInterrupt(pPvt, 1);
                }
            } else {
                /* If we are currently connected and there is a callback registered, cancel it */    
                if (isConnected && currentlyPosting) {
                    status |= setArrayInterrupt(pPvt, 0);
                }
            }
            break;
       case NDStdArrayAddr:
            connectToArrayPort(pPvt);
            break;
        default:
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeInt32 error, status=%d function=%d, value=%d\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeInt32: function=%d, value=%d\n", 
              driverName, function, value);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

static asynStatus getBounds(void *drvPvt, asynUser *pasynUser,
                            epicsInt32 *low, epicsInt32 *high)
{
    /* This is only needed for the asynInt32 interface when the device uses raw units.
       Our interface is using engineering units. */
    *low = 0;
    *high = 65535;
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s::getBounds,low=%d, high=%d\n", 
              driverName, *low, *high);
    return(asynSuccess);
}


/* asynFloat64 interface methods */
static asynStatus readFloat64(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 *value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    
    epicsMutexLock(pPvt->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getDouble(pPvt->params, function, value);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:readFloat64 error, status=%d function=%d, value=%f\n", 
              driverName, status, function, *value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readFloat64: function=%d, value=%f\n", 
              driverName, function, *value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeFloat64(void *drvPvt, asynUser *pasynUser, 
                               epicsFloat64 value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char* functionName = "writeFloat64";

    epicsMutexLock(pPvt->mutexId);

    /* Set the parameter in the parameter library. */
    status |= ADParam->setDouble(pPvt->params, function, value);

    switch(function) {
        /* We don't currently need to do anything special when these functions are received */
        default:
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


/* asynOctet interface methods */
static asynStatus readOctet(void *drvPvt, asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
   
    epicsMutexLock(pPvt->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getString(pPvt->params, function, maxChars, value);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:readOctet error, status=%d function=%d, value=%s\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readOctet: function=%d, value=%s\n", 
              driverName, function, value);
    *eomReason = ASYN_EOM_END;
    *nActual = strlen(value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeOctet(void *drvPvt, asynUser *pasynUser,
                             const char *value, size_t nChars, size_t *nActual)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(pPvt->mutexId);
    /* Set the parameter in the parameter library. */
    status |= ADParam->setString(pPvt->params, function, (char *)value);

    switch(function) {
        case NDStdArrayPort:
            connectToArrayPort(pPvt);
        default:
            break;
    }
    
     /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);

    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeOctet error, status=%d function=%d, value=%s\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeOctet: function=%d, value=%s\n", 
              driverName, function, value);
    *nActual = nChars;
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


/* The following macros save a lot of code, since we have 5 array types to support */
#define DEFINE_READ_ARRAY(FUNCTION_NAME, EPICS_TYPE, AD_TYPE) \
static asynStatus FUNCTION_NAME(void *drvPvt, asynUser *pasynUser, \
                                EPICS_TYPE *value, size_t nelements, size_t *nIn) \
{ \
    drvADPvt *pPvt = (drvADPvt *)drvPvt; \
    int command = pasynUser->reason; \
    asynStatus status = asynSuccess; \
    NDArray_t *pOutput; \
    NDArrayInfo_t arrayInfo; \
    NDDimension_t outDims[ND_ARRAY_MAX_DIMS]; \
    int i; \
    int ncopy; \
    int dataType; \
    \
    epicsMutexLock(pPvt->mutexId); \
    switch(command) { \
        case NDStdData: \
            dataType = pPvt->pArray->dataType; \
            NDArrayBuff->getInfo(pPvt->pArray, &arrayInfo); \
            if (arrayInfo.nElements > (int)nelements) { \
                /* We have been requested fewer pixels than we have.  \
                 * Just pass the first nelements. */ \
                 arrayInfo.nElements = nelements; \
            } \
            /* We are guaranteed to have the most recent data in our buffer.  No need to call the driver, \
             * just copy the data from our buffer */ \
            /* Convert data from its actual data type.  */ \
            if (!pPvt->pArray || !pPvt->pArray->pData) break; \
            for (i=0; i<pPvt->pArray->ndims; i++)  {\
                NDArrayBuff->initDimension(&outDims[i], pPvt->pArray->dims[i].size); \
            } \
            status = NDArrayBuff->convert(pPvt->pArray, \
                                          &pOutput, \
                                          AD_TYPE, \
                                          outDims); \
            break; \
        case NDStdDimensions: \
            ncopy = ND_ARRAY_MAX_DIMS; \
            if (nelements < ncopy) ncopy = nelements; \
            memcpy(value, pPvt->dimsPrev, ncopy*sizeof(*pPvt->dimsPrev)); \
            *nIn = ncopy;\
            break;\
        default: \
            asynPrint(pasynUser, ASYN_TRACE_ERROR, \
                      "%s::readArray, unknown command %d\n" \
                      driverName, command); \
            status = asynError; \
    } \
    epicsMutexUnlock(pPvt->mutexId); \
    return(asynSuccess); \
}

#define DEFINE_WRITE_ARRAY(FUNCTION_NAME, EPICS_TYPE, AD_TYPE) \
static asynStatus FUNCTION_NAME(void *drvPvt, asynUser *pasynUser, \
                                EPICS_TYPE *value, size_t nelements) \
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
static asynStatus drvUserCreate(void *drvPvt, asynUser *pasynUser,
                                const char *drvInfo, 
                                const char **pptypeName, size_t *psize)
{
    int status;
    int param;

    /* See if this is one of the standard parameters */
    status = ADUtils->findParam(ADStandardParamString, NUM_AD_STANDARD_PARAMS,
                                drvInfo, &param);

    /* If we did not find it in that table try our driver-specific table */
    if (status) status = ADUtils->findParam(NDStdArraysParamString, NUM_STD_ARRAY_PARAMS, 
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
                  "%s::drvUserCreate, drvInfo=%s, param=%d\n", 
                  driverName, drvInfo, param);
        return(asynSuccess);
    } else {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "%s::drvUserCreate, unknown drvInfo=%s", 
                     driverName, drvInfo);
        return(asynError);
    }
}

    
static asynStatus drvUserGetType(void *drvPvt, asynUser *pasynUser,
                                 const char **pptypeName, size_t *psize)
{
    /* This is not currently supported, because we can't get the strings for driver-specific commands */

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::drvUserGetType entered", driverName);

    *pptypeName = NULL;
    *psize = 0;
    return(asynError);
}

static asynStatus drvUserDestroy(void *drvPvt, asynUser *pasynUser)
{
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::drvUserDestroy, drvPvt=%p, pasynUser=%p\n",
              driverName, drvPvt, pasynUser);

    return(asynSuccess);
}


/* asynCommon interface methods */

static void report(void *drvPvt, FILE *fp, int details)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    interruptNode *pnode;
    ELLLIST *pclientList;
    asynStandardInterfaces *pInterfaces = &pPvt->asynStdInterfaces;

    fprintf(fp, "Port: %s\n", pPvt->portName);
    if (details >= 1) {
        /* Report int32 interrupts */
        pasynManager->interruptStart(pInterfaces->int32InterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynInt32Interrupt *pint32Interrupt = pnode->drvPvt;
            fprintf(fp, "    int32 callback client address=%p, addr=%d, reason=%d\n",
                    pint32Interrupt->callback, pint32Interrupt->addr, 
                    pint32Interrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pInterfaces->int32InterruptPvt);

        /* Report float64 interrupts */
        pasynManager->interruptStart(pInterfaces->float64InterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynFloat64Interrupt *pfloat64Interrupt = pnode->drvPvt;
            fprintf(fp, "    float64 callback client address=%p, addr=%d, reason=%d\n",
                    pfloat64Interrupt->callback, pfloat64Interrupt->addr, 
                    pfloat64Interrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pInterfaces->float64InterruptPvt);

        /* Report asynInt32Array interrupts */
        pasynManager->interruptStart(pInterfaces->int32ArrayInterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynInt32ArrayInterrupt *pint32ArrayInterrupt = pnode->drvPvt;
            fprintf(fp, "    int32Array callback client address=%p, addr=%d, reason=%d\n",
                    pint32ArrayInterrupt->callback, pint32ArrayInterrupt->addr, 
                    pint32ArrayInterrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pInterfaces->int32ArrayInterruptPvt);

    }
    if (details > 5) {
        ADParam->dump(pPvt->params);
        NDArrayBuff->report(details);
    }
}

static asynStatus connect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionConnect(pasynUser);

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::connect, pasynUser=%p\n", 
              driverName, pasynUser);
    return(asynSuccess);
}


static asynStatus disconnect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionDisconnect(pasynUser);
    return(asynSuccess);
}


/* Structures with function pointers for each of the asyn interfaces */
static asynCommon ifaceCommon = {
    report,
    connect,
    disconnect
};

static asynInt32 ifaceInt32 = {
    writeInt32,
    readInt32,
    getBounds
};

static asynFloat64 ifaceFloat64 = {
    writeFloat64,
    readFloat64
};

static asynOctet ifaceOctet = {
    writeOctet,
    NULL,
    readOctet,
};

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

static asynDrvUser ifaceDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};


/* Configuration routine.  Called directly, or from the iocsh function in drvNDStdArraysEpics */

int drvNDStdArraysConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                        const char *arrayPort, int arrayAddr)
{
    drvADPvt *pPvt;
    asynStatus status;
    char *functionName = "drvNDStdArraysConfigure";
    asynStandardInterfaces *pInterfaces;
    asynUser *pasynUser;

    pPvt = callocMustSucceed(1, sizeof(*pPvt), functionName);
    pPvt->portName = epicsStrDup(portName);

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register port\n", functionName);
        return -1;
    }

    /* Create asynUser for debugging and for standardInterfacesBase */
    pPvt->pasynUser = pasynManager->createAsynUser(0, 0);

    /* Set addresses of asyn interfaces */
    pInterfaces = &pPvt->asynStdInterfaces;
    
    pInterfaces->common.pinterface        = (void *)&ifaceCommon;
    pInterfaces->drvUser.pinterface       = (void *)&ifaceDrvUser;
    pInterfaces->octet.pinterface         = (void *)&ifaceOctet;
    pInterfaces->int32.pinterface         = (void *)&ifaceInt32;
    pInterfaces->float64.pinterface       = (void *)&ifaceFloat64;
    pInterfaces->int8Array.pinterface     = (void *)&ifaceInt8Array;
    pInterfaces->int16Array.pinterface    = (void *)&ifaceInt16Array;
    pInterfaces->int32Array.pinterface    = (void *)&ifaceInt32Array;
    pInterfaces->float32Array.pinterface  = (void *)&ifaceFloat32Array;
    pInterfaces->float64Array.pinterface  = (void *)&ifaceFloat64Array;

    /* Define which interfaces can generate interrupts */
    pInterfaces->octetCanInterrupt        = 1;
    pInterfaces->int32CanInterrupt        = 1;
    pInterfaces->float64CanInterrupt      = 1;
    pInterfaces->int8ArrayCanInterrupt    = 1;
    pInterfaces->int16ArrayCanInterrupt   = 1;
    pInterfaces->int32ArrayCanInterrupt   = 1;
    pInterfaces->float32ArrayCanInterrupt = 1;
    pInterfaces->float64ArrayCanInterrupt = 1;

    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     pPvt->pasynUser, pPvt);
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register interfaces: %s.\n",
                functionName, pPvt->pasynUser->errorMessage);
        return -1;
    }
    
    /* Connect to our device for asynTrace */
    status = pasynManager->connectDevice(pPvt->pasynUser, portName, 0);
    if (status != asynSuccess) {
        printf("%s, connectDevice failed\n", functionName);
        return -1;
    }

    /* Create asynUser for communicating with array port */
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->userPvt = pPvt;
    pPvt->pasynUserHandle = pasynUser;

    /* Create the epicsMutex for locking access to data structures from other threads */
    pPvt->mutexId = epicsMutexCreate();
    if (!pPvt->mutexId) {
        printf("%s: epicsMutexCreate failure\n", functionName);
        return asynError;
    }

    /* Create the message queue for the input arrays */
    pPvt->msgQId = epicsMessageQueueCreate(queueSize, sizeof(NDArray_t*));
    if (!pPvt->msgQId) {
        printf("%s: epicsMessageQueueCreate failure\n", functionName);
        return asynError;
    }
    
    /* Initialize the parameter library */
    pPvt->params = ADParam->create(0, NDStdLastDriverParam, &pPvt->asynStdInterfaces);
    if (!pPvt->params) {
        printf("%s: unable to create parameter library\n", functionName);
        return asynError;
    }
        
   /* Create the thread that does the array callbacks */
    status = (epicsThreadCreate("NDStdArraysTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)NDStdArraysTask,
                                pPvt) == NULL);
    if (status) {
        printf("%s: epicsThreadCreate failure\n", functionName);
        return asynError;
    }

    /* Set the initial values of some parameters */
    ADParam->setInteger(pPvt->params, NDStdArrayCounter, 0);
    ADParam->setInteger(pPvt->params, NDStdDroppedArrays, 0);
    ADParam->setString (pPvt->params, NDStdArrayPort, arrayPort);
    ADParam->setInteger(pPvt->params, NDStdArrayAddr, arrayAddr);
    ADParam->setInteger(pPvt->params, NDStdBlockingCallbacks, 0);
   
    /* Try to connect to the array port */
    status = connectToArrayPort(pPvt);
    
    return asynSuccess;
}

