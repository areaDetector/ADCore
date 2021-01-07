/*
 * NDPluginStats.cpp
 *
 * Image statistics plugin
 * Author: Mark Rivers
 *
 * Created March 12, 2010
 */

#include <stdlib.h>
#include <math.h>

#include <iocsh.h>

#include "NDPluginStats.h"

#include <epicsExport.h>

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

/* Some systems do not define M_PI in math.h */
#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

static const char *driverName="NDPluginStats";

template <typename epicsType>
asynStatus NDPluginStats::doComputeHistogramT(NDArray *pArray, NDStats_t *pStats)
{
    epicsType *pData = (epicsType *)pArray->pData;
    size_t i;
    double scale, entropy;
    int bin;
    size_t nElements;
    double value, counts;
    NDArrayInfo arrayInfo;

    pArray->getInfo(&arrayInfo);
    nElements = arrayInfo.nElements;
    scale = (pStats->histSize - 1) / (pStats->histMax - pStats->histMin);

    pStats->histBelow = 0;
    pStats->histAbove = 0;
    for (i=0; i<nElements; i++) {
        value = (double)pData[i];
        bin = (int)(((value - pStats->histMin) * scale) + 0.5);
        if ((bin < 0) || (value < pStats->histMin))
            pStats->histBelow++;
        else if ((bin > (int)pStats->histSize-1) || (value > pStats->histMax))
            pStats->histAbove++;
        else
            pStats->histogram[bin]++;
    }

    entropy = 0;
    for (i=0; (int)i<pStats->histSize; i++) {
        counts = pStats->histogram[i];
        if (counts <= 0) counts = 1;
        entropy += counts * log(counts);
    }
    entropy = -entropy / nElements;
    pStats->histEntropy = entropy;

    return(asynSuccess);
}

asynStatus NDPluginStats::doComputeHistogram(NDArray *pArray, NDStats_t *pStats)
{
    asynStatus status;

    switch(pArray->dataType) {
        case NDInt8:
            status = doComputeHistogramT<epicsInt8>(pArray, pStats);
            break;
        case NDUInt8:
            status = doComputeHistogramT<epicsUInt8>(pArray, pStats);
            break;
        case NDInt16:
            status = doComputeHistogramT<epicsInt16>(pArray, pStats);
            break;
        case NDUInt16:
            status = doComputeHistogramT<epicsUInt16>(pArray, pStats);
            break;
        case NDInt32:
            status = doComputeHistogramT<epicsInt32>(pArray, pStats);
            break;
        case NDUInt32:
            status = doComputeHistogramT<epicsUInt32>(pArray, pStats);
            break;
        case NDInt64:
            status = doComputeHistogramT<epicsInt64>(pArray, pStats);
            break;
        case NDUInt64:
            status = doComputeHistogramT<epicsUInt64>(pArray, pStats);
            break;
        case NDFloat32:
            status = doComputeHistogramT<epicsFloat32>(pArray, pStats);
            break;
        case NDFloat64:
            status = doComputeHistogramT<epicsFloat64>(pArray, pStats);
            break;
        default:
            status = asynError;
        break;
    }
    return(status);
}

template <typename epicsType>
void NDPluginStats::doComputeStatisticsT(NDArray *pArray, NDStats_t *pStats)
{
    size_t i, imin, imax;
    epicsType *pData = (epicsType *)pArray->pData;
    NDArrayInfo arrayInfo;
    double value;

    pArray->getInfo(&arrayInfo);
    pStats->nElements = arrayInfo.nElements;
    pStats->min = (double) pData[0];
    imin = 0;
    pStats->max = (double) pData[0];
    imax = 0;
    pStats->total = 0.;
    pStats->sigma = 0.;
    for (i=0; i<pStats->nElements; i++) {
        value = (double)pData[i];
        if (value < pStats->min) {
            pStats->min = value;
            imin = i;
        }
        if (value > pStats->max) {
            pStats->max = value;
            imax = i;
        }
        pStats->total += value;
        pStats->sigma += value * value;
    }
    pStats->minX = imin % arrayInfo.xSize;
    pStats->minY = imin / arrayInfo.xSize;
    pStats->maxX = imax % arrayInfo.xSize;
    pStats->maxY = imax / arrayInfo.xSize;
    pStats->net = pStats->total;
    pStats->mean = pStats->total / pStats->nElements;
    pStats->sigma = sqrt((pStats->sigma / pStats->nElements) - (pStats->mean * pStats->mean));
}

int NDPluginStats::doComputeStatistics(NDArray *pArray, NDStats_t *pStats)
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
        case NDInt64:
            doComputeStatisticsT<epicsInt64>(pArray, pStats);
            break;
        case NDUInt64:
            doComputeStatisticsT<epicsUInt64>(pArray, pStats);
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
asynStatus NDPluginStats::doComputeCentroidT(NDArray *pArray, NDStats_t *pStats)
{
    epicsType *pData = (epicsType *)pArray->pData;
    double value, *pValue, *pThresh, varX, varY, varXY;
    size_t ix, iy;
    /*Raw moments */
    double M00 = 0.0;
    double M10 = 0.0, M01 = 0.0;
    double M20 = 0.0, M02 = 0.0, M11 = 0.0;
    double M30 = 0.0, M03 = 0.0;
    double M40 = 0.0, M04 = 0.0;
    /*Central moments */
    double mu20, mu02, mu11, mu30, mu03, mu40, mu04;

    if (pArray->ndims > 2) return(asynError);

    for (iy=0; iy<pStats->profileSizeY; iy++) {
        for (ix=0; ix<pStats->profileSizeX; ix++) {
            value = (double)*pData++;
            pStats->profileX[profAverage][ix] += value;
            pStats->profileY[profAverage][iy] += value;
            if (value >= pStats->centroidThreshold) {
                pStats->profileX[profThreshold][ix] += value;
                pStats->profileY[profThreshold][iy] += value;
                M11 += value * ix * iy;
            }
        }
    }

    /* Normalize the average profiles and compute the centroid from them */
    pValue  = pStats->profileX[profAverage];
    pThresh = pStats->profileX[profThreshold];
    for (ix=0; ix<pStats->profileSizeX; ix++, pValue++, pThresh++) {
        M00 += *pThresh;
        M10 += *pThresh * ix;
        M20 += *pThresh * ix * ix;
        M30 += *pThresh * ix * ix * ix;
        M40 += *pThresh * ix * ix * ix * ix;
        *pValue  /= pStats->profileSizeY;
        *pThresh /= pStats->profileSizeY;
    }
    pValue  = pStats->profileY[profAverage];
    pThresh = pStats->profileY[profThreshold];
    for (iy=0; iy<pStats->profileSizeY; iy++, pValue++, pThresh++) {
        M01 += *pThresh * iy;
        M02 += *pThresh * iy * iy;
        M03 += *pThresh * iy * iy * iy;
        M04 += *pThresh * iy * iy * iy * iy;
        *pValue  /= pStats->profileSizeX;
        *pThresh /= pStats->profileSizeX;
    }

    if (M00 > 0.) {
        /* Calculate central moments */
        mu20 = M20 - (M10 * M10) / M00;
        mu02 = M02 - (M01 * M01) / M00;
        mu11 = M11 - (M10 * M01) / M00;
        mu30 = M30 - ((3 * M10 * M20) / M00) + ((2 * M10 * M10 * M10) / (M00 * M00));
        mu03 = M03 - ((3 * M01 * M02) / M00) + ((2 * M01 * M01 * M01) / (M00 * M00));
        mu40 = M40 - ((4 * M30 * M10) / M00) + ((6 * M20 * M10 * M10) / (M00 * M00)) -
             (3 * M10 * M10 * M10 * M10) / (M00 * M00 * M00);
        mu04 = M04 - ((4 * M03 * M01) / M00) + ((6 * M02 * M01 * M01) / (M00 * M00)) -
             (3 * M01 * M01 * M01 * M01) / (M00 * M00 * M00);
        /* Calculate variances */
        varX  = mu20 / M00;
        varY  = mu02 / M00;
        varXY = mu11 / M00;
        /* Scientific output parameters */
        pStats->centroidTotal = M00;
        pStats->centroidX = M10 / M00;
        pStats->centroidY = M01 / M00;
        /* Calculate sigmas */
        pStats->sigmaX = sqrt(varX);
        pStats->sigmaY = sqrt(varY);
        if ((pStats->sigmaX != 0) && (pStats->sigmaY != 0)){
            pStats->sigmaXY = varXY / (pStats->sigmaX * pStats->sigmaY);
        }
        if (varX != 0) {
            pStats->skewX = mu30  / (M00 * pow(varX, 3.0/2.0));
            pStats->kurtosisX = (mu40 / (M00 * pow(varX, 2.0))) - 3.0;
        }
        if (varY != 0) {
            pStats->skewY = mu03  / (M00 * pow(varY, 3.0/2.0));
            pStats->kurtosisY = (mu04 / (M00 * pow(varY, 2.0))) - 3.0;
        }

        /* Calculate orientation and eccentricity */
        pStats->orientation = 0.5 * atan2((2.0 * varXY), (varX - varY));
        /* Orientation in degrees*/
        pStats->orientation  = pStats->orientation * 180 / M_PI;
        if ((mu20 + mu02) != 0){
            pStats->eccentricity = ((mu20 - mu02) * (mu20 - mu02) - 4 * mu11 * mu11) /
                                 ((mu20 + mu02) * (mu20 + mu02));
        }
    }
    return(asynSuccess);
}

asynStatus NDPluginStats::doComputeCentroid(NDArray *pArray, NDStats_t *pStats)
{
    asynStatus status;

    switch(pArray->dataType) {
        case NDInt8:
            status = doComputeCentroidT<epicsInt8>(pArray, pStats);
            break;
        case NDUInt8:
            status = doComputeCentroidT<epicsUInt8>(pArray, pStats);
            break;
        case NDInt16:
            status = doComputeCentroidT<epicsInt16>(pArray, pStats);
            break;
        case NDUInt16:
            status = doComputeCentroidT<epicsUInt16>(pArray, pStats);
            break;
        case NDInt32:
            status = doComputeCentroidT<epicsInt32>(pArray, pStats);
            break;
        case NDUInt32:
            status = doComputeCentroidT<epicsUInt32>(pArray, pStats);
            break;
        case NDInt64:
            status = doComputeCentroidT<epicsInt64>(pArray, pStats);
            break;
        case NDUInt64:
            status = doComputeCentroidT<epicsUInt64>(pArray, pStats);
            break;
        case NDFloat32:
            status = doComputeCentroidT<epicsFloat32>(pArray, pStats);
            break;
        case NDFloat64:
            status = doComputeCentroidT<epicsFloat64>(pArray, pStats);
            break;
        default:
            status = asynError;
        break;
    }
    return(status);
}

template <typename epicsType>
asynStatus NDPluginStats::doComputeProfilesT(NDArray *pArray, NDStats_t *pStats)
{
    epicsType *pData = (epicsType *)pArray->pData;
    epicsType *pCentroid, *pCursor;
    size_t ix, iy;

    if (pArray->ndims > 2) return(asynError);

    /* Compute the X and Y profiles at the centroid and cursor positions */
    iy = (size_t) (pStats->centroidY + 0.5);
    iy = MAX(iy, 0);
    iy = MIN(iy, pStats->profileSizeY-1);
    pCentroid = pData + iy*pStats->profileSizeX;
    iy = pStats->cursorY;
    iy = MAX(iy, 0);
    iy = MIN(iy, pStats->profileSizeY-1);
    pCursor = pData + iy*pStats->profileSizeX;
    for (ix=0; ix<pStats->profileSizeX; ix++) {
        pStats->profileX[profCentroid][ix] = (double)*pCentroid++;
        pStats->profileX[profCursor][ix]   = (double)*pCursor++;
    }
    ix = (size_t) (pStats->centroidX + 0.5);
    ix = MAX(ix, 0);
    ix = MIN(ix, pStats->profileSizeX-1);
    pCentroid = pData + ix;
    ix = pStats->cursorX;
    ix = MAX(ix, 0);
    ix = MIN(ix, pStats->profileSizeX-1);
    pCursor = pData + ix;
    /* Compute cursor value. */
    pStats->cursorValue = (double)*(pData + iy * pStats->profileSizeX + ix);
    for (iy=0; iy<pStats->profileSizeY; iy++) {
        pStats->profileY[profCentroid][iy] = (double)*pCentroid;
        pStats->profileY[profCursor][iy]   = (double)*pCursor;
        pCentroid += pStats->profileSizeX;
        pCursor   += pStats->profileSizeX;
    }

    return(asynSuccess);
}

asynStatus NDPluginStats::doComputeProfiles(NDArray *pArray, NDStats_t *pStats)
{
    asynStatus status;

    switch(pArray->dataType) {
        case NDInt8:
            status = doComputeProfilesT<epicsInt8>(pArray, pStats);
            break;
        case NDUInt8:
            status = doComputeProfilesT<epicsUInt8>(pArray, pStats);
            break;
        case NDInt16:
            status = doComputeProfilesT<epicsInt16>(pArray, pStats);
            break;
        case NDUInt16:
            status = doComputeProfilesT<epicsUInt16>(pArray, pStats);
            break;
        case NDInt32:
            status = doComputeProfilesT<epicsInt32>(pArray, pStats);
            break;
        case NDUInt32:
            status = doComputeProfilesT<epicsUInt32>(pArray, pStats);
            break;
        case NDInt64:
            status = doComputeProfilesT<epicsInt64>(pArray, pStats);
            break;
        case NDUInt64:
            status = doComputeProfilesT<epicsUInt64>(pArray, pStats);
            break;
        case NDFloat32:
            status = doComputeProfilesT<epicsFloat32>(pArray, pStats);
            break;
        case NDFloat64:
            status = doComputeProfilesT<epicsFloat64>(pArray, pStats);
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
    size_t bgdPixels;
    int bgdWidth;
    int dim;
    NDStats_t stats, *pStats=&stats, statsTemp, *pStatsTemp=&statsTemp;
    double bgdCounts, avgBgd;
    NDArray *pBgdArray=NULL;
    int computeStatistics, computeCentroid, computeProfiles, computeHistogram;
    size_t sizeX=0, sizeY=0;
    int i;
    int itemp;
    NDArrayInfo arrayInfo;
    static const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);

    pArray->getInfo(&arrayInfo);
    getIntegerParam(NDPluginStatsComputeStatistics,  &computeStatistics);
    getIntegerParam(NDPluginStatsComputeCentroid,    &computeCentroid);
    getIntegerParam(NDPluginStatsComputeProfiles,    &computeProfiles);
    getIntegerParam(NDPluginStatsComputeHistogram,   &computeHistogram);
    getIntegerParam(NDPluginStatsBgdWidth, &bgdWidth);
    getIntegerParam(NDPluginStatsCursorX, &itemp); pStats->cursorX = itemp;
    getIntegerParam(NDPluginStatsCursorY, &itemp); pStats->cursorY = itemp;
    getIntegerParam(NDPluginStatsHistSize, &pStats->histSize);
    getDoubleParam (NDPluginStatsHistMin,  &pStats->histMin);
    getDoubleParam (NDPluginStatsHistMax,  &pStats->histMax);
    getDoubleParam (NDPluginStatsCentroidThreshold,  &pStats->centroidThreshold);

    if (pArray->ndims > 0) sizeX = pArray->dims[0].size;
    if (pArray->ndims == 1) sizeY = 1;
    if (pArray->ndims > 1)  sizeY = pArray->dims[1].size;


    if (computeCentroid || computeProfiles) {
        pStats->profileSizeX = sizeX;
        setIntegerParam(NDPluginStatsProfileSizeX,  (int)pStats->profileSizeX);
        for (i=0; i<MAX_PROFILE_TYPES; i++) {
            pStats->profileX[i] = (double *)calloc(pStats->profileSizeX, sizeof(double));
        }
        pStats->profileSizeY = sizeY;
        setIntegerParam(NDPluginStatsProfileSizeY, (int)pStats->profileSizeY);
        for (i=0; i<MAX_PROFILE_TYPES; i++) {
            pStats->profileY[i] = (double *)calloc(pStats->profileSizeY, sizeof(double));
        }
    }

    if (computeHistogram) {
        pStats->histogram = (double *)calloc(pStats->histSize, sizeof(double));
    }

    // Release the lock.  While it is released we cannot access the parameter library or class member data.
    this->unlock();

    if (computeStatistics) {
        doComputeStatistics(pArray, pStats);
        /* If there is a non-zero background width then compute the background counts */
        // Note that the following algorithm is general in N-dimensions but does have a slight inaccuracy.
        // It computes the background region such that the pixels at the corners are counted twice.
        // The normalization correctly accounts for this when computing the average background per pixel,
        // but these pixels are given extra weight in the calculation.
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
                pDim->size = MIN((size_t)bgdWidth, pDim->size);
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
                pDim->offset = MAX(0, (int)(pDim->size - bgdWidth));
                pDim->size = MIN((size_t)bgdWidth, pArray->dims[dim].size - pDim->offset);
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
    }

    if (computeCentroid) {
         doComputeCentroid(pArray, pStats);
    }

    if (computeProfiles) {
        doComputeProfiles(pArray, pStats);
    }

    if (computeHistogram) {
        doComputeHistogram(pArray, pStats);
    }

    // Take the lock again.  The time-series data need to be protected.
    this->lock();

    size_t dims=MAX_TIME_SERIES_TYPES;
    NDArray *pTimeSeriesArray = this->pNDArrayPool->alloc(1, &dims, NDFloat64, 0, NULL);
    epicsFloat64 *timeSeries = (epicsFloat64 *)pTimeSeriesArray->pData;
    pTimeSeriesArray->uniqueId  = pArray->uniqueId;
    pTimeSeriesArray->timeStamp = pArray->timeStamp;
    pTimeSeriesArray->epicsTS   = pArray->epicsTS;

    timeSeries[TSMinValue]        = pStats->min;
    timeSeries[TSMinX]            = (double)pStats->minX;
    timeSeries[TSMinY]            = (double)pStats->minY;
    timeSeries[TSMaxValue]        = pStats->max;
    timeSeries[TSMaxX]            = (double)pStats->maxX;
    timeSeries[TSMaxY]            = (double)pStats->maxY;
    timeSeries[TSMeanValue]       = pStats->mean;
    timeSeries[TSSigmaValue]      = pStats->sigma;
    timeSeries[TSTotal]           = pStats->total;
    timeSeries[TSNet]             = pStats->net;
    timeSeries[TSCentroidTotal]   = pStats->centroidTotal;
    timeSeries[TSCentroidX]       = pStats->centroidX;
    timeSeries[TSCentroidY]       = pStats->centroidY;
    timeSeries[TSSigmaX]          = pStats->sigmaX;
    timeSeries[TSSigmaY]          = pStats->sigmaY;
    timeSeries[TSSigmaXY]         = pStats->sigmaXY;
    timeSeries[TSSkewX]           = pStats->skewX;
    timeSeries[TSSkewY]           = pStats->skewY;
    timeSeries[TSKurtosisX]       = pStats->kurtosisX;
    timeSeries[TSKurtosisY]       = pStats->kurtosisY;
    timeSeries[TSEccentricity]    = pStats->eccentricity;
    timeSeries[TSOrientation]     = pStats->orientation;
    timeSeries[TSTimestamp]       = pArray->timeStamp;
    doCallbacksGenericPointer(pTimeSeriesArray, NDArrayData, 1);
    pTimeSeriesArray->release();


    if (computeStatistics) {
        setDoubleParam(NDPluginStatsMinValue,    pStats->min);
        setDoubleParam(NDPluginStatsMinX,        (double)pStats->minX);
        setDoubleParam(NDPluginStatsMinY,        (double)pStats->minY);
        setDoubleParam(NDPluginStatsMaxValue,    pStats->max);
        setDoubleParam(NDPluginStatsMaxX,        (double)pStats->maxX);
        setDoubleParam(NDPluginStatsMaxY,        (double)pStats->maxY);
        setDoubleParam(NDPluginStatsMeanValue,   pStats->mean);
        setDoubleParam(NDPluginStatsSigmaValue,  pStats->sigma);
        setDoubleParam(NDPluginStatsTotal,       pStats->total);
        setDoubleParam(NDPluginStatsNet,         pStats->net);
        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
            "%s:%s min=%f, max=%f, mean=%f, total=%f, net=%f\n",
            driverName, functionName, pStats->min, pStats->max, pStats->mean, pStats->total, pStats->net);
    }

    if (computeCentroid) {
        setDoubleParam(NDPluginStatsCentroidTotal, pStats->centroidTotal);
        setDoubleParam(NDPluginStatsCentroidX,     pStats->centroidX);
        setDoubleParam(NDPluginStatsCentroidY,     pStats->centroidY);
        setDoubleParam(NDPluginStatsSigmaX,        pStats->sigmaX);
        setDoubleParam(NDPluginStatsSigmaY,        pStats->sigmaY);
        setDoubleParam(NDPluginStatsSigmaXY,       pStats->sigmaXY);
        setDoubleParam(NDPluginStatsSkewX,         pStats->skewX);
        setDoubleParam(NDPluginStatsSkewY,         pStats->skewY);
        setDoubleParam(NDPluginStatsKurtosisX,     pStats->kurtosisX);
        setDoubleParam(NDPluginStatsKurtosisY,     pStats->kurtosisY);
        setDoubleParam(NDPluginStatsEccentricity,  pStats->eccentricity);
        setDoubleParam(NDPluginStatsOrientation,   pStats->orientation);
    }

    if (computeProfiles) {
        doCallbacksFloat64Array(pStats->profileX[profAverage],   pStats->profileSizeX, NDPluginStatsProfileAverageX, 0);
        doCallbacksFloat64Array(pStats->profileY[profAverage],   pStats->profileSizeY, NDPluginStatsProfileAverageY, 0);
        doCallbacksFloat64Array(pStats->profileX[profThreshold], pStats->profileSizeX, NDPluginStatsProfileThresholdX, 0);
        doCallbacksFloat64Array(pStats->profileY[profThreshold], pStats->profileSizeY, NDPluginStatsProfileThresholdY, 0);
        doCallbacksFloat64Array(pStats->profileX[profCentroid],  pStats->profileSizeX, NDPluginStatsProfileCentroidX, 0);
        doCallbacksFloat64Array(pStats->profileY[profCentroid],  pStats->profileSizeY, NDPluginStatsProfileCentroidY, 0);
        doCallbacksFloat64Array(pStats->profileX[profCursor],    pStats->profileSizeX, NDPluginStatsProfileCursorX, 0);
        doCallbacksFloat64Array(pStats->profileY[profCursor],    pStats->profileSizeY, NDPluginStatsProfileCursorY, 0);
        setDoubleParam(NDPluginStatsCursorVal,     pStats->cursorValue);
    }

    if (computeHistogram) {
        setDoubleParam(NDPluginStatsHistEntropy, pStats->histEntropy);
        setIntegerParam(NDPluginStatsHistBelow, pStats->histBelow);
        setIntegerParam(NDPluginStatsHistAbove, pStats->histAbove);
        doCallbacksFloat64Array(pStats->histogram, pStats->histSize, NDPluginStatsHistArray, 0);
    }

    if (computeCentroid || computeProfiles) {
        for (i=0; i<MAX_PROFILE_TYPES; i++) {
            free(pStats->profileX[i]);
            free(pStats->profileY[i]);
        }
    }

    if (computeHistogram) {
        free(pStats->histogram);
    }

    NDPluginDriver::endProcessCallbacks(pArray, true, true);

    callParamCallbacks();
}

asynStatus NDPluginStats::computeHistX()
{
    int histSize;
    int i;
    double scale, histMin, histMax, *histX;

    getIntegerParam(NDPluginStatsHistSize, &histSize);
    getDoubleParam(NDPluginStatsHistMin, &histMin);
    getDoubleParam(NDPluginStatsHistMax, &histMax);
    if (histSize < 1) histSize = 1;
    histX = (double *)calloc(histSize, sizeof(double));
    scale = (histMax - histMin) / histSize;
    for (i=0; i<histSize; i++) {
        histX[i] = histMin + i*scale;
    }
    doCallbacksFloat64Array(histX, histSize, NDPluginStatsHistXArray, 0);
    free(histX);
    return asynSuccess;
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginStats::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeInt32";


    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    if (function == NDPluginStatsHistSize) {
          status = computeHistX();
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_STATS_PARAM)
            status = NDPluginDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
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
asynStatus  NDPluginStats::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeFloat64";

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(function, value);

    if ((function == NDPluginStatsHistMin)  ||
               (function == NDPluginStatsHistMax)) {
        status = computeHistX();
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_NDPLUGIN_STATS_PARAM) status = NDPluginDriver::writeFloat64(pasynUser, value);
    }

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
  * \param[in] maxThreads The maximum number of threads this driver is allowed to use. If 0 then 1 will be used.
  */
NDPluginStats::NDPluginStats(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize, int maxThreads)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 2, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   0, 1, priority, stackSize, maxThreads)
{
    //static const char *functionName = "NDPluginStats";

    /* Statistics */
    createParam(NDPluginStatsComputeStatisticsString, asynParamInt32,      &NDPluginStatsComputeStatistics);
    createParam(NDPluginStatsBgdWidthString,          asynParamInt32,      &NDPluginStatsBgdWidth);
    createParam(NDPluginStatsMinValueString,          asynParamFloat64,    &NDPluginStatsMinValue);
    createParam(NDPluginStatsMinXString,              asynParamFloat64,    &NDPluginStatsMinX);
    createParam(NDPluginStatsMinYString,              asynParamFloat64,    &NDPluginStatsMinY);
    createParam(NDPluginStatsMaxValueString,          asynParamFloat64,    &NDPluginStatsMaxValue);
    createParam(NDPluginStatsMaxXString,              asynParamFloat64,    &NDPluginStatsMaxX);
    createParam(NDPluginStatsMaxYString,              asynParamFloat64,    &NDPluginStatsMaxY);
    createParam(NDPluginStatsMeanValueString,         asynParamFloat64,    &NDPluginStatsMeanValue);
    createParam(NDPluginStatsSigmaValueString,        asynParamFloat64,    &NDPluginStatsSigmaValue);
    createParam(NDPluginStatsTotalString,             asynParamFloat64,    &NDPluginStatsTotal);
    createParam(NDPluginStatsNetString,               asynParamFloat64,    &NDPluginStatsNet);

    /* Centroid */
    createParam(NDPluginStatsComputeCentroidString,   asynParamInt32,      &NDPluginStatsComputeCentroid);
    createParam(NDPluginStatsCentroidThresholdString, asynParamFloat64,    &NDPluginStatsCentroidThreshold);
    createParam(NDPluginStatsCentroidTotalString,     asynParamFloat64,    &NDPluginStatsCentroidTotal);
    createParam(NDPluginStatsCentroidXString,         asynParamFloat64,    &NDPluginStatsCentroidX);
    createParam(NDPluginStatsCentroidYString,         asynParamFloat64,    &NDPluginStatsCentroidY);
    createParam(NDPluginStatsSigmaXString,            asynParamFloat64,    &NDPluginStatsSigmaX);
    createParam(NDPluginStatsSigmaYString,            asynParamFloat64,    &NDPluginStatsSigmaY);
    createParam(NDPluginStatsSigmaXYString,           asynParamFloat64,    &NDPluginStatsSigmaXY);
    createParam(NDPluginStatsSkewXString,             asynParamFloat64,    &NDPluginStatsSkewX);
    createParam(NDPluginStatsSkewYString,             asynParamFloat64,    &NDPluginStatsSkewY);
    createParam(NDPluginStatsKurtosisXString,         asynParamFloat64,    &NDPluginStatsKurtosisX);
    createParam(NDPluginStatsKurtosisYString,         asynParamFloat64,    &NDPluginStatsKurtosisY);
    createParam(NDPluginStatsEccentricityString,      asynParamFloat64,    &NDPluginStatsEccentricity);
    createParam(NDPluginStatsOrientationString,       asynParamFloat64,    &NDPluginStatsOrientation);

    /* Profiles */
    createParam(NDPluginStatsComputeProfilesString,   asynParamInt32,         &NDPluginStatsComputeProfiles);
    createParam(NDPluginStatsProfileSizeXString,      asynParamInt32,         &NDPluginStatsProfileSizeX);
    createParam(NDPluginStatsProfileSizeYString,      asynParamInt32,         &NDPluginStatsProfileSizeY);
    createParam(NDPluginStatsCursorXString,           asynParamInt32,         &NDPluginStatsCursorX);
    createParam(NDPluginStatsCursorYString,           asynParamInt32,         &NDPluginStatsCursorY);
    createParam(NDPluginStatsCursorValString,         asynParamFloat64,       &NDPluginStatsCursorVal);
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
    createParam(NDPluginStatsHistBelowString,         asynParamInt32,         &NDPluginStatsHistBelow);
    createParam(NDPluginStatsHistAboveString,         asynParamInt32,         &NDPluginStatsHistAbove);
    createParam(NDPluginStatsHistEntropyString,       asynParamFloat64,       &NDPluginStatsHistEntropy);
    createParam(NDPluginStatsHistArrayString,         asynParamFloat64Array,  &NDPluginStatsHistArray);
    createParam(NDPluginStatsHistXArrayString,        asynParamFloat64Array,  &NDPluginStatsHistXArray);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginStats");

    /* Try to connect to the array port */
    connectToArrayPort();
}

/** Configuration command */
extern "C" int NDStatsConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize, int maxThreads)
{
    NDPluginStats *pPlugin = new NDPluginStats(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                              maxBuffers, maxMemory, priority, stackSize, maxThreads);
    return pPlugin->start();
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
static const iocshArg initArg9 = { "maxThreads",iocshArgInt};
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
static const iocshFuncDef initFuncDef = {"NDStatsConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDStatsConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival,
                     args[9].ival);
}

extern "C" void NDStatsRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDStatsRegister);
}
