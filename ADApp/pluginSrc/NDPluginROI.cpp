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
#include <iocsh.h>
#include <epicsExport.h>

#include "NDArray.h"
#include "NDPluginROI.h"

static const char *driverName="NDPluginROI";

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)



template <typename epicsType>
void doComputeHistogramT(NDArray *pArray, NDROI *pROI)
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

int doComputeHistogram(NDArray *pArray, NDROI *pROI)
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
void doCorrectionsT(NDArray *pArray, NDROI *pROI)
{
    int i;
    epicsType *pData = (epicsType *)pArray->pData;
    NDArrayInfo arrayInfo;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    double *background=NULL, *flatField=NULL, *average, frac;
    double value;

    if (pROI->validBackground && pROI->enableBackground)
        background = (double *)pROI->pBackground->pData;
    if (pROI->validFlatField && pROI->enableFlatField)
        flatField = (double *)pROI->pFlatField->pData;
    for (i=0; i<pROI->nElements; i++) {
        value = (double)pData[i];
        if (background) value -= background[i];
        if (flatField) {
            if (flatField[i] != 0.) 
                value *= pROI->scaleFlatField / flatField[i];
            else
                value = pROI->scaleFlatField;
        }
        if (pROI->enableHighClip && (value > pROI->highClip)) value = pROI->highClip;
        if (pROI->enableLowClip  && (value < pROI->lowClip))  value = pROI->lowClip;
        pData[i] = (epicsType)value;
    }
    
    if (pROI->enableAverage) {
        if (pROI->pAverage) {
            pROI->pAverage->getInfo(&arrayInfo);
            if (pROI->nElements != arrayInfo.nElements) {
                pROI->pAverage->release();
                pROI->pAverage = NULL;
            }
        }
        if (!pROI->pAverage) {
            /* There is not a current average array */
            /* Make a copy of the current ROI array, converted to double type */
            /* For converted array set reverse, offset and binning to not change anything */
            for (i=0; i<pArray->ndims; i++) {
                dims[i].size    = pArray->dims[i].size;
                dims[i].reverse = 0;
                dims[i].offset  = 0;
                dims[i].binning = 1;
            }
            pArray->pNDArrayPool->convert(pArray, &pROI->pAverage, NDFloat64, dims);
            pROI->numAveraged = 1;
        } else {
            /* Merge the current array into the average, replace with average */
            if (pROI->numAveraged < pROI->numAverage) pROI->numAveraged++;
            average = (double *)pROI->pAverage->pData;
            frac =  1./pROI->numAveraged;
            for (i=0; i<pROI->nElements; i++) {
                average[i] = frac*pData[i] + (1.-frac)*average[i];
                pData[i] = (epicsType)average[i];
            }
        }
    }
}

int doCorrections(NDArray *pArray, NDROI *pROI)
{

    switch(pArray->dataType) {
        case NDInt8:
            doCorrectionsT<epicsInt8>(pArray, pROI);
            break;
        case NDUInt8:
            doCorrectionsT<epicsUInt8>(pArray, pROI);
            break;
        case NDInt16:
            doCorrectionsT<epicsInt16>(pArray, pROI);
            break;
        case NDUInt16:
            doCorrectionsT<epicsUInt16>(pArray, pROI);
            break;
        case NDInt32:
            doCorrectionsT<epicsInt32>(pArray, pROI);
            break;
        case NDUInt32:
            doCorrectionsT<epicsUInt32>(pArray, pROI);
            break;
        case NDFloat32:
            doCorrectionsT<epicsFloat32>(pArray, pROI);
            break;
        case NDFloat64:
            doCorrectionsT<epicsFloat64>(pArray, pROI);
            break;
        default:
            return(ND_ERROR);
        break;
    }
    return(ND_SUCCESS);
}

template <typename epicsType>
void doComputeStatisticsT(NDArray *pArray, NDROI *pROI)
{
    int i;
    epicsType *pData = (epicsType *)pArray->pData;
    NDArrayInfo arrayInfo;
    double value, centroidTotal;
    int ix, iy;

    pArray->getInfo(&arrayInfo);
    pROI->nElements = arrayInfo.nElements;
    pROI->min = (double) pData[0];
    pROI->max = (double) pData[0];
    pROI->total = 0.;
    pROI->sigma = 0.;
    centroidTotal = 0.;
    pROI->centroidX = 0;
    pROI->centroidY = 0;
    pROI->sigmaX = 0;
    pROI->sigmaY = 0;

    for (i=0; i<pROI->nElements; i++) {
        value = (double)pData[i];
        if (value < pROI->min) pROI->min = value;
        if (value > pROI->max) pROI->max = value;
        pROI->total += value;
        if (pROI->computeCentroid) {
            pROI->sigma += value * value;
            ix = i % pArray->dims[0].size;
            iy = i / pArray->dims[0].size;
            if (value < pROI->centroidThreshold) value = 0;
            centroidTotal   += value;
            pROI->centroidX += value * ix;
            pROI->centroidY += value * iy;
            pROI->sigmaX    += value * ix * ix;
            pROI->sigmaY    += value * iy * iy;
        }
    }

    pROI->net = pROI->total;
    pROI->mean = pROI->total / pROI->nElements;

    if (pROI->computeCentroid) {
        pROI->sigma = sqrt((pROI->sigma / pROI->nElements) - (pROI->mean * pROI->mean));
        pROI->centroidX /= centroidTotal;
        pROI->centroidY /= centroidTotal;
        pROI->sigmaX = sqrt((pROI->sigmaX / centroidTotal) - (pROI->centroidX * pROI->centroidX));
        pROI->sigmaY = sqrt((pROI->sigmaY / centroidTotal) - (pROI->centroidY * pROI->centroidY));
    }
}

int doComputeStatistics(NDArray *pArray, NDROI *pROI)
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
void highlightROIT(NDArray *pArray, NDROI *pROI, double dvalue)
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

int highlightROI(NDArray *pArray, NDROI *pROI, double dvalue)
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


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Extracts the NthrDArray data into each of the ROIs that are being used.
  * Computes statistics on the ROI if NDPluginROIComputeStatistics is 1.
  * Computes the histogram of ROI values if NDPluginROIComputeHistogram is 1.
  * \param[in] pArray  The NDArray from the callback.
  */
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
    NDROI *pROI, ROITemp, *pROITemp=&ROITemp;
    int highlight;
    int bgdPixels;
    double bgdCounts, avgBgd;
    int colorMode = NDColorModeMono;
    NDAttribute *pAttribute;
    const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

    getIntegerParam(NDPluginROIHighlight, &highlight);
    if (highlight && (pArray->ndims == 2)) {
        /* We need to highlight the ROIs.
         * We do this by searching for the maximum element in the data and setting the
         * outline of each ROI to this value.  However, we are not allowed to modify the data
         * since other callbacks could be using it, so we need to make a copy */
        pHighlights = this->pNDArrayPool->copy(pArray, NULL, 1);

        status = doComputeStatistics(pHighlights, pROITemp);

        if (pROITemp->max == 0) pROITemp->max=1;
        for (roi=0; roi<this->maxROIs; roi++) {
            pROI = &this->pROIs[roi];
            getIntegerParam(roi, NDPluginROIUse, &use);
            if (!use) continue;
            highlightROI(pHighlights, pROI, pROITemp->max);
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
        if (!use) continue;

        /* Need to fetch all of these parameters while we still have the mutex */
        getIntegerParam(roi, NDPluginROIDataType,           &dataType);

        getIntegerParam(roi, NDPluginROIEnableBackground,   &pROI->enableBackground);

        getIntegerParam(roi, NDPluginROIEnableFlatField,    &pROI->enableFlatField);
        getDoubleParam (roi, NDPluginROIScaleFlatField,     &pROI->scaleFlatField);
 
        getIntegerParam(roi, NDPluginROIEnableLowClip,      &pROI->enableLowClip);
        getDoubleParam (roi, NDPluginROILowClip,            &pROI->lowClip);
        getIntegerParam(roi, NDPluginROIEnableHighClip,     &pROI->enableHighClip);
        getDoubleParam (roi, NDPluginROIHighClip,           &pROI->highClip);

        getIntegerParam(roi, NDPluginROIEnableAverage,      &pROI->enableAverage);
        getIntegerParam(roi, NDPluginROINumAverage,         &pROI->numAverage);
        
        getIntegerParam(roi, NDPluginROIComputeStatistics,  &computeStatistics);
        getIntegerParam(roi, NDPluginROIComputeCentroid,    &pROI->computeCentroid);
        getDoubleParam (roi, NDPluginROICentroidThreshold,  &pROI->centroidThreshold);

        getIntegerParam(roi, NDPluginROIComputeProfiles,    &computeProfiles);

        getIntegerParam(roi, NDPluginROIComputeHistogram,   &computeHistogram);
        getDoubleParam (roi, NDPluginROIHistMin,            &pROI->histMin);
        getDoubleParam (roi, NDPluginROIHistMax,            &pROI->histMax);
        getIntegerParam(roi, NDPluginROIBgdWidth,           &pROI->bgdWidth);
        getIntegerParam(roi, NDPluginROIHistSize,           &histSize);

        /* Make sure dimensions are valid, fix them if they are not */
        /* We treat the case of RGB1 data specially, so that NX and NY are the X and Y dimensions of the
         * image, not the first 2 dimensions.  This makes it much easier to switch back and forth between
         * RGB1 and mono mode when using an ROI. */
        if (colorMode == NDColorModeRGB1) {
            userDims[0] = 1;
            userDims[1] = 2;
            userDims[2] = 0;
        }
        else if (colorMode == NDColorModeRGB2) {
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
        this->unlock();

        /* Extract this ROI from the input array.  The convert() function allocates
         * a new array and it is reserved (reference count = 1) */
        if (dataType == -1) dataType = (int)pArray->dataType;
        /* We treat the case of RGB1 data specially, so that NX and NY are the X and Y dimensions of the
         * image, not the first 2 dimensions.  This makes it much easier to switch back and forth between
         * RGB1 and mono mode when using an ROI. */
        if (colorMode == NDColorModeRGB1) {
            tempDim = dims[0];
            dims[0] = dims[2];
            dims[2] = dims[1];
            dims[1] = tempDim;
        }
        else if (colorMode == NDColorModeRGB2) {
            tempDim = dims[1];
            dims[1] = dims[2];
            dims[2] = tempDim;
        }
        this->pNDArrayPool->convert(pArray, &this->pArrays[roi], (NDDataType_t)dataType, dims);
        pROIArray  = this->pArrays[roi];

        pROIArray->getInfo(&arrayInfo);
        pROI->nElements = arrayInfo.nElements;
        pROI->validBackground = 0;
        if (pROI->pBackground && (pROI->nElements == pROI->nBackgroundElements)) pROI->validBackground = 1;
        setIntegerParam(roi, NDPluginROIValidBackground, pROI->validBackground);
        pROI->validFlatField = 0;
        if (pROI->pFlatField && (pROI->nElements == pROI->nFlatFieldElements)) pROI->validFlatField = 1;
        setIntegerParam(roi, NDPluginROIValidFlatField, pROI->validFlatField);

        /* If any image processing is to be done then call doCorrections */
        if ((pROI->enableBackground && pROI->validBackground) || 
            (pROI->enableFlatField && pROI->validFlatField)   ||
             pROI->enableLowClip                              || 
             pROI->enableHighClip                             ||
             pROI->enableAverage) {
            /* Unfortunately we must lock while doing the corrections because the background and average arrays
             * can be changed from another thread if we don't */
            this->lock();
            doCorrections(pROIArray, pROI);
            this->unlock();
        }
            
        if (computeStatistics) {
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
            setIntegerParam(roi,NDPluginROINumAveraged, pROI->numAveraged);
            setDoubleParam(roi, NDPluginROIMinValue,    pROI->min);
            setDoubleParam(roi, NDPluginROIMaxValue,    pROI->max);
            setDoubleParam(roi, NDPluginROIMeanValue,   pROI->mean);
            setDoubleParam(roi, NDPluginROISigmaValue,  pROI->sigma);
            setDoubleParam(roi, NDPluginROICentroidX,   pROI->centroidX);
            setDoubleParam(roi, NDPluginROICentroidY,   pROI->centroidY);
            setDoubleParam(roi, NDPluginROISigmaX,      pROI->sigmaX);
            setDoubleParam(roi, NDPluginROISigmaY,      pROI->sigmaY);
            setDoubleParam(roi, NDPluginROITotal,       pROI->total);
            setDoubleParam(roi, NDPluginROINet,         pROI->net);
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
        setIntegerParam(roi, NDArraySizeX, this->pArrays[roi]->dims[userDims[0]].size);
        setIntegerParam(roi, NDArraySizeY, this->pArrays[roi]->dims[userDims[1]].size);
        setIntegerParam(roi, NDArraySizeZ, this->pArrays[roi]->dims[userDims[2]].size);

        /* We must enter the loop and exit with the mutex locked */
        this->lock();
        callParamCallbacks(roi, roi);
    }

    /* Build the arrays of total and net counts for fast scanning, do callbacks */
    for (roi=0; roi<this->maxROIs; roi++) {
        getIntegerParam(roi, NDPluginROIUse, &use);
        if (!use) continue;
        /* Get the attributes for this driver */
        this->getAttributes(this->pArrays[roi]->pAttributeList);
        /* Call any clients who have registered for NDArray callbacks */
        this->unlock();
        doCallbacksGenericPointer(this->pArrays[roi], NDArrayData, roi);
        this->lock();
        this->totalArray[roi] = (epicsInt32)this->pROIs[roi].total;
        this->netArray[roi] = (epicsInt32)this->pROIs[roi].net;
    }

    doCallbacksInt32Array(this->totalArray, this->maxROIs, NDPluginROITotalArray, 0);
    doCallbacksInt32Array(this->netArray, this->maxROIs, NDPluginROINetArray, 0);

    /* If we made a copy of the array for highlighting then free it */
    if (pHighlights) pHighlights->release();

    callParamCallbacks();

}


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including minimum, size, binning, etc. for each ROI.
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginROI::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi=0;
    int i;
    NDROI *pROI;
    NDArray *pArray;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS];
    NDArrayInfo arrayInfo;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, &roi); if (status != asynSuccess) return(status);

    pROI = &this->pROIs[roi];
    /* Set parameter and readback in parameter library */
    status = setIntegerParam(roi , function, value);
    if (function == NDPluginROIDim0Min)
            pROI->dims[0].offset = value;
    else if (function == NDPluginROIDim0Size)
            pROI->dims[0].size = value;
    else if (function == NDPluginROIDim0Bin)
            pROI->dims[0].binning = value;
    else if (function == NDPluginROIDim0Reverse)
            pROI->dims[0].reverse = value;
    else if (function == NDPluginROIDim1Min)
            pROI->dims[1].offset = value;
    else if (function == NDPluginROIDim1Size)
            pROI->dims[1].size = value;
    else if (function == NDPluginROIDim1Bin)
            pROI->dims[1].binning = value;
    else if (function == NDPluginROIDim1Reverse)
            pROI->dims[1].reverse = value;
    else if (function == NDPluginROIDim2Min)
            pROI->dims[2].offset = value;
    else if (function == NDPluginROIDim2Size)
            pROI->dims[2].size = value;
    else if (function == NDPluginROIDim2Bin)
            pROI->dims[2].binning = value;
    else if (function == NDPluginROIDim2Reverse)
            pROI->dims[2].reverse = value;
    else if (function == NDPluginROISaveBackground && value) {
        if (pROI->pBackground) pROI->pBackground->release();
        pROI->pBackground = NULL;
        pArray = this->pArrays[roi];
        if (pArray) {
            /* Make a copy of the current ROI array, converted to double type */
            /* For for converted array set reverse, offset and binning to not change anything */
            for (i=0; i<pArray->ndims; i++) {
                dims[i].size    = pArray->dims[i].size;
                dims[i].reverse = 0;
                dims[i].offset  = 0;
                dims[i].binning = 1;
            }
            /* Make a copy of the current ROI array, converted to double type */
            this->pNDArrayPool->convert(pArray, &pROI->pBackground, NDFloat64, dims);
            pROI->pBackground->getInfo(&arrayInfo);
            pROI->nBackgroundElements = arrayInfo.nElements;
        }
        setIntegerParam(roi, NDPluginROISaveBackground, 0);
    }
    else if (function == NDPluginROISaveFlatField && value) {
        if (pROI->pFlatField) pROI->pFlatField->release();
        pROI->pFlatField = NULL;
        pArray = this->pArrays[roi];
        if (pArray) {
            /* Make a copy of the current ROI array, converted to double type */
            /* For for converted array set reverse, offset and binning to not change anything */
            for (i=0; i<pArray->ndims; i++) {
                dims[i].size    = pArray->dims[i].size;
                dims[i].reverse = 0;
                dims[i].offset  = 0;
                dims[i].binning = 1;
            }
            /* Make a copy of the current ROI array, converted to double type */
            this->pNDArrayPool->convert(pArray, &pROI->pFlatField, NDFloat64, dims);
            pROI->pFlatField->getInfo(&arrayInfo);
            pROI->nFlatFieldElements = arrayInfo.nElements;
        }
        setIntegerParam(roi, NDPluginROISaveFlatField, 0);
    }
    else if ((function == NDPluginROIEnableAverage) ||
             (function == NDPluginROINumAverage)) {
        /* If averaging is turned off or on, or the number to average changes, delete the average array
         * forcing averaging to restart */
        if (pROI->pAverage) pROI->pAverage->release();
        pROI->pAverage = NULL;
    }
    else {
        /* This was not a parameter that this driver understands, try the base class */
        status = NDPluginDriver::writeInt32(pasynUser, value);
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

/** Called when asyn clients call pasynFloat64Array->read().
  * Returns the histogram array when pasynUser->reason=NDPluginROIHistArray.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Pointer to the array to read.
  * \param[in] nElements Number of elements to read.
  * \param[out] nIn Number of elements actually read. */
asynStatus NDPluginROI::readFloat64Array(asynUser *pasynUser,
                                   epicsFloat64 *value, size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int roi;
    NDROI *pROI;
    const char* functionName = "readFloat64Array";

    status = getAddress(pasynUser, &roi); if (status != asynSuccess) return(status);
    if (function < FIRST_NDPLUGIN_ROI_PARAM) {
        /* We don't support reading asynFloat64 arrays except for ROIs */
        status = asynError;
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "%s:%s: invalid request",
                     driverName, functionName);
    } else {
        size_t ncopy;
        pROI = &this->pROIs[roi];
        if (function == NDPluginROIHistArray) {
            ncopy = pROI->histSize;
            if (ncopy > nElements) ncopy = nElements;
            memcpy(value, pROI->histogram, ncopy*sizeof(epicsFloat64));
            *nIn = ncopy;
        } else {
            status = asynError;
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                         "%s:%s: invalid request",
                         driverName, functionName);
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


/** Constructor for NDPluginROI; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * ROI parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxROIs The maximum number of ROIs this plugin supports. 1 is minimum.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginROI::NDPluginROI(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, int maxROIs,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, maxROIs, NUM_NDPLUGIN_ROI_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    asynStatus status;
    int roi;
    const char *functionName = "NDPluginROI";

    this->maxROIs = maxROIs;
    this->pROIs = new NDROI[maxROIs] ;
    if(!this->pROIs) {cantProceed(functionName);}
    this->totalArray = (epicsInt32 *)callocMustSucceed(maxROIs, sizeof(epicsInt32), functionName);
    this->netArray = (epicsInt32 *)callocMustSucceed(maxROIs, sizeof(epicsInt32), functionName);

    /* ROI general parameters */
    createParam(NDPluginROINameString,              asynParamOctet, &NDPluginROIName);
    createParam(NDPluginROIUseString,               asynParamInt32, &NDPluginROIUse);
    createParam(NDPluginROIHighlightString,         asynParamInt32, &NDPluginROIHighlight);

     /* ROI definition */
    createParam(NDPluginROIDim0MinString,           asynParamInt32, &NDPluginROIDim0Min);
    createParam(NDPluginROIDim0SizeString,          asynParamInt32, &NDPluginROIDim0Size);
    createParam(NDPluginROIDim0MaxSizeString,       asynParamInt32, &NDPluginROIDim0MaxSize);
    createParam(NDPluginROIDim0BinString,           asynParamInt32, &NDPluginROIDim0Bin);
    createParam(NDPluginROIDim0ReverseString,       asynParamInt32, &NDPluginROIDim0Reverse);
    createParam(NDPluginROIDim1MinString,           asynParamInt32, &NDPluginROIDim1Min);
    createParam(NDPluginROIDim1SizeString,          asynParamInt32, &NDPluginROIDim1Size);
    createParam(NDPluginROIDim1MaxSizeString,       asynParamInt32, &NDPluginROIDim1MaxSize);
    createParam(NDPluginROIDim1BinString,           asynParamInt32, &NDPluginROIDim1Bin);
    createParam(NDPluginROIDim1ReverseString,       asynParamInt32, &NDPluginROIDim1Reverse);
    createParam(NDPluginROIDim2MinString,           asynParamInt32, &NDPluginROIDim2Min);
    createParam(NDPluginROIDim2SizeString,          asynParamInt32, &NDPluginROIDim2Size);
    createParam(NDPluginROIDim2MaxSizeString,       asynParamInt32, &NDPluginROIDim2MaxSize);
    createParam(NDPluginROIDim2BinString,           asynParamInt32, &NDPluginROIDim2Bin);
    createParam(NDPluginROIDim2ReverseString,       asynParamInt32, &NDPluginROIDim2Reverse);
    createParam(NDPluginROIDataTypeString,          asynParamInt32, &NDPluginROIDataType);

    /* ROI background array subtraction */
    createParam(NDPluginROISaveBackgroundString,    asynParamInt32, &NDPluginROISaveBackground);
    createParam(NDPluginROIEnableBackgroundString,  asynParamInt32, &NDPluginROIEnableBackground);
    createParam(NDPluginROIValidBackgroundString,   asynParamInt32, &NDPluginROIValidBackground);

    /* ROI flat field normalization */
    createParam(NDPluginROISaveFlatFieldString,     asynParamInt32,   &NDPluginROISaveFlatField);
    createParam(NDPluginROIEnableFlatFieldString,   asynParamInt32,   &NDPluginROIEnableFlatField);
    createParam(NDPluginROIValidFlatFieldString,    asynParamInt32,   &NDPluginROIValidFlatField);
    createParam(NDPluginROIScaleFlatFieldString,    asynParamFloat64, &NDPluginROIScaleFlatField);

    /* ROI high and low clipping */
    createParam(NDPluginROILowClipString,           asynParamFloat64, &NDPluginROILowClip);
    createParam(NDPluginROIEnableLowClipString,     asynParamInt32,   &NDPluginROIEnableLowClip);
    createParam(NDPluginROIHighClipString,          asynParamFloat64, &NDPluginROIHighClip);
    createParam(NDPluginROIEnableHighClipString,    asynParamInt32,   &NDPluginROIEnableHighClip);

    /* ROI frame averaging */
    createParam(NDPluginROIEnableAverageString,     asynParamInt32, &NDPluginROIEnableAverage);
    createParam(NDPluginROINumAverageString,        asynParamInt32, &NDPluginROINumAverage);
    createParam(NDPluginROINumAveragedString,       asynParamInt32, &NDPluginROINumAveraged);   

    /* ROI statistics */
    createParam(NDPluginROIComputeStatisticsString, asynParamInt32,      &NDPluginROIComputeStatistics);
    createParam(NDPluginROIBgdWidthString,          asynParamInt32,      &NDPluginROIBgdWidth);
    createParam(NDPluginROIMinValueString,          asynParamFloat64,    &NDPluginROIMinValue);
    createParam(NDPluginROIMaxValueString,          asynParamFloat64,    &NDPluginROIMaxValue);
    createParam(NDPluginROIMeanValueString,         asynParamFloat64,    &NDPluginROIMeanValue);
    createParam(NDPluginROISigmaValueString,        asynParamFloat64,    &NDPluginROISigmaValue);
    createParam(NDPluginROITotalArrayString,        asynParamInt32Array, &NDPluginROITotalArray);
    createParam(NDPluginROINetArrayString,          asynParamInt32Array, &NDPluginROINetArray);

    /* ROI centroid */
    createParam(NDPluginROIComputeCentroidString,   asynParamInt32,      &NDPluginROIComputeCentroid);
    createParam(NDPluginROICentroidThresholdString, asynParamFloat64,    &NDPluginROICentroidThreshold);
    createParam(NDPluginROICentroidXString,         asynParamFloat64,    &NDPluginROICentroidX);
    createParam(NDPluginROICentroidYString,         asynParamFloat64,    &NDPluginROICentroidY);
    createParam(NDPluginROISigmaXString,            asynParamFloat64,    &NDPluginROISigmaX);
    createParam(NDPluginROISigmaYString,            asynParamFloat64,    &NDPluginROISigmaY);

    /* ROI histogram */
    createParam(NDPluginROIComputeHistogramString,  asynParamInt32,         &NDPluginROIComputeHistogram);
    createParam(NDPluginROIHistSizeString,          asynParamInt32,         &NDPluginROIHistSize);
    createParam(NDPluginROIHistMinString,           asynParamFloat64,       &NDPluginROIHistMin);
    createParam(NDPluginROIHistMaxString,           asynParamFloat64,       &NDPluginROIHistMax);
    createParam(NDPluginROIHistEntropyString,       asynParamFloat64,       &NDPluginROIHistEntropy);
    createParam(NDPluginROIHistArrayString,         asynParamFloat64Array,  &NDPluginROIHistArray);

    /* ROI profiles - not yet implemented */
    createParam(NDPluginROIComputeProfilesString,   asynParamInt32, &NDPluginROIComputeProfiles);

    /* Arrays of total and net counts for MCA or waveform record */   
    createParam(NDPluginROITotalString,             asynParamFloat64, &NDPluginROITotal);
    createParam(NDPluginROINetString,               asynParamFloat64, &NDPluginROINet);

    setIntegerParam(0, NDPluginROIHighlight,         0);
    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginROI");

    for (roi=0; roi<this->maxROIs; roi++) {
        setStringParam (roi,  NDPluginROIName,              "");
        setIntegerParam(roi , NDPluginROIUse,               0);
        setIntegerParam(roi , NDPluginROIComputeStatistics, 0);
        setIntegerParam(roi , NDPluginROIComputeHistogram,  0);
        setIntegerParam(roi , NDPluginROIComputeProfiles,   0);
        
        setIntegerParam(roi , NDPluginROISaveBackground,    0);
        setIntegerParam(roi , NDPluginROIEnableBackground,  0);
        setIntegerParam(roi , NDPluginROIValidBackground,   0);

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
        setIntegerParam(roi , NDArraySizeX,                 0);
        setIntegerParam(roi , NDArraySizeY,                 0);
        setIntegerParam(roi , NDArraySizeZ,                 0);
        
        setDoubleParam (roi , NDPluginROICentroidX,         0.);
        setDoubleParam (roi , NDPluginROICentroidY,         0.);
    }

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDROIConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr, int maxROIs,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginROI *pPlugin =
        new NDPluginROI(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxROIs,
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
static const iocshArg initArg5 = { "maxROIs",iocshArgInt};
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
static const iocshFuncDef initFuncDef = {"NDROIConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDROIConfigure(args[0].sval, args[1].ival, args[2].ival,
                   args[3].sval, args[4].ival, args[5].ival,
                   args[6].ival, args[7].ival, args[8].ival,
                   args[9].ival);
}

extern "C" void NDROIRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDROIRegister);
}
