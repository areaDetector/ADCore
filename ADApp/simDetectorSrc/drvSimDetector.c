/* drvSimDetector.c
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

#include <asynStandardInterfaces.h>

/* Defining this will create the static table of standard parameters in ADInterface.h */
#define DEFINE_STANDARD_PARAM_STRINGS 1
#include "ADParamLib.h"
#include "ADUtils.h"

#include "ADInterface.h"
#include "asynADImage.h"
#include "drvSimDetector.h"


static char *driverName = "drvSimDetector";

/* Note that the file format enum must agree with the mbbo/mbbi records in the simDetector.template file */
typedef enum {
   SimFormatBinary,
   SimFormatASCII
} SimFormat_t;

/* If we have any private driver parameters they begin with ADFirstDriverParam and should end
   with ADLastDriverParam, which is used for setting the size of the parameter library table */
typedef enum {
   SimGainX = ADFirstDriverParam,
   SimGainY,
   SimResetImage,
   ADLastDriverParam
} SimDetParam_t;

/* The command strings are the input to ADUtils->FindParam, which returns the corresponding parameter enum value */
static ADParamString_t SimDetParamString[] = {
    {SimGainX,      "SIM_GAINX"},  
    {SimGainY,      "SIM_GAINY"},  
    {SimResetImage, "RESET_IMAGE"}  
};

#define NUM_SIM_DET_PARAMS (sizeof(SimDetParamString)/sizeof(SimDetParamString[0]))



typedef struct drvADPvt {
    /* The first set of items in this structure will be needed by all drivers */
    char *portName;
    epicsMutexId mutexId;              /* A mutex to lock access to data structures. */
    PARAMS params;
    /* asyn interfaces */
    asynStandardInterfaces asynInterfaces;
    asynInterface asynADImage;
    void *ADImageInterruptPvt;
    asynUser *pasynUser;

    /* These items are specific to the Simulator driver */
    int framesRemaining;
    epicsEventId eventId;
    void *rawBuffer;
    void *imageBuffer;
    int bufferSize;
} drvADPvt;


static int simAllocateBuffer(drvADPvt *pPvt, int sizeX, int sizeY, int dataType)
{
    int status = asynSuccess;
    int bytesPerPixel;
    int bufferSize;
    
    /* Make sure the buffers we have allocated are large enough.
     * rawBuffer is for the entire image, imageBuffer is for the subregion with binning
     * We allocated them both the same size for simplicity and efficiency */
    status |= ADUtils->bytesPerPixel(dataType, &bytesPerPixel);
    bufferSize = sizeX * sizeY * bytesPerPixel;
    if (bufferSize != pPvt->bufferSize) {
        free(pPvt->rawBuffer);
        free(pPvt->imageBuffer);
        pPvt->rawBuffer   = malloc(bytesPerPixel*sizeX*sizeY);
        pPvt->imageBuffer = malloc(bytesPerPixel*sizeX*sizeY);
        pPvt->bufferSize = bufferSize;
    }
    return(status);
}

static int simWriteFile(drvADPvt *pPvt)
{
    /* Writes current frame to disk in simple binary or ASCII format.
     * In either case the data written are imageSizeX, imageSizeY, dataType, data */
    int status = asynSuccess;
    char fullFileName[MAX_FILENAME_LEN];
    int fileFormat;
    int fileNumber;
    int imageSizeX, imageSizeY, imageSize, dataType, bytesPerPixel;
    int i, autoIncrement;
    FILE *fp;

    /* Get the current parameters */
    ADParam->getInteger(pPvt->params, ADImageSizeX, &imageSizeX);
    ADParam->getInteger(pPvt->params, ADImageSizeY, &imageSizeY);
    ADParam->getInteger(pPvt->params, ADImageSize,  &imageSize);
    ADParam->getInteger(pPvt->params, ADDataType,   &dataType);
    ADParam->getInteger(pPvt->params, ADFileNumber, &fileNumber);
    ADParam->getInteger(pPvt->params, ADAutoIncrement, &autoIncrement);
    ADUtils->bytesPerPixel(dataType, &bytesPerPixel);

    status |= ADUtils->createFileName(pPvt->params, MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
              "%s:SimWriteFile error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, fullFileName, status);
        return(status);
    }
    status |= ADParam->getInteger(pPvt->params, ADFileFormat, &fileFormat);
    switch (fileFormat) {
    case SimFormatBinary:
        fp = fopen(fullFileName, "wb");
        if (!fp) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "%s:SimWriteFile error creating file, fullFileName=%s, errno=%d\n", 
                  driverName, fullFileName, errno);
            return(asynError);
        }
        fwrite(&imageSizeX, sizeof(imageSizeX), 1, fp);
        fwrite(&imageSizeY, sizeof(imageSizeY), 1, fp);
        fwrite(&dataType, sizeof(dataType), 1, fp);
        fwrite(pPvt->imageBuffer, bytesPerPixel, imageSizeX*imageSizeY, fp);
        fclose(fp);
        break;
    case SimFormatASCII:
        fp = fopen(fullFileName, "w");
        if (!fp) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "%s:SimWriteFile error creating file, fullFileName=%s, errno=%d\n", 
                  driverName, fullFileName, errno);
            return(asynError);
        }
        fprintf(fp, "%d\n", imageSizeX);
        fprintf(fp, "%d\n", imageSizeY);
        fprintf(fp, "%d\n", dataType);
        switch (dataType) {
            case ADInt8: {
                epicsInt8 *pData = (epicsInt8 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fprintf(fp, "%d\n", pData[i]); }
                break;
            case ADUInt8: {
                epicsUInt8 *pData = (epicsUInt8 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fprintf(fp, "%u\n", pData[i]); }
                break;
            case ADInt16: {
                epicsInt16 *pData = (epicsInt16 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fprintf(fp, "%d\n", pData[i]); }
                break;
            case ADUInt16: {
                epicsUInt16 *pData = (epicsUInt16 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fprintf(fp, "%u\n", pData[i]); }
                break;
            case ADInt32: {
                epicsInt32 *pData = (epicsInt32 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fprintf(fp, "%d\n", pData[i]); }
                break;
            case ADUInt32: {
                epicsUInt32 *pData = (epicsUInt32 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fprintf(fp, "%u\n", pData[i]); }
                break;
            case ADFloat32: {
                epicsFloat32 *pData = (epicsFloat32 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fprintf(fp, "%f\n", pData[i]); }
                break;
            case ADFloat64: {
                epicsFloat64 *pData = (epicsFloat64 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fprintf(fp, "%f\n", pData[i]); }
                break;
        }
        fclose(fp);
        break;
    }

    /* If we got an error then return */
    if (status) return(status);
    
    /* Update the full file name */
    ADParam->setString(pPvt->params, ADFullFileName, fullFileName);

    /* If autoincrement is set then increment file number */
    if (autoIncrement) {
        fileNumber++;
        ADParam->setInteger(pPvt->params, ADFileNumber, fileNumber);
    }
    return(status);
}

static int simReadFile(drvADPvt *pPvt)
{
    /* Reads a file written by simWriteFile from disk in either binary or ASCII format. */
    int status = asynSuccess;
    char fullFileName[MAX_FILENAME_LEN];
    int fileFormat, fileNumber;
    int imageSizeX, imageSizeY, dataType, bytesPerPixel;
    int i, autoIncrement;
    FILE *fp;

    /* Get the current parameters */
    ADParam->getInteger(pPvt->params, ADAutoIncrement, &autoIncrement);
    ADParam->getInteger(pPvt->params, ADFileNumber,    &fileNumber);

    status |= ADUtils->createFileName(pPvt->params, MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
              "%s:SimReadFile error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, fullFileName, status);
        return(status);
    }
    status |= ADParam->getInteger(pPvt->params, ADFileFormat, &fileFormat);
    switch (fileFormat) {
    case SimFormatBinary:
        fp = fopen(fullFileName, "rb");
        if (!fp) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "%s:SimReadFile error opening file, fullFileName=%s, errno=%d\n", 
                  driverName, fullFileName, errno);
            return(asynError);
        }
        fread(&imageSizeX, sizeof(imageSizeX), 1, fp);
        fread(&imageSizeY, sizeof(imageSizeY), 1, fp);
        fread(&dataType, sizeof(dataType), 1, fp);
        ADUtils->bytesPerPixel(dataType, &bytesPerPixel);
        simAllocateBuffer(pPvt, imageSizeX, imageSizeY, dataType);
        fread(pPvt->imageBuffer, bytesPerPixel, imageSizeX*imageSizeY, fp);
        fclose(fp);
        break;
    case SimFormatASCII:
        fp = fopen(fullFileName, "r");
        if (!fp) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "%s:SimReadFile error opening file, fullFileName=%s, errno=%d\n", 
                  driverName, fullFileName, errno);
            return(asynError);
        }
        fscanf(fp, "%d", &imageSizeX);
        fscanf(fp, "%d", &imageSizeY);
        fscanf(fp, "%d", &dataType);
        simAllocateBuffer(pPvt, imageSizeX, imageSizeY, dataType);
        switch (dataType) {
            case ADInt8: {
                int tmp;
                epicsInt8 *pData = (epicsInt8 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) {fscanf(fp, "%d", &tmp); pData[i]=tmp;}}
                break;
            case ADUInt8: {
                unsigned int tmp;
                epicsUInt8 *pData = (epicsUInt8 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) {fscanf(fp, "%u", &tmp); pData[i]=tmp;}}
                break;
            case ADInt16: {
                epicsInt16 *pData = (epicsInt16 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%hd", &pData[i]); }
                break;
            case ADUInt16: {
                epicsUInt16 *pData = (epicsUInt16 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%hu", &pData[i]); }
                break;
            case ADInt32: {
                epicsInt32 *pData = (epicsInt32 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%d", &pData[i]); }
                break;
            case ADUInt32: {
                epicsUInt32 *pData = (epicsUInt32 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%u", &pData[i]); }
                break;
            case ADFloat32: {
                epicsFloat32 *pData = (epicsFloat32 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%f", &pData[i]); }
                break;
            case ADFloat64: {
                epicsFloat64 *pData = (epicsFloat64 *)pPvt->imageBuffer;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%lf", &pData[i]); }
                break;
        }
        fclose(fp);
        break;
    }

    /* If we got an error then return */
    if (status) return(status);
    
    /* Update the full file name */
    ADParam->setString(pPvt->params, ADFullFileName, fullFileName);

    /* If autoincrement is set then increment file number */
    if (autoIncrement) {
        fileNumber++;
        ADParam->setInteger(pPvt->params, ADFileNumber, fileNumber);
    }
    
    /* Update the new values of imageSizeX, imageSizeY, dataType and the image data */
    ADParam->setInteger(pPvt->params, ADImageSizeX, imageSizeX);
    ADParam->setInteger(pPvt->params, ADImageSizeY, imageSizeY);
    ADParam->setInteger(pPvt->params, ADDataType, dataType);
    ADUtils->ADImageCallback(pPvt->ADImageInterruptPvt, 
                             pPvt->imageBuffer,
                             dataType, imageSizeX, imageSizeY);
    
    return(status);
}

static int simComputeImage(drvADPvt *pPvt)
{
    int status = asynSuccess;
    int dataType;
    int binX, binY, minX, minY, sizeX, sizeY, maxSizeX, maxSizeY, resetImage;
    int imageSizeX, imageSizeY, imageSize, bytesPerPixel;
    double exposureTime, gain, gainX, gainY, scaleX, scaleY;
    double increment;
    int i, j;

    /* NOTE: The caller of this function must have taken the mutex */
    
    status |= ADParam->getInteger(pPvt->params, ADBinX,        &binX);
    status |= ADParam->getInteger(pPvt->params, ADBinY,        &binY);
    status |= ADParam->getInteger(pPvt->params, ADMinX,        &minX);
    status |= ADParam->getInteger(pPvt->params, ADMinY,        &minY);
    status |= ADParam->getInteger(pPvt->params, ADSizeX,       &sizeX);
    status |= ADParam->getInteger(pPvt->params, ADSizeY,       &sizeY);
    status |= ADParam->getInteger(pPvt->params, ADMaxSizeX,    &maxSizeX);
    status |= ADParam->getInteger(pPvt->params, ADMaxSizeY,    &maxSizeY);
    status |= ADParam->getInteger(pPvt->params, ADDataType,    &dataType);
    status |= ADParam->getInteger(pPvt->params, SimResetImage, &resetImage);
    status |= ADParam->getDouble (pPvt->params, ADAcquireTime, &exposureTime);
    status |= ADParam->getDouble (pPvt->params, ADGain,        &gain);
    status |= ADParam->getDouble (pPvt->params, SimGainX,      &gainX);
    status |= ADParam->getDouble (pPvt->params, SimGainY,      &gainY);
    status |= ADUtils->bytesPerPixel(dataType, &bytesPerPixel);

    /* Make sure parameters are consistent, fix them if they are not */
    if (binX < 0) {binX = 0; status |= ADParam->setInteger(pPvt->params, ADBinX, binX);}
    if (binY < 0) {binY = 0; status |= ADParam->setInteger(pPvt->params, ADBinY, binY);}
    if (minX < 0) {minX = 0; status |= ADParam->setInteger(pPvt->params, ADMinX, minX);}
    if (minY < 0) {minY = 0; status |= ADParam->setInteger(pPvt->params, ADMinY, minY);}
    if (minX > maxSizeX-1) {minX = maxSizeX-1; status |= ADParam->setInteger(pPvt->params, ADMinX, minX);}
    if (minY > maxSizeY-1) {minY = maxSizeY-1; status |= ADParam->setInteger(pPvt->params, ADMinY, minY);}
    if (minX+sizeX > maxSizeX) {sizeX = maxSizeX-minX; status |= ADParam->setInteger(pPvt->params, ADSizeX, sizeX);}
    if (minY+sizeY > maxSizeY) {sizeY = maxSizeY-minY; status |= ADParam->setInteger(pPvt->params, ADSizeY, sizeY);}

    /* Make sure the buffers we have allocated are large enough. */
    status |= simAllocateBuffer(pPvt, maxSizeX, maxSizeY, dataType);

    /* The intensity at each pixel[i,j] is:
     * (i * gainX + j* gainY) + frameCounter * gain * exposureTime * 1000. */
    increment = gain * exposureTime * 1000.;
    scaleX = 0.;
    scaleY = 0.;
    
    /* The following macro simplifies the code */

    #define COMPUTE_ARRAY(DATA_TYPE) {                      \
        DATA_TYPE *pData = (DATA_TYPE *)pPvt->rawBuffer; \
        DATA_TYPE inc = (DATA_TYPE)increment;               \
        if (resetImage) {                                   \
            for (i=0; i<maxSizeY; i++) {                    \
                scaleX = 0.;                                \
                for (j=0; j<maxSizeX; j++) {                \
                    (*pData++) = (DATA_TYPE)(scaleX + scaleY + inc);  \
                    scaleX += gainX;                        \
                }                                           \
                scaleY += gainY;                            \
            }                                               \
        } else {                                            \
            for (i=0; i<maxSizeY; i++) {                    \
                for (j=0; j<maxSizeX; j++) {                \
                     *pData++ += inc;                       \
                }                                           \
            }                                               \
        }                                                   \
    }
        

    switch (dataType) {
        case ADInt8: 
            COMPUTE_ARRAY(epicsInt8);
            break;
        case ADUInt8: 
            COMPUTE_ARRAY(epicsUInt8);
            break;
        case ADInt16: 
            COMPUTE_ARRAY(epicsInt16);
            break;
        case ADUInt16: 
            COMPUTE_ARRAY(epicsUInt16);
            break;
        case ADInt32: 
            COMPUTE_ARRAY(epicsInt32);
            break;
        case ADUInt32: 
            COMPUTE_ARRAY(epicsUInt32);
            break;
        case ADFloat32: 
            COMPUTE_ARRAY(epicsFloat32);
            break;
        case ADFloat64: 
            COMPUTE_ARRAY(epicsFloat64);
            break;
    }
    
    /* Extract the region of interest with binning.  
     * If the entire image is being used (no ROI or binning) that's OK because
     * convertImage detects that case and is very efficient */
    status |= ADUtils->convertImage(pPvt->rawBuffer, 
                                    dataType,
                                    maxSizeX, maxSizeY,
                                    pPvt->imageBuffer,
                                    dataType,
                                    binX, binY,
                                    minX, minY,
                                    sizeX, sizeY,
                                    &imageSizeX, &imageSizeY);
    
    imageSize = imageSizeX * imageSizeY * bytesPerPixel;
    status |= ADParam->setInteger(pPvt->params, ADImageSize,  imageSize);
    status |= ADParam->setInteger(pPvt->params, ADImageSizeX, imageSizeX);
    status |= ADParam->setInteger(pPvt->params, ADImageSizeY, imageSizeY);
    status |= ADParam->setInteger(pPvt->params,SimResetImage, 0);
    return(status);
}

static void simTask(drvADPvt *pPvt)
{
    /* This thread computes new frame data and does the callbacks to send it to higher layers */
    int status = asynSuccess;
    int dataType;
    int imageSizeX, imageSizeY, imageSize;
    int frameCounter;
    int acquire, autoSave;
    ADStatus_t acquiring;
    double acquireTime, acquirePeriod, delay;
    epicsTimeStamp startTime, endTime;
    double computeTime;

    /* Loop forever */
    while (1) {
    
        epicsMutexLock(pPvt->mutexId);

        /* Is acquisition active? */
        ADParam->getInteger(pPvt->params, ADAcquire, &acquire);
        
        /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquire) {
            ADParam->setInteger(pPvt->params, ADStatus, ADStatusIdle);
            ADParam->callCallbacks(pPvt->params);
            /* Release the lock while we wait for an event that says acquire has started, then lock again */
            epicsMutexUnlock(pPvt->mutexId);
            asynPrint(pPvt->pasynUser, ASYN_TRACE_FLOW, 
                "%s:simTask: waiting for acquire to start\n", driverName);
            status = epicsEventWait(pPvt->eventId);
            epicsMutexLock(pPvt->mutexId);
        }
        
        /* We are acquiring. */
        /* Get the current time */
        epicsTimeGetCurrent(&startTime);
        
        acquiring = ADStatusAcquire;
        ADParam->setInteger(pPvt->params, ADStatus, acquiring);
        
        /* Update the image */
        simComputeImage(pPvt);
        
        /* Get the current parameters */
        ADParam->getInteger(pPvt->params, ADImageSizeX, &imageSizeX);
        ADParam->getInteger(pPvt->params, ADImageSizeY, &imageSizeY);
        ADParam->getInteger(pPvt->params, ADImageSize,  &imageSize);
        ADParam->getInteger(pPvt->params, ADDataType,   &dataType);
        ADParam->getInteger(pPvt->params, ADAutoSave,   &autoSave);
        ADParam->getInteger(pPvt->params, ADFrameCounter, &frameCounter);
        frameCounter++;
        ADParam->setInteger(pPvt->params, ADFrameCounter, frameCounter);

        /* Call the imageData callback */
        asynPrint(pPvt->pasynUser, ASYN_TRACE_FLOW, 
             "%s:simTask: calling imageData callback\n", driverName);
        ADUtils->ADImageCallback(pPvt->ADImageInterruptPvt, 
                                 pPvt->imageBuffer,
                                 dataType, imageSizeX, imageSizeY);

        /* See if acquisition is done */
        if (pPvt->framesRemaining > 0) pPvt->framesRemaining--;
        if (pPvt->framesRemaining == 0) {
            acquiring = ADStatusIdle;
            ADParam->setInteger(pPvt->params, ADAcquire, acquiring);
            asynPrint(pPvt->pasynUser, ASYN_TRACE_FLOW, 
                  "%s:simTask: acquisition completed\n", driverName);
        }
        
        /* If autosave is enabled then save the file */
        if (autoSave) simWriteFile(pPvt);
        
        /* Call the callbacks to update any changes */
        ADParam->callCallbacks(pPvt->params);
        
        ADParam->getDouble(pPvt->params, ADAcquireTime, &acquireTime);
        ADParam->getDouble(pPvt->params, ADAcquirePeriod, &acquirePeriod);
        
        /* We are done accessing data structures, release the lock */
        epicsMutexUnlock(pPvt->mutexId);
        
        /* If we are acquiring then wait for the larger of the exposure time or the exposure period,
           minus the time we have already spent computing this image. */
        if (acquiring) {
            epicsTimeGetCurrent(&endTime);
            delay = acquireTime;
            computeTime = epicsTimeDiffInSeconds(&endTime, &startTime);
            if (acquirePeriod > delay) delay = acquirePeriod;
            delay -= computeTime;
            asynPrint(pPvt->pasynUser, ASYN_TRACE_FLOW, 
                  "%s:simTask: computeTime=%f, delay=%f\n",
                  driverName, computeTime, delay);            
            if (delay > 0) status = epicsEventWaitWithTimeout(pPvt->eventId, delay);
        }
    }
}


/* asynInt32 interface functions */
static asynStatus readInt32(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 *value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(pPvt->mutexId);
    
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getInteger(pPvt->params, function, value);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
                  "%s:readInt32 error, status=%d function=%d, value=%d\n", 
                  driverName, status, function, *value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readInt32: function=%d, value=%d\n", 
              driverName, function, *value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeInt32(void *drvPvt, asynUser *pasynUser, 
                             epicsInt32 value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int reset=0;

    epicsMutexLock(pPvt->mutexId);

    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= ADParam->setInteger(pPvt->params, function, value);

    /* For a real detector this is where the parameter is sent to the hardware */
    switch (function) {
    case ADAcquire:
        if (value) {
            /* We need to set the number of frames we expect to collect, so the frame callback function
               can know when acquisition is complete.  We need to find out what mode we are in and how
               many frames have been requested.  If we are in continuous mode then set the number of
               remaining frames to -1. */
            int frameMode, numFrames;
            status |= ADParam->getInteger(pPvt->params, ADFrameMode, &frameMode);
            status |= ADParam->getInteger(pPvt->params, ADNumFrames, &numFrames);
            switch(frameMode) {
            case ADFrameSingle:
                pPvt->framesRemaining = 1;
                break;
            case ADFrameMultiple:
                pPvt->framesRemaining = numFrames;
                break;
            case ADFrameContinuous:
                pPvt->framesRemaining = -1;
                break;
            }
            reset = 1;
            /* Send an event to wake up the simulation task.  
             * It won't actually start generating new images until we release the lock below */
            epicsEventSignal(pPvt->eventId);
        } 
        break;
    case ADBinX:
    case ADBinY:
    case ADMinX:
    case ADMinY:
    case ADSizeX:
    case ADSizeY:
    case ADDataType:
        reset = 1;
        break;
    case SimResetImage:
        if (value) reset = 1;
        break;
    case ADFrameMode: 
        /* The frame mode may have changed while we are acquiring, 
         * set the frames remaining appropriately. */
        switch (value) {
        case ADFrameSingle:
            pPvt->framesRemaining = 1;
            break;
        case ADFrameMultiple: {
            int numFrames;
            ADParam->getInteger(pPvt->params, ADNumFrames, &numFrames);
            pPvt->framesRemaining = numFrames; }
            break;
        case ADFrameContinuous:
            pPvt->framesRemaining = -1;
            break;
        }
        break;
    case ADWriteFile:
        status = simWriteFile(pPvt);
        break; 
    case ADReadFile:
        status = simReadFile(pPvt);
        break; 
    }
    
    /* Reset the image if the reset flag was set above */
    if (reset) {
        status |= ADParam->setInteger(pPvt->params, SimResetImage, 1);
        /* Compute the image when parameters change.  
         * This won't post data, but will cause any parameter changes to be computed and readbacks to update.
         * Don't compute the image if this is an accquire command, since that will be done next. */
        if (function != ADAcquire) simComputeImage(pPvt);
    }
    
    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(pPvt->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeInt32 error, status=%d function=%d, value=%d\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeInt32: function=%d, value=%d\n", 
              driverName, function, value);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


static asynStatus getBounds(void *drvPvt, asynUser *pasynUser,
                            epicsInt32 *low, epicsInt32 *high)
{
    /* This is only needed for the asynInt32 interface when the device uses raw units.
       Our interface is using engineering units. */
    *low = 0;
    *high = 65535;
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s::getBounds,low=%d, high=%d\n", driverName, *low, *high);
    return(asynSuccess);
}


/* asynFloat64 interface methods */
static asynStatus readFloat64(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 *value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    
    epicsMutexLock(pPvt->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getDouble(pPvt->params, function, value);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:readFloat64 error, status=%d function=%d, value=%f\n", 
              driverName, status, function, *value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readFloat64: function=%d, value=%f\n", 
              driverName, function, *value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeFloat64(void *drvPvt, asynUser *pasynUser, 
                               epicsFloat64 value)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(pPvt->mutexId);

    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= ADParam->setDouble(pPvt->params, function, value);

    /* Changing any of the following parameters requires recomputing the base image */
    switch (function) {
    case ADAcquireTime:
    case ADGain:
    case SimGainX:
    case SimGainY:
        status |= ADParam->setInteger(pPvt->params, SimResetImage, 1);
        /* Compute the image.  This won't post data, but will cause any readbacks to update */
        simComputeImage(pPvt);
        break;
    }

    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(pPvt->params);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeFloat64 error, status=%d function=%d, value=%f\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeFloat64: function=%d, value=%f\n", 
              driverName, function, value);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


/* asynOctet interface methods */
static asynStatus readOctet(void *drvPvt, asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
   
    epicsMutexLock(pPvt->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = ADParam->getString(pPvt->params, function, maxChars, value);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:readOctet error, status=%d function=%d, value=%s\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readOctet: function=%d, value=%s\n", 
              driverName, function, value);
    *eomReason = ASYN_EOM_END;
    *nActual = strlen(value);
    epicsMutexUnlock(pPvt->mutexId);
    return(status);
}

static asynStatus writeOctet(void *drvPvt, asynUser *pasynUser,
                             const char *value, size_t nChars, size_t *nActual)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(pPvt->mutexId);
    /* Set the parameter in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= ADParam->setString(pPvt->params, function, (char *)value);
    /* Do callbacks so higher layers see any changes */
    ADParam->callCallbacks(pPvt->params);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeOctet error, status=%d function=%d, value=%s\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeOctet: function=%d, value=%s\n", 
              driverName, function, value);
    *nActual = nChars;
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

/* asynADImage interface methods */
static asynStatus readADImage(void *drvPvt, asynUser *pasynUser, void *data, int maxBytes,
                       int *dataType, int *nx, int *ny)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int imageSize, bytesPerPixel;
    int status = asynSuccess;
    
    epicsMutexLock(pPvt->mutexId);
    status |= ADParam->getInteger(pPvt->params, ADImageSizeX, nx);
    status |= ADParam->getInteger(pPvt->params, ADImageSizeY, ny);
    status |= ADParam->getInteger(pPvt->params, ADDataType, dataType);
    status |= ADUtils->bytesPerPixel(*dataType, &bytesPerPixel);
    imageSize = bytesPerPixel * *nx * *ny;
    if (imageSize > maxBytes) imageSize = maxBytes;
    memcpy(data, pPvt->imageBuffer, imageSize);
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:readADImage error, status=%d maxBytes=%d, data=%p\n", 
              driverName, status, maxBytes, data);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:readADImage error, maxBytes=%d, data=%p\n", 
              driverName, maxBytes, data);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

static asynStatus writeADImage(void *drvPvt, asynUser *pasynUser, void *data,
                        int dataType, int nx, int ny)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    int status = asynSuccess;
    
    if (pPvt == NULL) return asynError;
    epicsMutexLock(pPvt->mutexId);

    /* The simDetector does not allow downloading image data */    
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
          "%s:ADSetImage not currently supported\n", driverName);
    status = asynError;
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}


/* asynDrvUser routines */
static asynStatus drvUserCreate(void *drvPvt, asynUser *pasynUser,
                                const char *drvInfo, 
                                const char **pptypeName, size_t *psize)
{
    int status;
    int param;

    /* See if this is one of the standard parameters */
    status = ADUtils->findParam(ADStandardParamString, NUM_AD_STANDARD_PARAMS, 
                                drvInfo, &param);
                                
    /* If we did not find it in that table try our driver-specific table */
    if (status) status = ADUtils->findParam(SimDetParamString, NUM_SIM_DET_PARAMS, 
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
                  "%s::drvUserCreate, drvInfo=%s, param=%d\n", 
                  driverName, drvInfo, param);
        return(asynSuccess);
    } else {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "%s::drvUserCreate, unknown drvInfo=%s", 
                     driverName, drvInfo);
        return(asynError);
    }
}
    
static asynStatus drvUserGetType(void *drvPvt, asynUser *pasynUser,
                                 const char **pptypeName, size_t *psize)
{
    /* This is not currently supported, because we can't get the strings for driver-specific commands */

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::drvUserGetType entered",
              driverName);
    *pptypeName = NULL;
    *psize = 0;
    return(asynError);
}

static asynStatus drvUserDestroy(void *drvPvt, asynUser *pasynUser)
{
    /* Nothing to do because we did not allocate any resources */
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::drvUserDestroy, drvPvt=%p, pasynUser=%p\n",
              driverName, drvPvt, pasynUser);
    return(asynSuccess);
}


/* asynCommon interface methods */

static asynStatus connect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionConnect(pasynUser);

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
          "%s::connect, pasynUser=%p\n", 
          driverName, pasynUser);
    return(asynSuccess);
}


static asynStatus disconnect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionDisconnect(pasynUser);
    return(asynSuccess);
}

static void report(void *drvPvt, FILE *fp, int details)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;

    fprintf(fp, "Simulation detector %s\n", pPvt->portName);
    if (details > 0) {
        int nx, ny, dataType;
        ADParam->getInteger(pPvt->params, ADSizeX, &nx);
        ADParam->getInteger(pPvt->params, ADSizeY, &ny);
        ADParam->getInteger(pPvt->params, ADDataType, &dataType);
        fprintf(fp, "  NX, NY:            %d  %d\n", nx, ny);
        fprintf(fp, "  Data type:         %d\n", dataType);
    }
    if (details > 5) {
        fprintf(fp, "\nParameter library contents:\n");
        ADParam->dump(pPvt->params);
    }
}



/* Structures with function pointers for each of the asyn interfaces */
static asynCommon ifaceCommon = {
    report,
    connect,
    disconnect
};

static asynInt32 ifaceInt32 = {
    writeInt32,
    readInt32,
    getBounds
};

static asynFloat64 ifaceFloat64 = {
    writeFloat64,
    readFloat64
};

static asynOctet ifaceOctet = {
    writeOctet,
    NULL,
    readOctet,
};

static asynDrvUser ifaceDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};

static asynADImage ifaceADImage = {
    writeADImage,
    readADImage
};

int simDetectorConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType)

{
    drvADPvt *pPvt;
    int status = asynSuccess;
    char *functionName = "simDetectorConfig";
    asynStandardInterfaces *pInterfaces;

    pPvt = callocMustSucceed(1, sizeof(*pPvt), functionName);
    pPvt->portName = epicsStrDup(portName);

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register port\n", functionName);
        return(asynError);
    }

    /* Create asynUser for debugging */
    pPvt->pasynUser = pasynManager->createAsynUser(0, 0);

    pInterfaces = &pPvt->asynInterfaces;
    
    /* Initialize interface pointers */
    pInterfaces->common.pinterface        = (void *)&ifaceCommon;
    pInterfaces->drvUser.pinterface       = (void *)&ifaceDrvUser;
    pInterfaces->octet.pinterface         = (void *)&ifaceOctet;
    pInterfaces->int32.pinterface         = (void *)&ifaceInt32;
    pInterfaces->float64.pinterface       = (void *)&ifaceFloat64;

    /* Define which interfaces can generate interrupts */
    pInterfaces->octetCanInterrupt        = 1;
    pInterfaces->int32CanInterrupt        = 1;
    pInterfaces->float64CanInterrupt      = 1;

    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     pPvt->pasynUser, pPvt);
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register interfaces: %s.\n",
               functionName, pPvt->pasynUser->errorMessage);
        return(asynError);
    }
    
    /* Register the asynADImage interface */
    pPvt->asynADImage.interfaceType = asynADImageType;
    pPvt->asynADImage.pinterface = (void *)&ifaceADImage;
    pPvt->asynADImage.drvPvt = pPvt;
    status = pasynADImageBase->initialize(portName, &pPvt->asynADImage);
    if (status != asynSuccess) {
        printf("%s: Can't register asynADImage\n", functionName);
        return(asynError);
    }
    status = pasynManager->registerInterruptSource(portName, &pPvt->asynADImage,
                                                   &pPvt->ADImageInterruptPvt);
    if (status != asynSuccess) {
        printf("%s: Can't register asynADImage interrupt\n", functionName);
        return(asynError);
    }

    /* Create the epicsMutex for locking access to data structures from other threads */
    pPvt->mutexId = epicsMutexCreate();
    if (!pPvt->mutexId) {
        printf("%s: epicsMutexCreate failure\n", functionName);
        return asynError;
    }
    
    /* Create the epicsEvent for signaling to the simulate task when acquisition starts */
    pPvt->eventId = epicsEventCreate(epicsEventEmpty);
    if (!pPvt->eventId) {
        printf("%s: epicsEventCreate failure\n", functionName);
        return asynError;
    }
    
    /* Initialize the parameter library */
    pPvt->params = ADParam->create(0, ADLastDriverParam, &pPvt->asynInterfaces);
    if (!pPvt->params) {
        printf("%s: unable to create parameter library\n", functionName);
        return asynError;
    }
    
    /* Use the utility library to set some defaults */
    status = ADUtils->setParamDefaults(pPvt->params);
    
    /* Set some default values for parameters */
    status =  ADParam->setString (pPvt->params, ADManufacturer, "Simulated detector");
    status |= ADParam->setString (pPvt->params, ADModel, "Basic simulator");
    status |= ADParam->setInteger(pPvt->params, ADMaxSizeX, maxSizeX);
    status |= ADParam->setInteger(pPvt->params, ADMaxSizeY, maxSizeY);
    status |= ADParam->setInteger(pPvt->params, ADSizeX, maxSizeX);
    status |= ADParam->setInteger(pPvt->params, ADSizeY, maxSizeY);
    status |= ADParam->setInteger(pPvt->params, ADImageSizeX, maxSizeX);
    status |= ADParam->setInteger(pPvt->params, ADImageSizeY, maxSizeY);
    status |= ADParam->setInteger(pPvt->params, ADDataType, dataType);
    status |= ADParam->setInteger(pPvt->params, ADFrameMode, ADFrameContinuous);
    status |= ADParam->setDouble (pPvt->params, ADAcquireTime, .001);
    status |= ADParam->setDouble (pPvt->params, ADAcquirePeriod, .005);
    status |= ADParam->setInteger(pPvt->params, ADNumFrames, 100);
    status |= ADParam->setInteger(pPvt->params, SimResetImage, 1);
    status |= ADParam->setDouble (pPvt->params, SimGainX, 1);
    status |= ADParam->setDouble (pPvt->params, SimGainY, 1);
    if (status) {
        printf("%s: unable to set camera parameters\n", functionName);
        return asynError;
    }
    
    /* Create the thread that updates the images */
    status = (epicsThreadCreate("SimDetTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)simTask,
                                pPvt) == NULL);
    if (status) {
        printf("%s: epicsThreadCreate failure for image task\n", functionName);
        return asynError;
    }

    /* Compute the first image */
    simComputeImage(pPvt);
    
    return asynSuccess;
}
