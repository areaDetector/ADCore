
/*
 * NDPluginAttribute.cpp
 *
 * Extract an Attribute from an NDArray and publish the value in an array.
 * Author: Matthew Pearson
 *
 * March 2014.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "NDArray.h"
#include "NDPluginAttribute.h"
#include <epicsExport.h>

const epicsInt32 NDPluginAttribute::MAX_ATTR_NAME_ = 256;


/** 
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginAttribute::processCallbacks(NDArray *pArray)
{
  /*
     * This function is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */

  int status = 0;
  int dataType;
  char attrName[MAX_ATTR_NAME_] = {0};
  NDAttribute *pAttribute = NULL;
  NDAttributeList *pAttrList = NULL;
  epicsFloat64 attrValue = 0.0;
  epicsFloat64 updatePeriod = 0.0;

  const char *functionName = "NDPluginAttribute::processCallbacks";
  
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
	    "Starting %s. currentPoint_: %d\n", functionName, currentPoint_);

  

  /* Get the time and decide if we update the array.*/
  getDoubleParam(NDPluginAttributeUpdatePeriod, &updatePeriod);
  epicsTimeGetCurrent(&nowTime_);
  nowTimeSecs_ = nowTime_.secPastEpoch + (nowTime_.nsec / 1.e9);
  if ((nowTimeSecs_ - lastTimeSecs_) < (updatePeriod / 1000.0)) {
    arrayUpdate_ = 0;
  } else {
    arrayUpdate_ = 1;
    lastTimeSecs_ = nowTimeSecs_;
  }
 
  /* Get all parameters while we have the mutex */
  getIntegerParam(NDPluginAttributeDataType,    &dataType);

  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);
  
  /* Get the attributes for this driver */
  pAttrList = pArray->pAttributeList;
  getStringParam(NDPluginAttributeAttrName, MAX_ATTR_NAME_, attrName);
  
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "Finding the attribute %s\n", attrName);
  pAttribute = pAttrList->find(attrName);
  if (pAttribute) {
    status = pAttribute->getValue(NDAttrFloat64, &attrValue);
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "Attribute %s is %f\n", attrName, attrValue);
    if (status == asynSuccess) {
      setDoubleParam(NDPluginAttributeVal, attrValue);
      valueSum_ = valueSum_ + attrValue;
      setDoubleParam(NDPluginAttributeValSum, valueSum_);
      if (currentPoint_ < maxTimeSeries_) {
	pTimeSeries_[currentPoint_] = attrValue;
	++currentPoint_;
      }
    }

    callParamCallbacks();
    if (arrayUpdate_) {
      doCallbacksFloat64Array(this->pTimeSeries_, currentPoint_, NDPluginAttributeArray, 0);
    }

  } else {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s: Error reading NDAttribute %s. \n", functionName, attrName);
  }

  

}

asynStatus NDPluginAttribute::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char *functionName = "NDPluginAttribute::writeInt32";

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    if (function == NDPluginAttributeReset) {
      setDoubleParam(NDPluginAttributeVal, 0.0);
      setDoubleParam(NDPluginAttributeValSum, 0.0);
      //Clear the time series array
      memset(pTimeSeries_, 0, maxTimeSeries_*sizeof(epicsFloat64));
      doCallbacksFloat64Array(this->pTimeSeries_, maxTimeSeries_, NDPluginAttributeArray, 0);
      currentPoint_ = 0;
      valueSum_ = 0.0;
    }
    else if (function == NDPluginAttributeUpdate) {
      //Update the data array by hand.
      doCallbacksFloat64Array(this->pTimeSeries_, maxTimeSeries_, NDPluginAttributeArray, 0);
    }
    else {
      /* If this parameter belongs to a base class call its method */
      if (function < FIRST_NDPLUGIN_ATTR_PARAM) 
	status = NDPluginDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks();
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s: status=%d, function=%d, value=%d", 
                  functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s: function=%d, value=%d\n", 
              functionName, function, value);
    return status;
}


/** Constructor for NDPluginAttribute; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  *
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
  * \param[in] maxTimeSeries The max size of the time series array
  * \param[in] attrName The name of the NDArray attribute
  */
NDPluginAttribute::NDPluginAttribute(const char *portName, int queueSize, int blockingCallbacks,
				     const char *NDArrayPort, int NDArrayAddr,
				     int maxBuffers, size_t maxMemory,
				     int priority, int stackSize, 
				     int maxTimeSeries, const char *attrName)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_ATTR_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    const char *functionName = "NDPluginAttribute::NDPluginAttribute";

    /* parameters */
    createParam(NDPluginAttributeNameString,              asynParamOctet, &NDPluginAttributeName);
    createParam(NDPluginAttributeAttrNameString, asynParamOctet, &NDPluginAttributeAttrName);
    createParam(NDPluginAttributeResetString,          asynParamInt32, &NDPluginAttributeReset);
    createParam(NDPluginAttributeUpdateString,          asynParamInt32, &NDPluginAttributeUpdate);
    createParam(NDPluginAttributeValString,          asynParamFloat64, &NDPluginAttributeVal);
    createParam(NDPluginAttributeValSumString,          asynParamFloat64, &NDPluginAttributeValSum);
    createParam(NDPluginAttributeArrayString,          asynParamFloat64Array, &NDPluginAttributeArray);
    createParam(NDPluginAttributeDataTypeString,          asynParamInt32, &NDPluginAttributeDataType);
    createParam(NDPluginAttributeUpdatePeriodString,          asynParamFloat64, &NDPluginAttributeUpdatePeriod);
    
   
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginAttribute");

    maxTimeSeries_ = maxTimeSeries;
    pTimeSeries_ = static_cast<epicsFloat64*>(calloc(maxTimeSeries_, sizeof(epicsFloat64)));
    if (pTimeSeries_ == NULL) {
      perror(functionName);
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: Error from calloc for pTimeSeries_.\n", functionName);
    }

    currentPoint_ = 0;
    arrayUpdate_ = 1;
    valueSum_ = 0.0;

    /* Set the attribute name */
    /* This can be set at runtime too.*/
    setStringParam(NDPluginAttributeAttrName, attrName);

    setDoubleParam(NDPluginAttributeVal, 0.0);
    setDoubleParam(NDPluginAttributeValSum, 0.0);

    /* Try to connect to the array port */
    connectToArrayPort();

    callParamCallbacks();

}

/** Configuration command */
extern "C" int NDAttrConfigure(const char *portName, int queueSize, int blockingCallbacks,
			       const char *NDArrayPort, int NDArrayAddr,
			       int maxBuffers, size_t maxMemory,
			       int priority, int stackSize, 
			       int maxTimeSeries, const char *attrName)
{
    new NDPluginAttribute(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
		    maxBuffers, maxMemory, priority, stackSize, maxTimeSeries, attrName);
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
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg initArg9 = { "maxTimeSeries",iocshArgInt};
static const iocshArg initArg10 = { "attrName",iocshArgString};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
					    &initArg9,
                                            &initArg10};
static const iocshFuncDef initFuncDef = {"NDAttrConfigure",11,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDAttrConfigure(args[0].sval, args[1].ival, args[2].ival,
                   args[3].sval, args[4].ival, args[5].ival,
		    args[6].ival, args[7].ival, args[8].ival, args[9].ival, args[10].sval);
}

extern "C" void NDAttrRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDAttrRegister);
}
