/*
 * NDPluginOverlay.cpp
 *
 * Overlay plugin
 * Author: Mark Rivers
 *
 * Created March 22, 2010
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginOverlayTextFont.h"
#include "NDPluginOverlay.h"

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

//static const char *driverName="NDPluginOverlay";

template <typename epicsType>
void NDPluginOverlay::setPixel(epicsType *pValue, NDOverlay_t *pOverlay)
{
    if ((this->arrayInfo.colorMode == NDColorModeRGB1) ||
        (this->arrayInfo.colorMode == NDColorModeRGB2) ||
        (this->arrayInfo.colorMode == NDColorModeRGB3)) {
        if (pOverlay->drawMode == NDOverlaySet) {
            *pValue = (epicsType)pOverlay->red;
            pValue += this->arrayInfo.colorStride;
            *pValue = (epicsType)pOverlay->green;
            pValue += this->arrayInfo.colorStride;
            *pValue = (epicsType)pOverlay->blue;
        } else if (pOverlay->drawMode == NDOverlayXOR) {
            *pValue = (epicsType)((int)*pValue ^ (int)pOverlay->red);
            pValue += this->arrayInfo.colorStride;
            *pValue = (epicsType)((int)*pValue ^ (int)pOverlay->green);
            pValue += this->arrayInfo.colorStride;
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
void NDPluginOverlay::doOverlayT(NDArray *pArray, NDOverlay_t *pOverlay)
{
    size_t xmin, xmax, ymin, ymax, ix, iy, ii, jj, ib;
    size_t xwide, ywide, xwidemax_line, xwidemin_line;
    epicsType *pRow;
    char textOutStr[512];                    // our string, maybe with a time stamp, to place into the image array
    char *cp;                                // character pointer to current character being rendered
    int bmc;                                 // current byte in the font bitmap
    int mask;                                // selects the bit in bmc to look at
    char tstr[64];                           // Used to build the time string
    NDPluginOverlayTextFontBitmapType *bmp;  // pointer to our font information (bitmap pointer, perhaps misnamed)
    int bpc;                                 // bytes per char, ie, 1 for 6x13 font, 2 for 9x15 font
    int sbc;                                 // "sub" byte counter to keep track of which byte we are looking at for multi byte fonts
    
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
        "NDPluginOverlay::DoOverlayT, shape=%d, Xpos=%ld, Ypos=%ld, Xsize=%ld, Ysize=%ld\n",
        pOverlay->shape, (long)pOverlay->PositionX, (long)pOverlay->PositionY, 
        (long)pOverlay->SizeX, (long)pOverlay->SizeY);

    switch(pOverlay->shape) {
        case NDOverlayCross:
            xmin = 0;
            if (pOverlay->PositionX > pOverlay->SizeX)
                xmin = pOverlay->PositionX - pOverlay->SizeX;
            xmax = pOverlay->PositionX + pOverlay->SizeX;
            xmax = MIN(xmax, this->arrayInfo.xSize-1);
            ymin = 0;
            if (pOverlay->PositionY > pOverlay->SizeY)
                ymin = pOverlay->PositionY - pOverlay->SizeY;
            ymax = pOverlay->PositionY + pOverlay->SizeY;
            ymax = MIN(ymax, this->arrayInfo.ySize-1);
            xwide = (pOverlay->WidthX == 1) ? 0 : pOverlay->WidthX / 2;
            ywide = (pOverlay->WidthY == 1) ? 0 : pOverlay->WidthY / 2;
            xwide = MIN(xwide, pOverlay->SizeX-1);
            ywide = MIN(ywide, pOverlay->SizeY);

            for (iy=ymin; iy<ymax; iy++) {
                pRow = (epicsType *)pArray->pData + iy*this->arrayInfo.yStride;
                if ((iy >= (pOverlay->PositionY - ywide)) && (iy <= (pOverlay->PositionY + ywide))) {
                    for (ix=xmin; ix<xmax; ++ix) {
                        setPixel(&pRow[ix*this->arrayInfo.xStride], pOverlay);
                    }
                } else {
                    xwidemin_line = pOverlay->PositionX - xwide;
                    xwidemax_line = pOverlay->PositionX + xwide;
                    for (size_t line=xwidemin_line; line<=xwidemax_line; ++line) {
                        setPixel(&pRow[line*this->arrayInfo.xStride], pOverlay);
                    }
                }
            }
            break;

        case NDOverlayRectangle:
            xmin = pOverlay->PositionX;
            xmin = MAX(xmin, 0);
            xmax = pOverlay->PositionX + pOverlay->SizeX;
            xmax = MIN(xmax, this->arrayInfo.xSize);
            ymin = pOverlay->PositionY;
            ymin = MAX(ymin, 0);
            ymax = pOverlay->PositionY + pOverlay->SizeY;
            ymax = MIN(ymax, this->arrayInfo.ySize);
            xwide = (pOverlay->WidthX == 1) ? 0 : pOverlay->WidthX / 2;
            ywide = (pOverlay->WidthY == 1) ? 0 : pOverlay->WidthY / 2;
            xwide = MIN(xwide, pOverlay->SizeX-1);
            ywide = MIN(ywide, pOverlay->SizeY);

            //For non-zero width, grow the rectangle towards the center.
            for (iy=ymin; iy<ymax; iy++) {
                pRow = (epicsType *)pArray->pData + iy*arrayInfo.yStride;
                if ((iy >= ymin) && (iy <= (ymin + ywide))) {
                    for (ix=xmin; ix<xmax; ix++) setPixel(&pRow[ix*this->arrayInfo.xStride], pOverlay);
                } else if ((iy >= (ymax-1 - ywide)) && (iy <= ymax-1)) {
                    for (ix=xmin; ix<xmax; ix++) setPixel(&pRow[ix*this->arrayInfo.xStride], pOverlay);
                } else {
                    for (size_t line=xmin; line<=xmin+xwide; ++line) {
                        setPixel(&pRow[line*this->arrayInfo.xStride], pOverlay);
                    }
                    for (size_t line=(xmax-xwide); line<=xmax; ++line) {
                        setPixel(&pRow[(line-1)*this->arrayInfo.xStride], pOverlay);
                    }
                }
            }
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
            xmin = MAX(xmin, 0);
            xmax = pOverlay->PositionX + pOverlay->SizeX;
            xmax = MAX(xmax, this->arrayInfo.xSize);
            ymin = pOverlay->PositionY;
            ymin = MAX(ymin, 0);
            ymax = pOverlay->PositionY + pOverlay->SizeY;
            ymax = MIN(ymax, pOverlay->PositionY + bmp->height);
            ymax = MIN(ymax, this->arrayInfo.ySize);

            // Loop over vertical lines
            for (jj=0, iy=ymin; iy<ymax; jj++, iy++) {
                pRow = (epicsType *)pArray->pData + iy*arrayInfo.yStride;

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
                            setPixel( &pRow[ix*this->arrayInfo.xStride], pOverlay);
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

    }
}

int NDPluginOverlay::doOverlay(NDArray *pArray, NDOverlay_t *pOverlay)
{
    switch(pArray->dataType) {
        case NDInt8:
            doOverlayT<epicsInt8>(pArray, pOverlay);
            break;
        case NDUInt8:
            doOverlayT<epicsUInt8>(pArray, pOverlay);
            break;
        case NDInt16:
            doOverlayT<epicsInt16>(pArray, pOverlay);
            break;
        case NDUInt16:
            doOverlayT<epicsUInt16>(pArray, pOverlay);
            break;
        case NDInt32:
            doOverlayT<epicsInt32>(pArray, pOverlay);
            break;
        case NDUInt32:
            doOverlayT<epicsUInt32>(pArray, pOverlay);
            break;
        case NDFloat32:
            doOverlayT<epicsFloat32>(pArray, pOverlay);
            break;
        case NDFloat64:
            doOverlayT<epicsFloat64>(pArray, pOverlay);
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

    int use;
    int itemp;
    int overlay;
    NDArray *pOutput;
    //static const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    /* We always keep the last array so read() can use it.
     * Release previous one. */
    if (this->pArrays[0]) {
        this->pArrays[0]->release();
    }
    /* Copy the input array so we can modify it. */
    this->pArrays[0] = this->pNDArrayPool->copy(pArray, NULL, 1);
    pOutput = this->pArrays[0];
    
    /* Get information about the array needed later */
    pOutput->getInfo(&this->arrayInfo);
    setIntegerParam(NDPluginOverlayMaxSizeX, (int)arrayInfo.xSize);
    setIntegerParam(NDPluginOverlayMaxSizeY, (int)arrayInfo.ySize);
   
    /* Loop over the overlays in this driver */
    for (overlay=0; overlay<this->maxOverlays; overlay++) {
        pOverlay = &this->pOverlays[overlay];
        getIntegerParam(overlay, NDPluginOverlayUse, &use);
        asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
            "NDPluginOverlay::processCallbacks, overlay=%d, use=%d\n",
            overlay, use);
        if (!use) continue;
        /* Need to fetch all of these parameters while we still have the mutex */
        getIntegerParam(overlay, NDPluginOverlayPositionX,  &itemp); pOverlay->PositionX = itemp;
        pOverlay->PositionX = MAX(pOverlay->PositionX, 0);
        pOverlay->PositionX = MIN(pOverlay->PositionX, this->arrayInfo.xSize-1);
        getIntegerParam(overlay, NDPluginOverlayPositionY,  &itemp); pOverlay->PositionY = itemp;
        pOverlay->PositionY = MAX(pOverlay->PositionY, 0);
        pOverlay->PositionY = MIN(pOverlay->PositionY, this->arrayInfo.ySize-1);
        getIntegerParam(overlay, NDPluginOverlaySizeX,      &itemp); pOverlay->SizeX = itemp;
        getIntegerParam(overlay, NDPluginOverlaySizeY,      &itemp); pOverlay->SizeY = itemp;
        getIntegerParam(overlay, NDPluginOverlayWidthX,     &itemp); pOverlay->WidthX = itemp;
        getIntegerParam(overlay, NDPluginOverlayWidthY,     &itemp); pOverlay->WidthY = itemp;
        getIntegerParam(overlay, NDPluginOverlayShape,      &itemp); pOverlay->shape = (NDOverlayShape_t)itemp;
        getIntegerParam(overlay, NDPluginOverlayDrawMode,   &itemp); pOverlay->drawMode = (NDOverlayDrawMode_t)itemp;
        getIntegerParam(overlay, NDPluginOverlayRed,        &pOverlay->red);
        getIntegerParam(overlay, NDPluginOverlayGreen,      &pOverlay->green);
        getIntegerParam(overlay, NDPluginOverlayBlue,       &pOverlay->blue);
        getStringParam( overlay, NDPluginOverlayTimeStampFormat, sizeof(pOverlay->TimeStampFormat), pOverlay->TimeStampFormat);
        getIntegerParam(overlay, NDPluginOverlayFont,       &pOverlay->Font);
        getStringParam( overlay, NDPluginOverlayDisplayText, sizeof(pOverlay->DisplayText), pOverlay->DisplayText);

        pOverlay->DisplayText[sizeof(pOverlay->DisplayText)-1] = 0;

        /* This function is called with the lock taken, and it must be set when we exit.
         * The following code can be exected without the mutex because we are not accessing memory
         * that other threads can access. */
        this->unlock();
        this->doOverlay(pOutput, pOverlay);
        this->lock();
    }
    /* Get the attributes for this driver */
    this->getAttributes(this->pArrays[0]->pAttributeList);
    /* Call any clients who have registered for NDArray callbacks */
    this->unlock();
    doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);
    this->lock();
    callParamCallbacks();
}



/** Constructor for NDPluginOverlay; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
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
  * \param[in] maxOverlays The maximum number ofoverlays this plugin supports. 1 is minimum.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginOverlay::NDPluginOverlay(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, int maxOverlays,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, maxOverlays, NUM_NDPLUGIN_OVERLAY_PARAMS, maxBuffers, maxMemory,
                   asynGenericPointerMask,
                   asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    static const char *functionName = "NDPluginOverlay";


    this->maxOverlays = maxOverlays;
    this->pOverlays = (NDOverlay_t *)callocMustSucceed(maxOverlays, sizeof(*this->pOverlays), functionName);

    createParam(NDPluginOverlayMaxSizeXString,      asynParamInt32, &NDPluginOverlayMaxSizeX);
    createParam(NDPluginOverlayMaxSizeYString,      asynParamInt32, &NDPluginOverlayMaxSizeY);
    createParam(NDPluginOverlayNameString,          asynParamOctet, &NDPluginOverlayName);
    createParam(NDPluginOverlayUseString,           asynParamInt32, &NDPluginOverlayUse);
    createParam(NDPluginOverlayPositionXString,     asynParamInt32, &NDPluginOverlayPositionX);
    createParam(NDPluginOverlayPositionYString,     asynParamInt32, &NDPluginOverlayPositionY);
    createParam(NDPluginOverlaySizeXString,         asynParamInt32, &NDPluginOverlaySizeX);
    createParam(NDPluginOverlaySizeYString,         asynParamInt32, &NDPluginOverlaySizeY);
    createParam(NDPluginOverlayWidthXString,        asynParamInt32, &NDPluginOverlayWidthX);
    createParam(NDPluginOverlayWidthYString,        asynParamInt32, &NDPluginOverlayWidthY);
    createParam(NDPluginOverlayShapeString,         asynParamInt32, &NDPluginOverlayShape);
    createParam(NDPluginOverlayDrawModeString,      asynParamInt32, &NDPluginOverlayDrawMode);
    createParam(NDPluginOverlayRedString,           asynParamInt32, &NDPluginOverlayRed);
    createParam(NDPluginOverlayGreenString,         asynParamInt32, &NDPluginOverlayGreen);
    createParam(NDPluginOverlayBlueString,          asynParamInt32, &NDPluginOverlayBlue);
    createParam(NDPluginOverlayTimeStampFormatString, asynParamOctet, &NDPluginOverlayTimeStampFormat);
    createParam(NDPluginOverlayFontString,          asynParamInt32, &NDPluginOverlayFont);
    createParam(NDPluginOverlayDisplayTextString,   asynParamOctet, &NDPluginOverlayDisplayText);      

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginOverlay");

    // Enable ArrayCallbacks.  
    // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
    setIntegerParam(NDArrayCallbacks, 1);

    /* Try to connect to the array port */
    connectToArrayPort();
}

/** Configuration command */
extern "C" int NDOverlayConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr, int maxOverlays,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    new NDPluginOverlay(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxOverlays,
                        maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
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
static const iocshFuncDef initFuncDef = {"NDOverlayConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDOverlayConfigure(args[0].sval, args[1].ival, args[2].ival,
                       args[3].sval, args[4].ival, args[5].ival,
                       args[6].ival, args[7].ival, args[8].ival,
                       args[9].ival);
}

extern "C" void NDOverlayRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDOverlayRegister);
}
