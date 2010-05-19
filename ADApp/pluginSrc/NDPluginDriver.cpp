/*
 * NDPluginDriver.cpp
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

#include "NDPluginDriver.h"


static const char *driverName="NDPluginDriver";

/** Method that is normally called at the beginning of the processCallbacks
  * method in derived classes.
  * \param[in] pArray  The NDArray from the callback.
  *
  * This method takes care of some bookkeeping for callbacks, updating parameters
  * from data in the class and in the NDArray.  It does asynInt32Array callbacks
  * for the dimensions array if the dimensions of the NDArray data have changed. */ 
    void NDPluginDriver::processCallbacks(NDArray *pArray)
{
    int arrayCounter;
    int i, dimsChanged, size;
    NDAttribute *pAttribute;
    int colorMode=NDColorModeMono, bayerPattern=NDBayerRGGB;
    
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);
    pAttribute = pArray->pAttributeList->find("BayerPattern");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &bayerPattern);
    
    getIntegerParam(NDArrayCounter, &arrayCounter);
    arrayCounter++;
    setIntegerParam(NDArrayCounter, arrayCounter);
    setIntegerParam(NDNDimensions, pArray->ndims);
    setIntegerParam(NDDataType, pArray->dataType);
    setIntegerParam(NDColorMode, colorMode);
    setIntegerParam(NDBayerPattern, bayerPattern);
    setIntegerParam(NDUniqueId, pArray->uniqueId);
    setDoubleParam(NDTimeStamp, pArray->timeStamp);
    /* See if the array dimensions have changed.  If so then do callbacks on them. */
    for (i=0, dimsChanged=0; i<ND_ARRAY_MAX_DIMS; i++) {
        size = pArray->dims[i].size;
        if (i >= pArray->ndims) size = 0;
        if (size != this->dimsPrev[i]) {
            this->dimsPrev[i] = size;
            dimsChanged = 1;
        }
    }
    if (dimsChanged) {
        doCallbacksInt32Array(this->dimsPrev, ND_ARRAY_MAX_DIMS, NDDimensions, 0);
    }
}

extern "C" {static void driverCallback(void *drvPvt, asynUser *pasynUser, void *genericPointer)
{
    NDPluginDriver *pNDPluginDriver = (NDPluginDriver *)drvPvt;
    pNDPluginDriver->driverCallback(pasynUser, genericPointer);
}}

/** Method that is called from the driver with a new NDArray.
  * It calls the processCallbacks function, which typically is implemented in the
  * derived class.
  * It can either do the callbacks directly (if NDPluginDriverBlockingCallbacks=1) or by queueing
  * the arrays to be processed by a background task (if NDPluginDriverBlockingCallbacks=0).
  * In the latter case arrays can be dropped if the queue is full.  This method should really
  * be private, but it must be called from a C-linkage callback function, so it must be public.
  * \param[in] pasynUser  The pasynUser from the asyn client.
  * \param[in] genericPointer The pointer to the NDArray */ 
void NDPluginDriver::driverCallback(asynUser *pasynUser, void *genericPointer)
{
     
    NDArray *pArray = (NDArray *)genericPointer;
    epicsTimeStamp tNow;
    double minCallbackTime, deltaTime;
    int status=0;
    int blockingCallbacks;
    int arrayCounter, droppedArrays;
    const char *functionName = "driverCallback";

    this->lock();

    status |= getDoubleParam(NDPluginDriverMinCallbackTime, &minCallbackTime);
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
                status |= getIntegerParam(NDArrayCounter, &arrayCounter);
                status |= getIntegerParam(NDPluginDriverDroppedArrays, &droppedArrays);
                asynPrint(pasynUser, ASYN_TRACE_FLOW, 
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
    this->unlock();
}



void processTask(void *drvPvt)
{
    NDPluginDriver *pPvt = (NDPluginDriver *)drvPvt;
    
    pPvt->processTask();
}

/** Method runs as a separate thread, waiting for NDArrays to arrive in a message queue
  * and processing them.
  * This thread is used when NDPluginDriverBlockingCallbacks=0.
  * This method should really be private, but it must be called from a 
  * C-linkage callback function, so it must be public. */ 
void NDPluginDriver::processTask(void)
{
    /* This thread processes a new array when it arrives */

    /* Loop forever */
    NDArray *pArray;
    
    while (1) {
        /* Wait for an array to arrive from the queue */    
        epicsMessageQueueReceive(this->msgQId, &pArray, sizeof(&pArray));
        
        /* Take the lock.  The function we are calling must release the lock
         * during time-consuming operations when it does not need it. */
        this->lock();
        /* Call the function that does the callbacks to standard asyn interfaces */
        processCallbacks(pArray); 
        this->unlock();
        
        /* We are done with this array buffer */
        pArray->release();
    }
}

/** Register or unregister to receive asynGenericPointer (NDArray) callbacks from the driver.
  * Note: this function must be called with the lock released, otherwise a deadlock can occur
  * in the call to cancelInterruptUser.
  * \param[in] enableCallbacks 1 to enable callbacks, 0 to disable callbacks */ 
asynStatus NDPluginDriver::setArrayInterrupt(int enableCallbacks)
{
    asynStatus status = asynSuccess;
    const char *functionName = "setArrayInterrupt";
    
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
    return(asynSuccess);
}

/** Connect this plugin to an NDArray port driver; disconnect from any existing driver first, register
  * for callbacks if enabled. */
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


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including NDPluginDriverEnableCallbacks and
  * NDPluginDriverArrayAddr.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    int isConnected;
    int currentlyPosting;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);

    /* See if we are connected */
    status = pasynManager->isConnected(this->pasynUserGenericPointer, &isConnected);
    if (status) {isConnected=0; status=asynSuccess;}

    /* See if we are currently getting callbacks so we don't add more than 1 callback request */
    currentlyPosting = (this->asynGenericPointerInterruptPvt != NULL);

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(addr, function, value);

    if (function == NDPluginDriverEnableCallbacks) {
        if (value) {  
            if (isConnected && !currentlyPosting) {
                /* We need to register to be called with interrupts from the detector driver on 
                 * the asynGenericPointer interface. Must do this with the lock released. */
                this->unlock();
                status = setArrayInterrupt(1);
                this->lock();
            }
        } else {
            /* If we are currently connected and there is a callback registered, cancel it */    
            if (isConnected && currentlyPosting) {
                this->unlock();
                status = setArrayInterrupt(0);
                this->lock();
            }
        }
    } else if (function == NDPluginDriverArrayAddr) {
        this->unlock();
        connectToArrayPort();
        this->lock();
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_PARAM) 
            status = asynNDArrayDriver::writeInt32(pasynUser, value);
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


/** Called when asyn clients call pasynOctet->write().
  * This function performs actions for some parameters, including NDPluginDriverArrayPort.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus NDPluginDriver::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
    int addr=0;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "writeOctet";

    status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(addr, function, (char *)value);

    if (function == NDPluginDriverArrayPort) {
        this->unlock();
        connectToArrayPort();
        this->lock();
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_PARAM) 
            status = asynNDArrayDriver::writeOctet(pasynUser, value, nChars, nActual);
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

/** Called when asyn clients call pasynInt32Array->read().
  * Returns the value of the array dimensions for the last NDArray.  
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginDriver::readInt32Array(asynUser *pasynUser, epicsInt32 *value, 
                                         size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    int addr=0;
    size_t ncopy;
    asynStatus status = asynSuccess;
    const char *functionName = "readInt32Array";

    status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
    if (function == NDDimensions) {
            ncopy = ND_ARRAY_MAX_DIMS;
            if (nElements < ncopy) ncopy = nElements;
            memcpy(value, this->dimsPrev, ncopy*sizeof(*this->dimsPrev));
            *nIn = ncopy;
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_PARAM) 
            status = asynNDArrayDriver::readInt32Array(pasynUser, value, nElements, nIn);
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
    


/** Constructor for NDPluginDriver; most parameters are simply passed to asynNDArrayDriver::asynNDArrayDriver.
  * After calling the base class constructor this method creates a thread to execute the NDArray callbacks, 
  * and sets reasonable default values for all of the parameters defined in NDPluginDriver.h.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxAddr The maximum  number of asyn addr addresses this driver supports. 1 is minimum.
  * \param[in] numParams The number of parameters that the derived class supports.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] interfaceMask Bit mask defining the asyn interfaces that this driver supports.
  * \param[in] interruptMask Bit mask definining the asyn interfaces that can generate interrupts (callbacks)
  * \param[in] asynFlags Flags when creating the asyn port driver; includes ASYN_CANBLOCK and ASYN_MULTIDEVICE.
  * \param[in] autoConnect The autoConnect flag for the asyn port driver.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginDriver::NDPluginDriver(const char *portName, int queueSize, int blockingCallbacks, 
                               const char *NDArrayPort, int NDArrayAddr, int maxAddr, int numParams,
                               int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask,
                               int asynFlags, int autoConnect, int priority, int stackSize)

    : asynNDArrayDriver(portName, maxAddr, numParams+NUM_NDPLUGIN_PARAMS, maxBuffers, maxMemory,
          interfaceMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynInt32ArrayMask | asynDrvUserMask,
          interruptMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynInt32ArrayMask,
          asynFlags, autoConnect, priority, stackSize)    
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
    
    /* We use the same stack size for our callback thread as for the port thread */
    if (stackSize <= 0) stackSize = epicsThreadGetStackSize(epicsThreadStackMedium);

    /* Create the thread that handles the NDArray callbacks */
    status = (asynStatus)(epicsThreadCreate("NDPluginTask",
                          epicsThreadPriorityMedium,
                          stackSize,
                          (EPICSTHREADFUNC)::processTask,
                          this) == NULL);
    if (status) {
        printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
        return;
    }
    createParam(NDPluginDriverArrayPortString,         asynParamOctet, &NDPluginDriverArrayPort);
    createParam(NDPluginDriverArrayAddrString,         asynParamInt32, &NDPluginDriverArrayAddr);
    createParam(NDPluginDriverPluginTypeString,        asynParamOctet, &NDPluginDriverPluginType);
    createParam(NDPluginDriverDroppedArraysString,     asynParamInt32, &NDPluginDriverDroppedArrays);
    createParam(NDPluginDriverEnableCallbacksString,   asynParamInt32, &NDPluginDriverEnableCallbacks);
    createParam(NDPluginDriverBlockingCallbacksString, asynParamInt32, &NDPluginDriverBlockingCallbacks);
    createParam(NDPluginDriverMinCallbackTimeString,   asynParamFloat64, &NDPluginDriverMinCallbackTime);

    /* Here we set the values of read-only parameters and of read/write parameters that cannot
     * or should not get their values from the database.  Note that values set here will override
     * those in the database for output records because if asyn device support reads a value from 
     * the driver with no error during initialization then it sets the output record to that value.  
     * If a value is not set here then the read request will return an error (uninitialized).
     * Values set here will be overridden by values from save/restore if they exist. */
    setStringParam (NDPluginDriverArrayPort, NDArrayPort);
    setIntegerParam(NDPluginDriverArrayAddr, NDArrayAddr);
    setIntegerParam(NDPluginDriverDroppedArrays, 0);
}

