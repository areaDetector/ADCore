/*
 * drvNDROI.c
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
#include <math.h>

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
#include "drvNDROI.h"

#define driverName "drvNDROI"


typedef enum
{
    NDROIArrayPort=ADFirstDriverParam, /* (asynOctet,    r/w) The port for the NDArray driver */
    NDROIArrayAddr,           /* (asynInt32,    r/w) The address on the port */
    NDROIUpdateTime,          /* (asynFloat64,  r/w) Minimum time between array updates */
    NDROIDroppedArrays,       /* (asynInt32,    r/w) Number of dropped arrays */
    NDROIArrayCounter,        /* (asynInt32,    r/w) Number of arrays processed */
    NDROIBlockingCallbacks,   /* (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
    NDROILastDriverParam
} NDROIParam_t;
    
/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t NDROIParamString[] = {
    {NDROIArrayPort,          "NDARRAY_PORT"},
    {NDROIArrayAddr,          "NDARRAY_ADDR"},
    {NDROIUpdateTime,         "ARRAY_UPDATE_TIME"},
    {NDROIDroppedArrays,      "DROPPED_ARRAYS"},
    {NDROIArrayCounter,       "ARRAY_COUNTER"},
    {NDROIBlockingCallbacks,  "BLOCKING_CALLBACKS"}
};

#define NDROIFirstROIParam NDROILastDriverParam
/* The following enum is for each of the ROIs */
typedef enum {
    NDROIName = NDROIFirstROIParam,
                             /* (asynOctet,   r/w) Name of this ROI */
    NDROIUse,                /* (asynInt32,   r/w) Use this ROI? */
    NDROIComputeStatistics,  /* (asynInt32,   r/w) Compute statistics for this ROI? */
    NDROIComputeHistogram,   /* (asynInt32,   r/w) Compute histogram for this ROI? */
    NDROIComputeProfiles,    /* (asynInt32,   r/w) Compute profiles for this ROI? */
    NDROIHighlight,          /* (asynInt32,   r/w) Highlight other ROIs in this ROI? */
    
    /* ROI definition */
    NDROIDim0Min,            /* (asynInt32,   r/w) Starting element of ROI in each dimension */
    NDROIDim0Size,           /* (asynInt32,   r/w) Size of ROI in each dimension */
    NDROIDim0Bin,            /* (asynInt32,   r/w) Binning of ROI in each dimension */
    NDROIDim0Reverse,        /* (asynInt32,   r/w) Reversal of ROI in each dimension */
    NDROIDim1Min,            /* (asynInt32,   r/w) Starting element of ROI in each dimension */
    NDROIDim1Size,           /* (asynInt32,   r/w) Size of ROI in each dimension */
    NDROIDim1Bin,            /* (asynInt32,   r/w) Binning of ROI in each dimension */
    NDROIDim1Reverse,        /* (asynInt32,   r/w) Reversal of ROI in each dimension */
    
    /* ROI statistics */
    NDROIBgdWidth,           /* (asynFloat64, r/w) Width of background region when computing net */
    NDROIMinValue,           /* (asynFloat64, r/o) Minimum counts in any element */
    NDROIMaxValue,           /* (asynFloat64, r/o) Maximum counts in any element */
    NDROIMeanValue,          /* (asynFloat64, r/o) Mean counts of all elements */
    NDROITotal,              /* (asynFloat64, r/o) Sum of all elements */
    NDROINet,                /* (asynFloat64, r/o) Sum of all elements minus background */
    
    /* ROI histogram */
    NDROIHistSize,           /* (asynInt32,   r/w) Number of elements in histogram */
    NDROIHistMin,            /* (asynFloat64, r/w) Minimum value for histogram */
    NDROIHistMax,            /* (asynFloat64, r/w) Maximum value for histogram */
    NDROIHistEntropy,        /* (asynFloat64, r/o) Image entropy calculcated from histogram */
    NDROIHistArray,          /* (asynFloat64Array, r/o) Histogram array */

    NDROILastROINParam
} NDROINParam_t;

static ADParamString_t NDROINParamString[] = {
    {NDROIName,               "NAME"},
    {NDROIUse,                "USE"},
    {NDROIComputeStatistics,  "COMPUTE_STATISTICS"},
    {NDROIComputeHistogram,   "COMPUTE_HISTOGRAM"},
    {NDROIComputeProfiles,    "COMPUTE_PROFILES"},
    {NDROIHighlight,          "HIGHLIGHT"},

    {NDROIDim0Min,            "DIM0_MIN"},
    {NDROIDim0Size,           "DIM0_SIZE"},
    {NDROIDim0Bin,            "DIM0_BIN"},
    {NDROIDim0Reverse,        "DIM0_REVERSE"},
    {NDROIDim1Min,            "DIM1_MIN"},
    {NDROIDim1Size,           "DIM1_SIZE"},
    {NDROIDim1Bin,            "DIM1_BIN"},
    {NDROIDim1Reverse,        "DIM1_REVERSE"},

    {NDROIBgdWidth,           "BGD_WIDTH"},
    {NDROIMinValue,           "MIN_VALUE"},
    {NDROIMaxValue,           "MAX_VALUE"},
    {NDROIMeanValue,          "MEAN_VALUE"},
    {NDROITotal,              "TOTAL"},
    {NDROINet,                "NET"},

    {NDROIHistSize,           "HIST_SIZE"},
    {NDROIHistMin,            "HIST_MIN"},
    {NDROIHistMax,            "HIST_MAX"},
    {NDROIHistEntropy,        "HIST_ENTROPY"},
    {NDROIHistArray,          "HIST_ARRAY"},
};

#define NUM_ROI_PARAMS (sizeof(NDROIParamString)/sizeof(NDROIParamString[0]))
#define NUM_ROIN_PARAMS (sizeof(NDROINParamString)/sizeof(NDROINParamString[0]))

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

typedef struct NDROI {
    PARAMS params;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    NDArray_t *pArray;
    double *histogram;
    int histogramSize;
    double *profiles[ND_ARRAY_MAX_DIMS];
    int profileSize[ND_ARRAY_MAX_DIMS];
} NDROI_t;

typedef struct drvNDROIPvt {
    /* These fields will be needed by most asyn plug-in drivers */
    char *portName;
    epicsMutexId mutexId;
    epicsMessageQueueId msgQId;
    PARAMS params;
    
    /* The asyn interfaces this driver implements */
    asynStandardInterfaces asynStdInterfaces;
    
    /* The asyn interfaces we access as a client */
    asynHandle *pasynHandle;
    void *asynHandlePvt;
    void *asynHandleInterruptPvt;
    asynUser *pasynUserHandle;
    
    /* asynUser connected to ourselves for asynTrace */
    asynUser *pasynUser;
    
    /* These fields are specific to the NDROI driver */
    epicsTimeStamp lastArrayPostTime;
    int maxROIs;
    NDROI_t *pROIs;    /* Array of drvNDROI structures */
} drvNDROIPvt;


/* Local functions, not in any interface */

/* These macros saves a lot of code when handling different data types */
#define COMPUTE_HISTOGRAM(DATA_TYPE) { \
    int i; \
    double scale = histSize / (histMax - histMin); \
    int bin; \
    DATA_TYPE *pData = (DATA_TYPE *) pROI->pArray->pData; \
    for (i=0; i<arrayInfo.nElements; i++) { \
        value = (double)pData[i]; \
        bin = (int) (((value - histMin) * scale) + 0.5); \
        if ((bin >= 0) && (bin < histSize)) pROI->histogram[bin]++; \
    } \
}

#define COMPUTE_STATISTICS(DATA_TYPE) { \
    int i; \
    DATA_TYPE *pData = (DATA_TYPE *) pROI->pArray->pData; \
    min = (double) pData[0]; \
    max = (double) pData[0]; \
    total = 0.; \
    for (i=0; i<arrayInfo.nElements; i++) { \
        value = (double)pData[i]; \
        if (value < min) min = value; \
        if (value > max) max = value; \
        total += value; \
    } \
}

#define COMPUTE_FUNCTION(FUNCTION) { \
    switch(pROI->pArray->dataType) {              \
        case NDInt8:                              \
            FUNCTION(epicsInt8);     \
            break;                                \
        case NDUInt8:                             \
            FUNCTION(epicsUInt8);    \
            break;                                \
        case NDInt16:                             \
            FUNCTION(epicsInt16);    \
            break;                                \
        case NDUInt16:                            \
            FUNCTION(epicsUInt16);   \
            break;                                \
        case NDInt32:                             \
            FUNCTION(epicsInt32);    \
            break;                                \
        case NDUInt32:                            \
            FUNCTION(epicsUInt32);   \
            break;                                \
        case NDFloat32:                           \
            FUNCTION(epicsFloat32);  \
            break;                                \
        case NDFloat64:                           \
            FUNCTION(epicsFloat64);  \
            break;                                \
        default:                                  \
            status = ND_ERROR;         \
            break;                                \
    }                                             \
}



static void NDROIProcessROIs(drvNDROIPvt *pPvt, NDArray_t *pArray)
{
    /* This function calls back any registered clients on the standard asyn array interfaces with 
     * the data in our private buffer.
     * It is called with the mutex already locked.
     */
     
    NDROI_t *pROI;
    int use, computeStatistics, computeHistogram, computeProfiles;
    int i;
    double histMin, histMax, entropy;
    int histSize;
    int arrayCounter;
    NDArrayInfo_t arrayInfo;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS], *pDim;
    double min=0, max=0, mean, total=0, net, value;
    int roi, dim;
    double counts;
    int status;
    const char* functionName = "NDROIProcessROIs";
 
    /* Loop over the ROIs in this driver */
    for (roi=0; roi<pPvt->maxROIs; roi++) {
        pROI = &pPvt->pROIs[roi];
        ADParam->getInteger(pROI->params, NDROIUse, &use);
        /* Free the previous array */
        if (pROI->pArray) {
            NDArrayBuff->release(pROI->pArray);
            pROI->pArray = NULL;
        }
        if (!use) continue;
        /* Need to fetch all of these parameters while we still have the mutex */
        ADParam->getInteger(pROI->params, NDROIComputeStatistics, &computeStatistics);
        ADParam->getInteger(pROI->params, NDROIComputeHistogram, &computeHistogram);
        ADParam->getInteger(pROI->params, NDROIComputeProfiles, &computeProfiles);
        ADParam->getDouble(pROI->params, NDROIHistMin, &histMin);
        ADParam->getDouble(pROI->params, NDROIHistMax, &histMax);
        ADParam->getInteger(pROI->params, NDROIHistSize, &histSize);
        /* Make sure dimensions are valid, fix them if they are not */
        for (dim=0; dim<pArray->ndims; dim++) {
            pDim = &pROI->dims[dim];
            pDim->offset  = MAX(pDim->offset, 0);
            pDim->offset  = MIN(pDim->offset, pArray->dims[dim].size-1);
            pDim->size    = MAX(pDim->size, 1);
            pDim->size    = MIN(pDim->size, pArray->dims[dim].size - pDim->offset);
            pDim->binning = MAX(pDim->binning, 1);
            pDim->binning = MIN(pDim->binning, pDim->size);
        }
        /* Make a local copy of the fixed dimensions so we can release the mutex */
        memcpy(dims, pROI->dims, pArray->ndims*sizeof(NDDimension_t));
        
        /* This function is called with the lock taken, and it must be set when we exit.
         * The following code can be exected without the mutex because we are not accessing elements of
         * pPvt that other threads can access. */
        epicsMutexUnlock(pPvt->mutexId);
    
        /* Extract this ROI from the input array */
        NDArrayBuff->convert(pArray, &pROI->pArray, pArray->dataType, dims);
        /* Call any clients who have registered for NDArray callbacks */
        ADUtils->handleCallback(pPvt->asynStdInterfaces.handleInterruptPvt, 
                                pROI->pArray, NDArrayData, roi);
        NDArrayBuff->getInfo(pROI->pArray, &arrayInfo);
 
        if (computeStatistics) {
            COMPUTE_FUNCTION(COMPUTE_STATISTICS);
            net = total;
            mean = total / arrayInfo.nElements;
            ADParam->setDouble(pROI->params, NDROIMinValue, min);
            ADParam->setDouble(pROI->params, NDROIMaxValue, max);
            ADParam->setDouble(pROI->params, NDROIMeanValue, mean);
            ADParam->setDouble(pROI->params, NDROITotal, total);
            ADParam->setDouble(pROI->params, NDROINet, net);
            asynPrint(pPvt->pasynUser, ASYN_TRACEIO_DRIVER, 
                pROI->pArray->pData, arrayInfo.totalBytes,
                "%s:%s ROI=%d, min=%f, max=%f, mean=%f, total=%f, net=%f",
                driverName, functionName, roi, min, max, mean, total, net);
        }
        if (computeHistogram) {
            if (histSize > pROI->histogramSize) {
                free(pROI->histogram);
                pROI->histogram = (double *)calloc(histSize, sizeof(double));
                pROI->histogramSize = histSize;
            }
            memset(pROI->histogram, 0, histSize*sizeof(double));
            COMPUTE_FUNCTION(COMPUTE_HISTOGRAM);
            entropy = 0;
            for (i=0; i<histSize; i++) {
                counts = pROI->histogram[i];
                if (counts <= 0) counts = 1;
                entropy += counts * log(counts);
            }
            entropy = -entropy / arrayInfo.nElements;
            ADParam->setDouble(pROI->params, NDROIHistEntropy, entropy);
        }

        /* We must enter the loop and exit with the mutex locked */
        epicsMutexLock(pPvt->mutexId);
        ADParam->callCallbacksAddr(pROI->params, roi);
    }

    /* Update the parameters.  The counter should be updated after data are posted
     * because clients might use that to detect new data */
    ADParam->getInteger(pPvt->params, NDROIArrayCounter, &arrayCounter);    
    arrayCounter++;
    ADParam->setInteger(pPvt->params, NDROIArrayCounter, arrayCounter);    
    ADParam->callCallbacks(pPvt->params);
}

static void NDROICallback(void *drvPvt, asynUser *pasynUser, void *handle)
{
    /* This callback function is called from the detector driver when a new array arrives.
     * It computes ROI statistics and calls back registered clients.
     * It can either do the callbacks directly (if BlockingCallbacks=1) or by queueing
     * the arrays to be processed by a background task (if BlockingCallbacks=0).
     * In the latter case arrays can be dropped if the queue is full.
     */
     
    drvNDROIPvt *pPvt = drvPvt;
    NDArray_t *pArray = handle;
    epicsTimeStamp tNow;
    double minArrayUpdateTime, deltaTime;
    int status;
    int blockingCallbacks;
    int arrayCounter, droppedArrays;
    char *functionName = "NDROICallback";

    epicsMutexLock(pPvt->mutexId);

    status |= ADParam->getDouble(pPvt->params, NDROIUpdateTime, &minArrayUpdateTime);
    status |= ADParam->getInteger(pPvt->params, NDROIArrayCounter, &arrayCounter);
    status |= ADParam->getInteger(pPvt->params, NDROIDroppedArrays, &droppedArrays);
    status |= ADParam->getInteger(pPvt->params, NDROIBlockingCallbacks, &blockingCallbacks);
    
    epicsTimeGetCurrent(&tNow);
    deltaTime = epicsTimeDiffInSeconds(&tNow, &pPvt->lastArrayPostTime);

    if (deltaTime > minArrayUpdateTime) {  
        /* Time to post the next array */
        
        /* The callbacks can operate in 2 modes: blocking or non-blocking.
         * If blocking we call NDROIDoCallbacks directly, executing them
         * in the detector callback thread.
         * If non-blocking we put the array on the queue and it executes
         * in our background thread. */
        /* Update the time we last posted an array */
        epicsTimeGetCurrent(&tNow);
        memcpy(&pPvt->lastArrayPostTime, &tNow, sizeof(tNow));
        if (blockingCallbacks) {
            NDROIProcessROIs(pPvt, pArray);
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
                status |= ADParam->setInteger(pPvt->params, NDROIDroppedArrays, droppedArrays);
                 /* This buffer needs to be released twice, because it never made it onto the queue where
                 * it would be released later */
                NDArrayBuff->release(pArray);
            }
        }
    }
    ADParam->callCallbacks(pPvt->params);
    epicsMutexUnlock(pPvt->mutexId);
}


static void NDROITask(drvNDROIPvt *pPvt)
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
        NDROIProcessROIs(pPvt, pArray);
        epicsMutexUnlock(pPvt->mutexId); 
        
        /* We are done with this array buffer */ 
        NDArrayBuff->release(pArray);
    }
}

static int setArrayInterrupt(drvNDROIPvt *pPvt, int connect)
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
                    NDROICallback, pPvt, &pPvt->asynHandleInterruptPvt);
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

static int connectToArrayPort(drvNDROIPvt *pPvt)
{
    asynStatus status;
    asynInterface *pasynInterface;
    NDArray_t array;
    int isConnected;
    char arrayPort[20];
    int arrayAddr;
    const char *functionName = "connectToArrayPort";

    ADParam->getString(pPvt->params, NDROIArrayPort, sizeof(arrayPort), arrayPort);
    ADParam->getInteger(pPvt->params, NDROIArrayAddr, &arrayAddr);
    status = pasynManager->isConnected(pPvt->pasynUserHandle, &isConnected);
    if (status) isConnected=0;

    /* If we are currently connected and there is a callback registered, cancel it */    
    if (isConnected) {
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
    
    /* Enable interrupt callbacks */
    status = setArrayInterrupt(pPvt, 1);

    return(status);
}   



/* asynInt32 interface methods */
static asynStatus readInt32(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 *value)
{
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "readInt32";

    if (function >= NDROIFirstROIParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= pPvt->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, pPvt->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(pPvt->mutexId);
    if (function < NDROIFirstROIParam) {
        /* We just read the current value of the parameter from the parameter library.
         * Those values are updated whenever anything could cause them to change */
        status = ADParam->getInteger(pPvt->params, function, value);
    } else {
        pROI = &pPvt->pROIs[roi];
        status = ADParam->getInteger(pROI->params, function, value);
    }
    
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
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int isConnected;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "writeInt32";

    if (function >= NDROIFirstROIParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= pPvt->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, pPvt->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(pPvt->mutexId);
    /* See if we are connected */
    status = pasynManager->isConnected(pPvt->pasynUserHandle, &isConnected);
    if (status) {isConnected=0; status=asynSuccess;}

    if (function < NDROIFirstROIParam) {
        /* Set the parameter in the parameter library. */
        status |= ADParam->setInteger(pPvt->params, function, value);
        switch(function) {
           case NDROIArrayAddr:
                connectToArrayPort(pPvt);
                break;
            default:
                break;
        }
        /* Do callbacks so higher layers see any changes */
        status |= ADParam->callCallbacks(pPvt->params);
    } else {
        pROI = &pPvt->pROIs[roi];
        status |= ADParam->setInteger(pROI->params, function, value);
        switch(function) {
            case NDROIDim0Min:
                pROI->dims[0].offset = value;
                break;
            case NDROIDim0Size:
                pROI->dims[0].size = value;
                break;
            case NDROIDim0Bin:
                pROI->dims[0].binning = value;
                break;
            case NDROIDim0Reverse:
                pROI->dims[0].reverse = value;
                break;
            case NDROIDim1Min:
                pROI->dims[1].offset = value;
                break;
            case NDROIDim1Size:
                pROI->dims[1].size = value;
                break;
            case NDROIDim1Bin:
                pROI->dims[1].binning = value;
                break;
            case NDROIDim1Reverse:
                pROI->dims[1].reverse = value;
                break;
            default:
                break;
        }
        /* Do callbacks so higher layers see any changes */
        status |= ADParam->callCallbacksAddr(pROI->params, roi);
    }
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%d\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

static asynStatus getBounds(void *drvPvt, asynUser *pasynUser,
                            epicsInt32 *low, epicsInt32 *high)
{
    /* This is only needed for the asynInt32 interface when the device uses raw units.
       Our interface is using engineering units. */
    const char *functionName = "getBounds";
    
    *low = 0;
    *high = 65535;
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: low=%d, high=%d\n", 
              driverName, functionName, *low, *high);
    return(asynSuccess);
}


/* asynFloat64 interface methods */
static asynStatus readFloat64(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 *value)
{
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "readFloat64";

    if (function >= NDROIFirstROIParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= pPvt->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, pPvt->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(pPvt->mutexId);
    if (function < NDROIFirstROIParam) {
        /* We just read the current value of the parameter from the parameter library.
         * Those values are updated whenever anything could cause them to change */
        status = ADParam->getDouble(pPvt->params, function, value);
    } else {
        pROI = &pPvt->pROIs[roi];
        status = ADParam->getDouble(pROI->params, function, value);
    }
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, *value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, *value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeFloat64(void *drvPvt, asynUser *pasynUser, 
                               epicsFloat64 value)
{
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "writeFloat64";

    if (function >= NDROIFirstROIParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= pPvt->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, pPvt->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(pPvt->mutexId);
    if (function < NDROIFirstROIParam) {
        /* Set the parameter in the parameter library. */
        status |= ADParam->setDouble(pPvt->params, function, value);
        switch(function) {
            /* We don't currently need to do anything special when these functions are received */
            default:
                break;
        }
        /* Do callbacks so higher layers see any changes */
        status |= ADParam->callCallbacks(pPvt->params);
    } else {
        pROI = &pPvt->pROIs[roi];
        status |= ADParam->setDouble(pROI->params, function, value);
        switch(function) {
            /* We don't currently need to do anything special when these functions are received */
            default:
                break;
        }
        status |= ADParam->callCallbacksAddr(pROI->params, roi);
    }
    
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
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "readOctet";

    if (function >= NDROIFirstROIParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= pPvt->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, pPvt->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(pPvt->mutexId);
    if (function < NDROIFirstROIParam) {
        /* We just read the current value of the parameter from the parameter library.
         * Those values are updated whenever anything could cause them to change */
        status = ADParam->getString(pPvt->params, function, maxChars, value);
    } else {
        pROI = &pPvt->pROIs[roi];
        status = ADParam->getString(pROI->params, function, maxChars, value);
    }
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%s\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *eomReason = ASYN_EOM_END;
    *nActual = strlen(value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeOctet(void *drvPvt, asynUser *pasynUser,
                             const char *value, size_t nChars, size_t *nActual)
{
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "writeOctet";

    if (function >= NDROIFirstROIParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= pPvt->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, pPvt->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(pPvt->mutexId);

    if (function < NDROIFirstROIParam) {
        /* Set the parameter in the parameter library. */
        status |= ADParam->setString(pPvt->params, function, (char *)value);
        switch(function) {
            case NDROIArrayPort:
                connectToArrayPort(pPvt);
            default:
                break;
        }
         /* Do callbacks so higher layers see any changes */
        status |= ADParam->callCallbacks(pPvt->params);
    } else {
        pROI = &pPvt->pROIs[roi];
        status |= ADParam->setString(pROI->params, function, (char *)value);
         /* Do callbacks so higher layers see any changes */
        status |= ADParam->callCallbacksAddr(pPvt->params, roi);
    }
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%s\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *nActual = nChars;
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

static asynStatus readFloat64Array(void *drvPvt, asynUser *pasynUser,
                                   epicsFloat64 *value, size_t nelements, size_t *nIn)
{
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "readFloat64Array";

    if (function >= NDROIFirstROIParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= pPvt->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, pPvt->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(pPvt->mutexId);
    if (function < NDROIFirstROIParam) {
        /* We don't support reading asynFloat64 arrays except for ROIs */
        status = asynError;
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "%s:%s: invalid request", 
                     driverName, functionName);
    } else {
        size_t ncopy;
        pROI = &pPvt->pROIs[roi];
        switch(function) {
            case NDROIHistArray:
                ncopy = pROI->histogramSize;       
                if (ncopy > nelements) ncopy = nelements;
                memcpy(value, pROI->histogram, ncopy*sizeof(epicsFloat64));
                *nIn = ncopy;
                break;
            default:
                status = asynError;
                epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                             "%s:%s: invalid request", 
                             driverName, functionName);
                break;
        }
    }
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, *value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, *value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}


static asynStatus writeFloat64Array(void *drvPvt, asynUser *pasynUser, 
                                   epicsFloat64 *value, size_t nelements)
{
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    int status = asynSuccess;
    const char* functionName = "writeFloat64Array";
    
    if (pPvt == NULL) return asynError;
    epicsMutexLock(pPvt->mutexId);

    /* The ROI plugin does not support writing float64Array data */    
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
          "%s:%s not currently supported\n", 
          driverName, functionName);
    status = asynError;
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


   

/* asynHandle interface methods */
static asynStatus readNDArray(void *drvPvt, asynUser *pasynUser, void *handle)
{
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    NDArray_t *pArray = handle;
    NDArrayInfo_t arrayInfo;
    int dataSize=0;
    int roi;
    NDROI_t *pROI;
    int status = asynSuccess;
    const char* functionName = "readNDArray";
    
    pasynManager->getAddr(pasynUser, &roi);
    if (roi >= pPvt->maxROIs) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, invalid ROI=%d, max=%d\n", 
              driverName, functionName, roi, pPvt->maxROIs);
        return(asynError);
    }
    
    epicsMutexLock(pPvt->mutexId);
    pROI = &pPvt->pROIs[roi];
    if (!pROI->pArray) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, no valid array available\n", 
              driverName, functionName);
        status = asynError;
    } else {
        pArray->ndims = pROI->pArray->ndims;
        memcpy(pArray->dims, pROI->pArray->dims, sizeof(pArray->dims));
        pArray->dataType = pROI->pArray->dataType;
        NDArrayBuff->getInfo(pROI->pArray, &arrayInfo);
        dataSize = arrayInfo.totalBytes;
        if (dataSize > pArray->dataSize) dataSize = pArray->dataSize;
        memcpy(pArray->pData, pROI->pArray->pData, dataSize);
    }
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d pData=%p\n", 
              driverName, functionName, status, pArray->pData);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s error, maxBytes=%d, data=%p\n", 
              driverName, functionName, dataSize, pArray->pData);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

static asynStatus writeNDArray(void *drvPvt, asynUser *pasynUser, void *handle)
{
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    NDArray_t *pArray = handle;
    int status = asynSuccess;
    const char* functionName = "writeNDArray";
    
    if (pPvt == NULL) return asynError;
    epicsMutexLock(pPvt->mutexId);

    /* The ROI plugin does not allow downloading image data */    
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
          "%s:%s not currently supported, pArray=%p\n", 
          driverName, functionName, pArray);
    status = asynError;
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


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
    if (status) status = ADUtils->findParam(NDROIParamString, NUM_ROI_PARAMS, 
                                            drvInfo, &param);

    /* If we did not find it in that table try our ROI-specific table */
    if (status) status = ADUtils->findParam(NDROINParamString, NUM_ROIN_PARAMS, 
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
    drvNDROIPvt *pPvt = (drvNDROIPvt *)drvPvt;
    interruptNode *pnode;
    ELLLIST *pclientList;
    int i;
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

    }
    if (details > 5) {
        ADParam->dump(pPvt->params);
        for (i=0; i<pPvt->maxROIs; i++) ADParam->dump(pPvt->pROIs[i].params);
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

/*
static asynInt32Array ifaceInt32Array = {
    writeInt32Array,
    readInt32Array,
};
*/

static asynFloat64Array ifaceFloat64Array = {
    writeFloat64Array,
    readFloat64Array,
};


static asynDrvUser ifaceDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};

static asynHandle ifaceHandle = {
    writeNDArray,
    readNDArray
};


/* Configuration routine.  Called directly, or from the iocsh function in drvNDROIEpics */

int drvNDROIConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                      const char *arrayPort, int arrayAddr, int maxROIs)
{
    drvNDROIPvt *pPvt;
    asynStatus status;
    char *functionName = "drvNDROIConfigure";
    asynStandardInterfaces *pInterfaces;
    asynUser *pasynUser;
    int i;

    pPvt = callocMustSucceed(1, sizeof(*pPvt), functionName);
    pPvt->pROIs = callocMustSucceed(maxROIs, sizeof(*pPvt->pROIs), functionName);
    pPvt->portName = epicsStrDup(portName);
    pPvt->maxROIs = maxROIs;

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
/*
    pInterfaces->int32Array.pinterface    = (void *)&ifaceInt32Array;
*/
    pInterfaces->float64Array.pinterface  = (void *)&ifaceFloat64Array;
    pInterfaces->handle.pinterface        = (void *)&ifaceHandle;

    /* Define which interfaces can generate interrupts */
    pInterfaces->octetCanInterrupt        = 1;
    pInterfaces->int32CanInterrupt        = 1;
    pInterfaces->float64CanInterrupt      = 1;
/*
    pInterfaces->int32ArrayCanInterrupt   = 1;
*/
    pInterfaces->float64ArrayCanInterrupt = 1;
    pInterfaces->handleCanInterrupt       = 1;

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
    pPvt->pasynUserHandle->reason = NDArrayData;

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
    pPvt->params = ADParam->create(0, NDROILastDriverParam, &pPvt->asynStdInterfaces);
    if (!pPvt->params) {
        printf("%s: unable to create parameter library\n", functionName);
        return asynError;
    }
        
    /* Initialize the parameter library for each ROI*/
    for (i=0; i<pPvt->maxROIs; i++) {
        pPvt->pROIs[i].params = ADParam->create(NDROIFirstROIParam, 
                                                NUM_ROIN_PARAMS, &pPvt->asynStdInterfaces);
        if (!pPvt->pROIs[i].params) {
            printf("%s: unable to create parameter library for ROI %d\n", functionName, i);
            return asynError;
        }
    }
        
   /* Create the thread that does the array callbacks */
    status = (epicsThreadCreate("NDROITask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)NDROITask,
                                pPvt) == NULL);
    if (status) {
        printf("%s: epicsThreadCreate failure\n", functionName);
        return asynError;
    }

    /* Set the initial values of some parameters */
    ADParam->setInteger(pPvt->params, NDROIArrayCounter, 0);
    ADParam->setInteger(pPvt->params, NDROIDroppedArrays, 0);
    ADParam->setString (pPvt->params, NDROIArrayPort, arrayPort);
    ADParam->setInteger(pPvt->params, NDROIArrayAddr, arrayAddr);
    ADParam->setInteger(pPvt->params, NDROIBlockingCallbacks, 0);
   
    /* Try to connect to the array port */
    status = connectToArrayPort(pPvt);
    
    return asynSuccess;
}

