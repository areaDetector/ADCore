/*
 * NDPluginOverlay.cpp
 *
 * Overlay plugin
 * Author: Mark Rivers
 *
 * Created March 22, 2010
 */

#include <string.h>
#include <math.h>

#include <iocsh.h>
#include <epicsThread.h>

#include <asynDriver.h>

#include <epicsExport.h>

#include "NDPluginOverlayTextFont.h"
#include "NDPluginOverlay.h"

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

static const char *driverName="NDPluginOverlay";

void NDPluginOverlay::addPixel(NDOverlay_t *pOverlay, int ix, int iy, NDArrayInfo_t *pArrayInfo)
{
  if ((ix >= 0) && (ix < (int)pArrayInfo->xSize) &&
      (iy >= 0) && (iy < (int)pArrayInfo->ySize))
    pOverlay->pvt.addressOffset.push_back((int)(iy*pArrayInfo->yStride) + (int)(ix*pArrayInfo->xStride));
}

template <typename epicsType>
void NDPluginOverlay::setPixel(epicsType *pValue, NDOverlay_t *pOverlay, NDArrayInfo_t *pArrayInfo)
{
  if ((pArrayInfo->colorMode == NDColorModeRGB1) ||
      (pArrayInfo->colorMode == NDColorModeRGB2) ||
      (pArrayInfo->colorMode == NDColorModeRGB3)) {
    if (pOverlay->drawMode == NDOverlaySet) {
      *pValue = (epicsType)pOverlay->red;
      pValue += pArrayInfo->colorStride;
      *pValue = (epicsType)pOverlay->green;
      pValue += pArrayInfo->colorStride;
      *pValue = (epicsType)pOverlay->blue;
    } else if (pOverlay->drawMode == NDOverlayXOR) {
      *pValue = (epicsType)((int)*pValue ^ (int)pOverlay->red);
      pValue += pArrayInfo->colorStride;
      *pValue = (epicsType)((int)*pValue ^ (int)pOverlay->green);
      pValue += pArrayInfo->colorStride;
      *pValue = (epicsType)((int)*pValue ^ (int)pOverlay->blue);
    }
  }
  else {
    if (pOverlay->drawMode == NDOverlaySet)
      *pValue = (epicsType)pOverlay->green;
    else if (pOverlay->drawMode == NDOverlayXOR)
      *pValue = (epicsType)((int)*pValue ^ (int)pOverlay->green);
  }
}



template <typename epicsType>
void NDPluginOverlay::doOverlayT(NDArray *pArray, NDOverlay_t *pOverlay, NDArrayInfo_t *pArrayInfo)
{
  int xmin, xmax, ymin, ymax, xcent, ycent, xsize, ysize, ix, iy, ii, jj, ib;
  int xwide, ywide, xwidemax_line, xwidemin_line;
  std::vector<int>::iterator it;
  int nSteps;
  double theta, thetaStep;
  epicsType *pData=(epicsType *)pArray->pData;
  char textOutStr[512];                    // our string, maybe with a time stamp, to place into the image array
  char *cp;                                // character pointer to current character being rendered
  int bmc;                                 // current byte in the font bitmap
  int mask;                                // selects the bit in bmc to look at
  char tstr[64];                           // Used to build the time string
  NDPluginOverlayTextFontBitmapType *bmp;  // pointer to our font information (bitmap pointer, perhaps misnamed)
  int bpc;                                 // bytes per char, ie, 1 for 6x13 font, 2 for 9x15 font
  int sbc;                                 // "sub" byte counter to keep track of which byte we are looking at for multi byte fonts
  //static const char *functionName = "doOverlayT";

  asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
    "NDPluginOverlay::DoOverlayT, shape=%d, Xpos=%d, Ypos=%d, Xsize=%d, Ysize=%d\n",
    pOverlay->shape, (int)pOverlay->PositionX, (int)pOverlay->PositionY,
    (int)pOverlay->SizeX, (int)pOverlay->SizeY);

  if (pOverlay->pvt.changed) {
    pOverlay->pvt.addressOffset.clear();

    switch(pOverlay->shape) {
      case NDOverlayCross:
        xcent = pOverlay->PositionX + pOverlay->SizeX/2;
        ycent = pOverlay->PositionY + pOverlay->SizeY/2;
        xmin = xcent - pOverlay->SizeX/2;
        xmax = xcent + pOverlay->SizeX/2;
        ymin = ycent - pOverlay->SizeY/2;
        ymax = ycent + pOverlay->SizeY/2;
        xwide = pOverlay->WidthX / 2;
        ywide = pOverlay->WidthY / 2;

        for (iy=ymin; iy<=ymax; iy++) {
          if ((iy >= (ycent - ywide)) && (iy <= ycent + ywide)) {
            for (ix=xmin; ix<=xmax; ++ix) {
              addPixel(pOverlay, ix, iy, pArrayInfo);
            }
          } else {
            xwidemin_line = xcent - xwide;
            xwidemax_line = xcent + xwide;
            for (ix=xwidemin_line; ix<=xwidemax_line; ++ix) {
              addPixel(pOverlay, ix, iy, pArrayInfo);
            }
          }
        }
        break;

      case NDOverlayRectangle:
        xmin = pOverlay->PositionX;
        xmax = pOverlay->PositionX + pOverlay->SizeX;
        ymin = pOverlay->PositionY;
        ymax = pOverlay->PositionY + pOverlay->SizeY;
        xwide = pOverlay->WidthX;
        ywide = pOverlay->WidthY;
        xwide = MIN(xwide, (int)pOverlay->SizeX-1);
        ywide = MIN(ywide, (int)pOverlay->SizeY-1);

        //For non-zero width, grow the rectangle towards the center.
        for (iy=ymin; iy<=ymax; iy++) {
          if ((iy < (ymin + ywide)) ||
              (iy > (ymax - ywide))) {
            for (ix=xmin; ix<=xmax; ix++) {
              addPixel(pOverlay, ix, iy, pArrayInfo);
            }
          } else {
            for (ix=xmin; ix<(xmin+xwide); ++ix) {
              addPixel(pOverlay, ix, iy, pArrayInfo);
            }
            for (ix=(xmax-xwide+1); ix<=xmax; ++ix) {
              addPixel(pOverlay, ix, iy, pArrayInfo);
            }
          }
        }
        break;

      case NDOverlayEllipse:
        xwide = pOverlay->WidthX;
        ywide = pOverlay->WidthY;
        xwide = MIN(xwide, (int)pOverlay->SizeX-1);
        ywide = MIN(ywide, (int)pOverlay->SizeY-1);
        xcent = pOverlay->PositionX + pOverlay->SizeX/2;
        ycent = pOverlay->PositionY + pOverlay->SizeY/2;
        xsize = pOverlay->SizeX/2;
        ysize = pOverlay->SizeY/2;
        xmax = (int)(pArrayInfo->xSize-1);
        ymax = (int)(pArrayInfo->ySize-1);

        // Use the parametric equation for an ellipse.
        // Only need to compute 0 to pi/2, other quadrants by symmetry
        // Make 2*(xsize + ysize) angle points
        nSteps = 2*(xsize + ysize);
        thetaStep = M_PI / 2. / nSteps;
        for (ii=0, theta=0.; ii<=nSteps; ii++, theta+=thetaStep) {
          for (jj=0; jj<xwide; jj++) {
            ix = (int)((xsize-jj) * cos(theta) + 0.5);
            iy = (int)((ysize-jj) * sin(theta) + 0.5);
            addPixel(pOverlay, (xcent + ix), (ycent + iy), pArrayInfo);
            addPixel(pOverlay, (xcent + ix), (ycent - iy), pArrayInfo);
            addPixel(pOverlay, (xcent - ix), (ycent + iy), pArrayInfo);
            addPixel(pOverlay, (xcent - ix), (ycent - iy), pArrayInfo);
          }
        }
        // There may be duplicate pixels in the address list.
        // We must remove them or the XOR draw mode won't work because the pixel will be set and then unset
        std::sort(pOverlay->pvt.addressOffset.begin(), pOverlay->pvt.addressOffset.end());
        it = std::unique(pOverlay->pvt.addressOffset.begin(), pOverlay->pvt.addressOffset.end());
        pOverlay->pvt.addressOffset.resize(std::distance(pOverlay->pvt.addressOffset.begin(), it));
        break;

      case NDOverlayText:
        if ((pOverlay->Font >= 0) && (pOverlay->Font < NDPluginOverlayTextFontBitmapTypeN)) {
          bmp = &NDPluginOverlayTextFontBitmaps[pOverlay->Font];
        } else {
          // Really, no reason to go on if the font is ill defined
          return;
        }

        bpc = bmp->width / 8 + 1;

        if (strlen(pOverlay->TimeStampFormat) > 0) {
          epicsTimeToStrftime(tstr, sizeof(tstr)-1, pOverlay->TimeStampFormat, &pArray->epicsTS);
          epicsSnprintf(textOutStr, sizeof(textOutStr)-1, "%s%s", pOverlay->DisplayText, tstr);
        } else {
          epicsSnprintf(textOutStr, sizeof(textOutStr)-1, "%s", pOverlay->DisplayText);
        }
        textOutStr[sizeof(textOutStr)-1] = 0;

        cp   = textOutStr;
        xmin = pOverlay->PositionX;
        xmax = pOverlay->PositionX + pOverlay->SizeX;
        ymin = pOverlay->PositionY;
        ymax = pOverlay->PositionY + pOverlay->SizeY;
        ymax = MIN(ymax, pOverlay->PositionY + bmp->height);

        // Loop over vertical lines
        for (jj=0, iy=ymin; iy<ymax; jj++, iy++) {

          // Loop over characters
          for (ii=0; cp[ii]!=0; ii++) {
            if( cp[ii] < 32)
              continue;

            if (xmin+ii * bmp->width >= xmax)
              // None of this character can be written
              break;

            sbc = 0;
            bmc = bmp->bitmap[(bmp->height*(cp[ii] - 32) + jj)*bpc];
            mask = 0x80;
            for (ib=0; ib<bmp->width; ib++) {
              ix = xmin + ii * bmp->width + ib;
              if (ix >= xmax)
                break;
              if (mask & bmc) {
                addPixel(pOverlay, ix, iy, pArrayInfo);
              }
              mask >>= 1;
              if (!mask) {
                mask = 0x80;
                sbc++;
                bmc = bmp->bitmap[(bmp->height*(cp[ii] - 32) + jj)*bpc + sbc];
              }
            }
          }
        }
        break;
    } // switch(pOverlay->shape)
  } // if (pOverlay->pvt.changed)

  // Set the pixels in the image from the addressOffset vector list
  for (ii=0; ii<(int)pOverlay->pvt.addressOffset.size(); ii++) {
    setPixel(pData + pOverlay->pvt.addressOffset[ii], pOverlay, pArrayInfo);
  }
}

int NDPluginOverlay::doOverlay(NDArray *pArray, NDOverlay_t *pOverlay, NDArrayInfo_t *pArrayInfo)
{
  switch(pArray->dataType) {
    case NDInt8:
      doOverlayT<epicsInt8>(pArray, pOverlay, pArrayInfo);
      break;
    case NDUInt8:
      doOverlayT<epicsUInt8>(pArray, pOverlay, pArrayInfo);
      break;
    case NDInt16:
      doOverlayT<epicsInt16>(pArray, pOverlay, pArrayInfo);
      break;
    case NDUInt16:
      doOverlayT<epicsUInt16>(pArray, pOverlay, pArrayInfo);
      break;
    case NDInt32:
      doOverlayT<epicsInt32>(pArray, pOverlay, pArrayInfo);
      break;
    case NDUInt32:
      doOverlayT<epicsUInt32>(pArray, pOverlay, pArrayInfo);
      break;
    case NDInt64:
      doOverlayT<epicsInt64>(pArray, pOverlay, pArrayInfo);
      break;
    case NDUInt64:
      doOverlayT<epicsUInt64>(pArray, pOverlay, pArrayInfo);
      break;
    case NDFloat32:
      doOverlayT<epicsFloat32>(pArray, pOverlay, pArrayInfo);
      break;
    case NDFloat64:
      doOverlayT<epicsFloat64>(pArray, pOverlay, pArrayInfo);
      break;
    default:
      return(ND_ERROR);
    break;
  }
  return(ND_SUCCESS);
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Draws overlays on top of the array.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginOverlay::processCallbacks(NDArray *pArray)
{
  /* This function draws overlays
   * It is called with the mutex already locked.  It unlocks it during long calculations when private
   * structures don't need to be protected.
   */

  int overlay;
  int itemp;
  NDArray *pOutput;
  NDArrayInfo arrayInfo;
  std::vector<NDOverlay_t>pOverlays;
  NDOverlay_t *pOverlay;
  bool arrayInfoChanged;
  int overlayUserLen = sizeof(*pOverlay) - sizeof(pOverlay->pvt);
  static const char* functionName = "processCallbacks";

  /* Call the base class method */
  NDPluginDriver::beginProcessCallbacks(pArray);

  /* Copy the input array so we can modify it. */
  pOutput = this->pNDArrayPool->copy(pArray, NULL, 1);

  /* Get information about the array needed later */
  pOutput->getInfo(&arrayInfo);
  arrayInfoChanged = (memcmp(&arrayInfo, &this->prevArrayInfo_, sizeof(arrayInfo)) != 0);
  this->prevArrayInfo_ = arrayInfo;
  setIntegerParam(NDPluginOverlayMaxSizeX, (int)arrayInfo.xSize);
  setIntegerParam(NDPluginOverlayMaxSizeY, (int)arrayInfo.ySize);

  /* Copy the previous contents of each overlay */
  pOverlays = this->prevOverlays_;

  /* Loop over the overlays in this driver */
  for (overlay=0; overlay<this->maxOverlays_; overlay++) {
    pOverlay = &pOverlays[overlay];
    getIntegerParam(overlay, NDPluginOverlayUse, &pOverlay->use);
    if (!pOverlay->use) continue;
     /* Need to fetch all of these parameters while we still have the mutex */
    getIntegerParam(overlay, NDPluginOverlayPositionX,  &pOverlay->PositionX);
    getIntegerParam(overlay, NDPluginOverlayPositionY,  &pOverlay->PositionY);
    getIntegerParam(overlay, NDPluginOverlaySizeX,      &pOverlay->SizeX);
    getIntegerParam(overlay, NDPluginOverlaySizeY,      &pOverlay->SizeY);
    getIntegerParam(overlay, NDPluginOverlayWidthX,     &pOverlay->WidthX);
    getIntegerParam(overlay, NDPluginOverlayWidthY,     &pOverlay->WidthY);
    getIntegerParam(overlay, NDPluginOverlayShape,      &itemp); pOverlay->shape = (NDOverlayShape_t)itemp;
    getIntegerParam(overlay, NDPluginOverlayDrawMode,   &itemp); pOverlay->drawMode = (NDOverlayDrawMode_t)itemp;
    getIntegerParam(overlay, NDPluginOverlayRed,        &pOverlay->red);
    getIntegerParam(overlay, NDPluginOverlayGreen,      &pOverlay->green);
    getIntegerParam(overlay, NDPluginOverlayBlue,       &pOverlay->blue);
    getStringParam( overlay, NDPluginOverlayTimeStampFormat, sizeof(pOverlay->TimeStampFormat), pOverlay->TimeStampFormat);
    getIntegerParam(overlay, NDPluginOverlayFont,       &pOverlay->Font);
    getStringParam( overlay, NDPluginOverlayDisplayText, sizeof(pOverlay->DisplayText), pOverlay->DisplayText);

    pOverlay->DisplayText[sizeof(pOverlay->DisplayText)-1] = 0;

    // Compare to see if any fields in the overlay have changed
    pOverlay->pvt.changed = (memcmp(&this->prevOverlays_[overlay], pOverlay, overlayUserLen) != 0);
    if (arrayInfoChanged) pOverlay->pvt.changed = true;
    /* If this is a text overlay with a non-blank time stamp format then it always needs to be updated */
    if ((pOverlay->shape == NDOverlayText) && (strlen(pOverlay->TimeStampFormat) > 0)) {
        pOverlay->pvt.changed = true;
    }
  }
  /* This function is called with the lock taken, and it must be set when we exit.
   * The following code can be exected without the mutex because we are not accessing memory
   * that other threads can access. */
  this->unlock();
  for (overlay=0; overlay<this->maxOverlays_; overlay++) {
    pOverlay = &pOverlays[overlay];
    if (!pOverlay->use) continue;
    this->doOverlay(pOutput, pOverlay, &arrayInfo);
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
      "%s::%s overlay %d, changed=%d, points=%d\n",
      driverName, functionName, overlay, pOverlay->pvt.changed, (int)pOverlay->pvt.addressOffset.size());
  }
  this->lock();
  this->prevOverlays_ = pOverlays;
  NDPluginDriver::endProcessCallbacks(pOutput, false, true);
  callParamCallbacks();
}



/** Constructor for NDPluginOverlay; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * ROI parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *      NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *      at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *      0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *      of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxOverlays The maximum number ofoverlays this plugin supports. 1 is minimum.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *      allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *      allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] maxThreads The maximum number of threads this driver is allowed to use. If 0 then 1 will be used.
  */
NDPluginOverlay::NDPluginOverlay(const char *portName, int queueSize, int blockingCallbacks,
             const char *NDArrayPort, int NDArrayAddr, int maxOverlays,
             int maxBuffers, size_t maxMemory,
             int priority, int stackSize, int maxThreads)
  /* Invoke the base class constructor */
  : NDPluginDriver(portName, queueSize, blockingCallbacks,
           NDArrayPort, NDArrayAddr, maxOverlays, maxBuffers, maxMemory,
           asynGenericPointerMask,
           asynGenericPointerMask,
           ASYN_MULTIDEVICE, 1, priority, stackSize, maxThreads)
{
  //static const char *functionName = "NDPluginOverlay";


  this->maxOverlays_ = maxOverlays;
  this->prevOverlays_.resize(maxOverlays_);

  createParam(NDPluginOverlayMaxSizeXString,        asynParamInt32, &NDPluginOverlayMaxSizeX);
  createParam(NDPluginOverlayMaxSizeYString,        asynParamInt32, &NDPluginOverlayMaxSizeY);
  createParam(NDPluginOverlayNameString,            asynParamOctet, &NDPluginOverlayName);
  createParam(NDPluginOverlayUseString,             asynParamInt32, &NDPluginOverlayUse);
  createParam(NDPluginOverlayPositionXString,       asynParamInt32, &NDPluginOverlayPositionX);
  createParam(NDPluginOverlayPositionYString,       asynParamInt32, &NDPluginOverlayPositionY);
  createParam(NDPluginOverlayCenterXString,         asynParamInt32, &NDPluginOverlayCenterX);
  createParam(NDPluginOverlayCenterYString,         asynParamInt32, &NDPluginOverlayCenterY);
  createParam(NDPluginOverlaySizeXString,           asynParamInt32, &NDPluginOverlaySizeX);
  createParam(NDPluginOverlaySizeYString,           asynParamInt32, &NDPluginOverlaySizeY);
  createParam(NDPluginOverlayWidthXString,          asynParamInt32, &NDPluginOverlayWidthX);
  createParam(NDPluginOverlayWidthYString,          asynParamInt32, &NDPluginOverlayWidthY);
  createParam(NDPluginOverlayShapeString,           asynParamInt32, &NDPluginOverlayShape);
  createParam(NDPluginOverlayDrawModeString,        asynParamInt32, &NDPluginOverlayDrawMode);
  createParam(NDPluginOverlayRedString,             asynParamInt32, &NDPluginOverlayRed);
  createParam(NDPluginOverlayGreenString,           asynParamInt32, &NDPluginOverlayGreen);
  createParam(NDPluginOverlayBlueString,            asynParamInt32, &NDPluginOverlayBlue);
  createParam(NDPluginOverlayTimeStampFormatString, asynParamOctet, &NDPluginOverlayTimeStampFormat);
  createParam(NDPluginOverlayFontString,            asynParamInt32, &NDPluginOverlayFont);
  createParam(NDPluginOverlayDisplayTextString,     asynParamOctet, &NDPluginOverlayDisplayText);

  /* Set the plugin type string */
  setStringParam(NDPluginDriverPluginType, "NDPluginOverlay");

  // Enable ArrayCallbacks.
  // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
  setIntegerParam(NDArrayCallbacks, 1);

  /* Try to connect to the array port */
  connectToArrayPort();
}

/** Called when asyn clients call pasynInt32->write().
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value The value to write.
  * \return asynStatus
  */
asynStatus NDPluginOverlay::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  int addr = 0;
  NDOverlay_t *pOverlay;
  int positionX, positionY, sizeX, sizeY, centerX, centerY;
  static const char* functionName = "writeInt32";

  getAddress(pasynUser, &addr);
  pOverlay = &prevOverlays_[addr];

  /* Set parameter and readback in parameter library */
  setIntegerParam(addr, function, value);

  getIntegerParam(addr, NDPluginOverlayPositionX, &positionX);
  getIntegerParam(addr, NDPluginOverlayPositionY, &positionY);
  getIntegerParam(addr, NDPluginOverlayCenterX,   &centerX);
  getIntegerParam(addr, NDPluginOverlayCenterY,   &centerY);
  getIntegerParam(addr, NDPluginOverlaySizeX,     &sizeX);
  getIntegerParam(addr, NDPluginOverlaySizeY,     &sizeY);

  if (function == NDPluginOverlayCenterX) {
    positionX = value - sizeX/2;
    setIntegerParam(addr, NDPluginOverlayPositionX, positionX);
    pOverlay->pvt.freezePositionX = false;
  } else if (function == NDPluginOverlayCenterY) {
    positionY = value - sizeY/2;
    setIntegerParam(addr, NDPluginOverlayPositionY, positionY);
    pOverlay->pvt.freezePositionY = false;
  } else if (function == NDPluginOverlayPositionX) {
    centerX = value + sizeX/2;
    setIntegerParam(addr, NDPluginOverlayCenterX, centerX);
    pOverlay->pvt.freezePositionX = true;
  } else if (function == NDPluginOverlayPositionY) {
    centerY = value + sizeY/2;
    setIntegerParam(addr, NDPluginOverlayCenterY, centerY);
    pOverlay->pvt.freezePositionY = true;
  } else if (function == NDPluginOverlaySizeX) {
    if (pOverlay->pvt.freezePositionX) {
        centerX = positionX + value/2;
        setIntegerParam(addr, NDPluginOverlayCenterX, centerX);
    } else {
        positionX = centerX - value/2;
        setIntegerParam(addr, NDPluginOverlayPositionX, positionX);
    }
  } else if (function == NDPluginOverlaySizeY) {
    if (pOverlay->pvt.freezePositionY) {
        centerY = positionY + value/2;
        setIntegerParam(addr, NDPluginOverlayCenterY, centerY);
    } else {
        positionY = centerY - value/2;
        setIntegerParam(addr, NDPluginOverlayPositionY, positionY);
    }
  } else if (function < FIRST_NDPLUGIN_OVERLAY_PARAM) {
    NDPluginDriver::writeInt32(pasynUser, value);
  }

  /* Do callbacks so higher layers see any changes */
  callParamCallbacks(addr);

  asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
    "%s::%s function=%d, addr=%d, value=%d\n",
    driverName, functionName, function, addr, value);

  return status;
}


/** Configuration command */
extern "C" int NDOverlayConfigure(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr, int maxOverlays,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize, int maxThreads)
{
  NDPluginOverlay *pPlugin = new NDPluginOverlay(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                                 maxOverlays, maxBuffers, maxMemory, priority, stackSize, maxThreads);
  return pPlugin->start();
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxOverlays",iocshArgInt};
static const iocshArg initArg6 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg7 = { "maxMemory",iocshArgInt};
static const iocshArg initArg8 = { "priority",iocshArgInt};
static const iocshArg initArg9 = { "stackSize",iocshArgInt};
static const iocshArg initArg10 = { "maxThreads",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9,
                                            &initArg10};
static const iocshFuncDef initFuncDef = {"NDOverlayConfigure",11,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDOverlayConfigure(args[0].sval, args[1].ival, args[2].ival,
                     args[3].sval, args[4].ival, args[5].ival,
                     args[6].ival, args[7].ival, args[8].ival,
                     args[9].ival, args[10].ival);
}

extern "C" void NDOverlayRegister(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDOverlayRegister);
}

