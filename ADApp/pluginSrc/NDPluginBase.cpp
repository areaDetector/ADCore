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

#include <asynStandardInterfaces.h>

#include "ADInterface.h"
#include "ADUtils.h"

#include "NDPluginBase.h"

#define driverName "NDPluginBase"

void NDPluginBase::processCallbacks(NDArray_t *pArray)
{
}

static void driverCallbackC(void *drvPvt, asynUser *pasynUser, void *handle)
{
    NDPluginBase *pNDPluginBase = (NDPluginBase *)drvPvt;
    pNDPluginBase->driverCallback(pasynUser, handle);
}

void NDPluginBase::driverCallback(asynUser *pasynUser, void *handle)
{
    /* This callback function is called from the detector driver when a new array arrives.
     * It writes the array in a disk file.
     * It can either do the callbacks directly (if BlockingCallbacks=1) or by queueing
     * the arrays to be processed by a background task (if BlockingCallbacks=0).
     * In the latter case arrays can be dropped if the queue is full.
     */
     
    NDArray_t *pArray = (NDArray_t *)handle;
    epicsTimeStamp tNow;
    double minCallbackTime, deltaTime;
    int status;
    int blockingCallbacks;
    int arrayCounter, droppedArrays;
    char *functionName = "driverCallback";

    epicsMutexLock(mutexId);

    status |= ADParam->getDouble(this->params, NDPluginBaseMinCallbackTime, &minCallbackTime);
    status |= ADParam->getInteger(this->params, NDPluginBaseArrayCounter, &arrayCounter);
    status |= ADParam->getInteger(this->params, NDPluginBaseDroppedArrays, &droppedArrays);
    status |= ADParam->getInteger(this->params, NDPluginBaseBlockingCallbacks, &blockingCallbacks);
    
    epicsTimeGetCurrent(&tNow);
    deltaTime = epicsTimeDiffInSeconds(&tNow, &this->lastProcessTime);

    if (deltaTime > minCallbackTime) {  
        /* Time to process the next array */
        
        /* The callbacks can operate in 2 modes: blocking or non-blocking.
         * If blocking we call processCallbacks directly, executing them
         * in the detector callback thread.
         * If non-blocking we put the array on the queue and it executes
         * in our background thread. */
        /* We always keep the last array so read() can use it. Reserve it. */
        NDArrayBuff->reserve(pArray);
        /* Update the time we last posted an array */
        epicsTimeGetCurrent(&tNow);
        memcpy(&this->lastProcessTime, &tNow, sizeof(tNow));
        if (blockingCallbacks) {
            processCallbacks(pArray);
        } else {
            /* Increase the reference count again on this array
             * It will be released in the background task when processing is done */
            NDArrayBuff->reserve(pArray);
            /* Try to put this array on the message queue.  If there is no room then return
             * immediately. */
            status = epicsMessageQueueTrySend(this->msgQId, &pArray, sizeof(&pArray));
            if (status) {
                asynPrint(this->pasynUser, ASYN_TRACE_FLOW, 
                    "%s:%s message queue full, dropped array %d\n",
                    driverName, functionName, arrayCounter);
                droppedArrays++;
                status |= ADParam->setInteger(this->params, NDPluginBaseDroppedArrays, droppedArrays);
                /* This buffer needs to be released twice, because it never made it onto the queue where
                 * it would be released later */
                NDArrayBuff->release(pArray);
                NDArrayBuff->release(pArray);
            }
        }
    }
    ADParam->callCallbacks(this->params);
    epicsMutexUnlock(this->mutexId);
}



void processTaskC(void *drvPvt)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    pPvt->processTask();
}

void NDPluginBase::processTask(void)
{
    /* This thread prcoess a new array when it arrives */

    /* Loop forever */
    NDArray_t *pArray;
    
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
        NDArrayBuff->release(pArray);
    }
}

asynStatus NDPluginBase::setArrayInterrupt(int connect)
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
    if (connect) {
        status = this->pasynHandle->registerInterruptUser(
                    this->asynHandlePvt, this->pasynUserHandle,
                    driverCallbackC, this, &this->asynHandleInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't register for interrupt callbacks on detector port: %s\n",
                driverName, functionName, this->pasynUserHandle->errorMessage);
            return(status);
        }
    } else {
        status = this->pasynHandle->cancelInterruptUser(this->asynHandlePvt, 
                        this->pasynUserHandle, this->asynHandleInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't unregister for interrupt callbacks on detector port: %s\n",
                driverName, functionName, this->pasynUserHandle->errorMessage);
            return(status);
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
    NDArray_t array;
    int isConnected;
    char arrayPort[20];
    int arrayAddr;
    const char *functionName = "connectToArrayPort";

    ADParam->getString(this->params, NDPluginBaseArrayPort, sizeof(arrayPort), arrayPort);
    ADParam->getInteger(this->params, NDPluginBaseArrayAddr, &arrayAddr);
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

    /* Read the current array parameters from the array driver */
    /* Lock the port. Defintitely necessary to do this. */
    status = pasynManager->lockPort(this->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock array port: %s\n",
            driverName, functionName, this->pasynUserHandle->errorMessage);
        return(status);
    }
    /* Read the current array, but only request 0 bytes so no data are actually transferred */
    array.dataSize = 0;
    status = this->pasynHandle->read(this->asynHandlePvt, this->pasynUserHandle, &array);
    if (status != asynSuccess) {
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: reading array data:%s\n",
            driverName, functionName, this->pasynUserHandle->errorMessage);
    } else {
        ADParam->callCallbacks(this->params);
    }
    /* Unlock the port.  Definitely necessary to do this. */
    status = pasynManager->unlockPort(this->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock array port: %s\n",
            driverName, functionName, this->pasynUserHandle->errorMessage);
    }
    
    /* Enable interrupt callbacks */
    status = setArrayInterrupt(1);

    return(status);
}   



/* asynInt32 interface methods */
static asynStatus readInt32C(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 *value)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->readInt32(pasynUser, value));
}

asynStatus NDPluginBase::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static char *functionName = "readInt32";

    epicsMutexLock(this->mutexId);
    
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = (asynStatus) ADParam->getInteger(this->params, function, value);
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, *value);
    else        
        asynPrint(this->pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, *value);
    epicsMutexUnlock(this->mutexId);
    return(status);
}

static asynStatus writeInt32C(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 value)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->writeInt32(pasynUser, value));
}

asynStatus NDPluginBase::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int isConnected;
    int currentlyPosting;
    const char* functionName = "writeInt32";

    epicsMutexLock(this->mutexId);

    /* See if we are connected */
    status = pasynManager->isConnected(this->pasynUserHandle, &isConnected);
    if (status) {isConnected=0; status=asynSuccess;}

    /* See if we are currently getting callbacks so we don't add more than 1 callback request */
    status = (asynStatus)ADParam->getInteger(this->params, NDPluginBaseEnableCallbacks, &currentlyPosting);
    if (status) {currentlyPosting = 0; status=asynSuccess;}

    /* Set the parameter in the parameter library. */
    status = (asynStatus) ADParam->setInteger(this->params, function, value);

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
    status = (asynStatus) ADParam->callCallbacks(this->params);
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}

static asynStatus getBoundsC(void *drvPvt, asynUser *pasynUser,
                            epicsInt32 *low, epicsInt32 *high)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->getBounds(pasynUser, low, high));
}

asynStatus NDPluginBase::getBounds(asynUser *pasynUser,
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
static asynStatus readFloat64C(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 *value)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->readFloat64(pasynUser, value));
}

asynStatus NDPluginBase::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static char *functionName = "readFloat64";
    
    epicsMutexLock(this->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = (asynStatus) ADParam->getDouble(this->params, function, value);
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%f", 
                  driverName, functionName, status, function, *value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, *value);
    epicsMutexUnlock(this->mutexId);
    return(status);
}

static asynStatus writeFloat64C(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 value)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->writeFloat64(pasynUser, value));
}

asynStatus NDPluginBase::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
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
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacks(this->params);
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%f", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}


/* asynOctet interface methods */
static asynStatus readOctetC(void *drvPvt, asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->readOctet(pasynUser, value, maxChars, nActual, eomReason));
}

asynStatus NDPluginBase::readOctet(asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "readOctet";
   
    epicsMutexLock(this->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = (asynStatus)ADParam->getString(this->params, function, maxChars, value);
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%s", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *eomReason = ASYN_EOM_END;
    *nActual = strlen(value);
    epicsMutexUnlock(this->mutexId);
    return(status);
}

static asynStatus writeOctetC(void *drvPvt, asynUser *pasynUser,
                              const char *value, size_t maxChars, size_t *nActual)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->writeOctet(pasynUser, value, maxChars, nActual));
}

asynStatus NDPluginBase::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "writeOctet";

    epicsMutexLock(this->mutexId);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)ADParam->setString(this->params, function, (char *)value);

    switch(function) {
        case NDPluginBaseArrayPort:
            connectToArrayPort();
        default:
            break;
    }
    
     /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacks(this->params);

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

/* asynHandle interface methods */
static asynStatus readNDArrayC(void *drvPvt, asynUser *pasynUser, void *handle)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->readNDArray(pasynUser, handle));
}

asynStatus NDPluginBase::readNDArray(asynUser *pasynUser, void *handle)
{
    NDArray_t *pArray = (NDArray_t *)handle;
    NDArrayInfo_t arrayInfo;
    asynStatus status = asynSuccess;
    const char* functionName = "readNDArray";
    
    epicsMutexLock(this->mutexId);
    if (!this->pArray) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                    "%s:%s: error, no valid array available, pData=%p", 
                    driverName, functionName, pArray->pData);
        status = asynError;
    } else {
        pArray->ndims = this->pArray->ndims;
        memcpy(pArray->dims, this->pArray->dims, sizeof(pArray->dims));
        pArray->dataType = this->pArray->dataType;
        NDArrayBuff->getInfo(this->pArray, &arrayInfo);
        if (arrayInfo.totalBytes > pArray->dataSize) arrayInfo.totalBytes = pArray->dataSize;
        memcpy(pArray->pData, this->pArray->pData, arrayInfo.totalBytes);
    }
    if (!status) 
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: error, maxBytes=%d, data=%p\n", 
              driverName, functionName, arrayInfo.totalBytes, pArray->pData);
    epicsMutexUnlock(this->mutexId);
    return status;
}

static asynStatus writeNDArrayC(void *drvPvt, asynUser *pasynUser, void *handle)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->writeNDArray(pasynUser, handle));
}

asynStatus NDPluginBase::writeNDArray(asynUser *pasynUser, void *handle)
{
    asynStatus status = asynSuccess;
    
    epicsMutexLock(this->mutexId);
    
    /* Derived classes may do something here */
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacks(this->params);
    
    epicsMutexUnlock(this->mutexId);
    return status;
}




/* asynDrvUser interface methods */
static asynStatus drvUserCreateC(void *drvPvt, asynUser *pasynUser,
                                 const char *drvInfo, 
                                 const char **pptypeName, size_t *psize)
{ 
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->drvUserCreate(pasynUser, drvInfo, pptypeName, psize));
}

asynStatus NDPluginBase::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    int status;
    int param;
    static char *functionName = "drvUserCreate";

    /* If we did not find it in that table try our driver-specific table */
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

    
static asynStatus drvUserGetTypeC(void *drvPvt, asynUser *pasynUser,
                                 const char **pptypeName, size_t *psize)
{ 
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->drvUserGetType(pasynUser, pptypeName, psize));
}

asynStatus NDPluginBase::drvUserGetType(asynUser *pasynUser,
                                        const char **pptypeName, size_t *psize)
{
    /* This is not currently supported, because we can't get the strings for driver-specific commands */
    static char *functionName = "drvUserGetType";

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s: entered", driverName, functionName);

    *pptypeName = NULL;
    *psize = 0;
    return(asynError);
}

static asynStatus drvUserDestroyC(void *drvPvt, asynUser *pasynUser)
{ 
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->drvUserDestroy(pasynUser));
}

asynStatus NDPluginBase::drvUserDestroy(asynUser *pasynUser)
{
    static char *functionName = "drvUserDestroy";

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s: this=%p, pasynUser=%p\n",
              driverName, functionName, this, pasynUser);

    return(asynSuccess);
}


/* asynCommon interface methods */

static void reportC(void *drvPvt, FILE *fp, int details)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->report(fp, details));
}


void NDPluginBase::report(FILE *fp, int details)
{
    interruptNode *pnode;
    ELLLIST *pclientList;
    asynStandardInterfaces *pInterfaces = &this->asynStdInterfaces;

    /* The following macro saves a lot of code */
    #define REPORT_INTERRUPT(INTERRUPT_PVT, INTERRUPT_TYPE, INTERRUPT_TYPE_STRING) { \
        if (INTERRUPT_PVT) { \
            pasynManager->interruptStart(INTERRUPT_PVT, &pclientList); \
            pnode = (interruptNode *)ellFirst(pclientList); \
            while (pnode) { \
                INTERRUPT_TYPE *pInterrupt = (INTERRUPT_TYPE *)pnode->drvPvt; \
                fprintf(fp, "    %s callback client address=%p, addr=%d, reason=%d\n", \
                        INTERRUPT_TYPE_STRING, pInterrupt->callback, pInterrupt->addr, \
                        pInterrupt->pasynUser->reason); \
                pnode = (interruptNode *)ellNext(&pnode->node); \
            } \
            pasynManager->interruptEnd(INTERRUPT_PVT); \
        } \
    }
    fprintf(fp, "Port: %s\n", this->portName);
    if (details >= 1) {
        /* Report interrupt clients */
        REPORT_INTERRUPT(pInterfaces->int32InterruptPvt, asynInt32Interrupt, "int32");
        REPORT_INTERRUPT(pInterfaces->float64InterruptPvt, asynFloat64Interrupt, "float64");
        REPORT_INTERRUPT(pInterfaces->octetInterruptPvt, asynOctetInterrupt, "octet");
        REPORT_INTERRUPT(pInterfaces->int8ArrayInterruptPvt, asynInt8ArrayInterrupt, "int8Array");
        REPORT_INTERRUPT(pInterfaces->int16ArrayInterruptPvt, asynInt16ArrayInterrupt, "int16Array");
        REPORT_INTERRUPT(pInterfaces->int32ArrayInterruptPvt, asynInt32ArrayInterrupt, "int32Array");
        REPORT_INTERRUPT(pInterfaces->float32ArrayInterruptPvt, asynFloat32ArrayInterrupt, "float32Array");
        REPORT_INTERRUPT(pInterfaces->float64ArrayInterruptPvt, asynFloat64ArrayInterrupt, "float64Array");
        REPORT_INTERRUPT(pInterfaces->handleInterruptPvt, asynHandleInterrupt, "handle");
    }
    if (details > 5) {
        NDArrayBuff->report(details);
        ADParam->dump(this->params);
    }
}

static asynStatus connectC(void *drvPvt, asynUser *pasynUser)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->connect(pasynUser));
}

asynStatus NDPluginBase::connect(asynUser *pasynUser)
{
    static char *functionName = "connect";
    
    pasynManager->exceptionConnect(pasynUser);
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s:, pasynUser=%p\n", 
              driverName, functionName, pasynUser);
    return(asynSuccess);
}


static asynStatus disconnectC(void *drvPvt, asynUser *pasynUser)
{
    NDPluginBase *pPvt = (NDPluginBase *)drvPvt;
    
    return(pPvt->disconnect(pasynUser));
}

asynStatus NDPluginBase::disconnect(asynUser *pasynUser)
{
    static char *functionName = "disconnect";
    
    pasynManager->exceptionDisconnect(pasynUser);
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s:, pasynUser=%p\n", 
              driverName, functionName, pasynUser);
    return(asynSuccess);
}


/* Structures with function pointers for each of the asyn interfaces */
static asynCommon ifaceCommon = {
    reportC,
    connectC,
    disconnectC
};

static asynInt32 ifaceInt32 = {
    writeInt32C,
    readInt32C,
    getBoundsC
};

static asynFloat64 ifaceFloat64 = {
    writeFloat64C,
    readFloat64C
};

static asynOctet ifaceOctet = {
    writeOctetC,
    NULL,
    readOctetC,
};

static asynHandle ifaceHandle = {
    writeNDArrayC,
    readNDArrayC
};

static asynDrvUser ifaceDrvUser = {
    drvUserCreateC,
    drvUserGetTypeC,
    drvUserDestroyC
};


/* Configuration routine.  Called directly, or from the iocsh function in drvNDPluginEpics */

NDPluginBase::NDPluginBase(const char *portName, int queueSize, int blockingCallbacks, 
                                const char *NDArrayPort, int NDArrayAddr, int paramTableSize)
{
    asynStatus status;
    char *functionName = "NDPluginBase";
    asynStandardInterfaces *pInterfaces;
    asynUser *pasynUser;

    /* Initialize some members to 0 */
    pInterfaces = &this->asynStdInterfaces;
    memset(pInterfaces, 0, sizeof(asynStdInterfaces));
    memset(&this->lastProcessTime, 0, sizeof(this->lastProcessTime));
    this->pArray = NULL;
    this->pasynHandle = NULL;
    this->asynHandlePvt = NULL;
    this->asynHandleInterruptPvt = NULL;
    this->pasynUserHandle = NULL;
    this->pasynUser = NULL; 
       
    this->portName = epicsStrDup(portName);

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        printf("%s:%s: ERROR: Can't register port\n", driverName, functionName);
    }

    /* Create asynUser for debugging and for standardInterfacesBase */
    this->pasynUser = pasynManager->createAsynUser(0, 0);

     /* Set addresses of asyn interfaces */
    pInterfaces->common.pinterface        = (void *)&ifaceCommon;
    pInterfaces->drvUser.pinterface       = (void *)&ifaceDrvUser;
    pInterfaces->octet.pinterface         = (void *)&ifaceOctet;
    pInterfaces->int32.pinterface         = (void *)&ifaceInt32;
    pInterfaces->float64.pinterface       = (void *)&ifaceFloat64;
    pInterfaces->handle.pinterface        = (void *)&ifaceHandle;

    /* Define which interfaces can generate interrupts */
    pInterfaces->octetCanInterrupt        = 1;
    pInterfaces->int32CanInterrupt        = 1;
    pInterfaces->float64CanInterrupt      = 1;
    pInterfaces->handleCanInterrupt       = 1;

    /* Create asynUser for communicating with NDArray port */
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->userPvt = this;
    this->pasynUserHandle = pasynUser;
    this->pasynUserHandle->reason = NDArrayData;

    /* Create the epicsMutex for locking access to data structures from other threads */
    this->mutexId = epicsMutexCreate();
    if (!this->mutexId) {
        printf("%s: epicsMutexCreate failure\n", functionName);
        return;
    }

    /* Create the message queue for the input arrays */
    this->msgQId = epicsMessageQueueCreate(queueSize, sizeof(NDArray_t*));
    if (!this->msgQId) {
        printf("%s:%s: epicsMessageQueueCreate failure\n", driverName, functionName);
        return;
    }
    
    /* Initialize the parameter library */
    this->params = ADParam->create(0, paramTableSize, &this->asynStdInterfaces);
    if (!this->params) {
        printf("%s:%s: unable to create parameter library\n", driverName, functionName);
        return;
    }

   /* Create the thread that handles the NDArray callbacks */
    status = (asynStatus)(epicsThreadCreate("NDPluginTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)::processTaskC,
                                this) == NULL);
    if (status) {
        printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
        return;
    }

    /* Set the initial values of some parameters */
    ADParam->setInteger(this->params, NDPluginBaseArrayCounter, 0);
    ADParam->setInteger(this->params, NDPluginBaseDroppedArrays, 0);
    ADParam->setString (this->params, NDPluginBaseArrayPort, NDArrayPort);
    ADParam->setInteger(this->params, NDPluginBaseArrayAddr, NDArrayAddr);
    
}

