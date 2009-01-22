/*
 * NDPluginROI.cpp
 * 
 * Region-of-Interest (ROI) plugin
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

#include "NDArray.h"
#include "NDPluginROI.h"
#include "drvNDROI.h"

static asynParamString_t NDPluginROINParamString[] = {
    {NDPluginROIName,                   "NAME"},
    {NDPluginROIUse,                    "USE"},
    {NDPluginROIComputeStatistics,      "COMPUTE_STATISTICS"},
    {NDPluginROIComputeHistogram,       "COMPUTE_HISTOGRAM"},
    {NDPluginROIComputeProfiles,        "COMPUTE_PROFILES"},
    {NDPluginROIHighlight,              "HIGHLIGHT"},

    {NDPluginROIDim0Min,                "DIM0_MIN"},
    {NDPluginROIDim0Size,               "DIM0_SIZE"},
    {NDPluginROIDim0MaxSize,            "DIM0_MAX_SIZE"},
    {NDPluginROIDim0Bin,                "DIM0_BIN"},
    {NDPluginROIDim0Reverse,            "DIM0_REVERSE"},
    {NDPluginROIDim1Min,                "DIM1_MIN"},
    {NDPluginROIDim1Size,               "DIM1_SIZE"},
    {NDPluginROIDim1MaxSize,            "DIM1_MAX_SIZE"},
    {NDPluginROIDim1Bin,                "DIM1_BIN"},
    {NDPluginROIDim1Reverse,            "DIM1_REVERSE"},
    {NDPluginROIDim2Min,                "DIM2_MIN"},
    {NDPluginROIDim2Size,               "DIM2_SIZE"},
    {NDPluginROIDim2MaxSize,            "DIM2_MAX_SIZE"},
    {NDPluginROIDim2Bin,                "DIM2_BIN"},
    {NDPluginROIDim2Reverse,            "DIM2_REVERSE"},
    {NDPluginROIDataType,               "DATA_TYPE"},

    {NDPluginROIBgdWidth,               "BGD_WIDTH"},
    {NDPluginROIMinValue,               "MIN_VALUE"},
    {NDPluginROIMaxValue,               "MAX_VALUE"},
    {NDPluginROIMeanValue,              "MEAN_VALUE"},
    {NDPluginROITotal,                  "TOTAL"},
    {NDPluginROINet,                    "NET"},
    {NDPluginROITotalArray,             "TOTAL_ARRAY"},
    {NDPluginROINetArray,               "NET_ARRAY"},

    {NDPluginROIHistSize,               "HIST_SIZE"},
    {NDPluginROIHistMin,                "HIST_MIN"},
    {NDPluginROIHistMax,                "HIST_MAX"},
    {NDPluginROIHistEntropy,            "HIST_ENTROPY"},
    {NDPluginROIHistArray,              "HIST_ARRAY"},
};

static const char *driverName="NDPluginROI";

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)



template <typename epicsType> 
void doComputeHistogramT(NDArray *pArray, NDROI_t *pROI)
{
    epicsType *pData = (epicsType *)pArray->pData;
    int i;
    double scale = (double)pROI->histSize / (double)(pROI->histMax - pROI->histMin);
    int bin;
    double value;
    NDArrayInfo arrayInfo;

    pArray->getInfo(&arrayInfo);
    pROI->nElements = arrayInfo.nElements;

    for (i=0; i<pROI->nElements; i++) {
        value = (double)pData[i];
        bin = (int) (((value - pROI->histMin) * scale) + 0.5);
        if ((bin >= 0) && (bin < pROI->histSize))pROI->histogram[bin]++;
    }
}

int doComputeHistogram(NDArray *pArray, NDROI_t *pROI)
{
    switch(pArray->dataType) {
        case NDInt8:
            doComputeHistogramT<epicsInt8>(pArray, pROI);           
            break;
        case NDUInt8:
            doComputeHistogramT<epicsUInt8>(pArray, pROI);           
            break;
        case NDInt16:
            doComputeHistogramT<epicsInt16>(pArray, pROI);           
            break;
        case NDUInt16:
            doComputeHistogramT<epicsUInt16>(pArray, pROI);           
            break;
        case NDInt32:
            doComputeHistogramT<epicsInt32>(pArray, pROI);           
            break;
        case NDUInt32:
            doComputeHistogramT<epicsUInt32>(pArray, pROI);           
            break;
        case NDFloat32:
            doComputeHistogramT<epicsFloat32>(pArray, pROI);           
            break;
        case NDFloat64:
            doComputeHistogramT<epicsFloat64>(pArray, pROI);           
            break;
        default:
            return(ND_ERROR);
        break;
    }
    return(ND_SUCCESS);
}

template <typename epicsType> 
void doComputeStatisticsT(NDArray *pArray, NDROI_t *pROI)
{
    int i;
    epicsType *pData = (epicsType *)pArray->pData;
    NDArrayInfo arrayInfo;
    double value;

    pArray->getInfo(&arrayInfo);
    pROI->nElements = arrayInfo.nElements;
    pROI->min = (double) pData[0];
    pROI->max = (double) pData[0];
    pROI->total = 0.;
    for (i=0; i<pROI->nElements; i++) {
        value = (double)pData[i];
        if (value < pROI->min) pROI->min = value;
        if (value > pROI->max) pROI->max = value;
        pROI->total += value;
    }
    pROI->net = pROI->total;
    pROI->mean = pROI->total / pROI->nElements;
}

int doComputeStatistics(NDArray *pArray, NDROI_t *pROI)
{

    switch(pArray->dataType) {
        case NDInt8:
            doComputeStatisticsT<epicsInt8>(pArray, pROI);       
            break;
        case NDUInt8:
            doComputeStatisticsT<epicsUInt8>(pArray, pROI);       
            break;
        case NDInt16:
            doComputeStatisticsT<epicsInt16>(pArray, pROI);       
            break;
        case NDUInt16:
            doComputeStatisticsT<epicsUInt16>(pArray, pROI);       
            break;
        case NDInt32:
            doComputeStatisticsT<epicsInt32>(pArray, pROI);       
            break;
        case NDUInt32:
            doComputeStatisticsT<epicsUInt32>(pArray, pROI);       
            break;
        case NDFloat32:
            doComputeStatisticsT<epicsFloat32>(pArray, pROI);       
            break;
        case NDFloat64:
            doComputeStatisticsT<epicsFloat64>(pArray, pROI);       
            break;
        default:
            return(ND_ERROR);
        break;
    }
    return(ND_SUCCESS);
}

template <typename epicsType> 
void highlightROIT(NDArray *pArray, NDROI_t *pROI, double dvalue)
{
    int xmin, xmax, ymin, ymax, ix, iy;
    epicsType *pRow, value=(epicsType)dvalue;
    
    xmin = pROI->dims[0].offset;
    xmax = xmin + pROI->dims[0].size;
    ymin = pROI->dims[1].offset;
    ymax = ymin + pROI->dims[1].size;
    for (iy=ymin; iy<ymax; iy++) {
        pRow = (epicsType *)pArray->pData + iy*pArray->dims[0].size;
        if ((iy == ymin) || (iy == ymax-1)) {
            for (ix=xmin; ix<xmax; ix++) pRow[ix] = value;
        } else {
            pRow[xmin] = value; 
            pRow[xmax-1] = value; 
        }
    }
}

int highlightROI(NDArray *pArray, NDROI_t *pROI, double dvalue)
{
    switch(pArray->dataType) {
        case NDInt8:
            highlightROIT<epicsInt8>(pArray, pROI, dvalue);       
            break;
        case NDUInt8:
            highlightROIT<epicsUInt8>(pArray, pROI, dvalue);       
            break;
        case NDInt16:
            highlightROIT<epicsInt16>(pArray, pROI, dvalue);       
            break;
        case NDUInt16:
            highlightROIT<epicsUInt16>(pArray, pROI, dvalue);       
            break;
        case NDInt32:
            highlightROIT<epicsInt32>(pArray, pROI, dvalue);       
            break;
        case NDUInt32:
            highlightROIT<epicsUInt32>(pArray, pROI, dvalue);       
            break;
        case NDFloat32:
            highlightROIT<epicsFloat32>(pArray, pROI, dvalue);       
            break;
        case NDFloat64:
            highlightROIT<epicsFloat64>(pArray, pROI, dvalue);       
            break;
        default:
            return(ND_ERROR);
        break;
    }
    return(ND_SUCCESS);
}


void NDPluginROI::processCallbacks(NDArray *pArray)
{
    /* This function computes the ROIs.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
     
    int use, computeStatistics, computeHistogram, computeProfiles;
    int i;
    int dataType;
    int histSize;
    int roi, dim;
    int status;
    double counts, entropy;
    NDArray *pROIArray;
    NDArray *pHighlights=NULL, *pBgdArray=NULL;
    NDArrayInfo_t arrayInfo;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS], bgdDims[ND_ARRAY_MAX_DIMS], tempDim, *pDim;
    int userDims[ND_ARRAY_MAX_DIMS];
    NDROI_t *pROI, ROITemp, *pROITemp=&ROITemp;
    int highlight;
    int bgdPixels;
    double bgdCounts, avgBgd;
    const char* functionName = "processCallbacks";
     
    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
        
    getIntegerParam(NDPluginROIHighlight, &highlight);
    if (highlight && (pArray->ndims == 2)) {
        /* We need to highlight the ROIs.
         * We do this by searching for the maximum element in the data and setting the
         * outline of each ROI to this value.  However, we are not allowed to modify the data
         * since other callbacks could be using it, so we need to make a copy */
        pHighlights = this->pNDArrayPool->copy(pArray, NULL, 1);
        status = doComputeStatistics(pHighlights, &ROITemp);
        if (ROITemp.max == 0) ROITemp.max=1.;
        for (roi=0; roi<this->maxROIs; roi++) {
            pROI = &this->pROIs[roi];
            getIntegerParam(roi, NDPluginROIUse, &use);
            if (!use) continue;
            highlightROI(pHighlights, pROI, ROITemp.max);
        }
    }

    /* Loop over the ROIs in this driver */
    for (roi=0; roi<this->maxROIs; roi++) {
        pROI        = &this->pROIs[roi];
        /* We always keep the last array so read() can use it.  
         * Release previous one. Reserve new one below. */
        if (this->pArrays[roi]) {
            this->pArrays[roi]->release();
            this->pArrays[roi] = NULL;
        }
        getIntegerParam(roi, NDPluginROIUse, &use);
        if (!use) {
            continue;
        }
        
        /* Need to fetch all of these parameters while we still have the mutex */
        getIntegerParam(roi, NDPluginROIComputeStatistics, &computeStatistics);
        getIntegerParam(roi, NDPluginROIComputeHistogram, &computeHistogram);
        getIntegerParam(roi, NDPluginROIComputeProfiles, &computeProfiles);
        getIntegerParam(roi, NDPluginROIDataType, &dataType);
        getDoubleParam (roi, NDPluginROIHistMin,  &pROI->histMin);
        getDoubleParam (roi, NDPluginROIHistMax,  &pROI->histMax);
        getIntegerParam(roi, NDPluginROIBgdWidth, &pROI->bgdWidth);
        getIntegerParam(roi, NDPluginROIHistSize, &histSize);

        /* Make sure dimensions are valid, fix them if they are not */
        /* We treat the case of RGB1 data specially, so that NX and NY are the X and Y dimensions of the
         * image, not the first 2 dimensions.  This makes it much easier to switch back and forth between
         * RGB1 and mono mode when using an ROI. */
        if (pArray->colorMode == NDColorModeRGB1) {
            userDims[0] = 1;
            userDims[1] = 2;
            userDims[2] = 0;
        }
        else if (pArray->colorMode == NDColorModeRGB2) {
            userDims[0] = 0;
            userDims[1] = 2;
            userDims[2] = 1;
        } 
        else {
            for (dim=0; dim<ND_ARRAY_MAX_DIMS; dim++) userDims[dim] = dim;
        }
        for (dim=0; dim<pArray->ndims; dim++) {
            pDim = &pROI->dims[dim];
            pDim->offset  = MAX(pDim->offset, 0);
            pDim->offset  = MIN(pDim->offset, pArray->dims[userDims[dim]].size-1);
            pDim->size    = MAX(pDim->size, 1);
            pDim->size    = MIN(pDim->size, pArray->dims[userDims[dim]].size - pDim->offset);
            pDim->binning = MAX(pDim->binning, 1);
            pDim->binning = MIN(pDim->binning, pDim->size);
        }
        /* Make a local copy of the fixed dimensions so we can release the mutex */
        memcpy(dims, pROI->dims, pArray->ndims*sizeof(NDDimension_t));

        /* Update the parameters that may have changed */
        pDim = &pROI->dims[0];
        setIntegerParam(roi, NDPluginROIDim0Min,  pDim->offset);
        setIntegerParam(roi, NDPluginROIDim0Size, pDim->size);
        setIntegerParam(roi, NDPluginROIDim0MaxSize, pArray->dims[userDims[0]].size);
        setIntegerParam(roi, NDPluginROIDim0Bin,  pDim->binning);
        pDim = &pROI->dims[1];
        setIntegerParam(roi, NDPluginROIDim1Min,  pDim->offset);
        setIntegerParam(roi, NDPluginROIDim1Size, pDim->size);
        setIntegerParam(roi, NDPluginROIDim1MaxSize, pArray->dims[userDims[1]].size);
        setIntegerParam(roi, NDPluginROIDim1Bin,  pDim->binning);
        pDim = &pROI->dims[2];
        setIntegerParam(roi, NDPluginROIDim2Min,  pDim->offset);
        setIntegerParam(roi, NDPluginROIDim2Size, pDim->size);
        setIntegerParam(roi, NDPluginROIDim2MaxSize, pArray->dims[userDims[2]].size);
        setIntegerParam(roi, NDPluginROIDim2Bin,  pDim->binning);

        /* This function is called with the lock taken, and it must be set when we exit.
         * The following code can be exected without the mutex because we are not accessing elements of
         * pPvt that other threads can access. */
        epicsMutexUnlock(this->mutexId);
    
        /* Extract this ROI from the input array.  The convert() function allocates
         * a new array and it is reserved (reference count = 1) */
        if (dataType == -1) dataType = (int)pArray->dataType;
        /* We treat the case of RGB1 data specially, so that NX and NY are the X and Y dimensions of the
         * image, not the first 2 dimensions.  This makes it much easier to switch back and forth between
         * RGB1 and mono mode when using an ROI. */
        if (pArray->colorMode == NDColorModeRGB1) {
            tempDim = dims[0];
            dims[0] = dims[2];
            dims[2] = dims[1];
            dims[1] = tempDim;
        }
        else if (pArray->colorMode == NDColorModeRGB2) {
            tempDim = dims[1];
            dims[1] = dims[2];
            dims[2] = tempDim;
        }
        this->pNDArrayPool->convert(pArray, &this->pArrays[roi], (NDDataType_t)dataType, dims);
        pROIArray  = this->pArrays[roi];

        pROIArray->getInfo(&arrayInfo);

        if (computeStatistics) {
            /* Compute the total counts */
            status = doComputeStatistics(pROIArray, pROI);
            /* If there is a non-zero background width then compute the background counts */
            if (pROI->bgdWidth > 0) {
                bgdPixels = 0;
                bgdCounts = 0.;
                for (dim=0; dim<pArray->ndims; dim++) {
                    memcpy(bgdDims, dims, ND_ARRAY_MAX_DIMS*sizeof(NDDimension_t));
                    pDim = &bgdDims[dim];
                    /* The background width must be as large as the binning or we will get an error */
                    pROI->bgdWidth = MAX(pROI->bgdWidth, pDim->binning);
                    pDim->offset = MAX(pDim->offset - pROI->bgdWidth, 0);
                    pDim->size    = MIN(pROI->bgdWidth, pArray->dims[dim].size - pDim->offset);
                    this->pNDArrayPool->convert(pArray, &pBgdArray, (NDDataType_t)dataType, bgdDims);
                    if (!pBgdArray) {
                        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                            "%s::%s, error allocating array buffer in convert\n",
                            driverName, functionName);
                        continue;
                    }
                    status = doComputeStatistics(pBgdArray, pROITemp);
                    pBgdArray->release();
                    bgdPixels += pROITemp->nElements;
                    bgdCounts += pROITemp->total;
                    memcpy(bgdDims, dims, ND_ARRAY_MAX_DIMS*sizeof(NDDimension_t));
                    pDim->offset = MIN(pDim->offset + pDim->size, pArray->dims[dim].size - 1 - pROI->bgdWidth);
                    pDim->size    = MIN(pROI->bgdWidth, pArray->dims[dim].size - pDim->offset);
                    this->pNDArrayPool->convert(pArray, &pBgdArray, (NDDataType_t)dataType, bgdDims);
                    if (!pBgdArray) {
                        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                            "%s::%s, error allocating array buffer in convert\n",
                            driverName, functionName);
                        continue;
                    }
                    status = doComputeStatistics(pBgdArray, pROITemp);
                    pBgdArray->release();
                    bgdPixels += pROITemp->nElements;
                    bgdCounts += pROITemp->total;
                }
                if (bgdPixels < 1) bgdPixels = 1; 
                avgBgd = bgdCounts / bgdPixels;
                pROI->net = pROI->total - avgBgd*pROI->nElements;
            }                
            setDoubleParam(roi, NDPluginROIMinValue, pROI->min);
            setDoubleParam(roi, NDPluginROIMaxValue, pROI->max);
            setDoubleParam(roi, NDPluginROIMeanValue, pROI->mean);
            setDoubleParam(roi, NDPluginROITotal, pROI->total);
            setDoubleParam(roi, NDPluginROINet, pROI->net);
            asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER, 
                (char *)pROIArray->pData, arrayInfo.totalBytes,
                "%s:%s ROI=%d, min=%f, max=%f, mean=%f, total=%f, net=%f",
                driverName, functionName, roi, pROI->min, pROI->max, pROI->mean, pROI->total, pROI->net);
        }
        if (computeHistogram) {
            if (histSize != pROI->histSize) {
                free(pROI->histogram);
                pROI->histSize = histSize;
                pROI->histogram = (double *)calloc(pROI->histSize, sizeof(double));
            }
            memset(pROI->histogram, 0, pROI->histSize*sizeof(double));
            doComputeHistogram(pROIArray, pROI);
            entropy = 0;
            for (i=0; i<histSize; i++) {
                counts = pROI->histogram[i];
                if (counts <= 0) counts = 1;
                entropy += counts * log(counts);
            }
            entropy = -entropy / pROI->nElements;
            setDoubleParam(roi, NDPluginROIHistEntropy, entropy);
            doCallbacksFloat64Array(pROI->histogram, pROI->histSize, NDPluginROIHistArray, roi);
        }
        /* We have now computed the statistics on the original array without highlighting.  
          * If the highlight flag is set extract the ROI from the copy of the array with ROIs highlighted */
        if (highlight && (pArray->ndims == 2)) {
            this->pArrays[roi]->release();
            this->pNDArrayPool->convert(pHighlights, &this->pArrays[roi], (NDDataType_t)dataType, dims);
        }

        /* Set the image size of the ROI image data */
        setIntegerParam(roi, ADImageSizeX, this->pArrays[roi]->dims[userDims[0]].size);
        setIntegerParam(roi, ADImageSizeY, this->pArrays[roi]->dims[userDims[1]].size);
        setIntegerParam(roi, ADImageSizeZ, this->pArrays[roi]->dims[userDims[2]].size);
        
        /* Call any clients who have registered for NDArray callbacks */
        doCallbacksGenericPointer(this->pArrays[roi], NDArrayData, roi);
        
        /* We must enter the loop and exit with the mutex locked */
        epicsMutexLock(this->mutexId);
        callParamCallbacks(roi, roi);
    }

    /* Build the arrays of total and net counts for fast scanning, do callbacks */
    for (roi=0; roi<this->maxROIs; roi++) {
        this->totalArray[roi] = (epicsInt32)this->pROIs[roi].total;
        this->netArray[roi] = (epicsInt32)this->pROIs[roi].net;
    }
    doCallbacksInt32Array(this->totalArray, this->maxROIs, NDPluginROITotalArray, 0); 
    doCallbacksInt32Array(this->netArray, this->maxROIs, NDPluginROINetArray, 0); 
    
    /* If we made a copy of the array for highlighting then free it */
    if (pHighlights) pHighlights->release();
    
    callParamCallbacks();
}


asynStatus NDPluginROI::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi=0;
    NDROI_t *pROI;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, functionName, &roi); if (status != asynSuccess) return(status);

    pROI = &this->pROIs[roi];
    /* Set parameter and readback in parameter library */
    status = setIntegerParam(roi , function, value);
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
        case NDPluginROIDim2Min:
            pROI->dims[2].offset = value;
            break;
        case NDPluginROIDim2Size:
            pROI->dims[2].size = value;
            break;
        case NDPluginROIDim2Bin:
            pROI->dims[2].binning = value;
            break;
        case NDPluginROIDim2Reverse:
            pROI->dims[2].reverse = value;
            break;
        default:
            /* This was not a parameter that this driver understands, try the base class */
            status = NDPluginDriver::writeInt32(pasynUser, value);
            break;
    }
    /* Do callbacks so higher layers see any changes */
    status = callParamCallbacks(roi, roi);
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, roi=%d, value=%d\n", 
              driverName, functionName, function, roi, value);
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
                ncopy = pROI->histSize;       
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
    return(status);
}


/* asynDrvUser interface methods */
asynStatus NDPluginROI::drvUserCreate(asynUser *pasynUser,
                                      const char *drvInfo, 
                                      const char **pptypeName, size_t *psize)
{
    asynStatus status;
    int param;
    static const char *functionName = "drvUserCreate";

    /* Look in the driver table */
    status = findParam(NDPluginROINParamString, NUM_ROIN_PARAMS, 
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
                  "%s::%s, drvInfo=%s, param=%d\n", 
                  driverName, functionName, drvInfo, param);
        return(asynSuccess);
    }

    /* If we did not find it in that table try the plugin base */
    status = NDPluginDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return(status);
}

    

extern "C" int drvNDROIConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                                 const char *NDArrayPort, int NDArrayAddr, int maxROIs, 
                                 int maxBuffers, size_t maxMemory)
{
    new NDPluginROI(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxROIs, 
                    maxBuffers, maxMemory);
    return(asynSuccess);
}

NDPluginROI::NDPluginROI(const char *portName, int queueSize, int blockingCallbacks, 
                         const char *NDArrayPort, int NDArrayAddr, int maxROIsIn, 
                         int maxBuffersIn, size_t maxMemory)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, maxROIsIn, NDPluginROILastROINParam, maxBuffersIn, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask, 
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask)
{
    asynStatus status;
    int roi;
    const char *functionName = "NDPluginROI";


    this->maxROIs = maxROIsIn;
    this->pROIs = (NDROI_t *)callocMustSucceed(maxROIs, sizeof(*this->pROIs), functionName);
    this->totalArray = (epicsInt32 *)callocMustSucceed(maxROIs, sizeof(epicsInt32), functionName);
    this->netArray = (epicsInt32 *)callocMustSucceed(maxROIs, sizeof(epicsInt32), functionName);
    setIntegerParam(0, NDPluginROIHighlight,         0);

    for (roi=0; roi<this->maxROIs; roi++) {
        setStringParam (roi,  NDPluginROIName,              "");
        setIntegerParam(roi , NDPluginROIUse,               0);
        setIntegerParam(roi , NDPluginROIComputeStatistics, 0);
        setIntegerParam(roi , NDPluginROIComputeHistogram,  0);
        setIntegerParam(roi , NDPluginROIComputeProfiles,   0);

        setIntegerParam(roi , NDPluginROIDim0Min,           0);
        setIntegerParam(roi , NDPluginROIDim0Size,          0);
        setIntegerParam(roi , NDPluginROIDim0MaxSize,       0);
        setIntegerParam(roi , NDPluginROIDim0Bin,           1);
        setIntegerParam(roi , NDPluginROIDim0Reverse,       0);
        setIntegerParam(roi , NDPluginROIDim1Min,           0);
        setIntegerParam(roi , NDPluginROIDim1Size,          0);
        setIntegerParam(roi , NDPluginROIDim1MaxSize,       0);
        setIntegerParam(roi , NDPluginROIDim1Bin,           1);
        setIntegerParam(roi , NDPluginROIDim1Reverse,       0);
        setIntegerParam(roi , NDPluginROIDim2Min,           0);
        setIntegerParam(roi , NDPluginROIDim2Size,          0);
        setIntegerParam(roi , NDPluginROIDim2MaxSize,       0);
        setIntegerParam(roi , NDPluginROIDim2Bin,           1);
        setIntegerParam(roi , NDPluginROIDim2Reverse,       0);
        setIntegerParam(roi , NDPluginROIDataType,          0);
       
        setIntegerParam(roi , NDPluginROIBgdWidth,          0);
        setDoubleParam (roi , NDPluginROIMinValue,          0.0);
        setDoubleParam (roi , NDPluginROIMaxValue,          0.0);
        setDoubleParam (roi , NDPluginROIMeanValue,         0.0);
        setDoubleParam (roi , NDPluginROITotal,             0.0);
        setDoubleParam (roi , NDPluginROINet,               0.0);
        
        setIntegerParam(roi , NDPluginROIHistSize,          0);
        setDoubleParam (roi , NDPluginROIHistMin,           0);
        setDoubleParam (roi , NDPluginROIHistMax,           0);
        
        setDoubleParam (roi , NDPluginROIHistEntropy,       0.0);
        setIntegerParam(roi , ADImageSizeX,                 0);
        setIntegerParam(roi , ADImageSizeY,                 0);
        setIntegerParam(roi , ADImageSizeZ,                 0);
    }
    
    /* Try to connect to the array port */
    status = connectToArrayPort();
}

