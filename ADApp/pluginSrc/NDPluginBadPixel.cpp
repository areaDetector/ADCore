/*
 * NDPluginBadPixel.cpp
 *
 * Bad pixel processing plugin
 * Author: Mark Rivers
 *
 * Created April 20, 2021
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <json.hpp>
using nlohmann::json;

#include <iocsh.h>

#include "NDPluginBadPixel.h"

#include <epicsExport.h>

static const char *driverName="NDPluginBadPixel";

/** Constructor for NDPluginBadPixel; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
NDPluginBadPixel::NDPluginBadPixel(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int maxBuffers, size_t maxMemory,
                                   int priority, int stackSize, int maxThreads)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                     NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
                     asynOctetMask | asynGenericPointerMask,
                     asynOctetMask | asynGenericPointerMask,
                     0, 1, priority, stackSize, maxThreads)
{ 
    //static const char *functionName = "NDPluginBadPixel";

    /* Background array subtraction */
    createParam(NDPluginBadPixelFileNameString, asynParamOctet, &NDPluginBadPixelFileName);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginBadPixel");

    // Enable ArrayCallbacks.
    // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
    setIntegerParam(NDArrayCallbacks, 1);

    /* Try to connect to the array port */
    connectToArrayPort();
}

epicsInt64 NDPluginBadPixel::computePixelOffset(pixelCoordinate coord, badPixDimInfo_t& dimInfo, NDArrayInfo_t *pArrayInfo)
{
    // This function should return -1 if either the X or Y coordinate is out of range
    // It should be enhanced to deal with the following detector readout settings.
    // Non-zero offset in X or Y dimension offset, i.e. reading out only part of the detector
    // Binning in X or Y dimension
    // Reversal of pixels in X or Y dimension
    epicsInt64 offset = -1;
    epicsInt64 x = (coord.x - dimInfo.offsetX)/dimInfo.binX;
    epicsInt64 y = (coord.y - dimInfo.offsetY)/dimInfo.binY;
    
    if ((x >= 0) &&
        (y >= 0) &&
        (x < dimInfo.sizeX) &&
        (y < dimInfo.sizeY))
    {
        offset = y * pArrayInfo->xSize + x;
    }
    return offset;
}
template <typename epicsType>
void NDPluginBadPixel::fixBadPixelsT(NDArray *pArray, badPixelList_t &badPixels, NDArrayInfo_t *pArrayInfo)
{
    epicsType *pData=(epicsType *)pArray->pData;

    badPixDimInfo_t dimInfo;
    dimInfo.sizeX = pArrayInfo->xSize;
    dimInfo.offsetX = pArray->dims[pArrayInfo->xDim].offset;
    dimInfo.binX = pArray->dims[pArrayInfo->xDim].binning;
    if (pArray->ndims > 1) {
        dimInfo.sizeY = pArrayInfo->ySize;
        dimInfo.offsetY = pArray->dims[pArrayInfo->yDim].offset;
        dimInfo.binY = pArray->dims[pArrayInfo->yDim].binning;
    } else {
        dimInfo.sizeY = 1;
        dimInfo.offsetY = 0;
        dimInfo.binY = 1;
    }
    int scaleX = dimInfo.binX;
    int scaleY = dimInfo.binY;

    for (auto bp : badPixels) {
        epicsInt64 offset = computePixelOffset(bp.coordinate, dimInfo, pArrayInfo);
        if (offset < 0) continue;
        switch (bp.mode) {
          case badPixelModeSet:
            pData[offset]=(epicsType)bp.setValue;
            break;

          case badPixelModeReplace: {
            pixelCoordinate coord = {bp.coordinate.x + bp.replaceCoordinate.x*scaleX, 
                                     bp.coordinate.y + bp.replaceCoordinate.y*scaleY};
            badPixel dummy(coord);
            if (badPixels.find(dummy) != badPixels.end()) {
                asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "replacement pixel [%d,%d] is also bad\n", (int)coord.x, (int)coord.y);
                continue;
            }
            epicsInt64 replaceOffset = computePixelOffset(coord, dimInfo, pArrayInfo);
            if (replaceOffset < 0) continue;
            pData[offset] = pData[replaceOffset];
            break; }
          
          case badPixelModeMedian: {
            std::vector<double> medianValues;
            pixelCoordinate coord;
            epicsInt64 medianOffset;
            for (int i=-bp.medianCoordinate.y; i<=bp.medianCoordinate.y; i++) {
                coord.y = bp.coordinate.y + i*scaleY;
                for (int j=-bp.medianCoordinate.x; j<=bp.medianCoordinate.x; j++) {
                    if ((i==0) && (j==0)) continue;
                    coord.x = bp.coordinate.x + j*scaleX;
                    badPixel dummy(coord);
                    if (badPixels.find(dummy) != badPixels.end()) {
                        asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "replacement pixel [%d,%d] is also bad\n", (int)coord.x, (int)coord.y);
                        continue;
                    }
                    medianOffset = computePixelOffset(coord, dimInfo, pArrayInfo);
                    if (medianOffset < 0) continue;
                    medianValues.push_back(pData[medianOffset]);
                }
            }
            size_t numValues = medianValues.size();
            if (numValues > 0 ) {
                double replaceValue;
                size_t middle = numValues/2;
                std::sort(medianValues.begin(), medianValues.end());
                if ((numValues % 2) == 0) {
                    replaceValue = (medianValues[middle-1] + medianValues[middle]) / 2.;
                } else {
                    replaceValue = medianValues[middle];
                }
                pData[offset] = replaceValue;
            }
            break; }
        }
    }
}

int NDPluginBadPixel::fixBadPixels(NDArray *pArray, badPixelList_t &badPixels, NDArrayInfo_t *pArrayInfo)
{
    switch(pArray->dataType) {
      case NDInt8:
        fixBadPixelsT<epicsInt8>(pArray, badPixels, pArrayInfo);
        break;
      case NDUInt8:
        fixBadPixelsT<epicsUInt8>(pArray, badPixels, pArrayInfo);
        break;
      case NDInt16:
        fixBadPixelsT<epicsInt16>(pArray, badPixels, pArrayInfo);
        break;
      case NDUInt16:
        fixBadPixelsT<epicsUInt16>(pArray, badPixels, pArrayInfo);
        break;
      case NDInt32:
        fixBadPixelsT<epicsInt32>(pArray, badPixels, pArrayInfo);
        break;
      case NDUInt32:
        fixBadPixelsT<epicsUInt32>(pArray, badPixels, pArrayInfo);
        break;
      case NDInt64:
        fixBadPixelsT<epicsInt64>(pArray, badPixels, pArrayInfo);
        break;
      case NDUInt64:
        fixBadPixelsT<epicsUInt64>(pArray, badPixels, pArrayInfo);
        break;
      case NDFloat32:
        fixBadPixelsT<epicsFloat32>(pArray, badPixels, pArrayInfo);
        break;
      case NDFloat64:
        fixBadPixelsT<epicsFloat64>(pArray, badPixels, pArrayInfo);
        break;
      default:
        return(ND_ERROR);
      break;
    }
    return(ND_SUCCESS);
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Does bad pixel processing.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginBadPixel::processCallbacks(NDArray *pArray)
{
    /* This function does array processing.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
    NDArray *pArrayOut = NULL;
    static const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);
    
    auto badPixels = this->badPixelList;
    NDArrayInfo arrayInfo;
    pArray->getInfo(&arrayInfo);

    /* Release the lock now that we are only doing things that don't involve memory other thread
     * cannot access */
    this->unlock();

    /* Make a copy of the array because we cannot modify the input array */
    pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 1);
    if (NULL == pArrayOut) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s Processing aborted; cannot allocate an NDArray for storage of temporary data.\n",
            driverName, functionName);
        goto doCallbacks;
    }
    fixBadPixels(pArrayOut, badPixels, &arrayInfo);

    doCallbacks:
    /* We must exit with the mutex locked */
    this->lock();

    if ((NULL != pArrayOut)) {
        NDPluginDriver::endProcessCallbacks(pArrayOut, false, true);
    }

    callParamCallbacks();
}

asynStatus NDPluginBadPixel::readBadPixelFile(const char *fileName)
{
    json j;
    static const char *functionName = "readBadPixelFile";
    try {
        std::ifstream file(fileName);
        file >> j;
        auto badPixels = j["Bad pixels"];
        badPixelList.clear();
        pixelCoordinate coord;
        for (auto pixel : badPixels) {
            coord.x = pixel["Pixel"][0];
            coord.y = pixel["Pixel"][1];
            badPixel bp(coord);
            if (pixel.find("Median") != pixel.end()) {
                bp.mode = badPixelModeMedian;
                bp.medianCoordinate.x = pixel["Median"][0];
                bp.medianCoordinate.y = pixel["Median"][1];
             }
            if (pixel.find("Set") != pixel.end()) {
                bp.mode = badPixelModeSet;
                bp.setValue = pixel["Set"];
            }
            if (pixel.find("Replace") != pixel.end()) {
                bp.mode = badPixelModeReplace;
                bp.replaceCoordinate.x = pixel["Replace"][0];
                bp.replaceCoordinate.y = pixel["Replace"][1];
            }
            badPixelList.insert(bp);
        }
    }
    catch (const json::parse_error& e) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s::%s JSON error parsing bad pixel file: %s\n", driverName, functionName, e.what());
        return asynError;
    }
    catch (std::exception e) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s::%s other error parsing bad pixel file: %s\n", driverName, functionName, e.what());
        return asynError;
    }
    return asynSuccess;
}

/** Called when asyn clients call pasynOctet->write().
  * This function performs actions for some parameters, including AttributesFile.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus NDPluginBadPixel::writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual)
{
  int addr=0;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  const char *functionName = "writeOctet";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
  // Set the parameter in the parameter library.
  status = (asynStatus)setStringParam(addr, function, (char *)value);
  if (status != asynSuccess) return(status);

  if (function == NDPluginBadPixelFileName) {
      if ((nChars > 0) && (value[0] != 0)) {
          status = this->readBadPixelFile(value);
      }
  }

  else if (function < FIRST_NDPLUGIN_BAD_PIXEL_PARAM) {
      /* If this parameter belongs to a base class call its method */
      status = NDPluginDriver::writeOctet(pasynUser, value, nChars, nActual);
  }

  // Do callbacks so higher layers see any changes
  callParamCallbacks(addr);

  if (status){
      epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                    "%s:%s: status=%d, function=%d, value=%s",
                    driverName, functionName, status, function, value);
  } else {
      asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                "%s:%s: function=%d, value=%s\n",
                driverName, functionName, function, value);
  }
  *nActual = nChars;
  return status;
}

void NDPluginBadPixel::report(FILE *fp, int details)
{
  if (details > 0) {
      int i=0;
      for (auto bp : badPixelList) {
          fprintf(fp, "Bad pixel %d, coords=[%d,%d], mode=", (int)i, (int)bp.coordinate.x, (int)bp.coordinate.y);
          switch (bp.mode) {
            case badPixelModeSet:
              fprintf(fp, "Set, value=%f\n", bp.setValue);
              break;
            case badPixelModeMedian:
              fprintf(fp, "Median, size=[%d,%d]\n", (int)bp.medianCoordinate.x, (int)bp.medianCoordinate.y);
              break;
            case badPixelModeReplace:
              fprintf(fp, "Replace, relative coordinates=[%d,%d]\n", (int)bp.replaceCoordinate.x, (int)bp.replaceCoordinate.y);
              break;
          }
          i++;
      }
  }
  // Call the base class report
  NDPluginDriver::report(fp, details);
}

/** Configuration command */
extern "C" int NDBadPixelConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int maxBuffers, size_t maxMemory,
                                   int priority, int stackSize, int maxThreads)
{
    NDPluginBadPixel *pPlugin = new NDPluginBadPixel(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
                                            &initArg9,
                                            &initArg9};
static const iocshFuncDef initFuncDef = {"NDBadPixelConfigure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDBadPixelConfigure(args[0].sval, args[1].ival, args[2].ival,
                        args[3].sval, args[4].ival, args[5].ival,
                        args[6].ival, args[7].ival, args[8].ival,
                        args[9].ival);
}

extern "C" void NDBadPixelRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDBadPixelRegister);
}
