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

#include "NDPluginDriver.h"

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static asynParamString_t NDPluginDriverParamString[] = {
    {NDPluginDriverArrayPort,             "NDARRAY_PORT" },
    {NDPluginDriverArrayAddr,             "NDARRAY_ADDR" },
    {NDPluginDriverArrayCounter,          "ARRAY_COUNTER"},
    {NDPluginDriverDroppedArrays,         "DROPPED_ARRAYS" },
    {NDPluginDriverEnableCallbacks,       "ENABLE_CALLBACKS" },
    {NDPluginDriverBlockingCallbacks,     "BLOCKING_CALLBACKS" },
    {NDPluginDriverMinCallbackTime,       "MIN_CALLBACK_TIME" },
    {NDPluginDriverUniqueId,              "UNIQUE_ID" },
    {NDPluginDriverTimeStamp,             "TIME_STAMP" },
    {NDPluginDriverDataType,              "DATA_TYPE" },
    {NDPluginDriverColorMode,             "COLOR_MODE" },
    {NDPluginDriverBayerPattern,          "BAYER_PATTERN" },
    {NDPluginDriverNDimensions,           "ARRAY_NDIMENSIONS"},
    {NDPluginDriverDimensions,            "ARRAY_DIMENSIONS"}
};

#define NUM_ND_PLUGIN_BASE_PARAMS (sizeof(NDPluginDriverParamString)/sizeof(NDPluginDriverParamString[0]))

static const char *driverName="NDPluginDriver";

int NDPluginDriver::createFileName(int maxChars, char *fullFileName)
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
        status |= setIntegerParam(ADFileNumber, fileNumber);
    }
    return(status);   
}

int NDPluginDriver::createFileName(int maxChars, char *filePath, char *fileName)
{
    /* Formats a complete file name from the components defined in ADStdDriverParams.h */
    int status = asynSuccess;
    char fileTemplate[MAX_FILENAME_LEN];
    char name[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;
    
    status |= getStringParam(ADFilePath, maxChars, filePath); 
    status |= getStringParam(ADFileName, sizeof(name), name); 
    status |= getStringParam(ADFileTemplate, sizeof(fileTemplate), fileTemplate); 
    status |= getIntegerParam(ADFileNumber, &fileNumber);
    status |= getIntegerParam(ADAutoIncrement, &autoIncrement);
    if (status) return(status);
    len = epicsSnprintf(fileName, maxChars, fileTemplate, 
                        name, fileNumber);
    if (len < 0) {
        status |= asynError;
        return(status);
    }
    if (autoIncrement) {
        fileNumber++;
        status |= setIntegerParam(ADFileNumber, fileNumber);
        status |= setIntegerParam(ADFileNumber, fileNumber);
    }
    return(status);   
}

void NDPluginDriver::processCallbacks(NDArray *pArray)
{
    int arrayCounter;
    int i, dimsChanged;
    
    getIntegerParam(NDPluginDriverArrayCounter, &arrayCounter);
    arrayCounter++;
    setIntegerParam(NDPluginDriverArrayCounter, arrayCounter);
    setIntegerParam(NDPluginDriverNDimensions, pArray->ndims);
    setIntegerParam(NDPluginDriverDataType, pArray->dataType);
    setIntegerParam(NDPluginDriverColorMode, pArray->colorMode);
    setIntegerParam(NDPluginDriverBayerPattern, pArray->bayerPattern);
    setIntegerParam(NDPluginDriverUniqueId, pArray->uniqueId);
    setDoubleParam(NDPluginDriverTimeStamp, pArray->timeStamp);
    /* See if the array dimensions have changed.  If so then do callbacks on them. */
    for (i=0, dimsChanged=0; i<ND_ARRAY_MAX_DIMS; i++) {
        if (pArray->dims[i].size != this->dimsPrev[i]) {
            this->dimsPrev[i] = pArray->dims[i].size;
            dimsChanged = 1;
        }
    }
    if (dimsChanged) {
        doCallbacksInt32Array(this->dimsPrev, ND_ARRAY_MAX_DIMS, NDPluginDriverDimensions, 0);
    }
}

extern "C" {static void driverCallback(void *drvPvt, asynUser *pasynUser, void *genericPointer)
{
    NDPluginDriver *pNDPluginDriver = (NDPluginDriver *)drvPvt;
    pNDPluginDriver->driverCallback(pasynUser, genericPointer);
}}

void NDPluginDriver::driverCallback(asynUser *pasynUser, void *genericPointer)
{
    /* This callback function is called from the detector driver when a new array arrives.
     * It calls the processCallbacks function.
     * It can either do the callbacks directly (if BlockingCallbacks=1) or by queueing
     * the arrays to be processed by a background task (if BlockingCallbacks=0).
     * In the latter case arrays can be dropped if the queue is full.
     */
     
    NDArray *pArray = (NDArray *)genericPointer;
    epicsTimeStamp tNow;
    double minCallbackTime, deltaTime;
    int status=0;
    int blockingCallbacks;
    int arrayCounter, droppedArrays;
    const char *functionName = "driverCallback";

    epicsMutexLock(mutexId);

    status |= getDoubleParam(NDPluginDriverMinCallbackTime, &minCallbackTime);
    status |= getIntegerParam(NDPluginDriverArrayCounter, &arrayCounter);
    status |= getIntegerParam(NDPluginDriverDroppedArrays, &droppedArrays);
    status |= getIntegerParam(NDPluginDriverBlockingCallbacks, &blockingCallbacks);
    
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
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                    "%s:%s message queue full, dropped array %d\n",
                    driverName, functionName, arrayCounter);
                droppedArrays++;
                status |= setIntegerParam(NDPluginDriverDroppedArrays, droppedArrays);
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
    NDPluginDriver *pPvt = (NDPluginDriver *)drvPvt;
    
    pPvt->processTask();
}

void NDPluginDriver::processTask(void)
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

asynStatus NDPluginDriver::setArrayInterrupt(int enableCallbacks)
{
    asynStatus status = asynSuccess;
    const char *functionName = "setArrayInterrupt";
    
    /* Lock the port.  May not be necessary to do this. */
    status = pasynManager->lockPort(this->pasynUserGenericPointer);
    if (status != asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock array port: %s\n",
            driverName, functionName, this->pasynUserGenericPointer->errorMessage);
        return(status);
    }
    if (enableCallbacks) {
        status = this->pasynGenericPointer->registerInterruptUser(
                    this->asynGenericPointerPvt, this->pasynUserGenericPointer,
                    ::driverCallback, this, &this->asynGenericPointerInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't register for interrupt callbacks on detector port: %s\n",
                driverName, functionName, this->pasynUserGenericPointer->errorMessage);
            return(status);
        }
    } else {
        if (this->asynGenericPointerInterruptPvt) {
            status = this->pasynGenericPointer->cancelInterruptUser(this->asynGenericPointerPvt, 
                            this->pasynUserGenericPointer, this->asynGenericPointerInterruptPvt);
            this->asynGenericPointerInterruptPvt = NULL;
            if (status != asynSuccess) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: Can't unregister for interrupt callbacks on detector port: %s\n",
                    driverName, functionName, this->pasynUserGenericPointer->errorMessage);
                return(status);
            }
        }
    }
    /* Unlock the port.  May not be necessary to do this. */
    status = pasynManager->unlockPort(this->pasynUserGenericPointer);
    if (status != asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock array port: %s\n",
            driverName, functionName, this->pasynUserGenericPointer->errorMessage);
        return(status);
    }
    return(asynSuccess);
}

asynStatus NDPluginDriver::connectToArrayPort(void)
{
    asynStatus status;
    asynInterface *pasynInterface;
    int isConnected;
    int enableCallbacks;
    char arrayPort[20];
    int arrayAddr;
    const char *functionName = "connectToArrayPort";

    getStringParam(NDPluginDriverArrayPort, sizeof(arrayPort), arrayPort);
    getIntegerParam(NDPluginDriverArrayAddr, &arrayAddr);
    getIntegerParam(NDPluginDriverEnableCallbacks, &enableCallbacks);
    status = pasynManager->isConnected(this->pasynUserGenericPointer, &isConnected);
    if (status) isConnected=0;

    /* If we are currently connected cancel interrupt request */    
    if (isConnected) {       
        status = setArrayInterrupt(0);
    }
    
    /* Disconnect the array port from our asynUser.  Ignore error if there is no device
     * currently connected. */
    pasynManager->exceptionCallbackRemove(this->pasynUserGenericPointer);
    pasynManager->disconnect(this->pasynUserGenericPointer);

    /* Connect to the array port driver */
    status = pasynManager->connectDevice(this->pasynUserGenericPointer, arrayPort, arrayAddr);
    if (status != asynSuccess) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s ERROR: Can't connect to array port %s address %d: %s\n",
                  driverName, functionName, arrayPort, arrayAddr, this->pasynUserGenericPointer->errorMessage);
        pasynManager->exceptionDisconnect(this->pasynUserSelf);
        return (status);
    }

    /* Find the asynGenericPointer interface in that driver */
    pasynInterface = pasynManager->findInterface(this->pasynUserGenericPointer, asynGenericPointerType, 1);
    if (!pasynInterface) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::connectToPort ERROR: Can't find asynGenericPointer interface on array port %s address %d\n",
                  driverName, arrayPort, arrayAddr);
        pasynManager->exceptionDisconnect(this->pasynUserSelf);
        return(asynError);
    }
    this->pasynGenericPointer = (asynGenericPointer *)pasynInterface->pinterface;
    this->asynGenericPointerPvt = pasynInterface->drvPvt;
    pasynManager->exceptionConnect(this->pasynUserSelf);

    /* Enable or disable interrupt callbacks */
    status = setArrayInterrupt(enableCallbacks);

    return(status);
}   


asynStatus NDPluginDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    int isConnected;
    int currentlyPosting;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);

    /* See if we are connected */
    status = pasynManager->isConnected(this->pasynUserGenericPointer, &isConnected);
    if (status) {isConnected=0; status=asynSuccess;}

    /* See if we are currently getting callbacks so we don't add more than 1 callback request */
    currentlyPosting = (this->asynGenericPointerInterruptPvt != NULL);

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(addr, function, value);

    switch(function) {
        case NDPluginDriverEnableCallbacks:
            if (value) {  
                if (isConnected && !currentlyPosting) {
                    /* We need to register to be called with interrupts from the detector driver on 
                     * the asynGenericPointer interface. */
                    status = setArrayInterrupt(1);
                }
            } else {
                /* If we are currently connected and there is a callback registered, cancel it */    
                if (isConnected && currentlyPosting) {
                    status = setArrayInterrupt(0);
                }
            }
            break;
       case NDPluginDriverArrayAddr:
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
    return status;
}


asynStatus NDPluginDriver::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
    int addr=0;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "writeOctet";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(addr, function, (char *)value);

    switch(function) {
        case NDPluginDriverArrayPort:
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
              "%s:%s: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *nActual = nChars;
    return status;
}

asynStatus NDPluginDriver::readInt32Array(asynUser *pasynUser, epicsInt32 *value, 
                                         size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    int addr=0;
    size_t ncopy;
    asynStatus status = asynSuccess;
    const char *functionName = "readInt32Array";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    switch(function) {
        case NDPluginDriverDimensions:
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
              "%s:%s: function=%d\n", 
              driverName, functionName, function);
    return status;
}
    

asynStatus NDPluginDriver::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    int status;
    int param;
    const char *functionName = "drvUserCreate";

    /* See if this parameter is defined for the NDPluginDriver class */
    status = findParam(NDPluginDriverParamString, NUM_ND_PLUGIN_BASE_PARAMS, drvInfo, &param);

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
NDPluginDriver::NDPluginDriver(const char *portName, int queueSize, int blockingCallbacks, 
                           const char *NDArrayPort, int NDArrayAddr, int maxAddr, int paramTableSize,
                           int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask)

    : asynNDArrayDriver(portName, maxAddr, paramTableSize, maxBuffers, maxMemory,
          interfaceMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynInt32ArrayMask | asynDrvUserMask,
          interruptMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynInt32ArrayMask)    
{
    asynStatus status;
    const char *functionName = "NDPluginDriver";
    asynUser *pasynUser;

    /* Initialize some members to 0 */
    memset(&this->lastProcessTime, 0, sizeof(this->lastProcessTime));
    memset(&this->dimsPrev, 0, sizeof(this->dimsPrev));
    this->pasynGenericPointer = NULL;
    this->asynGenericPointerPvt = NULL;
    this->asynGenericPointerInterruptPvt = NULL;
       
    /* Create asynUser for communicating with NDArray port */
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->userPvt = this;
    this->pasynUserGenericPointer = pasynUser;
    this->pasynUserGenericPointer->reason = NDArrayData;

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
    setStringParam (ADPortNameSelf, portName);
    setStringParam (NDPluginDriverArrayPort, NDArrayPort);
    setIntegerParam(NDPluginDriverArrayAddr, NDArrayAddr);
    setIntegerParam(NDPluginDriverArrayCounter, 0);
    setIntegerParam(NDPluginDriverDroppedArrays, 0);
    setIntegerParam(NDPluginDriverEnableCallbacks, 0);
    setIntegerParam(NDPluginDriverBlockingCallbacks, 0);
    setDoubleParam (NDPluginDriverMinCallbackTime, 0.);
    setIntegerParam(NDPluginDriverUniqueId, 0);
    setDoubleParam (NDPluginDriverTimeStamp, 0.);
    setIntegerParam(NDPluginDriverDataType, 0);
    setIntegerParam(NDPluginDriverColorMode, 0);
    setIntegerParam(NDPluginDriverBayerPattern, 0);
    setIntegerParam(NDPluginDriverNDimensions, 0);
}

