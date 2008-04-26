/*
 * drvNDROI.c
 * 
 * Asyn driver for callbacks to standard asyn array interfaces for NDArray drivers.
 * This is commonly used for EPICS waveform records.
 *
 * Author: Mark Rivers
 *
 * Created April 23, 2008
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

#include "ADInterface.h"
#include "NDArrayBuff.h"
#include "ADParamLib.h"
#include "ADUtils.h"
#include "asynHandle.h"
#include "NDPluginROI.h"
#include "drvNDROI.h"

const char *driverName="NDPluginROI";


#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)



/* Local functions, not in any interface */
void NDPluginROI::float64ArrayCallback(epicsFloat64 *pData, int len, int reason, int addr)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    int address;
    
    pasynManager->interruptStart(this->asynStdInterfaces.float64ArrayInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynFloat64ArrayInterrupt *pInterrupt = (asynFloat64ArrayInterrupt *)pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &address); 
        if ((pInterrupt->pasynUser->reason == reason) && 
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt,
                                 pInterrupt->pasynUser,
                                 pData, len);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(this->asynStdInterfaces.float64ArrayInterruptPvt);
}


/* These macros save a lot of code when handling different data types */
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

#define DO_ALL_DATATYPES(FUNCTION) { \
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



void NDPluginROI::processCallbacks(NDArray_t *pArray)
{
    /* This function computes the ROIs.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
     
    int use, computeStatistics, computeHistogram, computeProfiles;
    int i, dataType;
    int histSize;
    int arrayCounter;
    int roi, dim;
    int status;
    double histMin, histMax, entropy;
    double min=0, max=0, mean, total=0, net, value;
    double counts;
    NDArrayInfo_t arrayInfo;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS], *pDim;
    NDROI_t *pROI;
    const char* functionName = "processCallbacks";
     
    /* Loop over the ROIs in this driver */
    for (roi=0; roi<this->maxROIs; roi++) {
        pROI = &this->pROIs[roi];
        ADParam->getInteger(pROI->params, NDPluginROIUse, &use);
        /* Free the previous array */
        if (pROI->pArray) {
            NDArrayBuff->release(pROI->pArray);
            pROI->pArray = NULL;
        }
        if (!use) continue;
        /* Need to fetch all of these parameters while we still have the mutex */
        ADParam->getInteger(pROI->params, NDPluginROIComputeStatistics, &computeStatistics);
        ADParam->getInteger(pROI->params, NDPluginROIComputeHistogram, &computeHistogram);
        ADParam->getInteger(pROI->params, NDPluginROIComputeProfiles, &computeProfiles);
        ADParam->getInteger(pROI->params, NDPluginROIDataType, &dataType);
        ADParam->getDouble(pROI->params, NDPluginROIHistMin, &histMin);
        ADParam->getDouble(pROI->params, NDPluginROIHistMax, &histMax);
        ADParam->getInteger(pROI->params, NDPluginROIHistSize, &histSize);
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
        epicsMutexUnlock(this->mutexId);
    
        /* Extract this ROI from the input array */
        if (dataType == -1) dataType = pArray->dataType;
        NDArrayBuff->convert(pArray, &pROI->pArray, dataType, dims);
        /* Call any clients who have registered for NDArray callbacks */
        ADUtils->handleCallback(this->asynStdInterfaces.handleInterruptPvt, 
                                pROI->pArray, NDArrayData, roi);
        NDArrayBuff->getInfo(pROI->pArray, &arrayInfo);
 
        if (computeStatistics) {
            DO_ALL_DATATYPES(COMPUTE_STATISTICS);
            net = total;
            mean = total / arrayInfo.nElements;
            ADParam->setDouble(pROI->params, NDPluginROIMinValue, min);
            ADParam->setDouble(pROI->params, NDPluginROIMaxValue, max);
            ADParam->setDouble(pROI->params, NDPluginROIMeanValue, mean);
            ADParam->setDouble(pROI->params, NDPluginROITotal, total);
            ADParam->setDouble(pROI->params, NDPluginROINet, net);
            asynPrint(this->pasynUser, ASYN_TRACEIO_DRIVER, 
                (char *)pROI->pArray->pData, arrayInfo.totalBytes,
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
            DO_ALL_DATATYPES(COMPUTE_HISTOGRAM);
            entropy = 0;
            for (i=0; i<histSize; i++) {
                counts = pROI->histogram[i];
                if (counts <= 0) counts = 1;
                entropy += counts * log(counts);
            }
            entropy = -entropy / arrayInfo.nElements;
            ADParam->setDouble(pROI->params, NDPluginROIHistEntropy, entropy);
            float64ArrayCallback(pROI->histogram, histSize, NDPluginROIHistArray, roi);
        }

        /* We must enter the loop and exit with the mutex locked */
        epicsMutexLock(this->mutexId);
        ADParam->callCallbacksAddr(pROI->params, roi);
    }
    
    /* The plugin base has reserved this array in case we want to keep it.
     * We don't, so release it */
    NDArrayBuff->release(pArray);

    /* Update the parameters.  The counter should be updated after data are posted
     * because clients might use that to detect new data */
    ADParam->getInteger(this->params, NDPluginBaseArrayCounter, &arrayCounter);    
    arrayCounter++;
    ADParam->setInteger(this->params, NDPluginBaseArrayCounter, arrayCounter);    
    ADParam->callCallbacks(this->params);
}


/* asynInt32 interface methods */
asynStatus NDPluginROI::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "readInt32";

    if (function >= NDPluginROIFirstROINParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= this->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, this->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(this->mutexId);
    if (function < NDPluginROIFirstROINParam) {
        /* We just read the current value of the parameter from the parameter library.
         * Those values are updated whenever anything could cause them to change */
        status = ADParam->getInteger(this->params, function, value);
    } else {
        pROI = &this->pROIs[roi];
        status = ADParam->getInteger(pROI->params, function, value);
    }
    
    if (status) 
       epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, *value);
    else        
        asynPrint(this->pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readInt32: function=%d, value=%d\n", 
              driverName, function, *value);
    epicsMutexUnlock(this->mutexId);
    return(status);
}

asynStatus NDPluginROI::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int isConnected;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "writeInt32";

    if (function >= NDPluginROIFirstROINParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= this->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, this->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(this->mutexId);
    /* See if we are connected */
    status = pasynManager->isConnected(this->pasynUserHandle, &isConnected);
    if (status) {isConnected=0; status=asynSuccess;}

    if (function < NDPluginROIFirstROINParam) {
        /* Set the parameter in the parameter library. */
        status = ADParam->setInteger(this->params, function, value);
        switch(function) {
            default:
                /* This was not a parameter that this driver understands, try the base class */
                epicsMutexUnlock(this->mutexId);
                status = NDPluginBase::writeInt32(pasynUser, value);
                epicsMutexLock(this->mutexId);
                break;
        }
        /* Do callbacks so higher layers see any changes */
        status = ADParam->callCallbacks(this->params);
    } else {
        pROI = &this->pROIs[roi];
        status = ADParam->setInteger(pROI->params, function, value);
        switch(function) {
            case NDPluginROIDim0Min:
                pROI->dims[0].offset = value;
                break;
            case NDPluginROIDim0Size:
                pROI->dims[0].size = value;
                break;
            case NDPluginROIDim0Bin:
                pROI->dims[0].binning = value;
                break;
            case NDPluginROIDim0Reverse:
                pROI->dims[0].reverse = value;
                break;
            case NDPluginROIDim1Min:
                pROI->dims[1].offset = value;
                break;
            case NDPluginROIDim1Size:
                pROI->dims[1].size = value;
                break;
            case NDPluginROIDim1Bin:
                pROI->dims[1].binning = value;
                break;
            case NDPluginROIDim1Reverse:
                pROI->dims[1].reverse = value;
                break;
            default:
                break;
        }
        /* Do callbacks so higher layers see any changes */
        status = ADParam->callCallbacksAddr(pROI->params, roi);
    }
    
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

/* asynFloat64 interface methods */
asynStatus NDPluginROI::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "readFloat64";

    if (function >= NDPluginROIFirstROINParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= this->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, this->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(this->mutexId);
    if (function < NDPluginROIFirstROINParam) {
        /* We just read the current value of the parameter from the parameter library.
         * Those values are updated whenever anything could cause them to change */
        status = ADParam->getDouble(this->params, function, value);
    } else {
        pROI = &this->pROIs[roi];
        status = ADParam->getDouble(pROI->params, function, value);
    }
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

asynStatus NDPluginROI::writeFloat64(asynUser *pasynUser, 
                               epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "writeFloat64";

    if (function >= NDPluginROIFirstROINParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= this->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, this->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(this->mutexId);
    if (function < NDPluginROIFirstROINParam) {
        /* Set the parameter in the parameter library. */
        status = ADParam->setDouble(this->params, function, value);
        switch(function) {
            /* We don't currently need to do anything special when these functions are received */
            default:
                /* This was not a parameter that this driver understands, try the base class */
                epicsMutexUnlock(this->mutexId);
                status = NDPluginBase::writeFloat64(pasynUser, value);
                epicsMutexLock(this->mutexId);
                break;
        }
        /* Do callbacks so higher layers see any changes */
        status = ADParam->callCallbacks(this->params);
    } else {
        pROI = &this->pROIs[roi];
        status = ADParam->setDouble(pROI->params, function, value);
        switch(function) {
            /* We don't currently need to do anything special when these functions are received */
            default:
                break;
        }
        status = ADParam->callCallbacksAddr(pROI->params, roi);
    }
    
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
asynStatus NDPluginROI::readOctet(asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "readOctet";

    if (function >= NDPluginROIFirstROINParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= this->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, this->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(this->mutexId);
    if (function < NDPluginROIFirstROINParam) {
        /* We just read the current value of the parameter from the parameter library.
         * Those values are updated whenever anything could cause them to change */
        status = ADParam->getString(this->params, function, maxChars, value);
    } else {
        pROI = &this->pROIs[roi];
        status = ADParam->getString(pROI->params, function, maxChars, value);
    }
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

asynStatus NDPluginROI::writeOctet(asynUser *pasynUser,
                                   const char *value, size_t nChars, size_t *nActual)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "writeOctet";

    if (function >= NDPluginROIFirstROINParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= this->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, this->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(this->mutexId);

    if (function < NDPluginROIFirstROINParam) {
        /* Set the parameter in the parameter library. */
        status = ADParam->setString(this->params, function, (char *)value);
        switch(function) {
            default:
                /* This was not a parameter that this driver understands, try the base class */
                epicsMutexUnlock(this->mutexId);
                status = NDPluginBase::writeOctet(pasynUser, value, nChars, nActual);
                epicsMutexLock(this->mutexId);
                break;
        }
         /* Do callbacks so higher layers see any changes */
        status = ADParam->callCallbacks(this->params);
    } else {
        pROI = &this->pROIs[roi];
        status = ADParam->setString(pROI->params, function, (char *)value);
         /* Do callbacks so higher layers see any changes */
        status = ADParam->callCallbacksAddr(this->params, roi);
    }
    
    if (status) 
       epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%s", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *nActual = nChars;
    epicsMutexUnlock(this->mutexId);
    return status;
}


static asynStatus readFloat64Array(void *drvPvt, asynUser *pasynUser, 
                                    epicsFloat64 *value, size_t nElements, size_t *nIn)
{
    NDPluginROI *pPvt = (NDPluginROI *)drvPvt;
    
    return(pPvt->readFloat64Array(pasynUser, value, nElements, nIn));
}


asynStatus NDPluginROI::readFloat64Array(asynUser *pasynUser,
                                   epicsFloat64 *value, size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "readFloat64Array";

    if (function >= NDPluginROIFirstROINParam) {
        pasynManager->getAddr(pasynUser, &roi);
        if (roi >= this->maxROIs) {
            asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error, invalid ROI=%d, max=%d\n", 
                  driverName, functionName, roi, this->maxROIs);
            return(asynError);
        }
    }

    epicsMutexLock(this->mutexId);
    if (function < NDPluginROIFirstROINParam) {
        /* We don't support reading asynFloat64 arrays except for ROIs */
        status = asynError;
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "%s:%s: invalid request", 
                     driverName, functionName);
    } else {
        size_t ncopy;
        pROI = &this->pROIs[roi];
        switch(function) {
            case NDPluginROIHistArray:
                ncopy = pROI->histogramSize;       
                if (ncopy > nElements) ncopy = nElements;
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

static asynStatus writeFloat64Array(void *drvPvt, asynUser *pasynUser, 
                                    epicsFloat64 *value, size_t nElements)
{
    NDPluginROI *pPvt = (NDPluginROI *)drvPvt;
    
    return(pPvt->writeFloat64Array(pasynUser, value, nElements));
}


asynStatus NDPluginROI::writeFloat64Array(asynUser *pasynUser, 
                                          epicsFloat64 *value, size_t nelements)
{
    asynStatus status = asynSuccess;
    const char* functionName = "writeFloat64Array";
    
    epicsMutexLock(this->mutexId);

    /* The ROI plugin does not support writing float64Array data */    
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
          "%s:%s not currently supported\n", 
          driverName, functionName);
    status = asynError;
    epicsMutexUnlock(this->mutexId);
    return status;
}


   

/* asynHandle interface methods */
asynStatus NDPluginROI::readNDArray(asynUser *pasynUser, void *handle)
{
    NDArray_t *pArray = (NDArray_t *)handle;
    NDArrayInfo_t arrayInfo;
    int dataSize=0;
    int roi;
    NDROI_t *pROI;
    asynStatus status = asynSuccess;
    const char* functionName = "readNDArray";
    
    pasynManager->getAddr(pasynUser, &roi);
    if (roi >= this->maxROIs) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, invalid ROI=%d, max=%d\n", 
              driverName, functionName, roi, this->maxROIs);
        return(asynError);
    }
    
    epicsMutexLock(this->mutexId);
    pROI = &this->pROIs[roi];
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
    epicsMutexUnlock(this->mutexId);
    return status;
}

/* asynDrvUser interface methods */
asynStatus NDPluginROI::drvUserCreate(asynUser *pasynUser,
                                      const char *drvInfo, 
                                      const char **pptypeName, size_t *psize)
{
    int status;
    int param;

    /* Look in the driver table */
    status = ADUtils->findParam(NDPluginROINParamString, NUM_ROIN_PARAMS, 
                                            drvInfo, &param);

    /* If we did not find it in that table try the plugin base */
    if (status) status = ADUtils->findParam(NDPluginBaseParamString, NUM_ND_PLUGIN_BASE_PARAMS, 
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

    

/* asynCommon interface methods */

void NDPluginROI::report(FILE *fp, int details)
{
    int i;
    
    NDPluginBase::report(fp, details);
    if (details > 5) {
        for (i=0; i<this->maxROIs; i++) ADParam->dump(this->pROIs[i].params);
    }
}

static asynFloat64Array ifaceFloat64Array = {
    writeFloat64Array,
    readFloat64Array,
};



extern "C" int drvNDROIConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                                 const char *NDArrayPort, int NDArrayAddr, int maxROIs)
{
    new NDPluginROI(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxROIs);
    return(asynSuccess);
}

NDPluginROI::NDPluginROI(const char *portName, int queueSize, int blockingCallbacks, 
                         const char *NDArrayPort, int NDArrayAddr, int maxROIs)
    /* Invoke the base class constructor */
    : NDPluginBase(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, NDPluginBaseLastParam)
{
    asynStatus status;
    char *functionName = "NDPluginROI";
    asynStandardInterfaces *pInterfaces;
    int i;

    this->maxROIs = maxROIs;
    this->pROIs = (NDROI_t *)callocMustSucceed(maxROIs, sizeof(*this->pROIs), functionName);

   /* Set addresses of asyn interfaces */
    pInterfaces = &this->asynStdInterfaces;
    
    pInterfaces->float64Array.pinterface  = (void *)&ifaceFloat64Array;
    pInterfaces->float64ArrayCanInterrupt = 1;

    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     this->pasynUser, this);
    if (status != asynSuccess) {
        printf("%s:%s ERROR: Can't register interfaces: %s.\n",
               driverName, functionName, this->pasynUser->errorMessage);
        return;
    }
    
    /* Connect to our device for asynTrace */
    status = pasynManager->connectDevice(this->pasynUser, portName, 0);
    if (status != asynSuccess) {
        printf("%s, connectDevice failed\n", functionName);
        return;
    }

    /* Initialize the parameter library for each ROI*/
    for (i=0; i<this->maxROIs; i++) {
        this->pROIs[i].params = ADParam->create(NDPluginROIFirstROINParam, 
                                                NUM_ROIN_PARAMS, &this->asynStdInterfaces);
        if (!this->pROIs[i].params) {
            printf("%s:%s unable to create parameter library for ROI %d\n", 
                driverName, functionName, i);
            return;
        }
    }
        
    /* Try to connect to the array port */
    status = connectToArrayPort();
}

