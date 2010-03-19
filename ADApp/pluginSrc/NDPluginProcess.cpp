/*
 * NDPluginProcess.cpp
 *
 * Image processing plugin
 * Author: Mark Rivers
 *
 * Created March 12, 2010
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>
#include <epicsExport.h>

#include "NDArray.h"
#include "NDPluginProcess.h"

static const char *driverName="NDPluginProcess";


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Does image processing.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginProcess::processCallbacks(NDArray *pArray)
{
    /* This function does array processing.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
    int i;
    NDArray *pScratch;
    double  *data, newData, newFilter;
    NDArrayInfo arrayInfo;
    double  *background=NULL, *flatField=NULL, *filter=NULL;
    double  value;
    int     nElements;
    int     validBackground;
    int     enableBackground;
    int     validFlatField;
    int     enableFlatField;
    double  scaleFlatField;
    double  lowClip=0;
    int     enableLowClip;
    double  highClip=0;
    int     enableHighClip;
    int     enableFilter;
    int     numFilter;
    int     dataType;
    int     anyProcess;
    double  oc0, oc1, oc2, oc3, oc4;
    double  fc0, fc1, fc2, fc3, fc4;
    double  rc0, rc1;
    double  F1, F2, O1, O2;

    //const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    if (this->newBackground) {
        this->newBackground = 0;
        if (this->pBackground) this->pBackground->release();
        this->pBackground = NULL;
        if (this->pArrays[0]) {
            /* Make a copy of the current array, converted to double type */
            this->pNDArrayPool->convert(this->pArrays[0], &this->pBackground, NDFloat64);
            this->pBackground->getInfo(&arrayInfo);
            this->nBackgroundElements = arrayInfo.nElements;
        }
    }

    if (this->newFlatField) {
        this->newFlatField = 0;
        if (this->pFlatField) this->pFlatField->release();
        this->pFlatField = NULL;
        if (this->pArrays[0]) {
            /* Make a copy of the current array, converted to double type */
            this->pNDArrayPool->convert(this->pArrays[0], &this->pFlatField, NDFloat64);
            this->pFlatField->getInfo(&arrayInfo);
            this->nFlatFieldElements = arrayInfo.nElements;
        }
    }
    /* Need to fetch all of these parameters while we still have the mutex */
    getIntegerParam(NDPluginProcessDataType,            &dataType);
    getIntegerParam(NDPluginProcessEnableBackground,    &enableBackground);
    getIntegerParam(NDPluginProcessEnableFlatField,     &enableFlatField);
    getDoubleParam (NDPluginProcessScaleFlatField,      &scaleFlatField);
    getIntegerParam(NDPluginProcessEnableLowClip,       &enableLowClip);
    getIntegerParam(NDPluginProcessEnableHighClip,      &enableHighClip);
    getIntegerParam(NDPluginProcessEnableFilter,        &enableFilter);
    if (enableLowClip) 
        getDoubleParam (NDPluginProcessLowClip,         &lowClip);
    if (enableHighClip) 
        getDoubleParam (NDPluginProcessHighClip,        &highClip);
    if (enableFilter) {
        getIntegerParam(NDPluginProcessNumFilter,       &numFilter);
        getDoubleParam (NDPluginProcessOC0,             &oc0);
        getDoubleParam (NDPluginProcessOC1,             &oc1);
        getDoubleParam (NDPluginProcessOC2,             &oc2);
        getDoubleParam (NDPluginProcessOC3,             &oc3);
        getDoubleParam (NDPluginProcessOC4,             &oc4);
        getDoubleParam (NDPluginProcessFC0,             &fc0);
        getDoubleParam (NDPluginProcessFC1,             &fc1);
        getDoubleParam (NDPluginProcessFC2,             &fc2);
        getDoubleParam (NDPluginProcessFC3,             &fc3);
        getDoubleParam (NDPluginProcessFC4,             &fc4);
        getDoubleParam (NDPluginProcessRC0,             &rc0);
        getDoubleParam (NDPluginProcessRC1,             &rc1);
    }

    if (this->newFilter) {
        this->newFilter = 0;
        /* Filtering was turned off or on, or the number to filter changed. Delete the filter array
         * forcing filtering to restart */
        if (this->pFilter) this->pFilter->release();
        this->pFilter = NULL;
    }

    /* Release the lock now that we are only doing things that don't involve memory other thread
     * cannot access */
    this->unlock();
    /* Special case for automatic data type */
    if (dataType == -1) dataType = (int)pArray->dataType;
    
    pArray->getInfo(&arrayInfo);
    nElements = arrayInfo.nElements;

    validBackground = 0;
    if (this->pBackground && (nElements == this->nBackgroundElements)) validBackground = 1;
    setIntegerParam(NDPluginProcessValidBackground, validBackground);
    validFlatField = 0;
    if (this->pFlatField && (nElements == this->nFlatFieldElements)) validFlatField = 1;
    setIntegerParam(NDPluginProcessValidFlatField, validFlatField);

    if (validBackground && enableBackground)
        background = (double *)this->pBackground->pData;
    if (validFlatField && enableFlatField)
        flatField = (double *)this->pFlatField->pData;

    anyProcess = ((enableBackground && validBackground) ||
                  (enableFlatField && validFlatField)   ||
                  enableHighClip || enableLowClip       ||
                  enableFilter);
    /* If no processing is to be done just convert the input array and do callbacks */
    if (!anyProcess) {
        /* Convert the array to the desired output data type */
        if (this->pArrays[0]) this->pArrays[0]->release();
        this->pNDArrayPool->convert(pArray, &this->pArrays[0], (NDDataType_t)dataType);
        goto done;
    }
    
    /* Make a copy of the array converted to double, because we cannot modify the input array */
    this->pNDArrayPool->convert(pArray, &pScratch, NDFloat64);
    data = (double *)pScratch->pData;

    for (i=0; i<nElements; i++) {
        value = data[i];
        if (background) value -= background[i];
        if (flatField) {
            if (flatField[i] != 0.) 
                value *= scaleFlatField / flatField[i];
            else
                value = scaleFlatField;
        }
        if (enableHighClip && (value > highClip)) value = highClip;
        if (enableLowClip  && (value < lowClip))  value = lowClip;
        data[i] = value;
    }
    
    if (enableFilter) {
        if (this->pFilter) {
            this->pFilter->getInfo(&arrayInfo);
            if (nElements != arrayInfo.nElements) {
                this->pFilter->release();
                this->pFilter = NULL;
            }
        }
        if (!this->pFilter) {
            /* There is not a current filter array */
            /* Make a copy of the current array, converted to double type */
            this->pNDArrayPool->convert(pScratch, &this->pFilter, NDFloat64);
            filter = (double *)this->pFilter->pData;
            for (i=0; i<nElements; i++) {
                filter[i] = rc0 * rc1*data[i];
            }           
            this->numFiltered = 1;
        } else {
            /* Do the filtering */
            if (this->numFiltered < numFilter) this->numFiltered++;
            filter = (double *)this->pFilter->pData;
            O1 = oc1 + oc2/this->numFiltered;
            O2 = oc3 + oc4/this->numFiltered;
            F1 = fc1 + fc2/this->numFiltered;
            F2 = fc3 + fc4/this->numFiltered;
            for (i=0; i<nElements; i++) {
                newData   = oc0;
                if (O1) newData += O1 * filter[i];
                if (O2) newData += O2 * data[i];
                newFilter = fc0;
                if (F1) newFilter += F1 * filter[i];
                if (F2) newFilter += F2 * data[i];
                data[i] = newData;
                filter[i] = newFilter;
            }
        }
    }
    setIntegerParam(NDPluginProcessNumFiltered, this->numFiltered);
    
    /* Convert the array to the desired output data type */
    if (this->pArrays[0]) this->pArrays[0]->release();
    this->pNDArrayPool->convert(pScratch, &this->pArrays[0], (NDDataType_t)dataType);
    pScratch->release();

    done:
    /* Call any clients who have registered for NDArray callbacks */
    doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);

    /* We must enter the loop and exit with the mutex locked */
    this->lock();
    callParamCallbacks();
}


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters.
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginProcess::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char* functionName = "writeInt32";

    /* Set parameter and readback in parameter library */
    status = setIntegerParam(function, value);

    if (function == NDPluginProcessSaveBackground && value) {
        this->newBackground = 1;
        setIntegerParam(NDPluginProcessSaveBackground, 0);
    }
    else if (function == NDPluginProcessSaveFlatField && value) {
        this->newFlatField = 1;
        setIntegerParam(NDPluginProcessSaveFlatField, 0);
    }
    else if ((function == NDPluginProcessEnableFilter) ||
             (function == NDPluginProcessNumFilter)) {
        this->newFilter = 1;
    }
    else {
        /* This was not a parameter that this driver understands, try the base class */
        status = NDPluginDriver::writeInt32(pasynUser, value);
    }
    /* Do callbacks so higher layers see any changes */
    status = callParamCallbacks();

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


/** Constructor for NDPluginProcess; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
NDPluginProcess::NDPluginProcess(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_PROCESS_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    asynStatus status;
    //const char *functionName = "NDPluginProcess";

    /* Background array subtraction */
    createParam(NDPluginProcessSaveBackgroundString,    asynParamInt32,     &NDPluginProcessSaveBackground);
    createParam(NDPluginProcessEnableBackgroundString,  asynParamInt32,     &NDPluginProcessEnableBackground);
    createParam(NDPluginProcessValidBackgroundString,   asynParamInt32,     &NDPluginProcessValidBackground);

    /* Flat field normalization */
    createParam(NDPluginProcessSaveFlatFieldString,     asynParamInt32,     &NDPluginProcessSaveFlatField);
    createParam(NDPluginProcessEnableFlatFieldString,   asynParamInt32,     &NDPluginProcessEnableFlatField);
    createParam(NDPluginProcessValidFlatFieldString,    asynParamInt32,     &NDPluginProcessValidFlatField);
    createParam(NDPluginProcessScaleFlatFieldString,    asynParamFloat64,   &NDPluginProcessScaleFlatField);

    /* High and low clipping */
    createParam(NDPluginProcessLowClipString,           asynParamFloat64,   &NDPluginProcessLowClip);
    createParam(NDPluginProcessEnableLowClipString,     asynParamInt32,     &NDPluginProcessEnableLowClip);
    createParam(NDPluginProcessHighClipString,          asynParamFloat64,   &NDPluginProcessHighClip);
    createParam(NDPluginProcessEnableHighClipString,    asynParamInt32,     &NDPluginProcessEnableHighClip);

    /* Frame filtering */
    createParam(NDPluginProcessEnableFilterString,      asynParamInt32,     &NDPluginProcessEnableFilter);
    createParam(NDPluginProcessNumFilterString,         asynParamInt32,     &NDPluginProcessNumFilter);
    createParam(NDPluginProcessNumFilteredString,       asynParamInt32,     &NDPluginProcessNumFiltered);   
    createParam(NDPluginProcessOC0String,               asynParamFloat64,   &NDPluginProcessOC0);   
    createParam(NDPluginProcessOC1String,               asynParamFloat64,   &NDPluginProcessOC1);   
    createParam(NDPluginProcessOC2String,               asynParamFloat64,   &NDPluginProcessOC2);   
    createParam(NDPluginProcessOC3String,               asynParamFloat64,   &NDPluginProcessOC3);   
    createParam(NDPluginProcessOC4String,               asynParamFloat64,   &NDPluginProcessOC4);   
    createParam(NDPluginProcessFC0String,               asynParamFloat64,   &NDPluginProcessFC0);   
    createParam(NDPluginProcessFC1String,               asynParamFloat64,   &NDPluginProcessFC1);   
    createParam(NDPluginProcessFC2String,               asynParamFloat64,   &NDPluginProcessFC2);   
    createParam(NDPluginProcessFC3String,               asynParamFloat64,   &NDPluginProcessFC3);   
    createParam(NDPluginProcessFC4String,               asynParamFloat64,   &NDPluginProcessFC4);   
    createParam(NDPluginProcessRC0String,               asynParamFloat64,   &NDPluginProcessRC0);   
    createParam(NDPluginProcessRC1String,               asynParamFloat64,   &NDPluginProcessRC1);   
    
    /* Output data type */
    createParam(NDPluginProcessDataTypeString,          asynParamInt32,     &NDPluginProcessDataType);   

    this->pBackground = NULL;
    this->pFlatField  = NULL;
    this->pFilter     = NULL;    

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginProcess");

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDProcessConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginProcess *pPlugin =
        new NDPluginProcess(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                        maxBuffers, maxMemory, priority, stackSize);
    pPlugin = NULL;  /* This is just to eliminate compiler warning about unused variables/objects */
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
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDProcessConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDProcessConfigure(args[0].sval, args[1].ival, args[2].ival,
                       args[3].sval, args[4].ival, args[5].ival,
                       args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDProcessRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDProcessRegister);
}
