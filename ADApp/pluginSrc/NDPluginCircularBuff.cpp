/*
 * NDPluginCircularBuff.cpp
 *
 * Scope style triggered image recording
 * Author: Alan Greer
 *
 * Created June 21, 2013
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "NDArray.h"
#include "NDPluginCircularBuff.h"
#include <epicsExport.h>

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

static const char *driverName="NDPluginCircularBuff";

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Stores the number of pre-trigger images prior to the trigger in a ring buffer.
  * Once the trigger has been received stores the number of post-trigger buffers
  * and then exposes the buffers.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginCircularBuff::processCallbacks(NDArray *pArray)
{
    /* 
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
    int scopeControl, preCount, postCount, currentImage, currentPostCount, softTrigger;
    NDArray *pArrayCpy = NULL;
    NDAttribute *triggerAttribute;
    NDArrayInfo arrayInfo;
    int triggered = 0;

    const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    pArray->getInfo(&arrayInfo);

    // Retrieve the running state
    getIntegerParam(NDPluginCircularBuffControl,  &scopeControl);
    getIntegerParam(NDPluginCircularBuffPreTrigger,  &preCount);
    getIntegerParam(NDPluginCircularBuffPostTrigger,  &postCount);
    getIntegerParam(NDPluginCircularBuffCurrentImage,  &currentImage);
    getIntegerParam(NDPluginCircularBuffPostCount,  &currentPostCount);
    getIntegerParam(NDPluginCircularBuffSoftTrigger, &softTrigger);

    // Are we running?
    if (scopeControl){

      // Check for a soft trigger
      if (softTrigger){
        triggered = 1;
        setIntegerParam(NDPluginCircularBuffTriggered, triggered);
      } else {
	  getIntegerParam(NDPluginCircularBuffTriggered, &triggered);
	  if (!triggered) { 
	      // Check for the trigger meta-data in the NDArray
	      triggerAttribute = pArray->pAttributeList->find(NDPluginCircularBuffTriggeredAttribute);
	      if (triggerAttribute != NULL){
		  // Read the attribute to see if a trigger happened on this frame
		  triggerAttribute->getValue(NDAttrInt32, (void *)&triggered);
		  setIntegerParam(NDPluginCircularBuffTriggered, triggered);
	      }
	  }
      }
//printf("Triggered: %d\n", triggered);

      // First copy the buffer into our buffer pool so we can release the resource on the driver
      pArrayCpy = this->pNDArrayPool->copy(pArray, NULL, 1);
//printf("pArrayCpy: %d\n", pArrayCpy);

      if (pArrayCpy){

        // Set the Attribute to triggered
        if (softTrigger){
	    pArrayCpy->pAttributeList->add(NDPluginCircularBuffTriggeredAttribute, "External trigger (1 = detected)", NDAttrInt32, (void *)&softTrigger);
        }

        // Have we detected a trigger event yet?
        if (!triggered){
          // No trigger so add the NDArray to the pre-trigger ring
          pOldArray_ = preBuffer_->addToEnd(pArrayCpy);
          // If we overwrote an existing array in the ring, release it here
          if (pOldArray_){
            pOldArray_->release();
            pOldArray_ = NULL;
          }
          // Set the size
          setIntegerParam(NDPluginCircularBuffCurrentImage,  preBuffer_->size());
          if (preBuffer_->size() == preCount){
            setStringParam(NDPluginCircularBuffStatus, "Buffer Wrapping");
          }
        } else {
//printf("Num buffers: %d\n", this->pNDArrayPool->numBuffers());
//printf("Free buffers: %d\n", this->pNDArrayPool->numFree());
          // Trigger detected
          // Start making frames available if trigger has occured
          setStringParam(NDPluginCircularBuffStatus, "Flushing");

          // Has the trigger occured on this frame?
          if (previousTrigger_ == 0){
            previousTrigger_ = 1;
            // Yes, so flush the ring first

            if (preBuffer_->size() > 0){
              this->unlock();
              doCallbacksGenericPointer(preBuffer_->readFromStart(), NDArrayData, 0);
              this->lock();
              while (preBuffer_->hasNext()){
                this->unlock();
                doCallbacksGenericPointer(preBuffer_->readNext(), NDArrayData, 0);
                this->lock();
              }
            }
          }
	  
          currentPostCount++;
          setIntegerParam(NDPluginCircularBuffPostCount,  currentPostCount);

          this->unlock();
          doCallbacksGenericPointer(pArrayCpy, NDArrayData, 0);
          this->lock();
          if (pArrayCpy){
            pArrayCpy->release();
          }
        }

        // Stop recording once we have reached the post-trigger count, wait for a restart
        if (currentPostCount >= postCount){
          setIntegerParam(NDPluginCircularBuffControl, 0);
          setStringParam(NDPluginCircularBuffStatus, "Acquisition Completed");
        }
      } else {
//printf("pArray NULL, failed to copy the data\n");
      }
    } else {
      // Currently do nothing
    }
            

    callParamCallbacks();
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginCircularBuff::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int preCount;
    static const char *functionName = "writeInt32";



    if (function == NDPluginCircularBuffControl){
        if (value == 1){
          // If the control is turned on then create our new ring buffer
          getIntegerParam(NDPluginCircularBuffPreTrigger,  &preCount);
          if (preBuffer_){
            delete preBuffer_;
          }
          preBuffer_ = new NDArrayRing(preCount);
          if (pOldArray_){
            pOldArray_->release();
          }
          pOldArray_ = NULL;
 
          previousTrigger_ = 0;

          // Set the status to buffer filling
          setIntegerParam(NDPluginCircularBuffSoftTrigger, 0);
          setIntegerParam(NDPluginCircularBuffTriggered, 0);
          setIntegerParam(NDPluginCircularBuffPostCount, 0);
          setStringParam(NDPluginCircularBuffStatus, "Buffer filling");
        } else {
          // Control is turned off, before we have finished
          // Set the trigger value off, reset counter
          setIntegerParam(NDPluginCircularBuffSoftTrigger, 0);
          setIntegerParam(NDPluginCircularBuffTriggered, 0);
          setIntegerParam(NDPluginCircularBuffCurrentImage, 0);
          setStringParam(NDPluginCircularBuffStatus, "Acquisition Stopped");
        }

        // Set the parameter in the parameter library.
        status = (asynStatus) setIntegerParam(function, value);

    }  else if (function == NDPluginCircularBuffSoftTrigger){
        // Set the parameter in the parameter library.
        status = (asynStatus) setIntegerParam(function, value);

        // Set a soft trigger
        setIntegerParam(NDPluginCircularBuffTriggered, 1);

    }  else if (function == NDPluginCircularBuffPreTrigger){
        int postCount = 0;
        // Check the number of pre and post do not exceed max buffers
        getIntegerParam(NDPluginCircularBuffPostTrigger,  &postCount);
        if ((postCount + value) > (maxBuffers_ - 1)){
          setStringParam(NDPluginCircularBuffStatus, "Pre count too high");
        } else {
          // Set the parameter in the parameter library.
          status = (asynStatus) setIntegerParam(function, value);
        }
    }  else if (function == NDPluginCircularBuffPostTrigger){
        int preCount = 0;
        // Check the number of pre and post do not exceed max buffers
        getIntegerParam(NDPluginCircularBuffPreTrigger,  &preCount);
        if ((preCount + value) > (maxBuffers_ - 1)){
          setStringParam(NDPluginCircularBuffStatus, "Post count too high");
        } else {
          // Set the parameter in the parameter library.
          status = (asynStatus) setIntegerParam(function, value);
        }
    }  else {

        // Set the parameter in the parameter library.
        status = (asynStatus) setIntegerParam(function, value);

        // If this parameter belongs to a base class call its method
        if (function < FIRST_NDPLUGIN_CIRCULAR_BUFF_PARAM)
            status = NDPluginDriver::writeInt32(pasynUser, value);
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

/** Called when asyn clients call pasynFloat64->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus  NDPluginCircularBuff::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeFloat64";

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(function, value);

    /* If this parameter belongs to a base class call its method */
    if (function < FIRST_NDPLUGIN_CIRCULAR_BUFF_PARAM) status = NDPluginDriver::writeFloat64(pasynUser, value);

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    return status;
}


/** Constructor for NDPluginCircularBuff; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginCircularBuff::NDPluginCircularBuff(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_CIRCULAR_BUFF_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   0, 1, priority, stackSize)
{
    //const char *functionName = "NDPluginCircularBuff";
    preBuffer_ = NULL;

    maxBuffers_ = maxBuffers;

    // Scope
    createParam(NDPluginCircularBuffControlString,      asynParamInt32,      &NDPluginCircularBuffControl);
    createParam(NDPluginCircularBuffStatusString,       asynParamOctet,      &NDPluginCircularBuffStatus);
    createParam(NDPluginCircularBuffPreTriggerString,   asynParamInt32,      &NDPluginCircularBuffPreTrigger);
    createParam(NDPluginCircularBuffPostTriggerString,  asynParamInt32,      &NDPluginCircularBuffPostTrigger);
    createParam(NDPluginCircularBuffCurrentImageString, asynParamInt32,      &NDPluginCircularBuffCurrentImage);
    createParam(NDPluginCircularBuffPostCountString,    asynParamInt32,      &NDPluginCircularBuffPostCount);
    createParam(NDPluginCircularBuffSoftTriggerString,  asynParamInt32,      &NDPluginCircularBuffSoftTrigger);
    createParam(NDPluginCircularBuffTriggeredString,    asynParamInt32,      &NDPluginCircularBuffTriggered);

    // Set the plugin type string
    setStringParam(NDPluginDriverPluginType, "NDPluginCircularBuff");

    // Set the status to idle
    setStringParam(NDPluginCircularBuffStatus, "Idle");

    // Init the current frame count to zero
    setIntegerParam(NDPluginCircularBuffCurrentImage, 0);
    setIntegerParam(NDPluginCircularBuffPostCount, 0);

    // Init the scope control to off
    setIntegerParam(NDPluginCircularBuffControl, 0);

    // Init the pre and post count to 100
    setIntegerParam(NDPluginCircularBuffPreTrigger, 100);
    setIntegerParam(NDPluginCircularBuffPostTrigger, 100);

    // Try to connect to the array port
    connectToArrayPort();
}

/** Configuration command */
extern "C" int NDCircularBuffConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                const char *NDArrayPort, int NDArrayAddr,
                                int maxBuffers, size_t maxMemory)
{
    new NDPluginCircularBuff(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                      maxBuffers, maxMemory, 0, 2000000);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"NDCircularBuffConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDCircularBuffConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival);
}

extern "C" void NDCircularBuffRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDCircularBuffRegister);
}


