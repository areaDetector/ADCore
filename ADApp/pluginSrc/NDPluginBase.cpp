/*
 * drvNDPlugin.c
 * 
 * Asyn driver for callbacks to save area detector data to files.
 *
 * Author: Mark Rivers
 *
 * Created April 5, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsMessageQueue.h>
#include <cantProceed.h>

#define DEFINE_AD_STANDARD_PARAMS 1
#include "ADStdDriverParams.h"

#include "NDPluginBase.h"

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static asynParamString_t NDPluginBaseParamString[] = {
    {NDPluginBaseArrayPort,             "NDARRAY_PORT" },
    {NDPluginBaseArrayPort_RBV,         "NDARRAY_PORT_RBV" },
    {NDPluginBaseArrayAddr,             "NDARRAY_ADDR" },
    {NDPluginBaseArrayAddr_RBV,         "NDARRAY_ADDR_RBV" },
    {NDPluginBaseArrayCounter,          "ARRAY_COUNTER"},
    {NDPluginBaseArrayCounter_RBV,      "ARRAY_COUNTER_RBV"},
    {NDPluginBaseDroppedArrays,         "DROPPED_ARRAYS" },
    {NDPluginBaseDroppedArrays_RBV,     "DROPPED_ARRAYS_RBV" },
    {NDPluginBaseEnableCallbacks,       "ENABLE_CALLBACKS" },
    {NDPluginBaseEnableCallbacks_RBV,   "ENABLE_CALLBACKS_RBV" },
    {NDPluginBaseBlockingCallbacks,     "BLOCKING_CALLBACKS" },
    {NDPluginBaseBlockingCallbacks_RBV, "BLOCKING_CALLBACKS_RBV" },
    {NDPluginBaseMinCallbackTime,       "MIN_CALLBACK_TIME" },
    {NDPluginBaseMinCallbackTime_RBV,   "MIN_CALLBACK_TIME_RBV" },
    {NDPluginBaseUniqueId_RBV,          "UNIQUE_ID_RBV" },
    {NDPluginBaseTimeStamp_RBV,         "TIME_STAMP_RBV" },
    {NDPluginBaseDataType_RBV,          "DATA_TYPE_RBV" },
    {NDPluginBaseNDimensions_RBV,       "ARRAY_NDIMENSIONS_RBV"},
    {NDPluginBaseDimensions,            "ARRAY_DIMENSIONS"}
};

#define NUM_ND_PLUGIN_BASE_PARAMS (sizeof(NDPluginBaseParamString)/sizeof(NDPluginBaseParamString[0]))

static const char *driverName="NDPluginBase";

int NDPluginBase::createFileName(int maxChars, char *fullFileName)
{
    /* Formats a complete file name from the components defined in ADStdDriverParams.h */
    int status = asynSuccess;
    char filePath[MAX_FILENAME_LEN];
    char fileName[MAX_FILENAME_LEN];
    char fileTemplate[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;
    
    status |= getStringParam(ADFilePath, sizeof(filePath), filePath); 
    status |= getStringParam(ADFileName, sizeof(fileName), fileName); 
    status |= getStringParam(ADFileTemplate, sizeof(fileTemplate), fileTemplate); 
    status |= getIntegerParam(ADFileNumber, &fileNumber);
    status |= getIntegerParam(ADAutoIncrement, &autoIncrement);
    if (status) return(status);
    len = epicsSnprintf(fullFileName, maxChars, fileTemplate, 
                        filePath, fileName, fileNumber);
    if (len < 0) {
        status |= asynError;
        return(status);
    }
    if (autoIncrement) {
        fileNumber++;
        status |= setIntegerParam(ADFileNumber, fileNumber);
        status |= setIntegerParam(ADFileNumber_RBV, fileNumber);
    }
    return(status);   
}

void NDPluginBase::processCallbacks(NDArray *pArray)
{
    int arrayCounter;
    int i, dimsChanged;
    
    getIntegerParam(NDPluginBaseArrayCounter, &arrayCounter);
    arrayCounter++;
    setIntegerParam(NDPluginBaseArrayCounter, arrayCounter);
    setIntegerParam(NDPluginBaseArrayCounter_RBV, arrayCounter);
    setIntegerParam(NDPluginBaseNDimensions_RBV, pArray->ndims);
    setIntegerParam(NDPluginBaseNDimensions_RBV, pArray->ndims);
    setIntegerParam(NDPluginBaseDataType_RBV, pArray->dataType);
    setIntegerParam(NDPluginBaseUniqueId_RBV, pArray->uniqueId);
    setDoubleParam(NDPluginBaseTimeStamp_RBV, pArray->timeStamp);
    /* See if the array dimensions have changed.  If so then do callbacks on them. */
    for (i=0, dimsChanged=0; i<pArray->ndims; i++) {
        if (pArray->dims[i].size != this->dimsPrev[i]) {
            this->dimsPrev[i] = pArray->dims[i].size;
            dimsChanged = 1;
        }
    }
    if (dimsChanged) {
        doCallbacksInt32Array(this->dimsPrev, pArray->ndims, NDPluginBaseDimensions, 0);
    }
}

static void driverCallback(void *drvPvt, asynUser *pasynUser, void *handle)
{
    NDPluginBase *pNDPluginBase = (NDPluginBase *)drvPvt;
    pNDPluginBase->driverCallback(pasynUser, handle);
}

void NDPluginBase::driverCallback(asynUser *pasynUser, void *handle)
{
    /* This callback function is called from the detector driver when a new array arrives.
     * It calls the processCallbacks function.
     * It can either do the callbacks directly (if BlockingCallbacks=1) or by queueing
     * the arrays to be processed by a background task (if BlockingCallbacks=0).
     * In the latter case arrays can be dropped if the queue is full.
     */
     
    NDArray *pArray = (NDArray *)handle;
    epicsTimeStamp tNow;
    double minCallbackTime, deltaTime;
    int status;
    int blockingCallbacks;
    int arrayCounter, droppedArrays;
    char *functionName = "driverCallback";

    epicsMutexLock(mutexId);

    status |= getDoubleParam(NDPluginBaseMinCallbackTime, &minCallbackTime);
    status |= getIntegerParam(NDPluginBaseArrayCounter, &arrayCounter);
    status |= getIntegerParam(NDPluginBaseDroppedArrays, &droppedArrays);
    status |= getIntegerParam(NDPluginBaseBlockingCallbacks, &blockingCallbacks);
    
    epicsTimeGetCurrent(&tNow);
    deltaTime = epicsTimeDiffInSeconds(&tNow, &this->lastProcessTime);

    if (deltaTime > minCallbackTime) {  
        /* Time to process the next array */
        
        /* The callbacks can operate in 2 modes: blocking or non-blocking.
         * If blocking we call processCallbacks directly, executing them
         * in the detector callback thread.
         * If non-blocking we put the array on the queue and it executes
         * in our background thread. */
        /* Update the time we last posted an array */
        epicsTimeGetCurrent(&tNow);
        memcpy(&this->lastProcessTime, &tNow, sizeof(tNow));
        if (blockingCallbacks) {
            processCallbacks(pArray);
        } else {
            /* Increase the reference count again on this array
             * It will be released in the background task when processing is done */
            pArray->reserve();
            /* Try to put this array on the message queue.  If there is no room then return
             * immediately. */
            status = epicsMessageQueueTrySend(this->msgQId, &pArray, sizeof(&pArray));
            if (status) {
                asynPrint(this->pasynUser, ASYN_TRACE_FLOW, 
                    "%s:%s message queue full, dropped array %d\n",
                    driverName, functionName, arrayCounter);
                droppedArrays++;
                status |= setIntegerParam(NDPluginBaseDroppedArrays_RBV, droppedArrays);
                /* This buffer needs to be released */
                pArray->release();
            }
        }
    }
    callParamCallbacks();
    epicsMutexUnlock(this->mutexId);
}



void processTask(void *drvPvt)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    pPvt->processTask();
}

void NDPluginBase::processTask(void)
{
    /* This thread prcoess a new array when it arrives */

    /* Loop forever */
    NDArray *pArray;
    
    while (1) {
        /* Wait for an array to arrive from the queue */    
        epicsMessageQueueReceive(this->msgQId, &pArray, sizeof(&pArray));
        
        /* Take the lock.  The function we are calling must release the lock
         * during time-consuming operations when it does not need it. */
        epicsMutexLock(this->mutexId);
        /* Call the function that does the callbacks to standard asyn interfaces */
        processCallbacks(pArray); 
        epicsMutexUnlock(this->mutexId); 
        
        /* We are done with this array buffer */       
        pArray->release();
    }
}

asynStatus NDPluginBase::setArrayInterrupt(int enableCallbacks)
{
    asynStatus status = asynSuccess;
    const char *functionName = "setArrayInterrupt";
    
    /* Lock the port.  May not be necessary to do this. */
    status = pasynManager->lockPort(this->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock array port: %s\n",
            driverName, functionName, this->pasynUserHandle->errorMessage);
        return(status);
    }
    if (enableCallbacks) {
        status = this->pasynHandle->registerInterruptUser(
                    this->asynHandlePvt, this->pasynUserHandle,
                    ::driverCallback, this, &this->asynHandleInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't register for interrupt callbacks on detector port: %s\n",
                driverName, functionName, this->pasynUserHandle->errorMessage);
            return(status);
        }
    } else {
        if (this->asynHandleInterruptPvt) {
            status = this->pasynHandle->cancelInterruptUser(this->asynHandlePvt, 
                            this->pasynUserHandle, this->asynHandleInterruptPvt);
            this->asynHandleInterruptPvt = NULL;
            if (status != asynSuccess) {
                asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: Can't unregister for interrupt callbacks on detector port: %s\n",
                    driverName, functionName, this->pasynUserHandle->errorMessage);
                return(status);
            }
        }
    }
    /* Unlock the port.  May not be necessary to do this. */
    status = pasynManager->unlockPort(this->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock array port: %s\n",
            driverName, functionName, this->pasynUserHandle->errorMessage);
        return(status);
    }
    return(asynSuccess);
}

asynStatus NDPluginBase::connectToArrayPort(void)
{
    asynStatus status;
    asynInterface *pasynInterface;
    int isConnected;
    int enableCallbacks;
    char arrayPort[20];
    int arrayAddr;
    const char *functionName = "connectToArrayPort";

    getStringParam(NDPluginBaseArrayPort, sizeof(arrayPort), arrayPort);
    getIntegerParam(NDPluginBaseArrayAddr, &arrayAddr);
    getIntegerParam(NDPluginBaseEnableCallbacks, &enableCallbacks);
    status = pasynManager->isConnected(this->pasynUserHandle, &isConnected);
    if (status) isConnected=0;

    /* If we are currently connected cancel interrupt request */    
    if (isConnected) {       
        status = setArrayInterrupt(0);
    }
    
    /* Disconnect the array port from our asynUser.  Ignore error if there is no device
     * currently connected. */
    pasynManager->exceptionCallbackRemove(this->pasynUserHandle);
    pasynManager->disconnect(this->pasynUserHandle);

    /* Connect to the array port driver */
    status = pasynManager->connectDevice(this->pasynUserHandle, arrayPort, arrayAddr);
    if (status != asynSuccess) {
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                  "%s::%s ERROR: Can't connect to array port %s address %d: %s\n",
                  driverName, functionName, arrayPort, arrayAddr, this->pasynUserHandle->errorMessage);
        pasynManager->exceptionDisconnect(this->pasynUser);
        return (status);
    }

    /* Find the asynHandle interface in that driver */
    pasynInterface = pasynManager->findInterface(this->pasynUserHandle, asynHandleType, 1);
    if (!pasynInterface) {
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                  "%s::connectToPort ERROR: Can't find asynHandle interface on array port %s address %d\n",
                  driverName, arrayPort, arrayAddr);
        pasynManager->exceptionDisconnect(this->pasynUser);
        return(asynError);
    }
    this->pasynHandle = (asynHandle *)pasynInterface->pinterface;
    this->asynHandlePvt = pasynInterface->drvPvt;
    pasynManager->exceptionConnect(this->pasynUser);

    /* Enable or disable interrupt callbacks */
    status = setArrayInterrupt(enableCallbacks);

    return(status);
}   


asynStatus NDPluginBase::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    int isConnected;
    int currentlyPosting;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);

    /* See if we are connected */
    status = pasynManager->isConnected(this->pasynUserHandle, &isConnected);
    if (status) {isConnected=0; status=asynSuccess;}

    /* See if we are currently getting callbacks so we don't add more than 1 callback request */
    currentlyPosting = (this->asynHandleInterruptPvt != NULL);

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(addr, function, value);
    /* Set the readback (N+1) entry in the parameter library too */
    status = (asynStatus) setIntegerParam(addr, function+1, value);

    switch(function) {
        case NDPluginBaseEnableCallbacks:
            if (value) {  
                if (isConnected && !currentlyPosting) {
                    /* We need to register to be called with interrupts from the detector driver on 
                     * the asynHandle interface. */
                    status = setArrayInterrupt(1);
                }
            } else {
                /* If we are currently connected and there is a callback registered, cancel it */    
                if (isConnected && currentlyPosting) {
                    status = setArrayInterrupt(0);
                }
            }
            break;
       case NDPluginBaseArrayAddr:
            connectToArrayPort();
            break;
        default:
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks(addr, addr);
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d, isConnected=%d, currentlyPosting=%d\n", 
              driverName, functionName, function, value, isConnected, currentlyPosting);
    epicsMutexUnlock(this->mutexId);
    return status;
}

asynStatus NDPluginBase::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int addr=0;
    const char *functionName = "writeFloat64";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(addr, function, value);
    status = setDoubleParam(addr, function+1, value);

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks(addr, addr);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}


asynStatus NDPluginBase::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
    int addr=0;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "writeOctet";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(addr, function, (char *)value);
    /* Set the readback (N+1) entry in the parameter library too */
    status = (asynStatus) setStringParam(addr, function+1, (char *)value);

    switch(function) {
        case NDPluginBaseArrayPort:
            connectToArrayPort();
        default:
            break;
    }
    
     /* Do callbacks so higher layers see any changes */
    status = (asynStatus)callParamCallbacks(addr, addr);

    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%s", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeOctet: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *nActual = nChars;
    epicsMutexUnlock(this->mutexId);
    return status;
}

asynStatus NDPluginBase::readInt32Array(asynUser *pasynUser, epicsInt32 *value, 
                                         size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    int addr=0;
    size_t ncopy;
    asynStatus status = asynSuccess;
    const char *functionName = "readInt32Array";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);

    switch(function) {
        case NDPluginBaseDimensions:
            ncopy = ND_ARRAY_MAX_DIMS;
            if (nElements < ncopy) ncopy = nElements;
            memcpy(value, this->dimsPrev, ncopy*sizeof(*this->dimsPrev));
            *nIn = ncopy;
            break;
    }
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d", 
                  driverName, functionName, status, function);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeOctet: function=%d\n", 
              driverName, functionName, function);
    epicsMutexUnlock(this->mutexId);
    return status;
}
    

asynStatus NDPluginBase::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    int status;
    int param;
    static char *functionName = "drvUserCreate";

    /* See if this parameter is defined for the NDPluginBase class */
    status = findParam(NDPluginBaseParamString, NUM_ND_PLUGIN_BASE_PARAMS, drvInfo, &param);

    /* If not then try the ADStandard param strings */
    if (status) status = findParam(ADStdDriverParamString, NUM_AD_STANDARD_PARAMS, drvInfo, &param);
    
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

/* Constructor */
NDPluginBase::NDPluginBase(const char *portName, int queueSize, int blockingCallbacks, 
                           const char *NDArrayPort, int NDArrayAddr, int maxAddr, int paramTableSize,
                           int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask)

    : asynNDArrayBase(portName, maxAddr, paramTableSize, maxBuffers, maxMemory,
          interfaceMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynInt32ArrayMask | asynDrvUserMask,
          interruptMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynInt32ArrayMask)    
{
    asynStatus status;
    char *functionName = "NDPluginBase";
    asynUser *pasynUser;

    /* Initialize some members to 0 */
    memset(&this->lastProcessTime, 0, sizeof(this->lastProcessTime));
    memset(&this->dimsPrev, 0, sizeof(this->dimsPrev));
    this->pasynHandle = NULL;
    this->asynHandlePvt = NULL;
    this->asynHandleInterruptPvt = NULL;
       
    /* Create asynUser for communicating with NDArray port */
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->userPvt = this;
    this->pasynUserHandle = pasynUser;
    this->pasynUserHandle->reason = NDArrayData;

    /* Create the message queue for the input arrays */
    this->msgQId = epicsMessageQueueCreate(queueSize, sizeof(NDArray*));
    if (!this->msgQId) {
        printf("%s:%s: epicsMessageQueueCreate failure\n", driverName, functionName);
        return;
    }
    
    /* Create the thread that handles the NDArray callbacks */
    status = (asynStatus)(epicsThreadCreate("NDPluginTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)::processTask,
                                this) == NULL);
    if (status) {
        printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
        return;
    }

    /* Set the initial values of some parameters */
    setStringParam (NDPluginBaseArrayPort, NDArrayPort);
    setStringParam (NDPluginBaseArrayPort_RBV, NDArrayPort);
    setIntegerParam(NDPluginBaseArrayAddr, NDArrayAddr);
    setIntegerParam(NDPluginBaseArrayAddr_RBV, NDArrayAddr);
    setIntegerParam(NDPluginBaseArrayCounter, 0);
    setIntegerParam(NDPluginBaseArrayCounter_RBV, 0);
    setIntegerParam(NDPluginBaseDroppedArrays, 0);
    setIntegerParam(NDPluginBaseDroppedArrays_RBV, 0);
    setIntegerParam(NDPluginBaseEnableCallbacks, 0);
    setIntegerParam(NDPluginBaseEnableCallbacks_RBV, 0);
    setIntegerParam(NDPluginBaseBlockingCallbacks, 0);
    setIntegerParam(NDPluginBaseBlockingCallbacks_RBV, 0);
    setDoubleParam (NDPluginBaseMinCallbackTime, 0.);
    setDoubleParam (NDPluginBaseMinCallbackTime_RBV, 0.);
    setIntegerParam(NDPluginBaseUniqueId_RBV, 0);
    setDoubleParam (NDPluginBaseTimeStamp_RBV, 0.);
    setIntegerParam(NDPluginBaseDataType_RBV, 0);
    setIntegerParam(NDPluginBaseNDimensions_RBV, 0);
}

