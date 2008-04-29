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

#include <epicsString.h>
#include <epicsMutex.h>

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



template <typename epicsType> 
void doComputeHistogram(void *pROIData, int nElements, 
                      double histMin, double histMax, int histSize, 
                      double *histogram)
{
    int i;
    double scale = histSize / (histMax - histMin);
    int bin;
    epicsType *pData = (epicsType *)pROIData;
    double value;

    for (i=0; i<nElements; i++) {
        value = (double)pData[i];
        bin = (int) (((value - histMin) * scale) + 0.5);
        if ((bin >= 0) && (bin < histSize))histogram[bin]++;
    }
}

template <typename epicsType> 
void doComputeStatistics(void *pROIData, int nElements,double *min, double *max, double *total)
{
    int i;
    epicsType *pData = (epicsType *)pROIData;
    double value;

    *min = (double) pData[0];
    *max = (double) pData[0];
    *total = 0.; \
    for (i=0; i<nElements; i++) {
        value = (double)pData[i];
        if (value < *min) *min = value;
        if (value > *max) *max = value;
        *total += value;
    }
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
    int roi, dim;
    int status;
    double histMin, histMax, entropy;
    double min=0, max=0, mean, total=0, net;
    double counts;
    NDArray_t *pROIArray;
    PARAMS ROIParams;
    NDArrayInfo_t arrayInfo;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS], *pDim;
    NDROI_t *pROI;
    const char* functionName = "processCallbacks";
     
    /* Call the base class method */
    NDPluginBase::processCallbacks(pArray);
    
    /* Loop over the ROIs in this driver */
    for (roi=0; roi<this->maxROIs; roi++) {
        pROI       = &this->pROIs[roi];
        ROIParams  = this->params[roi];
        /* We always keep the last array so read() can use it.  
         * Release previous one. Reserve new one below. */
        if (this->pArrays[roi]) {
            NDArrayBuff->release(this->pArrays[roi]);
            this->pArrays[roi] = NULL;
        }
        ADParam->getInteger(ROIParams, NDPluginROIUse, &use);
        if (!use) continue;

        /* Need to fetch all of these parameters while we still have the mutex */
        ADParam->getInteger(ROIParams, NDPluginROIComputeStatistics, &computeStatistics);
        ADParam->getInteger(ROIParams, NDPluginROIComputeHistogram, &computeHistogram);
        ADParam->getInteger(ROIParams, NDPluginROIComputeProfiles, &computeProfiles);
        ADParam->getInteger(ROIParams, NDPluginROIDataType, &dataType);
        ADParam->getDouble (ROIParams, NDPluginROIHistMin, &histMin);
        ADParam->getDouble (ROIParams, NDPluginROIHistMax, &histMax);
        ADParam->getInteger(ROIParams, NDPluginROIHistSize, &histSize);
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
    
        /* Extract this ROI from the input array.  The convert() function allocates
         * a new array and it is reserved (reference count = 1) */
        if (dataType == -1) dataType = pArray->dataType;
        NDArrayBuff->convert(pArray, &this->pArrays[roi], dataType, dims);
        pROIArray  = this->pArrays[roi];

        /* Call any clients who have registered for NDArray callbacks */
        doCallbacksNDArray(pROIArray, NDArrayData, roi);

        NDArrayBuff->getInfo(pROIArray, &arrayInfo);

        if (computeStatistics) {
            switch(pROIArray->dataType) {
                case NDInt8:
                    doComputeStatistics<epicsInt8>(pROIArray->pData, 
                                        arrayInfo.nElements, &min, &max, &total);       
                    break;
                case NDUInt8:
                    doComputeStatistics<epicsUInt8>(pROIArray->pData, 
                                        arrayInfo.nElements, &min, &max, &total);       
                    break;
                case NDInt16:
                    doComputeStatistics<epicsInt16>(pROIArray->pData, 
                                        arrayInfo.nElements, &min, &max, &total);       
                    break;
                case NDUInt16:
                    doComputeStatistics<epicsUInt16>(pROIArray->pData, 
                                        arrayInfo.nElements, &min, &max, &total);       
                    break;
                case NDInt32:
                    doComputeStatistics<epicsInt32>(pROIArray->pData, 
                                        arrayInfo.nElements, &min, &max, &total);       
                    break;
                case NDUInt32:
                    doComputeStatistics<epicsUInt32>(pROIArray->pData, 
                                        arrayInfo.nElements, &min, &max, &total);       
                    break;
                case NDFloat32:
                    doComputeStatistics<epicsFloat32>(pROIArray->pData, 
                                        arrayInfo.nElements, &min, &max, &total);       
                    break;
                case NDFloat64:
                    doComputeStatistics<epicsFloat64>(pROIArray->pData, 
                                        arrayInfo.nElements, &min, &max, &total);       
                    break;
                default:
                    status = ND_ERROR;
                break;
            }
            net = total;
            mean = total / arrayInfo.nElements;
            ADParam->setDouble(ROIParams, NDPluginROIMinValue, min);
            ADParam->setDouble(ROIParams, NDPluginROIMaxValue, max);
            ADParam->setDouble(ROIParams, NDPluginROIMeanValue, mean);
            ADParam->setDouble(ROIParams, NDPluginROITotal, total);
            ADParam->setDouble(ROIParams, NDPluginROINet, net);
            asynPrint(this->pasynUser, ASYN_TRACEIO_DRIVER, 
                (char *)pROIArray->pData, arrayInfo.totalBytes,
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
            switch(pROIArray->dataType) {
                case NDInt8:
                    doComputeHistogram<epicsInt8>(pROIArray->pData, 
                                        arrayInfo.nElements, histMin, histMax, histSize, 
                                        pROI->histogram);           
                    break;
                case NDUInt8:
                    doComputeHistogram<epicsUInt8>(pROIArray->pData, 
                                        arrayInfo.nElements, histMin, histMax, histSize, 
                                        pROI->histogram);           
                    break;
                case NDInt16:
                    doComputeHistogram<epicsInt16>(pROIArray->pData, 
                                        arrayInfo.nElements, histMin, histMax, histSize, 
                                        pROI->histogram);           
                    break;
                case NDUInt16:
                    doComputeHistogram<epicsUInt16>(pROIArray->pData, 
                                        arrayInfo.nElements, histMin, histMax, histSize, 
                                        pROI->histogram);           
                    break;
                case NDInt32:
                    doComputeHistogram<epicsInt32>(pROIArray->pData, 
                                        arrayInfo.nElements, histMin, histMax, histSize, 
                                        pROI->histogram);           
                    break;
                case NDUInt32:
                    doComputeHistogram<epicsUInt32>(pROIArray->pData, 
                                        arrayInfo.nElements, histMin, histMax, histSize, 
                                        pROI->histogram);           
                    break;
                case NDFloat32:
                    doComputeHistogram<epicsFloat32>(pROIArray->pData, 
                                        arrayInfo.nElements, histMin, histMax, histSize, 
                                        pROI->histogram);           
                    break;
                case NDFloat64:
                    doComputeHistogram<epicsFloat64>(pROIArray->pData, 
                                        arrayInfo.nElements, histMin, histMax, histSize, 
                                        pROI->histogram);           
                    break;
                default:
                    status = ND_ERROR;
                break;
            }
            entropy = 0;
            for (i=0; i<histSize; i++) {
                counts = pROI->histogram[i];
                if (counts <= 0) counts = 1;
                entropy += counts * log(counts);
            }
            entropy = -entropy / arrayInfo.nElements;
            ADParam->setDouble(ROIParams, NDPluginROIHistEntropy, entropy);
            doCallbacksFloat64Array(pROI->histogram, histSize, NDPluginROIHistArray, roi);
        }

        /* We must enter the loop and exit with the mutex locked */
        epicsMutexLock(this->mutexId);
        ADParam->callCallbacksAddr(ROIParams, roi);
    }
    
    ADParam->callCallbacksAddr(this->params[0], 0);
}


asynStatus NDPluginROI::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi=0;
    NDROI_t *pROI;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, functionName, &roi); if (status != asynSuccess) return(status);

    epicsMutexLock(this->mutexId);
    pROI = &this->pROIs[roi];
    status = ADParam->setInteger(this->params[roi], function, value);
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
            /* This was not a parameter that this driver understands, try the base class */
            epicsMutexUnlock(this->mutexId);
            status = NDPluginBase::writeInt32(pasynUser, value);
            epicsMutexLock(this->mutexId);
            break;
    }
    /* Do callbacks so higher layers see any changes */
    status = ADParam->callCallbacksAddr(this->params[roi], roi);
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, roi=%d, value=%d\n", 
              driverName, functionName, function, roi, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}

asynStatus NDPluginROI::readFloat64Array(asynUser *pasynUser,
                                   epicsFloat64 *value, size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI_t *pROI;
    const char* functionName = "readFloat64Array";

    status = getAddress(pasynUser, functionName, &roi); if (status != asynSuccess) return(status);
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
                   NDArrayPort, NDArrayAddr, maxROIs, NDPluginROILastROINParam)
{
    asynStatus status;
    char *functionName = "NDPluginROI";
    asynStandardInterfaces *pInterfaces = &this->asynStdInterfaces;


    this->maxROIs = maxROIs;
    this->pROIs = (NDROI_t *)callocMustSucceed(maxROIs, sizeof(*this->pROIs), functionName);

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
        
    /* Try to connect to the array port */
    status = connectToArrayPort();
}

