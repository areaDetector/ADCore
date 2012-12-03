/* simDetector.cpp
 *
 * This is a driver for a simulated area detector.
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  March 20, 2008
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <cantProceed.h>
#include <iocsh.h>

#include "ADDriver.h"
#include <epicsExport.h>

static const char *driverName = "simDetector";

/** Simulation detector driver; demonstrates most of the features that areaDetector drivers can support. */
class simDetector : public ADDriver {
public:
    simDetector(const char *portName, int maxSizeX, int maxSizeY, NDDataType_t dataType,
                int maxBuffers, size_t maxMemory,
                int priority, int stackSize);

    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual void setShutter(int open);
    virtual void report(FILE *fp, int details);
    void simTask(); /**< Should be private, but gets called from C, so must be public */

protected:
    int SimGainX;
    #define FIRST_SIM_DETECTOR_PARAM SimGainX
    int SimGainY;
    int SimGainRed;
    int SimGainGreen;
    int SimGainBlue;
    int SimNoise;
    int SimResetImage;
    int SimMode;
    int SimPeakStartX;
    int SimPeakStartY;
    int SimPeakWidthX;
    int SimPeakWidthY;
    int SimPeakNumX;
    int SimPeakNumY;
    int SimPeakStepX;
    int SimPeakStepY;
    int SimPeakHeightVariation;

    #define LAST_SIM_DETECTOR_PARAM SimPeakHeightVariation

private:
    /* These are the methods that are new to this class */
    template <typename epicsType> int computeArray(int sizeX, int sizeY);
    template <typename epicsType> int computeLinearRampArray(int sizeX, int sizeY);
    template <typename epicsType> int computePeaksArray(int sizeX, int sizeY);
    int computeImage();

    /* Our data */
    epicsEventId startEventId;
    epicsEventId stopEventId;
    NDArray *pRaw;
};

typedef enum {
    SimModeLinearRamp,
    SimModePeaks,
}SimModes_t;

#define SimGainXString          "SIM_GAIN_X"
#define SimGainYString          "SIM_GAIN_Y"
#define SimGainRedString        "SIM_GAIN_RED"
#define SimGainGreenString      "SIM_GAIN_GREEN"
#define SimGainBlueString       "SIM_GAIN_BLUE"
#define SimNoiseString          "SIM_NOISE"
#define SimResetImageString     "RESET_IMAGE"
#define SimModeString           "SIM_MODE"
#define SimPeakStartXString     "SIM_PEAK_START_X"
#define SimPeakStartYString     "SIM_PEAK_START_Y"
#define SimPeakWidthXString     "SIM_PEAK_WIDTH_X"
#define SimPeakWidthYString     "SIM_PEAK_WIDTH_Y"
#define SimPeakNumXString       "SIM_PEAK_NUM_X"
#define SimPeakNumYString       "SIM_PEAK_NUM_Y"
#define SimPeakStepXString      "SIM_PEAK_STEP_X"
#define SimPeakStepYString      "SIM_PEAK_STEP_Y"
#define SimPeakHeightVariationString  "SIM_PEAK_HEIGHT_VARIATION"


#define NUM_SIM_DETECTOR_PARAMS ((int)(&LAST_SIM_DETECTOR_PARAM - &FIRST_SIM_DETECTOR_PARAM + 1))




/** Template function to compute the simulated detector data for any data type */
template <typename epicsType> int simDetector::computeArray(int sizeX, int sizeY)
{
    int simMode=0;
    int status = asynSuccess;

    status = getIntegerParam (SimMode, &simMode);
    switch(simMode) {
        case SimModeLinearRamp:
            status = computeLinearRampArray<epicsType>(sizeX, sizeY);
            break;
        case SimModePeaks:
            status = computePeaksArray<epicsType>(sizeX, sizeY);
            break;
    }
    return status;
}

/** Template function to compute the simulated detector data for any data type */
template <typename epicsType> int simDetector::computeLinearRampArray(int sizeX, int sizeY)
{
    epicsType *pMono=NULL, *pRed=NULL, *pGreen=NULL, *pBlue=NULL;
    int columnStep=0, rowStep=0, colorMode;
    epicsType incMono, incRed, incGreen, incBlue;
    int status = asynSuccess;
    double exposureTime, gain, gainX, gainY, gainRed, gainGreen, gainBlue;
    int resetImage;
    int i, j;

    status = getDoubleParam (ADGain,        &gain);
    status = getDoubleParam (SimGainX,      &gainX);
    status = getDoubleParam (SimGainY,      &gainY);
    status = getDoubleParam (SimGainRed,    &gainRed);
    status = getDoubleParam (SimGainGreen,  &gainGreen);
    status = getDoubleParam (SimGainBlue,   &gainBlue);
    status = getIntegerParam(SimResetImage, &resetImage);
    status = getIntegerParam(NDColorMode,   &colorMode);
    status = getDoubleParam (ADAcquireTime, &exposureTime);

    /* The intensity at each pixel[i,j] is:
     * (i * gainX + j* gainY) + imageCounter * gain * exposureTime * 1000. */
    incMono  = (epicsType) (gain      * exposureTime * 1000.);
    incRed   = (epicsType) gainRed   * incMono;
    incGreen = (epicsType) gainGreen * incMono;
    incBlue  = (epicsType) gainBlue  * incMono;

    switch (colorMode) {
        case NDColorModeMono:
            pMono = (epicsType *)this->pRaw->pData;
            break;
        case NDColorModeRGB1:
            columnStep = 3;
            rowStep = 0;
            pRed   = (epicsType *)this->pRaw->pData;
            pGreen = (epicsType *)this->pRaw->pData+1;
            pBlue  = (epicsType *)this->pRaw->pData+2;
            break;
        case NDColorModeRGB2:
            columnStep = 1;
            rowStep = 2 * sizeX;
            pRed   = (epicsType *)this->pRaw->pData;
            pGreen = (epicsType *)this->pRaw->pData + sizeX;
            pBlue  = (epicsType *)this->pRaw->pData + 2*sizeX;
            break;
        case NDColorModeRGB3:
            columnStep = 1;
            rowStep = 0;
            pRed   = (epicsType *)this->pRaw->pData;
            pGreen = (epicsType *)this->pRaw->pData + sizeX*sizeY;
            pBlue  = (epicsType *)this->pRaw->pData + 2*sizeX*sizeY;
            break;
    }
    this->pRaw->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);

    if (resetImage) {
        for (i=0; i<sizeY; i++) {
            switch (colorMode) {
                case NDColorModeMono:
                    for (j=0; j<sizeX; j++) {
                        (*pMono++) = (epicsType) (incMono * (gainX*j + gainY*i));
                    }
                    break;
                case NDColorModeRGB1:
                case NDColorModeRGB2:
                case NDColorModeRGB3:
                    for (j=0; j<sizeX; j++) {
                        *pRed   = (epicsType) (incRed   * (gainX*j + gainY*i));
                        *pGreen = (epicsType) (incGreen * (gainX*j + gainY*i));
                        *pBlue  = (epicsType) (incBlue  * (gainX*j + gainY*i));
                        pRed   += columnStep;
                        pGreen += columnStep;
                        pBlue  += columnStep;
                    }
                    pRed   += rowStep;
                    pGreen += rowStep;
                    pBlue  += rowStep;
                    break;
            }
        }
    } else {
        for (i=0; i<sizeY; i++) {
            switch (colorMode) {
                case NDColorModeMono:
                    for (j=0; j<sizeX; j++) {
                            *pMono++ += incMono;
                    }
                    break;
                case NDColorModeRGB1:
                case NDColorModeRGB2:
                case NDColorModeRGB3:
                    for (j=0; j<sizeX; j++) {
                        *pRed   += incRed;
                        *pGreen += incGreen;
                        *pBlue  += incBlue;
                        pRed   += columnStep;
                        pGreen += columnStep;
                        pBlue  += columnStep;
                    }
                    pRed   += rowStep;
                    pGreen += rowStep;
                    pBlue  += rowStep;
                    break;
            }
        }
    }
    return(status);
}

/** Compute array for array of peaks */
template <typename epicsType> int simDetector::computePeaksArray(int sizeX, int sizeY)
{
    epicsType *pMono=NULL, *pRed=NULL;
    epicsType *pMono2=NULL, *pRed2=NULL, *pGreen2=NULL, *pBlue2=NULL;
    int columnStep=0, colorMode;
    int peaksStartX, peaksStartY, peaksStepX, peaksStepY;
    int peaksNumX, peaksNumY, peaksWidthX, peaksWidthY;
     int status = asynSuccess;
    int i,j,k,l;
    int minX, maxX, minY,maxY;
    int offsetX, offsetY;
    int peakVariation, noisePct;
    double gainVariation, noise;
    double gain, gainX, gainY, gainRed, gainGreen, gainBlue;
    double gaussX, gaussY;
    double tmpValue;

    status = getIntegerParam(NDColorMode,   &colorMode);
    status = getDoubleParam (ADGain,        &gain);
    status = getDoubleParam (SimGainX,      &gainX);
    status = getDoubleParam (SimGainY,      &gainY);
    status = getDoubleParam (SimGainRed,    &gainRed);
    status = getDoubleParam (SimGainGreen,  &gainGreen);
    status = getDoubleParam (SimGainBlue,   &gainBlue);
    status = getIntegerParam (SimPeakStartX,  &peaksStartX);
    status = getIntegerParam (SimPeakStartY,  &peaksStartY);
    status = getIntegerParam (SimPeakStepX,  &peaksStepX);
    status = getIntegerParam (SimPeakStepY,  &peaksStepY);
    status = getIntegerParam (SimPeakNumX,  &peaksNumX);
    status = getIntegerParam (SimPeakNumY,  &peaksNumY);
    status = getIntegerParam (SimPeakWidthX,  &peaksWidthX);
    status = getIntegerParam (SimPeakWidthY,  &peaksWidthY);
    status = getIntegerParam (SimPeakHeightVariation,  &peakVariation);
    status = getIntegerParam (SimNoise,  &noisePct);

       switch (colorMode) {
        case NDColorModeMono:
            pMono = (epicsType *)this->pRaw->pData;
            break;
        case NDColorModeRGB1:
            columnStep = 3;
            pRed   = (epicsType *)this->pRaw->pData;
            break;
        case NDColorModeRGB2:
            columnStep = 1;
            pRed   = (epicsType *)this->pRaw->pData;
            break;
        case NDColorModeRGB3:
            columnStep = 1;
            pRed   = (epicsType *)this->pRaw->pData;
            break;
    }
    this->pRaw->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
    switch (colorMode) {
        case NDColorModeMono:
            // Clear the Image
            pMono2 = pMono;
            for (i = 0; i<sizeY; i++) {
                for (j = 0; j<sizeX; j++) {
                    (*pMono2++) = (epicsType)0;
                }
            }
            for (i = 0; i<peaksNumY; i++) {
                for (j = 0; j<peaksNumX; j++) {
                    gaussX = 0;
                    gaussY = 0;
                    if (peakVariation !=0) {
                        gainVariation = 1.0 + (rand()%peakVariation+1)/100.0;
                    }
                    else{
                        gainVariation = 1.0;
                    }
                    offsetY = i * peaksStepY + peaksStartY;
                    offsetX = j * peaksStepX + peaksStartX;
                    minX = (offsetX>4*peaksWidthX) ?(offsetX -4*peaksWidthX):0;
                    maxX = (offsetX+4*peaksWidthX<sizeX) ?(offsetX + 4*peaksWidthX):sizeX;
                    minY = (offsetY>4*peaksWidthY) ?(offsetY -4*peaksWidthY):0;
                    maxY = (offsetY+4*peaksWidthY<sizeY) ?(offsetY + 4*peaksWidthY):sizeY;
                    for (k =minY; k<maxY; k++) {
                        pMono2 = pMono + (minX + k*sizeX);
                        for (l=minX; l<maxX; l++) {
                            if (noisePct !=0) {
                                noise = 1.0 + (rand()%noisePct+1)/100.0;
                            }
                            else {
                                noise = 1.0;
                            }
                            gaussY = gainY * exp( -pow((double)(k-offsetY)/(double)peaksWidthY,2.0)/2.0 );
                            gaussX = gainX * exp( -pow((double)(l-offsetX)/(double)peaksWidthX,2.0)/2.0 );
                            tmpValue =  gainVariation*gain * gaussX * gaussY*noise;
                            (*pMono2) += (epicsType)tmpValue;
                            pMono2++;
                        }
                    }
                }
            }
            break;
        case NDColorModeRGB1:
        case NDColorModeRGB2:
        case NDColorModeRGB3:
            // Clear the Image
            pRed2 = pRed;
            for (i = 0; i<sizeY; i++) {
                for (j = 0; j<sizeX; j++) {
                    (*pRed2++) = (epicsType)0;  //Since we are just clearing the field we will do this with one pointer
                    (*pRed2++) = (epicsType)0;
                    (*pRed2++) = (epicsType)0;
                }
            }
            for (i = 0; i<peaksNumY; i++) {
                for (j = 0; j<peaksNumX; j++) {
                    if (peakVariation !=0) {
                        gainVariation = 1.0 + (rand()%peakVariation+1)/100.0;
                    }
                    else{
                        gainVariation = 1.0;
                    }
                    offsetY = i * peaksStepY + peaksStartY;
                    offsetX = j * peaksStepX + peaksStartX;
                    minX = (offsetX>4*peaksWidthX) ?(offsetX -4*peaksWidthX):0;
                    maxX = (offsetX+4*peaksWidthX<sizeX) ?(offsetX + 4*peaksWidthX):sizeX;
                    minY = (offsetY>4*peaksWidthY) ?(offsetY -4*peaksWidthY):0;
                    maxY = (offsetY+4*peaksWidthY<sizeY) ?(offsetY + 4*peaksWidthY):sizeY;
                    for (k =minY; k<maxY; k++) {
                        //Move to the starting point for this peak
                        switch (colorMode) {
                            case NDColorModeRGB1:
                                pRed2 = pRed + (minX*columnStep + k*sizeX*columnStep);
                                pGreen2 = pRed2 + 1;
                                pBlue2 = pRed2 + 2;
                                break;
                            case NDColorModeRGB2:
                                pRed2 = pRed + (minX*columnStep + k*3*sizeX*columnStep);
                                pGreen2 = pRed2 + sizeX;
                                pBlue2 = pRed2 + 2*sizeX;
                                break;
                            case NDColorModeRGB3:
                                pRed2 = pRed + (minX*columnStep + k*sizeX*columnStep);
                                pGreen2 = pRed2 + sizeX*sizeY;
                                pBlue2 = pRed2 + 2*sizeX*sizeY;
                                break;
                        }
                        //Fill in a row for this peak
                        for (l=minX; l<maxX; l++) {
                            if (noisePct !=0) {
                                noise = 1.0 + (rand()%noisePct+1)/100.0;
                            }
                            else {
                                noise = 1.0;
                            }
                            gaussY = gainY * exp( -pow((double)(k-offsetY)/(double)peaksWidthY,2.0)/2.0 );
                            gaussX = gainX * exp( -pow((double)(l-offsetX)/(double)peaksWidthX,2.0)/2.0 );
                            tmpValue =  gainVariation*gain * gaussX * gaussY*noise;
                            (*pRed2) += (epicsType)(gainRed*tmpValue);
                            (*pGreen2) += (epicsType)(gainGreen*tmpValue);
                            (*pBlue2) += (epicsType)(gainBlue*tmpValue);

                            pRed2 += columnStep;
                            pGreen2 += columnStep;
                            pBlue2 += columnStep;
                        }
                    }
                }
            }



            break;

    }
    return status;
}

/** Controls the shutter */
void simDetector::setShutter(int open)
{
    int shutterMode;

    getIntegerParam(ADShutterMode, &shutterMode);
    if (shutterMode == ADShutterModeDetector) {
        /* Simulate a shutter by just changing the status readback */
        setIntegerParam(ADShutterStatus, open);
    } else {
        /* For no shutter or EPICS shutter call the base class method */
        ADDriver::setShutter(open);
    }
}

/** Computes the new image data */
int simDetector::computeImage()
{
    int status = asynSuccess;
    NDDataType_t dataType;
    int itemp;
    int binX, binY, minX, minY, sizeX, sizeY, reverseX, reverseY;
    int xDim=0, yDim=1, colorDim=-1;
    int resetImage;
    int maxSizeX, maxSizeY;
    int colorMode;
    int ndims=0;
    NDDimension_t dimsOut[3];
    size_t dims[3];
    NDArrayInfo_t arrayInfo;
    NDArray *pImage;
    const char* functionName = "computeImage";

    /* NOTE: The caller of this function must have taken the mutex */

    status |= getIntegerParam(ADBinX,         &binX);
    status |= getIntegerParam(ADBinY,         &binY);
    status |= getIntegerParam(ADMinX,         &minX);
    status |= getIntegerParam(ADMinY,         &minY);
    status |= getIntegerParam(ADSizeX,        &sizeX);
    status |= getIntegerParam(ADSizeY,        &sizeY);
    status |= getIntegerParam(ADReverseX,     &reverseX);
    status |= getIntegerParam(ADReverseY,     &reverseY);
    status |= getIntegerParam(ADMaxSizeX,     &maxSizeX);
    status |= getIntegerParam(ADMaxSizeY,     &maxSizeY);
    status |= getIntegerParam(NDColorMode,    &colorMode);
    status |= getIntegerParam(NDDataType,     &itemp); dataType = (NDDataType_t)itemp;
    status |= getIntegerParam(SimResetImage,  &resetImage);
    if (status) asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: error getting parameters\n",
                    driverName, functionName);

    /* Make sure parameters are consistent, fix them if they are not */
    if (binX < 1) {
        binX = 1;
        status |= setIntegerParam(ADBinX, binX);
    }
    if (binY < 1) {
        binY = 1;
        status |= setIntegerParam(ADBinY, binY);
    }
    if (minX < 0) {
        minX = 0;
        status |= setIntegerParam(ADMinX, minX);
    }
    if (minY < 0) {
        minY = 0;
        status |= setIntegerParam(ADMinY, minY);
    }
    if (minX > maxSizeX-1) {
        minX = maxSizeX-1;
        status |= setIntegerParam(ADMinX, minX);
    }
    if (minY > maxSizeY-1) {
        minY = maxSizeY-1;
        status |= setIntegerParam(ADMinY, minY);
    }
    if (minX+sizeX > maxSizeX) {
        sizeX = maxSizeX-minX;
        status |= setIntegerParam(ADSizeX, sizeX);
    }
    if (minY+sizeY > maxSizeY) {
        sizeY = maxSizeY-minY;
        status |= setIntegerParam(ADSizeY, sizeY);
    }

    switch (colorMode) {
        case NDColorModeMono:
            ndims = 2;
            xDim = 0;
            yDim = 1;
            break;
        case NDColorModeRGB1:
            ndims = 3;
            colorDim = 0;
            xDim     = 1;
            yDim     = 2;
            break;
        case NDColorModeRGB2:
            ndims = 3;
            colorDim = 1;
            xDim     = 0;
            yDim     = 2;
            break;
        case NDColorModeRGB3:
            ndims = 3;
            colorDim = 2;
            xDim     = 0;
            yDim     = 1;
            break;
    }

    if (resetImage) {
    /* Free the previous raw buffer */
        if (this->pRaw) this->pRaw->release();
        /* Allocate the raw buffer we use to compute images. */
        dims[xDim] = maxSizeX;
        dims[yDim] = maxSizeY;
        if (ndims > 2) dims[colorDim] = 3;
        this->pRaw = this->pNDArrayPool->alloc(ndims, dims, dataType, 0, NULL);

        if (!this->pRaw) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s:%s: error allocating raw buffer\n",
                      driverName, functionName);
            return(status);
        }
    }

    switch (dataType) {
        case NDInt8:
            status |= computeArray<epicsInt8>(maxSizeX, maxSizeY);
            break;
        case NDUInt8:
            status |= computeArray<epicsUInt8>(maxSizeX, maxSizeY);
            break;
        case NDInt16:
            status |= computeArray<epicsInt16>(maxSizeX, maxSizeY);
            break;
        case NDUInt16:
            status |= computeArray<epicsUInt16>(maxSizeX, maxSizeY);
            break;
        case NDInt32:
            status |= computeArray<epicsInt32>(maxSizeX, maxSizeY);
            break;
        case NDUInt32:
            status |= computeArray<epicsUInt32>(maxSizeX, maxSizeY);
            break;
        case NDFloat32:
            status |= computeArray<epicsFloat32>(maxSizeX, maxSizeY);
            break;
        case NDFloat64:
            status |= computeArray<epicsFloat64>(maxSizeX, maxSizeY);
            break;
    }

    /* Extract the region of interest with binning.
     * If the entire image is being used (no ROI or binning) that's OK because
     * convertImage detects that case and is very efficient */
    this->pRaw->initDimension(&dimsOut[xDim], sizeX);
    this->pRaw->initDimension(&dimsOut[yDim], sizeY);
    if (ndims > 2) this->pRaw->initDimension(&dimsOut[colorDim], 3);
    dimsOut[xDim].binning = binX;
    dimsOut[xDim].offset  = minX;
    dimsOut[xDim].reverse = reverseX;
    dimsOut[yDim].binning = binY;
    dimsOut[yDim].offset  = minY;
    dimsOut[yDim].reverse = reverseY;
    /* We save the most recent image buffer so it can be used in the read() function.
     * Now release it before getting a new version. */
    if (this->pArrays[0]) this->pArrays[0]->release();
    status = this->pNDArrayPool->convert(this->pRaw,
                                         &this->pArrays[0],
                                         dataType,
                                         dimsOut);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: error allocating buffer in convert()\n",
                    driverName, functionName);
        return(status);
    }
    pImage = this->pArrays[0];
    pImage->getInfo(&arrayInfo);
    status = asynSuccess;
    status |= setIntegerParam(NDArraySize,  (int)arrayInfo.totalBytes);
    status |= setIntegerParam(NDArraySizeX, (int)pImage->dims[xDim].size);
    status |= setIntegerParam(NDArraySizeY, (int)pImage->dims[yDim].size);
    status |= setIntegerParam(SimResetImage, 0);
    if (status) asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: error setting parameters\n",
                    driverName, functionName);
    return(status);
}

static void simTaskC(void *drvPvt)
{
    simDetector *pPvt = (simDetector *)drvPvt;

    pPvt->simTask();
}

/** This thread calls computeImage to compute new image data and does the callbacks to send it to higher layers.
  * It implements the logic for single, multiple or continuous acquisition. */
void simDetector::simTask()
{
    int status = asynSuccess;
    int imageCounter;
    int numImages, numImagesCounter;
    int imageMode;
    int arrayCallbacks;
    int acquire=0;
    NDArray *pImage;
    double acquireTime, acquirePeriod, delay;
    epicsTimeStamp startTime, endTime;
    double elapsedTime;
    const char *functionName = "simTask";

    this->lock();
    /* Loop forever */
    while (1) {
       /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquire) {
          /* Release the lock while we wait for an event that says acquire has started, then lock again */
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: waiting for acquire to start\n", driverName, functionName);
            this->unlock();
            status = epicsEventWait(this->startEventId);
            this->lock();
            acquire = 1;
            setStringParam(ADStatusMessage, "Acquiring data");
            setIntegerParam(ADNumImagesCounter, 0);
        }

        /* We are acquiring. */
        /* Get the current time */
        epicsTimeGetCurrent(&startTime);
        getIntegerParam(ADImageMode, &imageMode);

        /* Get the exposure parameters */
        getDoubleParam(ADAcquireTime, &acquireTime);
        getDoubleParam(ADAcquirePeriod, &acquirePeriod);

        setIntegerParam(ADStatus, ADStatusAcquire);

        /* Open the shutter */
        setShutter(ADShutterOpen);

        /* Call the callbacks to update any changes */
        callParamCallbacks();

        /* Simulate being busy during the exposure time.  Use epicsEventWaitWithTimeout so that
         * manually stopping the acquisition will work */

        if (acquireTime > 0.0) {
            this->unlock();
            status = epicsEventWaitWithTimeout(this->stopEventId, acquireTime);
            this->lock();
        } else {
            status = epicsEventTryWait(this->stopEventId);
        }        
        if (status == epicsEventWaitOK) {
            acquire = 0;
            if (imageMode == ADImageContinuous) {
              setIntegerParam(ADStatus, ADStatusIdle);
            } else {
              setIntegerParam(ADStatus, ADStatusAborted);
            }
            callParamCallbacks();
        }
            

        /* Update the image */
        status = computeImage();
        if (status) continue;

        /* Close the shutter */
        setShutter(ADShutterClosed);
        
        if (!acquire) continue;

        setIntegerParam(ADStatus, ADStatusReadout);
        /* Call the callbacks to update any changes */
        callParamCallbacks();

        pImage = this->pArrays[0];

        /* Get the current parameters */
        getIntegerParam(NDArrayCounter, &imageCounter);
        getIntegerParam(ADNumImages, &numImages);
        getIntegerParam(ADNumImagesCounter, &numImagesCounter);
        getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
        imageCounter++;
        numImagesCounter++;
        setIntegerParam(NDArrayCounter, imageCounter);
        setIntegerParam(ADNumImagesCounter, numImagesCounter);

        /* Put the frame number and time stamp into the buffer */
        pImage->uniqueId = imageCounter;
        pImage->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;

        /* Get any attributes that have been defined for this driver */
        this->getAttributes(pImage->pAttributeList);

        if (arrayCallbacks) {
          /* Call the NDArray callback */
          /* Must release the lock here, or we can get into a deadlock, because we can
           * block on the plugin lock, and the plugin can be calling us */
          this->unlock();
          asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: calling imageData callback\n", driverName, functionName);
          doCallbacksGenericPointer(pImage, NDArrayData, 0);
          this->lock();
        }

        /* See if acquisition is done */
        if ((imageMode == ADImageSingle) ||
            ((imageMode == ADImageMultiple) &&
             (numImagesCounter >= numImages))) {

          /* First do callback on ADStatus. */
          setStringParam(ADStatusMessage, "Waiting for acquisition");
          setIntegerParam(ADStatus, ADStatusIdle);
          callParamCallbacks();

          acquire = 0;
          setIntegerParam(ADAcquire, acquire);
          asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: acquisition completed\n", driverName, functionName);
        }

        /* Call the callbacks to update any changes */
        callParamCallbacks();

        /* If we are acquiring then sleep for the acquire period minus elapsed time. */
        if (acquire) {
          epicsTimeGetCurrent(&endTime);
          elapsedTime = epicsTimeDiffInSeconds(&endTime, &startTime);
          delay = acquirePeriod - elapsedTime;
          asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                    "%s:%s: delay=%f\n",
                    driverName, functionName, delay);
          if (delay >= 0.0) {
            /* We set the status to waiting to indicate we are in the period delay */
            setIntegerParam(ADStatus, ADStatusWaiting);
            callParamCallbacks();
            this->unlock();
            status = epicsEventWaitWithTimeout(this->stopEventId, delay);
            this->lock();
            if (status == epicsEventWaitOK) {
              acquire = 0;
              if (imageMode == ADImageContinuous) {
                setIntegerParam(ADStatus, ADStatusIdle);
              } else {
                setIntegerParam(ADStatus, ADStatusAborted);
              }
              callParamCallbacks();
            }
          }
        }
    }
}


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including ADAcquire, ADColorMode, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus simDetector::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int adstatus;
    int acquiring;
    int imageMode;
    asynStatus status = asynSuccess;

    /* Ensure that ADStatus is set correctly before we set ADAcquire.*/
    getIntegerParam(ADStatus, &adstatus);
    getIntegerParam(ADAcquire, &acquiring);
    if (function == ADAcquire) {
      if (value && !acquiring) {
        setStringParam(ADStatusMessage, "Acquiring data");
        getIntegerParam(ADImageMode, &imageMode);
      }
      if (!value && acquiring) {
        setStringParam(ADStatusMessage, "Acquisition stopped");
        if (imageMode == ADImageContinuous) {
          setIntegerParam(ADStatus, ADStatusIdle);
        } else {
          setIntegerParam(ADStatus, ADStatusAborted);
        }
        setIntegerParam(ADStatus, ADStatusAcquire); 
      }
    }
    callParamCallbacks();
 
    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setIntegerParam(function, value);

    /* For a real detector this is where the parameter is sent to the hardware */
    if (function == ADAcquire) {
        if (value && !acquiring) {
            /* Send an event to wake up the simulation task.
             * It won't actually start generating new images until we release the lock below */
            epicsEventSignal(this->startEventId); 
        }
        if (!value && acquiring) {
            /* This was a command to stop acquisition */
            /* Send the stop event */
            epicsEventSignal(this->stopEventId); 
        }
    } else if ((function == NDDataType) || 
               (function == NDColorMode) ||
               (function == SimMode)) {
        status = setIntegerParam(SimResetImage, 1);
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_SIM_DETECTOR_PARAM) status = ADDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:writeInt32 error, status=%d function=%d, value=%d\n",
              driverName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:writeInt32: function=%d, value=%d\n",
              driverName, function, value);
    return status;
}


/** Called when asyn clients call pasynFloat64->write().
  * This function performs actions for some parameters, including ADAcquireTime, ADGain, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus simDetector::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(function, value);

    /* Changing any of the following parameters requires recomputing the base image */
    if ((function == ADAcquireTime) ||
        (function == ADGain) ||
        (function == SimGainX) ||
        (function == SimGainY) ||
        (function == SimGainRed) ||
        (function == SimGainGreen) ||
        (function == SimGainBlue)) {
            status = setIntegerParam(SimResetImage, 1);
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_SIM_DETECTOR_PARAM) status = ADDriver::writeFloat64(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();
    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:writeFloat64 error, status=%d function=%d, value=%f\n",
              driverName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:writeFloat64: function=%d, value=%f\n",
              driverName, function, value);
    return status;
}


/** Report status of the driver.
  * Prints details about the driver if details>0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >0 then driver details are printed.
  */
void simDetector::report(FILE *fp, int details)
{

    fprintf(fp, "Simulation detector %s\n", this->portName);
    if (details > 0) {
        int nx, ny, dataType;
        getIntegerParam(ADSizeX, &nx);
        getIntegerParam(ADSizeY, &ny);
        getIntegerParam(NDDataType, &dataType);
        fprintf(fp, "  NX, NY:            %d  %d\n", nx, ny);
        fprintf(fp, "  Data type:         %d\n", dataType);
    }
    /* Invoke the base class method */
    ADDriver::report(fp, details);
}

/** Constructor for simDetector; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method creates a thread to compute the simulated detector data,
  * and sets reasonable default values for parameters defined in this class, asynNDArrayDriver and ADDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxSizeX The maximum X dimension of the images that this driver can create.
  * \param[in] maxSizeY The maximum Y dimension of the images that this driver can create.
  * \param[in] dataType The initial data type (NDDataType_t) of the images that this driver will create.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
simDetector::simDetector(const char *portName, int maxSizeX, int maxSizeY, NDDataType_t dataType,
                         int maxBuffers, size_t maxMemory, int priority, int stackSize)

    : ADDriver(portName, 1, NUM_SIM_DETECTOR_PARAMS, maxBuffers, maxMemory,
               0, 0, /* No interfaces beyond those set in ADDriver.cpp */
               0, 1, /* ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=0, autoConnect=1 */
               priority, stackSize),
      pRaw(NULL)

{
    int status = asynSuccess;
    const char *functionName = "simDetector";

    /* Create the epicsEvents for signaling to the simulate task when acquisition starts and stops */
    this->startEventId = epicsEventCreate(epicsEventEmpty);
    if (!this->startEventId) {
        printf("%s:%s epicsEventCreate failure for start event\n",
            driverName, functionName);
        return;
    }
    this->stopEventId = epicsEventCreate(epicsEventEmpty);
    if (!this->stopEventId) {
        printf("%s:%s epicsEventCreate failure for stop event\n",
            driverName, functionName);
        return;
    }

    createParam(SimGainXString,       asynParamFloat64, &SimGainX);
    createParam(SimGainYString,       asynParamFloat64, &SimGainY);
    createParam(SimGainRedString,     asynParamFloat64, &SimGainRed);
    createParam(SimGainGreenString,   asynParamFloat64, &SimGainGreen);
    createParam(SimGainBlueString,    asynParamFloat64, &SimGainBlue);
    createParam(SimNoiseString,       asynParamInt32,   &SimNoise);
    createParam(SimResetImageString,  asynParamInt32,   &SimResetImage);
    createParam(SimModeString,        asynParamInt32,   &SimMode);
    createParam(SimPeakNumXString,    asynParamInt32,   &SimPeakNumX);
    createParam(SimPeakNumYString,    asynParamInt32,   &SimPeakNumY);
    createParam(SimPeakStepXString,   asynParamInt32,   &SimPeakStepX);
    createParam(SimPeakStepYString,   asynParamInt32,   &SimPeakStepY);
    createParam(SimPeakStartXString,  asynParamInt32,   &SimPeakStartX);
    createParam(SimPeakStartYString,  asynParamInt32,   &SimPeakStartY);
    createParam(SimPeakWidthXString,  asynParamInt32,   &SimPeakWidthX);
    createParam(SimPeakWidthYString,  asynParamInt32,   &SimPeakWidthY);
    createParam(SimPeakHeightVariationString,  asynParamInt32,   &SimPeakHeightVariation);

    /* Set some default values for parameters */
    status =  setStringParam (ADManufacturer, "Simulated detector");
    status |= setStringParam (ADModel, "Basic simulator");
    status |= setIntegerParam(ADMaxSizeX, maxSizeX);
    status |= setIntegerParam(ADMaxSizeY, maxSizeY);
    status |= setIntegerParam(ADSizeX, maxSizeX);
    status |= setIntegerParam(ADSizeX, maxSizeX);
    status |= setIntegerParam(ADSizeY, maxSizeY);
    status |= setIntegerParam(NDArraySizeX, maxSizeX);
    status |= setIntegerParam(NDArraySizeY, maxSizeY);
    status |= setIntegerParam(NDArraySize, 0);
    status |= setIntegerParam(NDDataType, dataType);
    status |= setIntegerParam(ADImageMode, ADImageContinuous);
    status |= setDoubleParam (ADAcquireTime, .001);
    status |= setDoubleParam (ADAcquirePeriod, .005);
    status |= setIntegerParam(ADNumImages, 100);
    status |= setIntegerParam(SimNoise, 3);
    status |= setIntegerParam(SimResetImage, 1);
    status |= setDoubleParam (SimGainX, 1);
    status |= setDoubleParam (SimGainY, 1);
    status |= setDoubleParam (SimGainRed, 1);
    status |= setDoubleParam (SimGainGreen, 1);
    status |= setDoubleParam (SimGainBlue, 1);
    status |= setIntegerParam(SimMode, 0);
    status |= setIntegerParam(SimPeakStartX, 1);
    status |= setIntegerParam(SimPeakStartY, 1);
    status |= setIntegerParam(SimPeakWidthX, 10);
    status |= setIntegerParam(SimPeakWidthY, 20);
    status |= setIntegerParam(SimPeakNumX, 1);
    status |= setIntegerParam(SimPeakNumY, 1);
    status |= setIntegerParam(SimPeakStepX, 1);
    status |= setIntegerParam(SimPeakStepY, 1);
    status |= setIntegerParam(SimPeakHeightVariation, 3);

    if (status) {
        printf("%s: unable to set camera parameters\n", functionName);
        return;
    }

    /* Create the thread that updates the images */
    status = (epicsThreadCreate("SimDetTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)simTaskC,
                                this) == NULL);
    if (status) {
        printf("%s:%s epicsThreadCreate failure for image task\n",
            driverName, functionName);
        return;
    }
}

/** Configuration command, called directly or from iocsh */
extern "C" int simDetectorConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType,
                                 int maxBuffers, int maxMemory, int priority, int stackSize)
{
    new simDetector(portName, maxSizeX, maxSizeY, (NDDataType_t)dataType,
                    (maxBuffers < 0) ? 0 : maxBuffers,
                    (maxMemory < 0) ? 0 : maxMemory, 
                    priority, stackSize);
    return(asynSuccess);
}

/** Code for iocsh registration */
static const iocshArg simDetectorConfigArg0 = {"Port name", iocshArgString};
static const iocshArg simDetectorConfigArg1 = {"Max X size", iocshArgInt};
static const iocshArg simDetectorConfigArg2 = {"Max Y size", iocshArgInt};
static const iocshArg simDetectorConfigArg3 = {"Data type", iocshArgInt};
static const iocshArg simDetectorConfigArg4 = {"maxBuffers", iocshArgInt};
static const iocshArg simDetectorConfigArg5 = {"maxMemory", iocshArgInt};
static const iocshArg simDetectorConfigArg6 = {"priority", iocshArgInt};
static const iocshArg simDetectorConfigArg7 = {"stackSize", iocshArgInt};
static const iocshArg * const simDetectorConfigArgs[] =  {&simDetectorConfigArg0,
                                                          &simDetectorConfigArg1,
                                                          &simDetectorConfigArg2,
                                                          &simDetectorConfigArg3,
                                                          &simDetectorConfigArg4,
                                                          &simDetectorConfigArg5,
                                                          &simDetectorConfigArg6,
                                                          &simDetectorConfigArg7};
static const iocshFuncDef configsimDetector = {"simDetectorConfig", 8, simDetectorConfigArgs};
static void configsimDetectorCallFunc(const iocshArgBuf *args)
{
    simDetectorConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival,
                      args[4].ival, args[5].ival, args[6].ival, args[7].ival);
}


static void simDetectorRegister(void)
{

    iocshRegister(&configsimDetector, configsimDetectorCallFunc);
}

extern "C" {
epicsExportRegistrar(simDetectorRegister);
}
