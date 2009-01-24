/*
 * NDPluginColorConvert.cpp
 * 
 * Plugin to convert from one color mode to another
 *
 * Author: Mark Rivers
 *
 * Created December 22, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsMutex.h>
#include <epicsString.h>

#include "NDPluginColorConvert.h"
#include "drvNDColorConvert.h"
#include "PvApi.h"

static asynParamString_t NDPluginColorConvertParamString[] = {
    {NDPluginColorConvertColorModeOut,   "COLOR_MODE_OUT"},
};

static const char *driverName="NDPluginColorConvert";

/* This function returns 1 if it did a conversion, 0 if it did not */
template <typename epicsType>
void NDPluginColorConvert::convertColor(NDArray *pArray)
{
    NDColorMode_t colorModeOut;
    const char* functionName = "convertColor";
    int i, j;
    epicsType *pIn, *pRedIn, *pGreenIn, *pBlueIn;
    epicsType *pOut, *pRedOut, *pGreenOut, *pBlueOut;
    epicsType *pDataIn  = (epicsType *)pArray->pData;
    epicsType *pDataOut;
    NDArray *pArrayOut=NULL;
    int imageSize, rowSize, numRows;
    tPvFrame PvFrame, *pFrame=&PvFrame;
    int dims[ND_ARRAY_MAX_DIMS];
    int ndims;
     
    getIntegerParam(NDPluginColorConvertColorModeOut, (int *)&colorModeOut);
       
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing elements of
     * pPvt that other threads can access. */
    epicsMutexUnlock(this->mutexId);
    switch (pArray->colorMode) {
        case NDColorModeBayer:
            switch (colorModeOut) {
                case NDColorModeRGB1:
                case NDColorModeRGB2:
                case NDColorModeRGB3:
                    rowSize   = pArray->dims[0].size;
                    numRows   = pArray->dims[1].size;
                    imageSize = rowSize * numRows;
                    ndims = 3;
                    dims[0] = 3;
                    dims[1] = rowSize;
                    dims[2] = numRows;
                    /* There is a problem: the uniqueId and timeStamp are not preserved! */
                    pArrayOut = this->pNDArrayPool->alloc(ndims, dims, pArray->dataType, 0, NULL);
                    pArrayOut->uniqueId = pArray->uniqueId;
                    pArrayOut->timeStamp = pArray->timeStamp;
                    pDataOut = (epicsType *)pArrayOut->pData;
                    /* For now we use the Prosilica library functions to convert Bayer to RGB */
                    /* This requires creating their tPvFrame data structure */
                    memset(pFrame, 0, sizeof(tPvFrame));
                    pFrame->Width = pArray->dims[0].size;
                    pFrame->Height = pArray->dims[1].size;
                    pFrame->RegionX = pArray->dims[0].offset;
                    pFrame->RegionY = pArray->dims[1].offset;
                    pFrame->ImageBuffer = pArray->pData;
                    pFrame->ImageBufferSize = pArray->dataSize;
                    pFrame->ImageSize = pFrame->ImageBufferSize;
                    pFrame->BayerPattern = (tPvBayerPattern)pArray->bayerPattern;
                    switch(pArray->dataType) {
                        case NDInt8:
                        case NDUInt8:
                            pFrame->Format = ePvFmtBayer8;
                            pFrame->BitDepth = 8;
                            break;
                        case NDInt16:
                        case NDUInt16:
                            pFrame->Format = ePvFmtBayer16;
                            pFrame->BitDepth = 16;
                            break;
                        default:
                            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                                "%s:%s: error unsupported data type=%d\n",
                                driverName, functionName, pArray->dataType);
                            break;
                    }
                    break;
                default: 
                    break;
            }
            switch (colorModeOut) {
                case NDColorModeRGB1:
                    PvUtilityColorInterpolate(pFrame, pDataOut,  pDataOut+1,  pDataOut+2, 2, 0);
                    pArrayOut->dims[0].size = 3;
                    memcpy(&pArrayOut->dims[1], &pArray->dims[0], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[2], &pArray->dims[1], sizeof(NDDimension_t));
                    pArrayOut->colorMode = NDColorModeRGB1;
                    break;
                
                case NDColorModeRGB2:
                    PvUtilityColorInterpolate(pFrame, pDataOut,  pDataOut+rowSize,  pDataOut+2*rowSize, 0, 2*rowSize);
                    memcpy(&pArrayOut->dims[0], &pArray->dims[0], sizeof(NDDimension_t));
                    pArrayOut->dims[1].size = 3;
                    memcpy(&pArrayOut->dims[2], &pArray->dims[1], sizeof(NDDimension_t));
                    pArrayOut->colorMode = NDColorModeRGB2;
                    break;

                case NDColorModeRGB3:
                    PvUtilityColorInterpolate(pFrame, pDataOut,  pDataOut+imageSize,  pDataOut+2*imageSize, 0, 0);
                    memcpy(&pArrayOut->dims[0], &pArray->dims[0], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[1], &pArray->dims[1], sizeof(NDDimension_t));
                    pArrayOut->dims[2].size = 3;
                    pArrayOut->colorMode = NDColorModeRGB3;
                    break;
                default:
                    break;
            }
            break;
        case NDColorModeRGB1:
            rowSize   = pArray->dims[1].size;
            numRows   = pArray->dims[2].size;
            imageSize = rowSize * numRows;
            switch (colorModeOut) {
                case NDColorModeRGB3:
                    pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 0);
                    pDataOut = (epicsType *)pArrayOut->pData;
                    pIn = pDataIn;
                    pRedOut   = pDataOut;
                    pGreenOut = pDataOut + imageSize;
                    pBlueOut  = pDataOut + 2*imageSize;
                    for (i=0; i<imageSize; i++) {
                        *pRedOut++   = *pIn++;
                        *pGreenOut++ = *pIn++;
                        *pBlueOut++  = *pIn++;
                    }
                    memcpy(&pArrayOut->dims[0], &pArray->dims[1], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[1], &pArray->dims[2], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[2], &pArray->dims[0], sizeof(NDDimension_t));
                    pArrayOut->colorMode = NDColorModeRGB3;
                    break;
                case NDColorModeRGB2:
                    pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 0);
                    pDataOut = (epicsType *)pArrayOut->pData;
                    pIn = pDataIn;
                    for (i=0; i<numRows; i++) {
                        pRedOut   = pDataOut + 3*i*rowSize;
                        pGreenOut = pRedOut + rowSize;
                        pBlueOut  = pRedOut + 2*rowSize;
                        for (j=0; j<rowSize; j++) {
                            *pRedOut++   = *pIn++;
                            *pGreenOut++ = *pIn++;
                            *pBlueOut++  = *pIn++;
                        }
                    }
                    memcpy(&pArrayOut->dims[0], &pArray->dims[1], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[1], &pArray->dims[0], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[2], &pArray->dims[2], sizeof(NDDimension_t));
                    pArrayOut->colorMode = NDColorModeRGB2;
                    break;
                default:
                    break;
            }        
            break;
        case NDColorModeRGB2:
            rowSize   = pArray->dims[0].size;
            numRows   = pArray->dims[2].size;
            imageSize = rowSize * numRows;
            switch (colorModeOut) {
                case NDColorModeRGB1:
                    pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 0);
                    pDataOut = (epicsType *)pArrayOut->pData;
                    pOut = pDataOut;
                    for (i=0; i<numRows; i++) {
                        pRedIn   = pDataIn + 3*i*rowSize;
                        pGreenIn = pRedIn + rowSize;
                        pBlueIn  = pRedIn + 2*rowSize;
                        for (j=0; j<rowSize; j++) {
                            *pOut++  = *pRedIn++;
                            *pOut++  = *pGreenIn++;
                            *pOut++  = *pBlueIn++;
                        }
                    }
                    memcpy(&pArrayOut->dims[0], &pArray->dims[1], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[1], &pArray->dims[0], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[2], &pArray->dims[2], sizeof(NDDimension_t));
                    pArrayOut->colorMode = NDColorModeRGB1;
                    break;
                case NDColorModeRGB3:
                    pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 0);
                    pDataOut = (epicsType *)pArrayOut->pData;
                    pRedOut   = pDataOut;
                    pGreenOut = pDataOut + imageSize;
                    pBlueOut  = pDataOut + 2*imageSize;
                    for (i=0; i<numRows; i++) {
                        pRedIn   = pDataIn + 3*i*rowSize;
                        pGreenIn = pRedIn + rowSize;
                        pBlueIn  = pRedIn + 2*rowSize;
                        for (j=0; j<rowSize; j++) {
                            *pRedOut++   = *pRedIn++;
                            *pGreenOut++ = *pGreenIn++;
                            *pBlueOut++  = *pBlueIn++;
                        }
                    }
                    memcpy(&pArrayOut->dims[0], &pArray->dims[0], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[1], &pArray->dims[2], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[2], &pArray->dims[1], sizeof(NDDimension_t));
                    pArrayOut->colorMode = NDColorModeRGB3;
                    break;
                default:
                    break;
            }        
            break;
        case NDColorModeRGB3:
            rowSize   = pArray->dims[0].size;
            numRows   = pArray->dims[1].size;
            imageSize = rowSize * numRows;
            switch (colorModeOut) {
                case NDColorModeRGB1:
                    pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 0);
                    pDataOut = (epicsType *)pArrayOut->pData;
                    pRedIn   = pDataIn;
                    pGreenIn = pDataIn + imageSize;
                    pBlueIn  = pDataIn + 2*imageSize;
                    pOut = pDataOut;
                    for (i=0; i<imageSize; i++) {
                        *pOut++ = *pRedIn++;
                        *pOut++ = *pGreenIn++;
                        *pOut++ = *pBlueIn++;
                    }
                    memcpy(&pArrayOut->dims[0], &pArray->dims[2], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[1], &pArray->dims[0], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[2], &pArray->dims[1], sizeof(NDDimension_t));
                    pArrayOut->colorMode = NDColorModeRGB1;                
                    break;
                case NDColorModeRGB2:
                    pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 0);
                    pDataOut = (epicsType *)pArrayOut->pData;
                    pRedIn   = pDataIn;
                    pGreenIn = pDataIn + imageSize;
                    pBlueIn  = pDataIn + 2*imageSize;
                    for (i=0; i<numRows; i++) {
                        pRedOut   = pDataOut + 3*i*rowSize;
                        pGreenOut = pRedOut + rowSize;
                        pBlueOut  = pRedOut + 2*rowSize;
                        for (j=0; j<rowSize; j++) {
                            *pRedOut++   = *pRedIn++;
                            *pGreenOut++ = *pGreenIn++;
                            *pBlueOut++  = *pBlueIn++;
                        }
                    }
                    memcpy(&pArrayOut->dims[0], &pArray->dims[0], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[1], &pArray->dims[2], sizeof(NDDimension_t));
                    memcpy(&pArrayOut->dims[2], &pArray->dims[1], sizeof(NDDimension_t));
                    pArrayOut->colorMode = NDColorModeRGB2;                
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    /* If the output array pointer is null then no conversion was done, copy the input to the output */
    if (!pArrayOut) pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 1);
    this->pArrays[0] = pArrayOut;
    epicsMutexLock(this->mutexId);
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
              "%s:%s: pArray->colorMode=%d, colorModeOut=%d, pArrayOut=%p\n",
              driverName, functionName, pArray->colorMode, colorModeOut, pArrayOut);
}

void NDPluginColorConvert::processCallbacks(NDArray *pArray)
{
    /* This function converts the color mode.
     * If no conversion can be performed it simply uses the input as the output
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
     
    const char* functionName = "processCallbacks";
     
    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    /* We always keep the last array so read() can use it.  
     * Release previous one. Reserve new one below. */
    if (this->pArrays[0]) {
        this->pArrays[0]->release();
        this->pArrays[0] = NULL;
    }

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
              "%s:%s: dataType=%d\n",
              driverName, functionName, pArray->dataType);

    switch (pArray->dataType) {
        case NDInt8:
            this->convertColor<epicsInt8>(pArray);
            break;
        case NDUInt8:
            this->convertColor<epicsUInt8>(pArray);
            break;
        case NDInt16:
            this->convertColor<epicsInt16>(pArray);
            break;
        case NDUInt16:
            this->convertColor<epicsUInt16>(pArray);
            break;
       case NDInt32:
            this->convertColor<epicsInt32>(pArray);
            break;
        case NDUInt32:
            this->convertColor<epicsUInt32>(pArray);
            break;
        case NDFloat32:
            this->convertColor<epicsFloat32>(pArray);
            break;
        case NDFloat64:
            this->convertColor<epicsFloat64>(pArray);
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                      "%s:%s: ERROR: unknown data type=%d\n",
                      driverName, functionName, pArray->dataType);
            break;
    }
    
    callParamCallbacks();
    /* Call any clients who have registered for NDArray callbacks */
    doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);
}




/* asynDrvUser interface methods */
asynStatus NDPluginColorConvert::drvUserCreate(asynUser *pasynUser,
                                               const char *drvInfo, 
                                               const char **pptypeName, size_t *psize)
{
    asynStatus status;
    int param;
    static const char *functionName = "drvUserCreate";

    /* Look in the driver table */
    status = findParam(NDPluginColorConvertParamString, NUM_COLOR_CONVERT_PARAMS, 
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
                  "%s:%s:, drvInfo=%s, param=%d\n", 
                  driverName, functionName, drvInfo, param);
        return(asynSuccess);
    }

    /* If we did not find it in that table try the plugin base */
    status = NDPluginDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return(status);
}

    

extern "C" int drvNDColorConvertConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                                          const char *NDArrayPort, int NDArrayAddr, 
                                          int maxBuffers, size_t maxMemory)
{  
    new NDPluginColorConvert(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, 
                             maxBuffers, maxMemory);
    return(asynSuccess);
}

NDPluginColorConvert::NDPluginColorConvert(const char *portName, int queueSize, int blockingCallbacks, 
                                           const char *NDArrayPort, int NDArrayAddr, 
                                           int maxBuffersIn, size_t maxMemory)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, 1, NDPluginColorConvertLastParam, maxBuffersIn, maxMemory,
                   asynGenericPointerMask, 
                   asynGenericPointerMask)
{
    asynStatus status;
    const char *functionName = "NDPluginColorConvert";


    status = setIntegerParam(0, NDPluginColorConvertColorModeOut,       NDColorModeMono);
    if (!status)  printf("%s:%s: failed to set integer param for color mode\n",
                         driverName, functionName);

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

