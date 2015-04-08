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

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginAttribute.h"

const epicsInt32 NDPluginAttribute::MAX_ATTR_NAME_      = 256;
const char*      NDPluginAttribute::UNIQUE_ID_NAME_     = "NDArrayUniqueId";
const char*      NDPluginAttribute::TIMESTAMP_NAME_     = "NDArrayTimeStamp";
const char*      NDPluginAttribute::EPICS_TS_SEC_NAME_  = "NDArrayEpicsTSSec";
const char*      NDPluginAttribute::EPICS_TS_NSEC_NAME_ = "NDArrayEpicsTSnSec";

typedef enum {
  TSEraseStart,
  TSStart,
  TSStop,
  TSRead
} NDAttributeTSControl_t;

#define DEFAULT_NUM_TSPOINTS 2048

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
  int currentTSPoint;
  int numTSPoints;
  int TSAcquiring;
  double valueSum;
  int i;
  char attrName[MAX_ATTR_NAME_] = {0};
  NDAttribute *pAttribute = NULL;
  NDAttributeList *pAttrList = NULL;
  epicsFloat64 attrValue = 0.0;

  static const char *functionName = "NDPluginAttribute::processCallbacks";
  
  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);
  
  /* Get the attributes for this driver */
  pAttrList = pArray->pAttributeList;
  
  getIntegerParam(NDPluginAttributeTSCurrentPoint, &currentTSPoint);
  getIntegerParam(NDPluginAttributeTSNumPoints,    &numTSPoints);
  getIntegerParam(NDPluginAttributeTSAcquiring,    &TSAcquiring);

  for (i=0; i<maxAttributes_; i++) {
    getStringParam(i, NDPluginAttributeAttrName, MAX_ATTR_NAME_, attrName);

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "Finding the attribute %s\n", attrName);

    if (strcmp(attrName, UNIQUE_ID_NAME_) == 0) {
      attrValue = (epicsFloat64) pArray->uniqueId;
    } else if (strcmp(attrName, TIMESTAMP_NAME_) == 0) {
      attrValue = pArray->timeStamp;
    } else if (strcmp(attrName, EPICS_TS_SEC_NAME_) == 0) {
      attrValue = (epicsFloat64)pArray->epicsTS.secPastEpoch;
    } else if (strcmp(attrName, EPICS_TS_NSEC_NAME_) == 0) {
      attrValue = (epicsFloat64)pArray->epicsTS.nsec;
    } else {
      pAttribute = pAttrList->find(attrName);
      if (pAttribute) {
        status = pAttribute->getValue(NDAttrFloat64, &attrValue);
        if (status != asynSuccess) {
          asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s: Error reading value for NDAttribute %s. \n", functionName, attrName);
          continue;
        }
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "Attribute %s value is %f\n", attrName, attrValue);
      } else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s: Error finding NDAttribute %s. \n", functionName, attrName);
        continue;
      }
    }
    setDoubleParam(i, NDPluginAttributeVal, attrValue);
    getDoubleParam(i, NDPluginAttributeValSum, &valueSum);
    valueSum += attrValue;
    setDoubleParam(i, NDPluginAttributeValSum, valueSum);
    if (TSAcquiring) {
      pTSArray_[i][currentTSPoint] = attrValue;
    }
    callParamCallbacks(i);
  }
  if (TSAcquiring) {
    currentTSPoint++;
    setIntegerParam(NDPluginAttributeTSCurrentPoint, currentTSPoint);
    if (currentTSPoint >= numTSPoints) {
        doTimeSeriesCallbacks();
        setIntegerParam(NDPluginAttributeTSAcquiring, 0);
    }
  }
}

void NDPluginAttribute::doTimeSeriesCallbacks()
{
  int currentTSPoint;
  int i;

  getIntegerParam(NDPluginAttributeTSCurrentPoint, &currentTSPoint);
  for (i=0; i<maxAttributes_; i++) {
    doCallbacksFloat64Array(pTSArray_[i], currentTSPoint, NDPluginAttributeTSArrayValue, i);
  }
}


asynStatus NDPluginAttribute::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  int numTSPoints, currentTSPoint;
  int i;
  static const char *functionName = "NDPluginAttribute::writeInt32";

  /* Set the parameter in the parameter library. */
  status = (asynStatus) setIntegerParam(function, value);

  if (function == NDPluginAttributeReset) {
  getIntegerParam(NDPluginAttributeTSNumPoints, &numTSPoints);
    for (i=0; i<maxAttributes_; i++) {
      setDoubleParam(i, NDPluginAttributeVal, 0.0);
      setDoubleParam(i, NDPluginAttributeValSum, 0.0);
      // Clear the time series array
      memset(pTSArray_[i], 0, numTSPoints*sizeof(epicsFloat64));
      doCallbacksFloat64Array(this->pTSArray_[i], numTSPoints, NDPluginAttributeTSArrayValue, i);
      callParamCallbacks(i);
    }
    setIntegerParam(NDPluginAttributeTSCurrentPoint, 0);
  } 
  else if (function == NDPluginAttributeTSNumPoints) {
    for (i=0; i<maxAttributes_; i++) {
      free(pTSArray_[i]);
      pTSArray_[i] = static_cast<epicsFloat64 *>(calloc(value, sizeof(epicsFloat64)));
    }
  } 
  else if (function == NDPluginAttributeTSControl) {
    switch (value) {
      case TSEraseStart:
        setIntegerParam(NDPluginAttributeTSCurrentPoint, 0);
        setIntegerParam(NDPluginAttributeTSAcquiring, 1);
        getIntegerParam(NDPluginAttributeTSNumPoints, &numTSPoints);
        for (i=0; i<maxAttributes_; i++) {
          memset(pTSArray_[i], 0, numTSPoints*sizeof(epicsFloat64));
        }
        doTimeSeriesCallbacks();
        break;
      case TSStart:
        getIntegerParam(NDPluginAttributeTSNumPoints, &numTSPoints);
        getIntegerParam(NDPluginAttributeTSCurrentPoint, &currentTSPoint);
        if (currentTSPoint < numTSPoints) {
          setIntegerParam(NDPluginAttributeTSAcquiring, 1);
        }
        break;
      case TSStop:
        setIntegerParam(NDPluginAttributeTSAcquiring, 0);
        doTimeSeriesCallbacks();
        break;
      case TSRead:
        doTimeSeriesCallbacks();
        break;
    }
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
  * \param[in] maxAttributes The maximum number of attributes that this plugin will support
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginAttribute::NDPluginAttribute(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr, int maxAttributes,
                                     int maxBuffers, size_t maxMemory,
                                     int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, maxAttributes, NUM_NDPLUGIN_ATTR_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
  int i;
  static const char *functionName = "NDPluginAttribute::NDPluginAttribute";

  maxAttributes_ = maxAttributes;
  if (maxAttributes_ < 1) maxAttributes_ = 1;
  /* parameters */
  createParam(NDPluginAttributeAttrNameString,       asynParamOctet,        &NDPluginAttributeAttrName);
  createParam(NDPluginAttributeResetString,          asynParamInt32,        &NDPluginAttributeReset);
  createParam(NDPluginAttributeValString,            asynParamFloat64,      &NDPluginAttributeVal);
  createParam(NDPluginAttributeValSumString,         asynParamFloat64,      &NDPluginAttributeValSum);
  createParam(NDPluginAttributeTSControlString,      asynParamInt32,        &NDPluginAttributeTSControl);
  createParam(NDPluginAttributeTSNumPointsString,    asynParamInt32,        &NDPluginAttributeTSNumPoints);
  createParam(NDPluginAttributeTSCurrentPointString, asynParamInt32,        &NDPluginAttributeTSCurrentPoint);
  createParam(NDPluginAttributeTSAcquiringString,    asynParamInt32,        &NDPluginAttributeTSAcquiring);
  createParam(NDPluginAttributeTSArrayValueString,   asynParamFloat64Array, &NDPluginAttributeTSArrayValue);

  /* Set the plugin type string */
  setStringParam(NDPluginDriverPluginType, "NDPluginAttribute");

  setIntegerParam(NDPluginAttributeTSNumPoints, DEFAULT_NUM_TSPOINTS);
  pTSArray_ = static_cast<epicsFloat64 **>(calloc(maxAttributes_, sizeof(epicsFloat64 *)));
  if (pTSArray_ == NULL) {
    perror(functionName);
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: Error from calloc for pTSArray_.\n", functionName);
  }
  for (i=0; i<maxAttributes_; i++) {
    pTSArray_[i] = static_cast<epicsFloat64*>(calloc(DEFAULT_NUM_TSPOINTS, sizeof(epicsFloat64)));
    setDoubleParam(i, NDPluginAttributeVal, 0.0);
    setDoubleParam(i, NDPluginAttributeValSum, 0.0);
    setStringParam(i, NDPluginAttributeAttrName, "");
    if (pTSArray_[i] == NULL) {
      perror(functionName);
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: Error from calloc for pTSArray_.\n", functionName);
    }
    callParamCallbacks(i);
  }

  // Disable ArrayCallbacks.  
  // This plugin currently does not do array callbacks, so make the setting reflect the behavior
  setIntegerParam(NDArrayCallbacks, 0);

  /* Try to connect to the array port */
  connectToArrayPort();

}

/** Configuration command */
extern "C" int NDAttrConfigure(const char *portName, int queueSize, int blockingCallbacks,
                               const char *NDArrayPort, int NDArrayAddr,
                               int maxAttributes, int maxBuffers, size_t maxMemory,
                               int priority, int stackSize)
{
    new NDPluginAttribute(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                          maxAttributes, maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxAttributes",iocshArgInt};
static const iocshArg initArg6 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg7 = { "maxMemory",iocshArgInt};
static const iocshArg initArg8 = { "priority",iocshArgInt};
static const iocshArg initArg9 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9};
static const iocshFuncDef initFuncDef = {"NDAttrConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDAttrConfigure(args[0].sval, args[1].ival, args[2].ival,
                  args[3].sval, args[4].ival, args[5].ival,
                  args[6].ival, args[7].ival, args[8].ival, args[9].ival);
}

extern "C" void NDAttrRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDAttrRegister);
}
