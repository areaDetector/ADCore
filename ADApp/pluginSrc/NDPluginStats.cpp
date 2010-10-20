/*
 * NDPluginStats.cpp
 *
 * Image statistics plugin
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

#include "NDArray.h"
#include "NDPluginStats.h"
#include <epicsExport.h>

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

static const char *driverName="NDPluginStats";

template <typename epicsType>
asynStatus NDPluginStats::doComputeHistogramT(NDArray *pArray)
{
    epicsType *pData = (epicsType *)pArray->pData;
    int i;
    double scale, entropy;
    int bin;
    int nElements;
    double value, counts;
    NDArrayInfo arrayInfo;

    if (this->histSizeNew != this->histogramSize) {
        free(this->histogram);
        this->histogramSize = this->histSizeNew;
        this->histogram = (double *)calloc(this->histogramSize, sizeof(double));
    }
    memset(this->histogram, 0, this->histogramSize*sizeof(double));
    pArray->getInfo(&arrayInfo);
    nElements = arrayInfo.nElements;
    scale = this->histogramSize / (histMax - histMin);

    for (i=0; i<nElements; i++) {
        value = (double)pData[i];
        bin = (int) (((value - histMin) * scale) + 0.5);
        if ((bin >= 0) && (bin < this->histogramSize))this->histogram[bin]++;
    }

    entropy = 0;
    for (i=0; i<this->histogramSize; i++) {
        counts = this->histogram[i];
        if (counts <= 0) counts = 1;
        entropy += counts * log(counts);
    }
    entropy = -entropy / nElements;
    this->histEntropy = entropy;
    
    return(asynSuccess);
}

asynStatus NDPluginStats::doComputeHistogram(NDArray *pArray)
{
    asynStatus status;
    
    switch(pArray->dataType) {
        case NDInt8:
            status = doComputeHistogramT<epicsInt8>(pArray);
            break;
        case NDUInt8:
            status = doComputeHistogramT<epicsUInt8>(pArray);
            break;
        case NDInt16:
            status = doComputeHistogramT<epicsInt16>(pArray);
            break;
        case NDUInt16:
            status = doComputeHistogramT<epicsUInt16>(pArray);
            break;
        case NDInt32:
            status = doComputeHistogramT<epicsInt32>(pArray);
            break;
        case NDUInt32:
            status = doComputeHistogramT<epicsUInt32>(pArray);
            break;
        case NDFloat32:
            status = doComputeHistogramT<epicsFloat32>(pArray);
            break;
        case NDFloat64:
            status = doComputeHistogramT<epicsFloat64>(pArray);
            break;
        default:
            status = asynError;
        break;
    }
    return(status);
}

template <typename epicsType>
void doComputeStatisticsT(NDArray *pArray, NDStats_t *pStats)
{
    int i;
    epicsType *pData = (epicsType *)pArray->pData;
    NDArrayInfo arrayInfo;
    double value;

    pArray->getInfo(&arrayInfo);
    pStats->nElements = arrayInfo.nElements;
    pStats->min = (double) pData[0];
    pStats->max = (double) pData[0];
    pStats->total = 0.;
    pStats->sigma = 0.;
    for (i=0; i<pStats->nElements; i++) {
        value = (double)pData[i];
        if (value < pStats->min) pStats->min = value;
        if (value > pStats->max) pStats->max = value;
        pStats->total += value;
        pStats->sigma += value * value;
    }
    pStats->net = pStats->total;
    pStats->mean = pStats->total / pStats->nElements;
    pStats->sigma = sqrt((pStats->sigma / pStats->nElements) - (pStats->mean * pStats->mean));
}

int doComputeStatistics(NDArray *pArray, NDStats_t *pStats)
{

    switch(pArray->dataType) {
        case NDInt8:
            doComputeStatisticsT<epicsInt8>(pArray, pStats);
            break;
        case NDUInt8:
            doComputeStatisticsT<epicsUInt8>(pArray, pStats);
            break;
        case NDInt16:
            doComputeStatisticsT<epicsInt16>(pArray, pStats);
            break;
        case NDUInt16:
            doComputeStatisticsT<epicsUInt16>(pArray, pStats);
            break;
        case NDInt32:
            doComputeStatisticsT<epicsInt32>(pArray, pStats);
            break;
        case NDUInt32:
            doComputeStatisticsT<epicsUInt32>(pArray, pStats);
            break;
        case NDFloat32:
            doComputeStatisticsT<epicsFloat32>(pArray, pStats);
            break;
        case NDFloat64:
            doComputeStatisticsT<epicsFloat64>(pArray, pStats);
            break;
        default:
            return(ND_ERROR);
        break;
    }
    return(ND_SUCCESS);
}

template <typename epicsType>
asynStatus NDPluginStats::doComputeCentroidT(NDArray *pArray)
{
    epicsType *pData = (epicsType *)pArray->pData;
    double value, *pValue, *pThresh, centroidTotal;
    int ix, iy;

    if (pArray->ndims != 2) return(asynError);
    
    getDoubleParam (NDPluginStatsCentroidThreshold,  &this->centroidThreshold);
    this->centroidX = 0;
    this->centroidY = 0;
    this->sigmaX = 0;
    this->sigmaY = 0;
    this->sigmaXY = 0;
    memset(this->profileX[profAverage], 0, this->profileSizeX*sizeof(double));  
    memset(this->profileY[profAverage], 0, this->profileSizeY*sizeof(double));
    memset(this->profileX[profThreshold], 0, this->profileSizeX*sizeof(double));  
    memset(this->profileY[profThreshold], 0, this->profileSizeY*sizeof(double));

    for (iy=0; iy<this->profileSizeY; iy++) {
        for (ix=0; ix<this->profileSizeX; ix++) {
            value = (double)*pData++;
            this->profileX[profAverage][ix] += value;
            this->profileY[profAverage][iy] += value;
            if (value >= this->centroidThreshold) {
                this->profileX[profThreshold][ix] += value;
                this->profileY[profThreshold][iy] += value;
                this->sigmaXY += value * ix * iy;
            }
        }
    }

    /* Normalize the average profiles and compute the centroid from them */
    this->centroidX = 0;
    this->sigmaX = 0;
    centroidTotal = 0;
    pValue  = this->profileX[profAverage];
    pThresh = this->profileX[profThreshold];
    for (ix=0; ix<this->profileSizeX; ix++, pValue++, pThresh++) {
        this->centroidX += *pThresh * ix;
        this->sigmaX    += *pThresh * ix * ix;
        centroidTotal   += *pThresh;
        *pValue  /= this->profileSizeY;
        *pThresh /= this->profileSizeY;
    }
    this->centroidY = 0;
    this->sigmaY = 0;
    pValue  = this->profileY[profAverage];
    pThresh = this->profileY[profThreshold];
    for (iy=0; iy<this->profileSizeY; iy++, pValue++, pThresh++) {
        this->centroidY += *pThresh * iy;
        this->sigmaY    += *pThresh * iy * iy;
        *pValue  /= this->profileSizeX;
        *pThresh /= this->profileSizeX;
   }
   if (centroidTotal > 0.) {
        this->centroidX /= centroidTotal;
        this->centroidY /= centroidTotal;
        this->sigmaX  = sqrt((this->sigmaX  / centroidTotal) - (this->centroidX * this->centroidX));
        this->sigmaY  = sqrt((this->sigmaY  / centroidTotal) - (this->centroidY * this->centroidY));
        this->sigmaXY =      (this->sigmaXY / centroidTotal) - (this->centroidX * this->centroidY);
        if ((this->sigmaX !=0) && (this->sigmaY != 0)) 
            this->sigmaXY /= (this->sigmaX * this->sigmaY);
    }
    setDoubleParam(NDPluginStatsCentroidX,   this->centroidX);
    setDoubleParam(NDPluginStatsCentroidY,   this->centroidY);
    setDoubleParam(NDPluginStatsSigmaX,      this->sigmaX);
    setDoubleParam(NDPluginStatsSigmaY,      this->sigmaY);
    setDoubleParam(NDPluginStatsSigmaXY,     this->sigmaXY);
    
    return(asynSuccess);
}

asynStatus NDPluginStats::doComputeCentroid(NDArray *pArray)
{
    asynStatus status;

    switch(pArray->dataType) {
        case NDInt8:
            status = doComputeCentroidT<epicsInt8>(pArray);
            break;
        case NDUInt8:
            status = doComputeCentroidT<epicsUInt8>(pArray);
            break;
        case NDInt16:
            status = doComputeCentroidT<epicsInt16>(pArray);
            break;
        case NDUInt16:
            status = doComputeCentroidT<epicsUInt16>(pArray);
            break;
        case NDInt32:
            status = doComputeCentroidT<epicsInt32>(pArray);
            break;
        case NDUInt32:
            status = doComputeCentroidT<epicsUInt32>(pArray);
            break;
        case NDFloat32:
            status = doComputeCentroidT<epicsFloat32>(pArray);
            break;
        case NDFloat64:
            status = doComputeCentroidT<epicsFloat64>(pArray);
            break;
        default:
            status = asynError;
        break;
    }
    return(status);
}

template <typename epicsType>
asynStatus NDPluginStats::doComputeProfilesT(NDArray *pArray)
{
    epicsType *pData = (epicsType *)pArray->pData;
    epicsType *pCentroid, *pCursor;
    int ix, iy;

    if (pArray->ndims != 2) return(asynError);

    /* Compute the X and Y profiles at the centroid and cursor positions */
    getIntegerParam (NDPluginStatsCursorX, &this->cursorX);
    getIntegerParam (NDPluginStatsCursorY, &this->cursorY);
    iy = (int) (this->centroidY + 0.5);
    iy = MAX(iy, 0);
    iy = MIN(iy, this->profileSizeY-1);
    pCentroid = pData + iy*this->profileSizeX;
    iy = this->cursorY;
    iy = MAX(iy, 0);
    iy = MIN(iy, this->profileSizeY-1);
    pCursor = pData + iy*this->profileSizeX;
    for (ix=0; ix<this->profileSizeX; ix++) {
        this->profileX[profCentroid][ix] = *pCentroid++;
        this->profileX[profCursor][ix]   = *pCursor++;
    }
    ix = (int) (this->centroidX + 0.5);
    ix = MAX(ix, 0);
    ix = MIN(ix, this->profileSizeX-1);
    pCentroid = pData + ix;
    ix = this->cursorX;
    ix = MAX(ix, 0);
    ix = MIN(ix, this->profileSizeX-1);
    pCursor = pData + ix;
    for (iy=0; iy<this->profileSizeY; iy++) {
        this->profileY[profCentroid][iy] = *pCentroid;
        this->profileY[profCursor][iy]   = *pCursor;
        pCentroid += this->profileSizeX;
        pCursor   += this->profileSizeX;
    }
    doCallbacksFloat64Array(this->profileX[profAverage],   this->profileSizeX, NDPluginStatsProfileAverageX, 0);
    doCallbacksFloat64Array(this->profileY[profAverage],   this->profileSizeY, NDPluginStatsProfileAverageY, 0);
    doCallbacksFloat64Array(this->profileX[profThreshold], this->profileSizeX, NDPluginStatsProfileThresholdX, 0);
    doCallbacksFloat64Array(this->profileY[profThreshold], this->profileSizeY, NDPluginStatsProfileThresholdY, 0);
    doCallbacksFloat64Array(this->profileX[profCentroid],  this->profileSizeX, NDPluginStatsProfileCentroidX, 0);
    doCallbacksFloat64Array(this->profileY[profCentroid],  this->profileSizeY, NDPluginStatsProfileCentroidY, 0);
    doCallbacksFloat64Array(this->profileX[profCursor],    this->profileSizeX, NDPluginStatsProfileCursorX, 0);
    doCallbacksFloat64Array(this->profileY[profCursor],    this->profileSizeY, NDPluginStatsProfileCursorY, 0);
    
    return(asynSuccess);
}

asynStatus NDPluginStats::doComputeProfiles(NDArray *pArray)
{
    asynStatus status;

    switch(pArray->dataType) {
        case NDInt8:
            status = doComputeProfilesT<epicsInt8>(pArray);
            break;
        case NDUInt8:
            status = doComputeProfilesT<epicsUInt8>(pArray);
            break;
        case NDInt16:
            status = doComputeProfilesT<epicsInt16>(pArray);
            break;
        case NDUInt16:
            status = doComputeProfilesT<epicsUInt16>(pArray);
            break;
        case NDInt32:
            status = doComputeProfilesT<epicsInt32>(pArray);
            break;
        case NDUInt32:
            status = doComputeProfilesT<epicsUInt32>(pArray);
            break;
        case NDFloat32:
            status = doComputeProfilesT<epicsFloat32>(pArray);
            break;
        case NDFloat64:
            status = doComputeProfilesT<epicsFloat64>(pArray);
            break;
        default:
            status = asynError;
        break;
    }
    return(status);
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Does image statistics.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginStats::processCallbacks(NDArray *pArray)
{
    /* This function does array statistics.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
    NDDimension_t bgdDims[ND_ARRAY_MAX_DIMS], *pDim;
    int bgdPixels;
    int bgdWidth;
    int dim;
    NDStats_t stats, *pStats=&stats, statsTemp, *pStatsTemp=&statsTemp;
    double bgdCounts, avgBgd;
    NDArray *pBgdArray=NULL;
    int computeStatistics, computeCentroid, computeProfiles, computeHistogram;
    int intTotal, intNet;
    int sizeX=0, sizeY=0;
    int i;
    NDArrayInfo arrayInfo;

    const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    pArray->getInfo(&arrayInfo);
    getIntegerParam(NDPluginStatsComputeStatistics,  &computeStatistics);
    getIntegerParam(NDPluginStatsComputeCentroid,    &computeCentroid);
    getIntegerParam(NDPluginStatsComputeProfiles,    &computeProfiles);
    getIntegerParam(NDPluginStatsComputeHistogram,   &computeHistogram);
    
    if (pArray->ndims > 0) sizeX = pArray->dims[0].size;
    if (pArray->ndims > 1) sizeY = pArray->dims[1].size;

	if (sizeX != this->profileSizeX) {
        this->profileSizeX = sizeX;
        setIntegerParam(NDPluginStatsProfileSizeX,  this->profileSizeX);
        for (i=0; i<MAX_PROFILE_TYPES; i++) {
            if (this->profileX[i]) free(this->profileX[i]);
            this->profileX[i] = (double *)malloc(this->profileSizeX * sizeof(double));
        }
    }
	if (sizeY != this->profileSizeY) {
        this->profileSizeY = sizeY;
        setIntegerParam(NDPluginStatsProfileSizeY,  this->profileSizeY);
        for (i=0; i<MAX_PROFILE_TYPES; i++) {
            if (this->profileY[i]) free(this->profileY[i]);
            this->profileY[i] = (double *)malloc(this->profileSizeY * sizeof(double));
        }
    }

    if (computeStatistics) {
        getIntegerParam(NDPluginStatsBgdWidth,           &bgdWidth);
        /* Now that we won't be accessing any memory other threads can access, unlock so we don't block drivers */
        this->unlock();
        doComputeStatistics(pArray, pStats);
        /* If there is a non-zero background width then compute the background counts */
        if (bgdWidth > 0) {
            bgdPixels = 0;
            bgdCounts = 0.;
            /* Initialize the dimensions of the background array */
            for (dim=0; dim<pArray->ndims; dim++) {
                pArray->initDimension(&bgdDims[dim], pArray->dims[dim].size);
            }
            for (dim=0; dim<pArray->ndims; dim++) {
                pDim = &bgdDims[dim];
                pDim->offset = 0;
                pDim->size = MIN(bgdWidth, pDim->size);
                this->pNDArrayPool->convert(pArray, &pBgdArray, pArray->dataType, bgdDims);
                pDim->size = pArray->dims[dim].size;
                if (!pBgdArray) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s::%s, error allocating array buffer in convert\n",
                        driverName, functionName);
                    continue;
                }
                doComputeStatistics(pBgdArray, pStatsTemp);
                pBgdArray->release();
                bgdPixels += pStatsTemp->nElements;
                bgdCounts += pStatsTemp->total;
                pDim->offset = MAX(0, pDim->size - 1 - bgdWidth);
                pDim->size = MIN(bgdWidth, pArray->dims[dim].size - pDim->offset);
                this->pNDArrayPool->convert(pArray, &pBgdArray, pArray->dataType, bgdDims);
                pDim->offset = 0;
                pDim->size = pArray->dims[dim].size;
                if (!pBgdArray) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s::%s, error allocating array buffer in convert\n",
                        driverName, functionName);
                    continue;
                }
                doComputeStatistics(pBgdArray, pStatsTemp);
                pBgdArray->release();
                bgdPixels += pStatsTemp->nElements;
                bgdCounts += pStatsTemp->total;
            }
            if (bgdPixels < 1) bgdPixels = 1;
            avgBgd = bgdCounts / bgdPixels;
            pStats->net = pStats->total - avgBgd*pStats->nElements;
        }
        this->lock();
        setDoubleParam(NDPluginStatsMinValue,    pStats->min);
        setDoubleParam(NDPluginStatsMaxValue,    pStats->max);
        setDoubleParam(NDPluginStatsMeanValue,   pStats->mean);
        setDoubleParam(NDPluginStatsSigmaValue,  pStats->sigma);
        setDoubleParam(NDPluginStatsTotal,       pStats->total);
        setDoubleParam(NDPluginStatsNet,         pStats->net);
        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
            (char *)pArray->pData, arrayInfo.totalBytes,
            "%s:%s min=%f, max=%f, mean=%f, total=%f, net=%f",
            driverName, functionName, pStats->min, pStats->max, pStats->mean, pStats->total, pStats->net);
        intTotal = (int)pStats->total;
        intNet   = (int)pStats->net;
        doCallbacksInt32Array(&intTotal, 1, NDPluginStatsTotalArray, 0);
        doCallbacksInt32Array(&intNet,   1, NDPluginStatsNetArray, 0);
    }

    if (computeCentroid) {
        doComputeCentroid(pArray);
    }
         
    if (computeProfiles) {
        doComputeProfiles(pArray);
    }
    
    if (computeHistogram) {
        getIntegerParam(NDPluginStatsHistSize, &this->histSizeNew);
        getDoubleParam (NDPluginStatsHistMin,  &this->histMin);
        getDoubleParam (NDPluginStatsHistMax,  &this->histMax);
        this->unlock();
        doComputeHistogram(pArray);
        this->lock();
        setDoubleParam(NDPluginStatsHistEntropy, this->histEntropy);
        doCallbacksFloat64Array(this->histogram, this->histogramSize, NDPluginStatsHistArray, 0);
    }
    
    /* Save a copy of this array for calculations when cursor is moved or threshold is changed */
    if (this->pArrays[0]) this->pArrays[0]->release();
    this->pArrays[0] = this->pNDArrayPool->copy(pArray, NULL, 1);

    callParamCallbacks();
}


/** Called when asyn clients call pasynFloat64Array->read().
  * Returns the histogram array when pasynUser->reason=NDPluginStatsHistArray,
  * or one of the profile arrays.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginStats::readFloat64Array(asynUser *pasynUser,
                                   epicsFloat64 *value, size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    size_t ncopy;
    const char* functionName = "readFloat64Array";

    if (function == NDPluginStatsHistArray) {
        ncopy = this->histogramSize;
        if (ncopy > nElements) ncopy = nElements;
        memcpy(value, this->histogram, ncopy*sizeof(epicsFloat64));
        *nIn = ncopy;
    } else if (function == NDPluginStatsProfileAverageX) {
        ncopy = this->profileSizeX;
        if (ncopy > nElements) ncopy = nElements;
        memcpy(value, this->profileX[profAverage], ncopy*sizeof(epicsFloat64));
        *nIn = ncopy;
    } else if (function == NDPluginStatsProfileAverageY) {
        ncopy = this->profileSizeY;
        if (ncopy > nElements) ncopy = nElements;
        memcpy(value, this->profileY[profAverage], ncopy*sizeof(epicsFloat64));
        *nIn = ncopy;
    } else if (function == NDPluginStatsProfileThresholdX) {
        ncopy = this->profileSizeX;
        if (ncopy > nElements) ncopy = nElements;
        memcpy(value, this->profileX[profThreshold], ncopy*sizeof(epicsFloat64));
        *nIn = ncopy;
    } else if (function == NDPluginStatsProfileThresholdY) {
        ncopy = this->profileSizeY;
        if (ncopy > nElements) ncopy = nElements;
        memcpy(value, this->profileY[profThreshold], ncopy*sizeof(epicsFloat64));
        *nIn = ncopy;
    } else if (function == NDPluginStatsProfileCentroidX) {
        ncopy = this->profileSizeX;
        if (ncopy > nElements) ncopy = nElements;
        memcpy(value, this->profileX[profCentroid], ncopy*sizeof(epicsFloat64));
        *nIn = ncopy;
    } else if (function == NDPluginStatsProfileCentroidY) {
        ncopy = this->profileSizeY;
        if (ncopy > nElements) ncopy = nElements;
        memcpy(value, this->profileY[profCentroid], ncopy*sizeof(epicsFloat64));
        *nIn = ncopy;
    } else if (function == NDPluginStatsProfileCursorX) {
        ncopy = this->profileSizeX;
        if (ncopy > nElements) ncopy = nElements;
        memcpy(value, this->profileX[profCursor], ncopy*sizeof(epicsFloat64));
        *nIn = ncopy;
    } else if (function == NDPluginStatsProfileCursorY) {
        ncopy = this->profileSizeY;
        if (ncopy > nElements) ncopy = nElements;
        memcpy(value, this->profileY[profCursor], ncopy*sizeof(epicsFloat64));
        *nIn = ncopy;
    } else {
        status = NDPluginDriver::readFloat64Array(pasynUser, value, nElements, nIn);
    }
    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "%s:%s: status=%d, function=%d, value=%f\n",
                  driverName, functionName, status, function, *value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%f\n",
              driverName, functionName, function, *value);
    return(status);
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginStats::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeInt32";

    status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(addr, function, value);

    if (function == NDPluginStatsCursorX) {
        this->cursorX = value;
        if (this->pArrays[0]) {
            doComputeProfiles(this->pArrays[0]);
        }
    } else if (function == NDPluginStatsCursorY) {
        this->cursorY = value;
        if (this->pArrays[0]) {
            doComputeProfiles(this->pArrays[0]);
        }
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_STATS_PARAM) 
            status = NDPluginDriver::writeInt32(pasynUser, value);
    }
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) callParamCallbacks(addr);
    
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
asynStatus  NDPluginStats::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    int computeCentroid, computeProfiles;
    static const char *functionName = "writeFloat64";

    status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(addr, function, value);

    if (function == NDPluginStatsCentroidThreshold) {
        getIntegerParam(NDPluginStatsComputeCentroid, &computeCentroid);
        if (computeCentroid && this->pArrays[0]) {
            doComputeCentroid(this->pArrays[0]);
            getIntegerParam(NDPluginStatsComputeProfiles, &computeProfiles);
            if (computeProfiles) doComputeProfiles(this->pArrays[0]);
        }
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_STATS_PARAM) status = NDPluginDriver::writeFloat64(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks(addr);
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



/** Constructor for NDPluginStats; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
NDPluginStats::NDPluginStats(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_STATS_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    asynStatus status;
    //const char *functionName = "NDPluginStats";
    
    memset(this->profileX, 0, sizeof(this->profileX));
    memset(this->profileY, 0, sizeof(this->profileY));
    this->histogram = NULL;

    /* Statistics */
    createParam(NDPluginStatsComputeStatisticsString, asynParamInt32,      &NDPluginStatsComputeStatistics);
    createParam(NDPluginStatsBgdWidthString,          asynParamInt32,      &NDPluginStatsBgdWidth);
    createParam(NDPluginStatsMinValueString,          asynParamFloat64,    &NDPluginStatsMinValue);
    createParam(NDPluginStatsMaxValueString,          asynParamFloat64,    &NDPluginStatsMaxValue);
    createParam(NDPluginStatsMeanValueString,         asynParamFloat64,    &NDPluginStatsMeanValue);
    createParam(NDPluginStatsSigmaValueString,        asynParamFloat64,    &NDPluginStatsSigmaValue);
    createParam(NDPluginStatsTotalArrayString,        asynParamInt32Array, &NDPluginStatsTotalArray);
    createParam(NDPluginStatsNetArrayString,          asynParamInt32Array, &NDPluginStatsNetArray);

    /* Centroid */
    createParam(NDPluginStatsComputeCentroidString,   asynParamInt32,      &NDPluginStatsComputeCentroid);
    createParam(NDPluginStatsCentroidThresholdString, asynParamFloat64,    &NDPluginStatsCentroidThreshold);
    createParam(NDPluginStatsCentroidXString,         asynParamFloat64,    &NDPluginStatsCentroidX);
    createParam(NDPluginStatsCentroidYString,         asynParamFloat64,    &NDPluginStatsCentroidY);
    createParam(NDPluginStatsSigmaXString,            asynParamFloat64,    &NDPluginStatsSigmaX);
    createParam(NDPluginStatsSigmaYString,            asynParamFloat64,    &NDPluginStatsSigmaY);
    createParam(NDPluginStatsSigmaXYString,           asynParamFloat64,    &NDPluginStatsSigmaXY);

    /* Profiles */
    createParam(NDPluginStatsComputeProfilesString,   asynParamInt32,         &NDPluginStatsComputeProfiles);
    createParam(NDPluginStatsProfileSizeXString,      asynParamInt32,         &NDPluginStatsProfileSizeX);
    createParam(NDPluginStatsProfileSizeYString,      asynParamInt32,         &NDPluginStatsProfileSizeY);
    createParam(NDPluginStatsCursorXString,           asynParamInt32,         &NDPluginStatsCursorX);
    createParam(NDPluginStatsCursorYString,           asynParamInt32,         &NDPluginStatsCursorY);
    createParam(NDPluginStatsProfileAverageXString,   asynParamFloat64Array,  &NDPluginStatsProfileAverageX);
    createParam(NDPluginStatsProfileAverageYString,   asynParamFloat64Array,  &NDPluginStatsProfileAverageY);
    createParam(NDPluginStatsProfileThresholdXString, asynParamFloat64Array,  &NDPluginStatsProfileThresholdX);
    createParam(NDPluginStatsProfileThresholdYString, asynParamFloat64Array,  &NDPluginStatsProfileThresholdY);
    createParam(NDPluginStatsProfileCentroidXString,  asynParamFloat64Array,  &NDPluginStatsProfileCentroidX);
    createParam(NDPluginStatsProfileCentroidYString,  asynParamFloat64Array,  &NDPluginStatsProfileCentroidY);
    createParam(NDPluginStatsProfileCursorXString,    asynParamFloat64Array,  &NDPluginStatsProfileCursorX);
    createParam(NDPluginStatsProfileCursorYString,    asynParamFloat64Array,  &NDPluginStatsProfileCursorY);

    /* Histogram */
    createParam(NDPluginStatsComputeHistogramString,  asynParamInt32,         &NDPluginStatsComputeHistogram);
    createParam(NDPluginStatsHistSizeString,          asynParamInt32,         &NDPluginStatsHistSize);
    createParam(NDPluginStatsHistMinString,           asynParamFloat64,       &NDPluginStatsHistMin);
    createParam(NDPluginStatsHistMaxString,           asynParamFloat64,       &NDPluginStatsHistMax);
    createParam(NDPluginStatsHistEntropyString,       asynParamFloat64,       &NDPluginStatsHistEntropy);
    createParam(NDPluginStatsHistArrayString,         asynParamFloat64Array,  &NDPluginStatsHistArray);

    /* Arrays of total and net counts for MCA or waveform record */   
    createParam(NDPluginStatsCallbackPeriodString,    asynParamFloat64, &NDPluginStatsCallbackPeriod);
    createParam(NDPluginStatsTotalString,             asynParamFloat64, &NDPluginStatsTotal);
    createParam(NDPluginStatsNetString,               asynParamFloat64, &NDPluginStatsNet);
    
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginStats");

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDStatsConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginStats *pPlugin =
        new NDPluginStats(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshFuncDef initFuncDef = {"NDStatsConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDStatsConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDStatsRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDStatsRegister);
}
