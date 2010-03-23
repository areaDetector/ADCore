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

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>
#include <epicsExport.h>

#include "NDArray.h"
#include "NDPluginOverlay.h"

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

//static const char *driverName="NDPluginOverlay";


template <typename epicsType>
void doOverlayT(NDArray *pArray, NDOverlay_t *pOverlay)
{
    int xmin, xmax, ymin, ymax, ix, iy;
    epicsType *pRow;
    epicsType value = (epicsType)pOverlay->green;

    switch(pOverlay->shape) {
        case NDOverlayCross:
            xmin = pOverlay->XPosition - pOverlay->XSize/2;
            xmin = MAX(xmin, 0);
            xmax = pOverlay->XPosition + pOverlay->XSize/2;
            xmax = MIN(xmax, pArray->dims[0].size);
            ymin = pOverlay->YPosition - pOverlay->YSize/2;
            ymin = MAX(ymin, 0);
            ymax = pOverlay->YPosition + pOverlay->YSize/2;
            ymax = MIN(ymax, pArray->dims[1].size);
            for (iy=ymin; iy<ymax; iy++) {
                pRow = (epicsType *)pArray->pData + iy*pArray->dims[0].size;
                if (iy == pOverlay->YPosition) {
                    for (ix=xmin; ix<xmax; ix++) pRow[ix] = value;
                } else {
                    pRow[pOverlay->XPosition] = value;
                }
            }
            break;
        case NDOverlayRectangle:
            xmin = pOverlay->XPosition;
            xmin = MAX(xmin, 0);
            xmax = pOverlay->XPosition + pOverlay->XSize;
            xmax = MIN(xmax, pArray->dims[0].size);
            ymin = pOverlay->YPosition;
            ymin = MAX(ymin, 0);
            ymax = pOverlay->YPosition + pOverlay->YSize;
            ymax = MIN(ymax, pArray->dims[1].size);
            for (iy=ymin; iy<ymax; iy++) {
                pRow = (epicsType *)pArray->pData + iy*pArray->dims[0].size;
                if ((iy == ymin) || (iy == ymax-1)) {
                    for (ix=xmin; ix<xmax; ix++) pRow[ix] = value;
                } else {
                    pRow[xmin] = value;
                    pRow[xmax-1] = value;
                }
            }
            break;
    }
}

int doOverlay(NDArray *pArray, NDOverlay_t *pOverlay)
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
    int overlay;
    int colorMode = NDColorModeMono;
    NDArray *pOutput;
    NDOverlay_t *pOverlay;
    NDAttribute *pAttribute;
    //const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
	if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

    /* We always keep the last array so read() can use it.
     * Release previous one. */
    if (this->pArrays[0]) {
        this->pArrays[0]->release();
    }
    /* Copy the input array so we can modify it. */
    this->pArrays[0] = this->pNDArrayPool->copy(pArray, NULL, 1);
    pOutput = this->pArrays[0];
    
    /* Loop over the overlays in this driver */
    for (overlay=0; overlay<this->maxOverlays; overlay++) {
        pOverlay = &this->pOverlays[overlay];
        getIntegerParam(overlay, NDPluginOverlayUse, &use);
        if (!use) continue;
        /* Need to fetch all of these parameters while we still have the mutex */
        getIntegerParam(overlay, NDPluginOverlayXPosition,  &pOverlay->XPosition);
        getIntegerParam(overlay, NDPluginOverlayYPosition,  &pOverlay->YPosition);
        getIntegerParam(overlay, NDPluginOverlayXSize,      &pOverlay->XSize);
        getIntegerParam(overlay, NDPluginOverlayYSize,      &pOverlay->YSize);
        getIntegerParam(overlay, NDPluginOverlayShape,       (int *)&pOverlay->shape);
        getIntegerParam(overlay, NDPluginOverlayDrawMode,   (int *)&pOverlay->drawMode);
        getIntegerParam(overlay, NDPluginOverlayRed,        &pOverlay->red);
        getIntegerParam(overlay, NDPluginOverlayGreen,      &pOverlay->green);
        getIntegerParam(overlay, NDPluginOverlayBlue,       &pOverlay->blue);

        /* This function is called with the lock taken, and it must be set when we exit.
         * The following code can be exected without the mutex because we are not accessing elements of
         * pPvt that other threads can access. */
        this->unlock();
        doOverlay(pOutput, pOverlay);
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
    asynStatus status;
    const char *functionName = "NDPluginOverlay";


    this->maxOverlays = maxOverlays;
    this->pOverlays = (NDOverlay_t *)callocMustSucceed(maxOverlays, sizeof(*this->pOverlays), functionName);

    createParam(NDPluginOverlayNameString,          asynParamOctet, &NDPluginOverlayName);
    createParam(NDPluginOverlayUseString,           asynParamInt32, &NDPluginOverlayUse);
    createParam(NDPluginOverlayXPositionString,     asynParamInt32, &NDPluginOverlayXPosition);
    createParam(NDPluginOverlayYPositionString,     asynParamInt32, &NDPluginOverlayYPosition);
    createParam(NDPluginOverlayXSizeString,         asynParamInt32, &NDPluginOverlayXSize);
    createParam(NDPluginOverlayYSizeString,         asynParamInt32, &NDPluginOverlayYSize);
    createParam(NDPluginOverlayShapeString,         asynParamInt32, &NDPluginOverlayShape);
    createParam(NDPluginOverlayDrawModeString,      asynParamInt32, &NDPluginOverlayDrawMode);
    createParam(NDPluginOverlayRedString,           asynParamInt32, &NDPluginOverlayRed);
    createParam(NDPluginOverlayGreenString,         asynParamInt32, &NDPluginOverlayGreen);
    createParam(NDPluginOverlayBlueString,          asynParamInt32, &NDPluginOverlayBlue);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginOverlay");

    /* Try to connect to the array port */
    status = connectToArrayPort();
}

/** Configuration command */
extern "C" int NDOverlayConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr, int maxOverlays,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    NDPluginOverlay *pPlugin =
        new NDPluginOverlay(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxOverlays,
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
