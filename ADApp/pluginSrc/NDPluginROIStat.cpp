/**
 * NDPluginROIStat.cpp
 *
 * Region of interest plugin that calculates simple statistics
 * on multiple regions. Each ROI is identified by an asyn address
 * (starting at 0). 
 * 
 * @author Matt Pearson 
 * @date Nov 2014
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

#include "NDArray.h"
#include "NDPluginROIStat.h"

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

#define DEFAULT_NUM_TSPOINTS 2048
  
/**
 * Templated function to calculate statistics on different NDArray data types.
 * \param[in] NDArray The pointer to the NDArray object
 * \param[in] NDROI The pointer to the NDROI object
 * \return asynStatus
 */
template <typename epicsType>
asynStatus NDPluginROIStat::doComputeStatisticsT(NDArray *pArray, NDROI *pROI)
{
  epicsType *pData = (epicsType *)pArray->pData;
  double value = 0;
  double bgd = 0;
  size_t sizeX = pROI->size[0];
  size_t sizeY = pROI->size[1];
  size_t offsetX = pROI->offset[0];
  size_t offsetY = pROI->offset[1];
  size_t x = 0;
  size_t y = 0;
  size_t nElements = 0;
  size_t nBgd = 0;
  size_t bgdWidthX = MIN(pROI->bgdWidth, sizeX);
  size_t bgdWidthY = MIN(pROI->bgdWidth, sizeY);
  bool initial = true;
  size_t yOffset = 0;

  pROI->min = 0;
  pROI->max = 0;
  pROI->total = 0;
  pROI->mean = 0;
  pROI->net = 0;

  if (pArray->ndims == 1) {
    nElements = sizeX;
    for (x=offsetX; x<offsetX+sizeX; ++x) {
      value = (double)pData[x];
      if (initial) {
        pROI->min = value;
        pROI->max = value;
        initial = false;
      }  
      if (value < pROI->min) pROI->min = value;
      if (value > pROI->max) pROI->max = value;
      pROI->total += value;
    }
    if (pROI->bgdWidth > 0) {
      for (x=offsetX; x<offsetX+bgdWidthX; ++x) {
        nBgd++;
        bgd += (double)pData[x];
      }
      for (x=offsetX+sizeX-bgdWidthX; x<offsetX+sizeX; ++x) {
        nBgd++;
        bgd += (double)pData[x];
      }
    }    
  } else if (pArray->ndims == 2) {
    nElements = sizeX * sizeY;
    for (y=offsetY; y<offsetY+sizeY; ++y) {
      yOffset = y*pROI->arraySize[0];
      for (x=offsetX; x<offsetX+sizeX; ++x) {
        value = (double)pData[x+yOffset];
        if (initial) {
          pROI->min = value;
          pROI->max = value;
          initial = false;
        } 
        if (value < pROI->min) pROI->min = value;
        if (value > pROI->max) pROI->max = value;
        pROI->total += value;
      }
    }
    if (pROI->bgdWidth > 0) {
      // Compute total counts in the bgdWidthY rows at the top
      for (y=offsetY; y<offsetY+bgdWidthY; ++y) {
        yOffset = y*pROI->arraySize[0];
        for (x=offsetX; x<offsetX+sizeX; ++x) {
          nBgd++;
          bgd += (double)pData[x+yOffset];
        }
      }
      // Compute total counts in the bgdWidthY rows at the bottom
      for (y=offsetY+sizeY-bgdWidthY; y<offsetY+sizeY; ++y) {
        yOffset = y*pROI->arraySize[0];
        for (x=offsetX; x<offsetX+sizeX; ++x) {
          nBgd++;
          bgd += (double)pData[x+yOffset];
        }
      }
      // Compute total counts in the bgdWidthX columns left and right
      for (y=offsetY+bgdWidthY; y<offsetY+sizeY-bgdWidthY; ++y) {
        yOffset = y*pROI->arraySize[0];
        for (x=offsetX; x<offsetX+bgdWidthX; ++x) {
          nBgd++;
          bgd += (double)pData[x+yOffset];
        }
        for (x=offsetX+sizeX-bgdWidthX; x<offsetX+sizeX; ++x) {
          nBgd++;
          bgd += (double)pData[x+yOffset];
        }
      }
    }
  }

  if (nBgd > 0) {
    bgd = bgd/nBgd * nElements;
  }
  pROI->net = pROI->total - bgd;

  if (nElements > 0) {
    pROI->mean = pROI->total / nElements;
  }

  return asynSuccess;

}

     
/**
 * Call the templated doComputeStatistics so we can cast correctly. 
 * \param[in] NDArray The pointer to the NDArray object
 * \param[in] NDROI The pointer to the NDROI object
 * \return asynStatus
 */
asynStatus NDPluginROIStat::doComputeStatistics(NDArray *pArray, NDROI *pROI)
{
  asynStatus status = asynSuccess;
  
  switch(pArray->dataType) {
  case NDInt8:
    status = doComputeStatisticsT<epicsInt8>(pArray, pROI);
    break;
  case NDUInt8:
    status = doComputeStatisticsT<epicsUInt8>(pArray, pROI);
    break;
  case NDInt16:
    status = doComputeStatisticsT<epicsInt16>(pArray, pROI);
    break;
  case NDUInt16:
    status = doComputeStatisticsT<epicsUInt16>(pArray, pROI);
    break;
  case NDInt32:
    status = doComputeStatisticsT<epicsInt32>(pArray, pROI);
    break;
  case NDUInt32:
    status = doComputeStatisticsT<epicsUInt32>(pArray, pROI);
    break;
  case NDFloat32:
    status = doComputeStatisticsT<epicsFloat32>(pArray, pROI);
    break;
  case NDFloat64:
    status = doComputeStatisticsT<epicsFloat64>(pArray, pROI);
    break;
  default:
    return asynError;
    break;
  }
  return status;
}


/** 
 * Callback function that is called by the NDArray driver with new NDArray data.
 * Computes statistics on the ROIs if NDPluginROIStatUse is 1.
 * If NDPluginROIStatNDArrayCallbacks is 1 then it also does NDArray callbacks.
 * \param[in] pArray The NDArray from the callback.
 */
void NDPluginROIStat::processCallbacks(NDArray *pArray)
{

  //This function is called with the mutex already locked.  
  //It unlocks it during long calculations when private structures don't need to be protected.
  
  int use = 0;
  int itemp = 0;
  int dim = 0;
  asynStatus status = asynSuccess;
  NDROI *pROI;
  int TSAcquiring;
  const char* functionName = "NDPluginROIStat::processCallbacks";

  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);

  // This plugin only works with 1-D or 2-D arrays
  if ((pArray->ndims < 1) || (pArray->ndims > 2)) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s: error, number of array dimensions must be 1 or 2\n",
        functionName);
  }

  //Set NDArraySize params to the input pArray, because this plugin doesn't change them
  if (pArray->ndims > 0) setIntegerParam(NDArraySizeX, (int)pArray->dims[0].size);
  if (pArray->ndims > 1) setIntegerParam(NDArraySizeY, (int)pArray->dims[1].size);

  getIntegerParam(NDPluginROIStatTSAcquiring,        &TSAcquiring);

  /* Loop over the ROIs in this driver */
  for (int roi=0; roi<maxROIs_; ++roi) {
    
    pROI = &pROIs_[roi];
    getIntegerParam(roi, NDPluginROIStatUse, &use);
    if (!use) {
      continue;
    }

    /* Need to fetch all of these parameters while we still have the mutex */
    getIntegerParam(roi, NDPluginROIStatDim0Min,      &itemp); pROI->offset[0] = itemp;
    getIntegerParam(roi, NDPluginROIStatDim1Min,      &itemp); pROI->offset[1] = itemp;
    getIntegerParam(roi, NDPluginROIStatDim0Size,     &itemp); pROI->size[0] = itemp;
    getIntegerParam(roi, NDPluginROIStatDim1Size,     &itemp); pROI->size[1] = itemp;
    getIntegerParam(roi, NDPluginROIStatBgdWidth,     &itemp); pROI->bgdWidth = itemp;
    
    for (dim=0; dim<pArray->ndims; dim++) {
      pROI->offset[dim]  = MAX(pROI->offset[dim], 0);
      pROI->offset[dim]  = MIN(pROI->offset[dim], pArray->dims[dim].size-1);
      pROI->size[dim]    = MAX(pROI->size[dim], 1);
      pROI->size[dim]    = MIN(pROI->size[dim], pArray->dims[dim].size - pROI->offset[dim]);
      pROI->arraySize[dim] = (int)pArray->dims[dim].size;
    }
    
    /* Update the parameters that may have changed */
    setIntegerParam(roi, NDPluginROIStatDim0MaxSize, 0);
    setIntegerParam(roi, NDPluginROIStatDim1MaxSize, 0);
    if (pArray->ndims > 0) {
      setIntegerParam(roi, NDPluginROIStatDim0MaxSize, (int)pArray->dims[0].size);
      setIntegerParam(roi, NDPluginROIStatDim0Min,  (int)pROI->offset[0]);
      setIntegerParam(roi, NDPluginROIStatDim0Size, (int)pROI->size[0]);
    }
    if (pArray->ndims > 1) {
      setIntegerParam(roi, NDPluginROIStatDim1MaxSize, (int)pArray->dims[1].size);
      setIntegerParam(roi, NDPluginROIStatDim1Min,  (int)pROI->offset[1]);
      setIntegerParam(roi, NDPluginROIStatDim1Size, (int)pROI->size[1]);
    }
        
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing elements of
     * pPvt that other threads can access. */
    this->unlock();
    
    status = doComputeStatistics(pArray, pROI);
    if (status != asynSuccess) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s: doComputeStatistics failed. status=%d\n", 
        functionName, status);
    }

    if (TSAcquiring) {
      double *pData = timeSeries_ + (roi * MAX_TIME_SERIES_TYPES * numTSPoints_);
      pData[TSMinValue*numTSPoints_ + currentTSPoint_]  = pROI->min;
      pData[TSMaxValue*numTSPoints_ + currentTSPoint_]  = pROI->max;
      pData[TSMeanValue*numTSPoints_ + currentTSPoint_] = pROI->mean;
      pData[TSTotal*numTSPoints_ + currentTSPoint_]     = pROI->total;
      pData[TSNet*numTSPoints_ + currentTSPoint_]       = pROI->net;
      pData[TSTimestamp*numTSPoints_ + currentTSPoint_] = pArray->timeStamp;
    }

    /* We must enter the loop and exit with the mutex locked */
    this->lock();
    setDoubleParam(roi, NDPluginROIStatMinValue,    pROI->min);
    setDoubleParam(roi, NDPluginROIStatMaxValue,    pROI->max);
    setDoubleParam(roi, NDPluginROIStatMeanValue,   pROI->mean);
    setDoubleParam(roi, NDPluginROIStatTotal,       pROI->total);
    setDoubleParam(roi, NDPluginROIStatNet,         pROI->net);
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
          "%s ROI=%d, min=%f, max=%f, mean=%f, total=%f, net=%f\n",
          functionName, roi, pROI->min, pROI->max, pROI->mean, pROI->total, pROI->net);

    callParamCallbacks(roi);
  }

  if (TSAcquiring) {
    currentTSPoint_++;
    setIntegerParam(NDPluginROIStatTSCurrentPoint, currentTSPoint_);
    if (currentTSPoint_ >= numTSPoints_) {
      setIntegerParam(NDPluginROIStatTSAcquiring, 0);
      doTimeSeriesCallbacks();
    }
  }

  int arrayCallbacks = 0;
  getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
  if (arrayCallbacks == 1) {
    NDArray *pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 1);
    if (pArrayOut != NULL) {
      this->getAttributes(pArrayOut->pAttributeList);
      this->unlock();
      doCallbacksGenericPointer(pArrayOut, NDArrayData, 0);
      this->lock();
      pArrayOut->release();
    }
    else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s: Couldn't allocate output array. Callbacks failed.\n", 
        functionName);
    }
  }
  callParamCallbacks();
}

/** Called when asyn clients call pasynInt32->write().
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value The value to write. 
  * \return asynStatus
  */
asynStatus NDPluginROIStat::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    bool stat = true;
    int roi = 0;
    const char* functionName = "NDPluginROIStat::writeInt32";

    status = getAddress(pasynUser, &roi); 
    if (status != asynSuccess) {
      return status;
    }

    /* Set parameter and readback in parameter library */
    stat = (setIntegerParam(roi, function, value) == asynSuccess) && stat;
    
    if (function == NDPluginROIStatReset) {
      stat = (clear(roi) == asynSuccess) && stat;
    } else if (function == NDPluginROIStatResetAll) {
      for (int i=0; i<maxROIs_; ++i) {
        stat = (clear(i) == asynSuccess) && stat;
      }
    } else if (function == NDPluginROIStatTSNumPoints) {
      free(timeSeries_);
      numTSPoints_ = value;
      timeSeries_ = (double *)calloc(MAX_TIME_SERIES_TYPES*maxROIs_*numTSPoints_, sizeof(double));
    } else if (function == NDPluginROIStatTSControl) {
        switch (value) {
          case TSEraseStart:
            currentTSPoint_ = 0;
            setIntegerParam(NDPluginROIStatTSCurrentPoint, currentTSPoint_);
            setIntegerParam(NDPluginROIStatTSAcquiring, 1);
            memset(timeSeries_, 0, maxROIs_*MAX_TIME_SERIES_TYPES*numTSPoints_*sizeof(double));
            break;
          case TSStart:
            if (currentTSPoint_ < numTSPoints_) {
                setIntegerParam(NDPluginROIStatTSAcquiring, 1);
            }
            break;
          case TSStop:
            setIntegerParam(NDPluginROIStatTSAcquiring, 0);
            doTimeSeriesCallbacks();
            break;
          case TSRead:
            doTimeSeriesCallbacks();
            break;
        }
    } else if (function < FIRST_NDPLUGIN_ROISTAT_PARAM) {
      stat = (NDPluginDriver::writeInt32(pasynUser, value) == asynSuccess) && stat;
    }
    
    /* Do callbacks so higher layers see any changes */
    stat = (callParamCallbacks(roi) == asynSuccess) && stat;
    stat = (callParamCallbacks() == asynSuccess) && stat;

    if (!stat) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
              "%s: status=%d, function=%d, value=%d",
              functionName, status, function, value);
    status = asynError;
    } else {
      asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
        "%s: function=%d, roi=%d, value=%d\n",
        functionName, function, roi, value);
    }
    
    return status;
}

/**
 * Reset the data for an ROI.
 * \param[in] roi number
 * \return asynStatus
 */
asynStatus NDPluginROIStat::clear(epicsUInt32 roi)
{
  asynStatus status = asynSuccess;
  bool stat = true;
  const char* functionName = "NDPluginROIStat::clear";
  
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
        "%s: Clearing params. roi=%d\n", functionName, roi);

  stat = (setDoubleParam(roi, NDPluginROIStatMinValue,  0.0) == asynSuccess) && stat;
  stat = (setDoubleParam(roi, NDPluginROIStatMaxValue,  0.0) == asynSuccess) && stat;
  stat = (setDoubleParam(roi, NDPluginROIStatMeanValue, 0.0) == asynSuccess) && stat;
  stat = (setDoubleParam(roi, NDPluginROIStatTotal,     0.0) == asynSuccess) && stat;
  stat = (setDoubleParam(roi, NDPluginROIStatNet,       0.0) == asynSuccess) && stat;

  stat = (callParamCallbacks(roi) == asynSuccess) && stat;

  if (!stat) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
          "%s: Error clearing params. roi=%d\n", functionName, roi);
    status = asynError;
  }

  return status;
}

void NDPluginROIStat::doTimeSeriesCallbacks()
{
  double *pData;
    
  /* Loop over the ROIs in this driver */
  for (int roi=0; roi<maxROIs_; ++roi) {
    pData = timeSeries_ + (roi * MAX_TIME_SERIES_TYPES * numTSPoints_);
    doCallbacksFloat64Array(pData + TSMinValue*numTSPoints_,   currentTSPoint_, NDPluginROIStatTSMinValue, roi);
    doCallbacksFloat64Array(pData + TSMaxValue*numTSPoints_,   currentTSPoint_, NDPluginROIStatTSMaxValue, roi);
    doCallbacksFloat64Array(pData + TSMeanValue*numTSPoints_,  currentTSPoint_, NDPluginROIStatTSMeanValue, roi);
    doCallbacksFloat64Array(pData + TSTotal*numTSPoints_,      currentTSPoint_, NDPluginROIStatTSTotal, roi);
    doCallbacksFloat64Array(pData + TSNet*numTSPoints_,        currentTSPoint_, NDPluginROIStatTSNet, roi);
    doCallbacksFloat64Array(pData + TSTimestamp*numTSPoints_,  currentTSPoint_, NDPluginROIStatTSTimestamp, roi);
  }
}



/** Constructor for NDPluginROIStat; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
NDPluginROIStat::NDPluginROIStat(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, int maxROIs,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
             NDArrayPort, NDArrayAddr, maxROIs, NUM_NDPLUGIN_ROISTAT_PARAMS, maxBuffers, maxMemory,
             asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             asynInt32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynGenericPointerMask,
             ASYN_MULTIDEVICE, 1, priority, stackSize)
{
  const char *functionName = "NDPluginROIStat::NDPluginROIStat";

  if (maxROIs < 1) {
    maxROIs = 1;
  }
  maxROIs_ = maxROIs;
  pROIs_ = new NDROI[maxROIs];
  if(!pROIs_) {cantProceed(functionName);}
  
  /* ROI general parameters */
  createParam(NDPluginROIStatFirstString,             asynParamInt32, &NDPluginROIStatFirst);
  createParam(NDPluginROIStatNameString,              asynParamOctet, &NDPluginROIStatName);
  createParam(NDPluginROIStatUseString,               asynParamInt32, &NDPluginROIStatUse);
  createParam(NDPluginROIStatResetString,             asynParamInt32, &NDPluginROIStatReset);
  createParam(NDPluginROIStatResetAllString,          asynParamInt32, &NDPluginROIStatResetAll);
  createParam(NDPluginROIStatBgdWidthString,          asynParamInt32, &NDPluginROIStatBgdWidth);
  
  /* ROI definition */
  createParam(NDPluginROIStatDim0MinString,           asynParamInt32, &NDPluginROIStatDim0Min);
  createParam(NDPluginROIStatDim0SizeString,          asynParamInt32, &NDPluginROIStatDim0Size);
  createParam(NDPluginROIStatDim0MaxSizeString,       asynParamInt32, &NDPluginROIStatDim0MaxSize);
  createParam(NDPluginROIStatDim1MinString,           asynParamInt32, &NDPluginROIStatDim1Min);
  createParam(NDPluginROIStatDim1SizeString,          asynParamInt32, &NDPluginROIStatDim1Size);
  createParam(NDPluginROIStatDim1MaxSizeString,       asynParamInt32, &NDPluginROIStatDim1MaxSize);
  createParam(NDPluginROIStatDim2MinString,           asynParamInt32, &NDPluginROIStatDim2Min);
  createParam(NDPluginROIStatDim2SizeString,          asynParamInt32, &NDPluginROIStatDim2Size);
  createParam(NDPluginROIStatDim2MaxSizeString,       asynParamInt32, &NDPluginROIStatDim2MaxSize);
  
  /* ROI statistics */
  createParam(NDPluginROIStatMinValueString,          asynParamFloat64, &NDPluginROIStatMinValue);
  createParam(NDPluginROIStatMaxValueString,          asynParamFloat64, &NDPluginROIStatMaxValue);
  createParam(NDPluginROIStatMeanValueString,         asynParamFloat64, &NDPluginROIStatMeanValue);
  createParam(NDPluginROIStatTotalString,             asynParamFloat64, &NDPluginROIStatTotal);
  createParam(NDPluginROIStatNetString,               asynParamFloat64, &NDPluginROIStatNet);

  /* Time series arrays */
  createParam(NDPluginROIStatTSControlString,           asynParamInt32, &NDPluginROIStatTSControl);
  createParam(NDPluginROIStatTSNumPointsString,         asynParamInt32, &NDPluginROIStatTSNumPoints);
  createParam(NDPluginROIStatTSCurrentPointString,      asynParamInt32, &NDPluginROIStatTSCurrentPoint);
  createParam(NDPluginROIStatTSAcquiringString,         asynParamInt32, &NDPluginROIStatTSAcquiring);
  createParam(NDPluginROIStatTSMinValueString,   asynParamFloat64Array, &NDPluginROIStatTSMinValue);
  createParam(NDPluginROIStatTSMaxValueString,   asynParamFloat64Array, &NDPluginROIStatTSMaxValue);
  createParam(NDPluginROIStatTSMeanValueString,  asynParamFloat64Array, &NDPluginROIStatTSMeanValue);
  createParam(NDPluginROIStatTSTotalString,      asynParamFloat64Array, &NDPluginROIStatTSTotal);
  createParam(NDPluginROIStatTSNetString,        asynParamFloat64Array, &NDPluginROIStatTSNet);
  createParam(NDPluginROIStatTSTimestampString,  asynParamFloat64Array, &NDPluginROIStatTSTimestamp);

  createParam(NDPluginROIStatLastString,              asynParamInt32, &NDPluginROIStatLast);
  
  //Note: params set to a default value here will overwrite a default database value

  /* Set the plugin type string */
  setStringParam(NDPluginDriverPluginType, "NDPluginROIStat");
  
  for (int roi=0; roi<maxROIs_; ++roi) {
    
    setIntegerParam(roi , NDPluginROIStatFirst,             0);
    setIntegerParam(roi , NDPluginROIStatLast,              0);

    setStringParam (roi,  NDPluginROIStatName,              "");
    setIntegerParam(roi , NDPluginROIStatUse,               0);
    
    setIntegerParam(roi , NDPluginROIStatDim0Min,           0);
    setIntegerParam(roi , NDPluginROIStatDim0Size,          0);
    setIntegerParam(roi , NDPluginROIStatDim0MaxSize,       0);
    setIntegerParam(roi , NDPluginROIStatDim1Min,           0);
    setIntegerParam(roi , NDPluginROIStatDim1Size,          0);
    setIntegerParam(roi , NDPluginROIStatDim1MaxSize,       0);
    setIntegerParam(roi , NDPluginROIStatDim2Min,           0);
    setIntegerParam(roi , NDPluginROIStatDim2Size,          0);
    setIntegerParam(roi , NDPluginROIStatDim2MaxSize,       0);
    
    setDoubleParam (roi , NDPluginROIStatMinValue,          0.0);
    setDoubleParam (roi , NDPluginROIStatMaxValue,          0.0);
    setDoubleParam (roi , NDPluginROIStatMeanValue,         0.0);
    setDoubleParam (roi , NDPluginROIStatTotal,             0.0);
    setDoubleParam (roi , NDPluginROIStatNet,               0.0);
    callParamCallbacks(roi);
  }

  numTSPoints_ = DEFAULT_NUM_TSPOINTS;
  setIntegerParam(NDPluginROIStatTSNumPoints, numTSPoints_);
  timeSeries_ = (double *)calloc(MAX_TIME_SERIES_TYPES*maxROIs_*numTSPoints_, sizeof(double));
  
  /* Try to connect to the array port */
  connectToArrayPort();

  callParamCallbacks();
  
}

/** Configuration command */
extern "C" int NDROIStatConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr, int maxROIs,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginROIStat *pPlugin =
        new NDPluginROIStat(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxROIs,
                        maxBuffers, maxMemory, priority, stackSize);
    //To take care of compiler warnings
    if (pPlugin) {
      pPlugin = NULL;  
    }
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
static const iocshFuncDef initFuncDef = {"NDROIStatConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDROIStatConfigure(args[0].sval, args[1].ival, args[2].ival,
                   args[3].sval, args[4].ival, args[5].ival,
                   args[6].ival, args[7].ival, args[8].ival,
                   args[9].ival);
}

extern "C" void NDROIStatRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDROIStatRegister);
}
