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
#include <epicsMath.h>
#include <iocsh.h>
#include <postfix.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDArray.h"
#include "NDPluginCircularBuff.h"

static const char *driverName="NDPluginCircularBuff";

asynStatus NDPluginCircularBuff::calculateTrigger(NDArray *pArray, int *trig)
{
    NDAttribute *trigger;
    char triggerString[256];
    double triggerValue;
    double calcResult;
    int status;
    int preTrigger, postTrigger, currentImage, triggered;
    static const char *functionName="calculateTrigger";
    
    *trig = 0;
    
    getIntegerParam(NDCircBuffPreTrigger,   &preTrigger);
    getIntegerParam(NDCircBuffPostTrigger,  &postTrigger);
    getIntegerParam(NDCircBuffCurrentImage, &currentImage);
    getIntegerParam(NDCircBuffTriggered,    &triggered);   

    triggerCalcArgs_[0] = epicsNAN;
    triggerCalcArgs_[1] = epicsNAN;
    triggerCalcArgs_[2] = preTrigger;
    triggerCalcArgs_[3] = postTrigger;
    triggerCalcArgs_[4] = currentImage;
    triggerCalcArgs_[5] = triggered;

    getStringParam(NDCircBuffTriggerA, sizeof(triggerString), triggerString);
    trigger = pArray->pAttributeList->find(triggerString);
    if (trigger != NULL) {
        status = trigger->getValue(NDAttrFloat64, &triggerValue);
        if (status == asynSuccess) {
            triggerCalcArgs_[0] = triggerValue;
        }
    }
    getStringParam(NDCircBuffTriggerB, sizeof(triggerString), triggerString);
    trigger = pArray->pAttributeList->find(triggerString);
    if (trigger != NULL) {
        status = trigger->getValue(NDAttrFloat64, &triggerValue);
        if (status == asynSuccess) {
            triggerCalcArgs_[1] = triggerValue;
        }
    }
    
    setDoubleParam(NDCircBuffTriggerAVal, triggerCalcArgs_[0]);
    setDoubleParam(NDCircBuffTriggerBVal, triggerCalcArgs_[1]);
    status = calcPerform(triggerCalcArgs_, &calcResult, triggerCalcPostfix_);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s error evaluating expression=%s\n",
            driverName, functionName, calcErrorStr(status));
        return asynError;
    }
    
    if (!isnan(calcResult) && !isinf(calcResult) && (calcResult != 0)) *trig = 1;
    setDoubleParam(NDCircBuffTriggerCalcVal, calcResult);

    return asynSuccess;
}
    

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
    int presetTriggerCount, actualTriggerCount;
    NDArray *pArrayCpy = NULL;
    NDArrayInfo arrayInfo;
    int triggered = 0;

    //const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    pArray->getInfo(&arrayInfo);

    // Retrieve the running state
    getIntegerParam(NDCircBuffControl,            &scopeControl);
    getIntegerParam(NDCircBuffPreTrigger,         &preCount);
    getIntegerParam(NDCircBuffPostTrigger,        &postCount);
    getIntegerParam(NDCircBuffCurrentImage,       &currentImage);
    getIntegerParam(NDCircBuffPostCount,          &currentPostCount);
    getIntegerParam(NDCircBuffSoftTrigger,        &softTrigger);
    getIntegerParam(NDCircBuffPresetTriggerCount, &presetTriggerCount);
    getIntegerParam(NDCircBuffPresetTriggerCount, &presetTriggerCount);
    getIntegerParam(NDCircBuffActualTriggerCount, &actualTriggerCount);

    // Are we running?
    if (scopeControl) {

      // Check for a soft trigger
      if (softTrigger){
        triggered = 1;
        setIntegerParam(NDCircBuffTriggered, triggered);
      } else {
        getIntegerParam(NDCircBuffTriggered, &triggered);
        if (!triggered) { 
          // Check for the trigger based on meta-data in the NDArray and the trigger calculation
          calculateTrigger(pArray, &triggered);
          setIntegerParam(NDCircBuffTriggered, triggered);
        }
      }

      // First copy the buffer into our buffer pool so we can release the resource on the driver
      pArrayCpy = this->pNDArrayPool->copy(pArray, NULL, 1);

      if (pArrayCpy){

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
          setIntegerParam(NDCircBuffCurrentImage,  preBuffer_->size());
          if (preBuffer_->size() == preCount){
            setStringParam(NDCircBuffStatus, "Buffer Wrapping");
          }
        } else {
          // Trigger detected
          // Start making frames available if trigger has occured
          setStringParam(NDCircBuffStatus, "Flushing");

          // Has the trigger occured on this frame?
          if (previousTrigger_ == 0){
            previousTrigger_ = 1;
            // Yes, so flush the ring first

            if (preBuffer_->size() > 0){
              this->unlock();
              doCallbacksGenericPointer(preBuffer_->readFromStart(), NDArrayData, 0);
              this->lock();
              while (preBuffer_->hasNext()) {
                this->unlock();
                doCallbacksGenericPointer(preBuffer_->readNext(), NDArrayData, 0);
                this->lock();
              }
            }
          }
      
          currentPostCount++;
          setIntegerParam(NDCircBuffPostCount,  currentPostCount);

          this->unlock();
          doCallbacksGenericPointer(pArrayCpy, NDArrayData, 0);
          this->lock();
          if (pArrayCpy){
            pArrayCpy->release();
          }
        }

        // Stop recording once we have reached the post-trigger count, wait for a restart
        if (currentPostCount >= postCount){
          actualTriggerCount++;
          setIntegerParam(NDCircBuffActualTriggerCount, actualTriggerCount);
          if ((presetTriggerCount == 0) ||
              ((presetTriggerCount > 0) && (actualTriggerCount < presetTriggerCount)))
          {
            previousTrigger_ = 0;
            // Set the status to buffer filling
            setIntegerParam(NDCircBuffControl, 1);
            setIntegerParam(NDCircBuffSoftTrigger, 0);
            setIntegerParam(NDCircBuffTriggered, 0);
            setIntegerParam(NDCircBuffPostCount, 0);
            setStringParam(NDCircBuffStatus, "Buffer filling");
          } else {
            setIntegerParam(NDCircBuffTriggered, 0);
            setIntegerParam(NDCircBuffControl, 0);
            setStringParam(NDCircBuffStatus, "Acquisition Completed");
          }
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

    if (function == NDCircBuffControl){
        if (value == 1){
          // If the control is turned on then create our new ring buffer
          getIntegerParam(NDCircBuffPreTrigger,  &preCount);
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
          setIntegerParam(NDCircBuffSoftTrigger, 0);
          setIntegerParam(NDCircBuffTriggered, 0);
          setIntegerParam(NDCircBuffPostCount, 0);
          setIntegerParam(NDCircBuffActualTriggerCount, 0);
          setStringParam(NDCircBuffStatus, "Buffer filling");
        } else {
          // Control is turned off, before we have finished
          // Set the trigger value off, reset counter
          setIntegerParam(NDCircBuffSoftTrigger, 0);
          setIntegerParam(NDCircBuffTriggered, 0);
          setIntegerParam(NDCircBuffCurrentImage, 0);
          setStringParam(NDCircBuffStatus, "Acquisition Stopped");
        }

        // Set the parameter in the parameter library.
        status = (asynStatus) setIntegerParam(function, value);

    }  else if (function == NDCircBuffSoftTrigger){
        // Set the parameter in the parameter library.
        status = (asynStatus) setIntegerParam(function, value);

        // Set a soft trigger
        setIntegerParam(NDCircBuffTriggered, 1);

    }  else if (function == NDCircBuffPreTrigger){
        // Check the value of pretrigger does not exceed max buffers
        if (value > (maxBuffers_ - 1)){
          setStringParam(NDCircBuffStatus, "Pre-count too high");
        } else {
          // Set the parameter in the parameter library.
          status = (asynStatus) setIntegerParam(function, value);
        }
    }  else {

        // Set the parameter in the parameter library.
        status = (asynStatus) setIntegerParam(function, value);

        // If this parameter belongs to a base class call its method
        if (function < FIRST_NDPLUGIN_CIRC_BUFF_PARAM)
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

/** Called when asyn clients call pasynOctet->write().
  * This function performs actions for some parameters, including AttributesFile.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus NDPluginCircularBuff::writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual)
{
  int addr=0;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  short postfixError;
  const char *functionName = "writeOctet";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
  // Set the parameter in the parameter library.
  status = (asynStatus)setStringParam(addr, function, (char *)value);
  if (status != asynSuccess) return(status);

  if (function == NDCircBuffTriggerCalc){
    if (nChars > sizeof(triggerCalcInfix_)) nChars = sizeof(triggerCalcInfix_);
    strncpy(triggerCalcInfix_, value, nChars);
    status = (asynStatus)postfix(triggerCalcInfix_, triggerCalcPostfix_, &postfixError);
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error processing infix expression=%s, error=%s\n",
      driverName, functionName, triggerCalcInfix_, calcErrorStr(postfixError));
    }
  } 
  
  else if (function < FIRST_NDPLUGIN_CIRC_BUFF_PARAM) {
      /* If this parameter belongs to a base class call its method */
      status = NDPluginDriver::writeOctet(pasynUser, value, nChars, nActual);
  }

  // Do callbacks so higher layers see any changes
  callParamCallbacks(addr, addr);

  if (status){
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, value=%s",
                  driverName, functionName, status, function, value);
  } else {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%s\n",
              driverName, functionName, function, value);
  }
  *nActual = nChars;
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
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_CIRC_BUFF_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   0, 1, priority, stackSize), pOldArray_(NULL)
{
    //const char *functionName = "NDPluginCircularBuff";
    preBuffer_ = NULL;

    maxBuffers_ = maxBuffers;

    // Scope
    createParam(NDCircBuffControlString,            asynParamInt32,      &NDCircBuffControl);
    createParam(NDCircBuffStatusString,             asynParamOctet,      &NDCircBuffStatus);
    createParam(NDCircBuffTriggerAString,           asynParamOctet,      &NDCircBuffTriggerA);
    createParam(NDCircBuffTriggerBString,           asynParamOctet,      &NDCircBuffTriggerB);
    createParam(NDCircBuffTriggerAValString,        asynParamFloat64,    &NDCircBuffTriggerAVal);
    createParam(NDCircBuffTriggerBValString,        asynParamFloat64,    &NDCircBuffTriggerBVal);
    createParam(NDCircBuffTriggerCalcString,        asynParamOctet,      &NDCircBuffTriggerCalc);
    createParam(NDCircBuffTriggerCalcValString,     asynParamFloat64,    &NDCircBuffTriggerCalcVal);
    createParam(NDCircBuffPresetTriggerCountString, asynParamInt32,      &NDCircBuffPresetTriggerCount);
    createParam(NDCircBuffActualTriggerCountString, asynParamInt32,      &NDCircBuffActualTriggerCount);
    createParam(NDCircBuffPreTriggerString,         asynParamInt32,      &NDCircBuffPreTrigger);
    createParam(NDCircBuffPostTriggerString,        asynParamInt32,      &NDCircBuffPostTrigger);
    createParam(NDCircBuffCurrentImageString,       asynParamInt32,      &NDCircBuffCurrentImage);
    createParam(NDCircBuffPostCountString,          asynParamInt32,      &NDCircBuffPostCount);
    createParam(NDCircBuffSoftTriggerString,        asynParamInt32,      &NDCircBuffSoftTrigger);
    createParam(NDCircBuffTriggeredString,          asynParamInt32,      &NDCircBuffTriggered);

    // Set the plugin type string
    setStringParam(NDPluginDriverPluginType, "NDPluginCircularBuff");

    // Set the status to idle
    setStringParam(NDCircBuffStatus, "Idle");

    // Init the current frame count to zero
    setIntegerParam(NDCircBuffCurrentImage, 0);
    setIntegerParam(NDCircBuffPostCount, 0);

    // Init the scope control to off
    setIntegerParam(NDCircBuffControl, 0);

    // Init the pre and post count to 100
    setIntegerParam(NDCircBuffPreTrigger, 100);
    setIntegerParam(NDCircBuffPostTrigger, 100);
    
    // Init the preset trigger count to 1
    setIntegerParam(NDCircBuffPresetTriggerCount, 1);
    setIntegerParam(NDCircBuffActualTriggerCount, 0);
    
    // Set the trigger calculation to "0" which will not trigger
    setStringParam(NDCircBuffTriggerCalc, "0");

    // Enable ArrayCallbacks.  
    // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
    setIntegerParam(NDArrayCallbacks, 1);

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


