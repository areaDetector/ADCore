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
#include <epicsExport.h>

#include "NDArray.h"
#include "NDPluginStats.h"

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

static const char *driverName="NDPluginStats";

typedef struct NDStats {
    int     nElements;
    double  total;
    double  net;
    double  mean;
    double  sigma;
    double  min;
    double  max;
    int     computeCentroid;
    double  centroidThreshold;
    double  centroidX;
    double  centroidY;
    double  sigmaX;
    double  sigmaY;
} NDStats_t;

template <typename epicsType>
void NDPluginStats::doComputeHistogramT(NDArray *pArray)
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
}

int NDPluginStats::doComputeHistogram(NDArray *pArray)
{
    switch(pArray->dataType) {
        case NDInt8:
            doComputeHistogramT<epicsInt8>(pArray);
            break;
        case NDUInt8:
            doComputeHistogramT<epicsUInt8>(pArray);
            break;
        case NDInt16:
            doComputeHistogramT<epicsInt16>(pArray);
            break;
        case NDUInt16:
            doComputeHistogramT<epicsUInt16>(pArray);
            break;
        case NDInt32:
            doComputeHistogramT<epicsInt32>(pArray);
            break;
        case NDUInt32:
            doComputeHistogramT<epicsUInt32>(pArray);
            break;
        case NDFloat32:
            doComputeHistogramT<epicsFloat32>(pArray);
            break;
        case NDFloat64:
            doComputeHistogramT<epicsFloat64>(pArray);
            break;
        default:
            return(ND_ERROR);
        break;
    }
    return(ND_SUCCESS);
}

template <typename epicsType>
void doComputeStatisticsT(NDArray *pArray, NDStats_t *pStats)
{
    int i;
    epicsType *pData = (epicsType *)pArray->pData;
    NDArrayInfo arrayInfo;
    double value, centroidTotal;
    int ix, iy;

    pArray->getInfo(&arrayInfo);
    pStats->nElements = arrayInfo.nElements;
    pStats->min = (double) pData[0];
    pStats->max = (double) pData[0];
    pStats->total = 0.;
    pStats->sigma = 0.;
    centroidTotal = 0.;
    pStats->centroidX = 0;
    pStats->centroidY = 0;
    pStats->sigmaX = 0;
    pStats->sigmaY = 0;

    for (i=0; i<pStats->nElements; i++) {
        value = (double)pData[i];
        if (value < pStats->min) pStats->min = value;
        if (value > pStats->max) pStats->max = value;
        pStats->total += value;
        pStats->sigma += value * value;
        if (pStats->computeCentroid) {
            ix = i % pArray->dims[0].size;
            iy = i / pArray->dims[0].size;
            if (value < pStats->centroidThreshold) value = 0;
            centroidTotal   += value;
            pStats->centroidX += value * ix;
            pStats->centroidY += value * iy;
            pStats->sigmaX    += value * ix * ix;
            pStats->sigmaY    += value * iy * iy;
        }
    }

    pStats->net = pStats->total;
    pStats->mean = pStats->total / pStats->nElements;
    pStats->sigma = sqrt((pStats->sigma / pStats->nElements) - (pStats->mean * pStats->mean));

    if (pStats->computeCentroid) {
        pStats->centroidX /= centroidTotal;
        pStats->centroidY /= centroidTotal;
        pStats->sigmaX = sqrt((pStats->sigmaX / centroidTotal) - (pStats->centroidX * pStats->centroidX));
        pStats->sigmaY = sqrt((pStats->sigmaY / centroidTotal) - (pStats->centroidY * pStats->centroidY));
    }
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
    int computeStatistics, computeProfiles, computeHistogram;
    int intTotal, intNet;
    NDArrayInfo arrayInfo;

    const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    pArray->getInfo(&arrayInfo);
    getIntegerParam(NDPluginStatsComputeStatistics,  &computeStatistics);
    getIntegerParam(NDPluginStatsComputeCentroid,    &pStats->computeCentroid);
    getDoubleParam (NDPluginStatsCentroidThreshold,  &pStats->centroidThreshold);
    getIntegerParam(NDPluginStatsComputeProfiles,    &computeProfiles);
    getIntegerParam(NDPluginStatsComputeHistogram,   &computeHistogram);
    getIntegerParam(NDPluginStatsBgdWidth,           &bgdWidth);
    
    if (computeStatistics) {
        /* Now that we won't be accessing any memory other threads can access, unlock so we don't block drivers */
        this->unlock();
        doComputeStatistics(pArray, pStats);
        /* If there is a non-zero background width then compute the background counts */
        if (bgdWidth > 0) {
            bgdPixels = 0;
            bgdCounts = 0.;
            for (dim=0; dim<pArray->ndims; dim++) {
                memcpy(bgdDims, &pArray->dims, ND_ARRAY_MAX_DIMS*sizeof(NDDimension_t));
                pDim = &bgdDims[dim];
                pDim->offset = MAX(pDim->offset - bgdWidth, 0);
                pDim->size    = MIN(bgdWidth, pArray->dims[dim].size - pDim->offset);
                this->pNDArrayPool->convert(pArray, &pBgdArray, pArray->dataType, bgdDims);
                if (!pBgdArray) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s::%s, error allocating array buffer in convert\n",
                        driverName, functionName);
                    continue;
                }
                pStatsTemp->computeCentroid = 0;
                doComputeStatistics(pBgdArray, pStatsTemp);
                pBgdArray->release();
                bgdPixels += pStatsTemp->nElements;
                bgdCounts += pStatsTemp->total;
                memcpy(bgdDims, &pArray->dims, ND_ARRAY_MAX_DIMS*sizeof(NDDimension_t));
                pDim->offset = MIN(pDim->offset + pDim->size, pArray->dims[dim].size - 1 - bgdWidth);
                pDim->size    = MIN(bgdWidth, pArray->dims[dim].size - pDim->offset);
                this->pNDArrayPool->convert(pArray, &pBgdArray, pArray->dataType, bgdDims);
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
        setDoubleParam(NDPluginStatsCentroidX,   pStats->centroidX);
        setDoubleParam(NDPluginStatsCentroidY,   pStats->centroidY);
        setDoubleParam(NDPluginStatsSigmaX,      pStats->sigmaX);
        setDoubleParam(NDPluginStatsSigmaY,      pStats->sigmaY);
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
    callParamCallbacks();
}


/** Called when asyn clients call pasynFloat64Array->read().
  * Returns the histogram array when pasynUser->reason=NDPluginStatsHistArray.
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

    /* ROI statistics */
    createParam(NDPluginStatsComputeStatisticsString, asynParamInt32,      &NDPluginStatsComputeStatistics);
    createParam(NDPluginStatsBgdWidthString,          asynParamInt32,      &NDPluginStatsBgdWidth);
    createParam(NDPluginStatsMinValueString,          asynParamFloat64,    &NDPluginStatsMinValue);
    createParam(NDPluginStatsMaxValueString,          asynParamFloat64,    &NDPluginStatsMaxValue);
    createParam(NDPluginStatsMeanValueString,         asynParamFloat64,    &NDPluginStatsMeanValue);
    createParam(NDPluginStatsSigmaValueString,        asynParamFloat64,    &NDPluginStatsSigmaValue);
    createParam(NDPluginStatsTotalArrayString,        asynParamInt32Array, &NDPluginStatsTotalArray);
    createParam(NDPluginStatsNetArrayString,          asynParamInt32Array, &NDPluginStatsNetArray);

    /* ROI centroid */
    createParam(NDPluginStatsComputeCentroidString,   asynParamInt32,      &NDPluginStatsComputeCentroid);
    createParam(NDPluginStatsCentroidThresholdString, asynParamFloat64,    &NDPluginStatsCentroidThreshold);
    createParam(NDPluginStatsCentroidXString,         asynParamFloat64,    &NDPluginStatsCentroidX);
    createParam(NDPluginStatsCentroidYString,         asynParamFloat64,    &NDPluginStatsCentroidY);
    createParam(NDPluginStatsSigmaXString,            asynParamFloat64,    &NDPluginStatsSigmaX);
    createParam(NDPluginStatsSigmaYString,            asynParamFloat64,    &NDPluginStatsSigmaY);

    /* ROI histogram */
    createParam(NDPluginStatsComputeHistogramString,  asynParamInt32,         &NDPluginStatsComputeHistogram);
    createParam(NDPluginStatsHistSizeString,          asynParamInt32,         &NDPluginStatsHistSize);
    createParam(NDPluginStatsHistMinString,           asynParamFloat64,       &NDPluginStatsHistMin);
    createParam(NDPluginStatsHistMaxString,           asynParamFloat64,       &NDPluginStatsHistMax);
    createParam(NDPluginStatsHistEntropyString,       asynParamFloat64,       &NDPluginStatsHistEntropy);
    createParam(NDPluginStatsHistArrayString,         asynParamFloat64Array,  &NDPluginStatsHistArray);

    /* ROI profiles - not yet implemented */
    createParam(NDPluginStatsComputeProfilesString,   asynParamInt32, &NDPluginStatsComputeProfiles);

    /* Arrays of total and net counts for MCA or waveform record */   
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
