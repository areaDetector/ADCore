/*
 * drvADFile.c
 * 
 * Asyn driver for callbacks to save area detector data to files.
 *
 * Author: Mark Rivers
 *
 * Created April 5, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsMessageQueue.h>
#include <cantProceed.h>

#include <asynStandardInterfaces.h>

#define DEFINE_STANDARD_PARAM_STRINGS 1
#include "ADInterface.h"
#include "NDArrayBuff.h"
#include "ADParamLib.h"
#include "ADUtils.h"
#include "asynHandle.h"
#include "drvADFile.h"

#define driverName "drvADFile"

/* Note that the file format enum must agree with the mbbo/mbbi records in the ADFile.template file */
typedef enum {
    ADFileFormatBinary,
    ADFileFormatASCII
} ADFileFormat_t;

typedef enum {
    ADFileModeSingle,
    ADFileModeCapture,
    ADFileModeStream
} ADFileMode_t;

typedef enum
{
    ADFileImagePort           /* (asynOctet,    r/w) The port for the ADImage interface */
      = ADFirstDriverParam,
    ADFileImageAddr,          /* (asynInt32,    r/w) The address on the port */
    ADFileMinWriteTime,       /* (asynFloat64,  r/w) Minimum time between file writes */
    ADFileDroppedImages,      /* (asynInt32,    r/w) Number of dropped images */
    ADFileBlockingCallbacks,  /* (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
    ADFileWriteMode,           /* (asynInt32,    r/w) File saving mode (ADFileMode_t) */
    ADFileNumCapture,         /* (asynInt32,    r/w) Number of images to capture */
    ADFileNumCaptured,        /* (asynInt32,    r/w) Number of images already captured */
    ADFileCapture,            /* (asynInt32,    r/w) Start or stop capturing images */
    ADFileLastDriverParam
} ADFileParam_t;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t ADFileParamString[] = {
    {ADFileImagePort,         "IMAGE_PORT" },
    {ADFileImageAddr,         "IMAGE_ADDR" },
    {ADFileMinWriteTime,      "FILE_MIN_TIME" },
    {ADFileDroppedImages,     "DROPPED_IMAGES" },
    {ADFileBlockingCallbacks, "BLOCKING_CALLBACKS" },
    {ADFileWriteMode,         "WRITE_MODE" },
    {ADFileNumCapture,        "NUM_CAPTURE" },
    {ADFileNumCaptured,       "NUM_CAPTURED" },
    {ADFileCapture,           "CAPTURE" },
};

#define NUM_AD_FILE_PARAMS (sizeof(ADFileParamString)/sizeof(ADFileParamString[0]))

typedef struct drvADPvt {
    /* These fields will be needed by most asyn plug-in drivers */
    char *portName;
    epicsMutexId mutexId;
    epicsMessageQueueId msgQId;
    PARAMS params;
    NDArray_t *pImage;

    /* The asyn interfaces this driver implements */
    asynStandardInterfaces asynStdInterfaces;
    
    /* The asyn interfaces we access as a client */
    asynHandle *pasynHandle;
    void *asynHandlePvt;
    void *asynHandleInterruptPvt;
    asynUser *pasynUserHandle;
    
    /* asynUser connected to ourselves for asynTrace */
    asynUser *pasynUser;
    
    /* These fields are specific to the ADFile driver */
    epicsTimeStamp lastFileWriteTime;
    FILE *fp;
    char *capturePointer;
    char *captureBuffer;
    NDArray_t *pCapture;
} drvADPvt;


/* Local functions, not in any interface */

static int ADFileWriteData(drvADPvt *pPvt, char *fullFileName, int fileFormat, 
                           int nframes, NDArray_t *pImage, int close)
{
    /* Writes current frame to disk in simple binary or ASCII format.
     * In either case the data written are ndims, dims, datatype, data.
     
     * This function can do 3 things:
     *  -   write files containing single images
     *  -   write files containing multiple images from a single */
    int status = asynSuccess;
    NDArrayInfo_t arrayInfo;
    int i;
    const char* functionName = "ADFileWriteFile";

    epicsMutexUnlock(pPvt->mutexId);
    
    switch (fileFormat) {
    case ADFileFormatBinary:
        if (fullFileName) {
            if (!pPvt->fp) pPvt->fp = fopen(fullFileName, "wb");
            if (!pPvt->fp) {
                asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                      "%s:%s error creating file, fullFileName=%s, errno=%d\n", 
                      driverName, functionName, fullFileName, errno);
                goto done;
            }
            rewind(pPvt->fp); 
            fwrite(&nframes, sizeof(nframes), 1, pPvt->fp);
        }
        if (pImage) {
            NDArrayBuff->getInfo(pImage, &arrayInfo);
            fwrite(&pImage->ndims, sizeof(pImage->ndims), 1, pPvt->fp);
            for (i=0; i<pImage->ndims; i++) fwrite(&pImage->dims[i].size, 
                sizeof(pImage->dims[i].size), 1, pPvt->fp);
            fwrite(&pImage->dataType, sizeof(pImage->dataType), 1, pPvt->fp);
            fwrite(pImage->pData, arrayInfo.totalBytes, 1, pPvt->fp);
        }
        if (close) {
            fclose(pPvt->fp);
            pPvt->fp = 0;
        }
        break;
    case ADFileFormatASCII:
        if (fullFileName) {
            pPvt->fp = fopen(fullFileName, "w");
            if (!pPvt->fp) {
                asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                      "%s:%s error creating file, fullFileName=%s, errno=%d\n", 
                      driverName, functionName, fullFileName, errno);
                goto done;
            }
            fprintf(pPvt->fp, "%d\n", nframes);
        }
        if (pImage) {
            NDArrayBuff->getInfo(pImage, &arrayInfo);
            fprintf(pPvt->fp, "%d\n", pImage->ndims);
            for (i=0; i<pImage->ndims; i++) fprintf(pPvt->fp, "%d\n", pImage->dims[i].size);
            fprintf(pPvt->fp, "%d\n", pImage->dataType);
            switch (pImage->dataType) {
                case NDInt8: {
                    epicsInt8 *pData = (epicsInt8 *)pImage->pData;
                    for (i=0; i<arrayInfo.nElements; i++) fprintf(pPvt->fp, "%d\n", pData[i]); }
                    break;
                case NDUInt8: {
                    epicsUInt8 *pData = (epicsUInt8 *)pImage->pData;
                    for (i=0; i<arrayInfo.nElements; i++) fprintf(pPvt->fp, "%u\n", pData[i]); }
                    break;
                case NDInt16: {
                    epicsInt16 *pData = (epicsInt16 *)pImage->pData;
                    for (i=0; i<arrayInfo.nElements; i++) fprintf(pPvt->fp, "%d\n", pData[i]); }
                    break;
                case NDUInt16: {
                    epicsUInt16 *pData = (epicsUInt16 *)pImage->pData;
                    for (i=0; i<arrayInfo.nElements; i++) fprintf(pPvt->fp, "%u\n", pData[i]); }
                    break;
                case NDInt32: {
                    epicsInt32 *pData = (epicsInt32 *)pImage->pData;
                    for (i=0; i<arrayInfo.nElements; i++) fprintf(pPvt->fp, "%d\n", pData[i]); }
                    break;
                case NDUInt32: {
                    epicsUInt32 *pData = (epicsUInt32 *)pImage->pData;
                    for (i=0; i<arrayInfo.nElements; i++) fprintf(pPvt->fp, "%u\n", pData[i]); }
                    break;
                case NDFloat32: {
                    epicsFloat32 *pData = (epicsFloat32 *)pImage->pData;
                    for (i=0; i<arrayInfo.nElements; i++) fprintf(pPvt->fp, "%f\n", pData[i]); }
                    break;
                case NDFloat64: {
                    epicsFloat64 *pData = (epicsFloat64 *)pImage->pData;
                    for (i=0; i<arrayInfo.nElements; i++) fprintf(pPvt->fp, "%f\n", pData[i]); }
                    break;
            }
        }
        if (close) {
            fclose(pPvt->fp);
            pPvt->fp = 0;
        }
        break;
    }
    done:
    epicsMutexLock(pPvt->mutexId);
    return(status);
}

static int ADFileReadFile(drvADPvt *pPvt)
{
    /* Reads a file written by ADFileWriteFile from disk in either binary or ASCII format. */
    int status = asynSuccess;
    char fullFileName[MAX_FILENAME_LEN];
    int fileFormat, fileNumber;
    int imageSizeX, imageSizeY, dataType;
    int i, autoIncrement;
    int ndims=2;
    int dims[2];
    NDArray_t *pImage=NULL;
    FILE *fp;
    const char* functionName = "ADFileReadFile";

    /* Get the current parameters */
    ADParam->getInteger(pPvt->params, ADAutoIncrement, &autoIncrement);
    ADParam->getInteger(pPvt->params, ADFileNumber,    &fileNumber);

    status |= ADUtils->createFileName(pPvt->params, MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, functionName, fullFileName, status);
        return(status);
    }
    status |= ADParam->getInteger(pPvt->params, ADFileFormat, &fileFormat);
    switch (fileFormat) {
    case ADFileFormatBinary:
        fp = fopen(fullFileName, "rb");
        if (!fp) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error opening file, fullFileName=%s, errno=%d\n", 
                  driverName, functionName, fullFileName, errno);
            return(asynError);
        }
        fread(&imageSizeX, sizeof(imageSizeX), 1, fp);
        fread(&imageSizeY, sizeof(imageSizeY), 1, fp);
        fread(&dataType, sizeof(dataType), 1, fp);
        dims[0] = imageSizeX;
        dims[1] = imageSizeY;
        pImage = NDArrayBuff->alloc(ndims, dims, dataType, 0, NULL);
        if (!pImage) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error allocating buffer\n", 
                  driverName, functionName);
            return(asynError);
        }
        fread(pImage->pData, 1, pImage->dataSize, fp);
        fclose(fp);
        break;
    case ADFileFormatASCII:
        fp = fopen(fullFileName, "r");
        if (!fp) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error opening file, fullFileName=%s, errno=%d\n", 
                  driverName, functionName, fullFileName, errno);
            return(asynError);
        }
        fscanf(fp, "%d", &imageSizeX);
        fscanf(fp, "%d", &imageSizeY);
        fscanf(fp, "%d", &dataType);
        dims[0] = imageSizeX;
        dims[1] = imageSizeY;
        pImage = NDArrayBuff->alloc(ndims, dims, dataType, 0, NULL);
        if (!pImage) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR, 
                  "%s:%s error allocating buffer\n", 
                  driverName, functionName);
            return(asynError);
        }
        switch (dataType) {
            case NDInt8: {
                int tmp;
                epicsInt8 *pData = (epicsInt8 *)pImage->pData;
                for (i=0; i<imageSizeX*imageSizeY; i++) {fscanf(fp, "%d", &tmp); pData[i]=tmp;}}
                break;
            case NDUInt8: {
                unsigned int tmp;
                epicsUInt8 *pData = (epicsUInt8 *)pImage->pData;
                for (i=0; i<imageSizeX*imageSizeY; i++) {fscanf(fp, "%u", &tmp); pData[i]=tmp;}}
                break;
            case NDInt16: {
                epicsInt16 *pData = (epicsInt16 *)pImage->pData;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%hd", &pData[i]); }
                break;
            case NDUInt16: {
                epicsUInt16 *pData = (epicsUInt16 *)pImage->pData;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%hu", &pData[i]); }
                break;
            case NDInt32: {
                epicsInt32 *pData = (epicsInt32 *)pImage->pData;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%d", &pData[i]); }
                break;
            case NDUInt32: {
                epicsUInt32 *pData = (epicsUInt32 *)pImage->pData;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%u", &pData[i]); }
                break;
            case NDFloat32: {
                epicsFloat32 *pData = (epicsFloat32 *)pImage->pData;
                for (i=0; i<imageSizeX*imageSizeY; i++) fscanf(fp, "%f", &pData[i]); }
                break;
            case NDFloat64: {
                epicsFloat64 *pData = (epicsFloat64 *)pImage->pData;
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
    
    /* Call any registered clients */
    ADUtils->handleCallback(pPvt->asynStdInterfaces.handleInterruptPvt, pImage);

    /* Set the last image to be this one */
    NDArrayBuff->release(pPvt->pImage);
    pPvt->pImage = pImage;    
    
    return(status);
}

static int ADFileWriteFile(drvADPvt *pPvt) 
{
    int status = asynSuccess;
    int close;
    int fileWriteMode, capture;
    int fileNumber, autoIncrement, fileFormat;
    char fullFileName[MAX_FILENAME_LEN];
    int numCapture, numCaptured;
    int fileOpenComplete = 0;
    const char* functionName = "ADFileWriteFile";

    /* Make sure there is a valid image */
    if (!pPvt->pImage) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must collect an image to get dimensions first\n",
            driverName, functionName);
        return(asynError);
    }
    
    ADParam->getInteger(pPvt->params, ADFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(pPvt->params, ADFileNumber, &fileNumber);
    ADParam->getInteger(pPvt->params, ADAutoIncrement, &autoIncrement);
    ADParam->getInteger(pPvt->params, ADFileFormat, &fileFormat);
    ADParam->getInteger(pPvt->params, ADFileCapture, &capture);    
    ADParam->getInteger(pPvt->params, ADFileCapture, &numCapture);    
    ADParam->getInteger(pPvt->params, ADFileNumCaptured, &numCaptured);

    switch(fileWriteMode) {
        case ADFileModeSingle:
            status = ADUtils->createFileName(pPvt->params, sizeof(fullFileName), fullFileName);
            close = 1;
            status = ADFileWriteData(pPvt, fullFileName, fileFormat, 1, pPvt->pImage, close);
            if (status == asynSuccess) fileOpenComplete = 1;
            break;
        case ADFileModeCapture:
            if (numCaptured > 0) {
                status = ADUtils->createFileName(pPvt->params, sizeof(fullFileName), fullFileName);
                pPvt->pCapture->dims[pPvt->pCapture->ndims-1].size = numCaptured;
                close = 1;
                status = ADFileWriteData(pPvt, fullFileName, fileFormat, 1, pPvt->pCapture, close);
                free(pPvt->captureBuffer);
                pPvt->captureBuffer = NULL;
                pPvt->pCapture->pData = NULL;
                pPvt->pCapture->dataSize = 0;
                NDArrayBuff->release(pPvt->pCapture);
                ADParam->setInteger(pPvt->params, ADFileNumCaptured, 0);
                if (status == asynSuccess) fileOpenComplete = 1;
            }
            break;
        case ADFileModeStream:
            if (capture) {
                if (numCaptured == 0) {
                    /* Streaming was just started, write the header only.  We set the number of frames
                     * to the maximum, this will be changed at the end */
                    status = ADUtils->createFileName(pPvt->params, sizeof(fullFileName), fullFileName);
                    close = 0;
                    status = ADFileWriteData(pPvt, fullFileName, fileFormat, 
                                    numCapture, NULL, close);
                    if (status == asynSuccess) fileOpenComplete = 1;
                } else {
                    /* Streaming  is in progress */
                    close = 0;
                    status = ADFileWriteData(pPvt, NULL, fileFormat, 
                                    1, pPvt->pImage, close);
                }
            } else {
                /* Capture is complete.  Re-write the header and close the file */
                close = 1;
                status = ADFileWriteData(pPvt, "unused", fileFormat, 
                                    numCaptured, NULL, close);
            }
            break;
        default:
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s:%s: ERROR, unknown fileWriteMode %d\n", 
                driverName, functionName, fileWriteMode);
            break;
    }

    if (fileOpenComplete) {
        ADParam->setString(pPvt->params, ADFullFileName, fullFileName);
        /* If autoincrement is set then increment file number */
        if (autoIncrement) {
            fileNumber++;
            ADParam->setInteger(pPvt->params, ADFileNumber, fileNumber);
        }
    }
    return(status);
}

static int ADFileDoCapture(drvADPvt *pPvt) 
{
    /* This function is called from write32 whenever capture is started or stopped */
    int status = asynSuccess;
    int fileWriteMode, capture;
    NDArray_t array;
    NDArrayInfo_t arrayInfo;
    int numCapture, ndims, dims[ND_ARRAY_MAX_DIMS], i;
    const char* functionName = "ADFileDoCapture";
    
    ADParam->getInteger(pPvt->params, ADFileCapture, &capture);    
    ADParam->getInteger(pPvt->params, ADFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(pPvt->params, ADFileNumCapture, &numCapture);

    switch(fileWriteMode) {
        case ADFileModeSingle:
            /* It is an error to set capture=1 in this mode, set to 0 */
            ADParam->setInteger(pPvt->params, ADFileCapture, 0);
            break;
        case ADFileModeCapture:
            if (capture) {
                /* Capturing was just started */
                /* We need to read an image from our image source to get its dimensions */
                array.dataSize = 0;
                status = pPvt->pasynHandle->read(pPvt->asynHandlePvt,pPvt->pasynUserHandle, &array);
                ADParam->setInteger(pPvt->params, ADFileNumCaptured, 0);
                NDArrayBuff->getInfo(&array, &arrayInfo);
                /* The dataSize is this value times numCapture */
                arrayInfo.totalBytes = arrayInfo.totalBytes * numCapture;
                free(pPvt->captureBuffer);
                pPvt->captureBuffer = malloc(arrayInfo.totalBytes);
                if (!pPvt->captureBuffer) {
                    asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                        "%s:%s ERROR: cannot allocate capture buffer\n",
                        driverName, functionName);
                    status = asynError;
                }
                ndims = array.ndims;
                for (i=0; i<ndims; i++) dims[i] = array.dims[i].size;
                dims[ndims++] = numCapture;
                pPvt->pCapture = NDArrayBuff->alloc(ndims, dims, array.dataType, 
                                                    arrayInfo.totalBytes, pPvt->captureBuffer);
                if (!pPvt->pCapture) {
                    asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                        "%s:%s ERROR: cannot allocate capture array\n",
                        driverName, functionName);
                    status = asynError;
                }
                pPvt->capturePointer = pPvt->captureBuffer;
            } else {
                /* Stop capturing, nothing to do, setting the parameter is all that is needed */
            }
            break;
        case ADFileModeStream:
            if (capture) {
                /* Streaming was just started */
                ADParam->setInteger(pPvt->params, ADFileNumCaptured, 0);
                ADFileWriteFile(pPvt);
            } else {
                /* Streaming was just stopped */
                ADFileWriteFile(pPvt);
            }
    }
    return(status);
}

static int ADProcessFile(drvADPvt *pPvt, NDArray_t *pImage)
{
    int fileWriteMode, autoSave, capture;
    int imageCounter;
    int status=asynSuccess;
    NDArrayInfo_t arrayInfo;
    int numCapture, numCaptured;

    ADParam->getInteger(pPvt->params, ADAutoSave, &autoSave);
    ADParam->getInteger(pPvt->params, ADFileCapture, &capture);    
    ADParam->getInteger(pPvt->params, ADFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(pPvt->params, ADImageCounter, &imageCounter);    
    ADParam->getInteger(pPvt->params, ADFileNumCapture, &numCapture);    
    ADParam->getInteger(pPvt->params, ADFileNumCaptured, &numCaptured); 

    /* We always keep the last image so read() can use it.  Release it now */
    if (pPvt->pImage) NDArrayBuff->release(pPvt->pImage);
    pPvt->pImage = pImage;
    
    switch(fileWriteMode) {
        case ADFileModeSingle:
            if (autoSave) {
                imageCounter++;
                status = ADFileWriteFile(pPvt);
            }
            break;
        case ADFileModeCapture:
            if (capture) {
                imageCounter++;
                if (numCaptured < numCapture) {
                    NDArrayBuff->getInfo(pImage, &arrayInfo);
                    memcpy(pPvt->capturePointer, pImage->pData, arrayInfo.totalBytes);
                    pPvt->capturePointer += arrayInfo.totalBytes;
                    numCaptured++;
                    ADParam->setInteger(pPvt->params, ADFileNumCaptured, numCaptured);
                } 
                if (numCaptured == numCapture) {
                    capture = 0;
                    ADParam->setInteger(pPvt->params, ADFileCapture, capture);
                    if (autoSave) {
                        status = ADFileWriteFile(pPvt);
                    }
                }
            }
            break;
        case ADFileModeStream:
            if (capture) {
                imageCounter++;
                numCaptured++;
                ADParam->setInteger(pPvt->params, ADFileNumCaptured, numCaptured);
                status = ADFileWriteFile(pPvt);
                if (numCaptured == numCapture) {
                    capture = 0;
                    ADParam->setInteger(pPvt->params, ADFileCapture, capture);
                    /* Calling ADFileDoCapture with capture=0 will write the final
                     * number of frames in the header */
                    ADFileDoCapture(pPvt);
                }
            }
            break;
    }

    /* Update the parameters.  */
    ADParam->setInteger(pPvt->params, ADImageSizeX, pImage->dims[0].size);
    ADParam->setInteger(pPvt->params, ADImageSizeY, pImage->dims[1].size);
    ADParam->setInteger(pPvt->params, ADDataType, pImage->dataType);
    ADParam->setInteger(pPvt->params, ADImageCounter, imageCounter);    
    ADParam->callCallbacks(pPvt->params);
    return(status);
}


static void ADFileCallback(void *drvPvt, asynUser *pasynUser, void *handle)
{
    /* This callback function is called from the detector driver when a new image arrives.
     * It writes the image in a disk file.
     * It can either do the callbacks directly (if BlockingCallbacks=1) or by queueing
     * the images to be processed by a background task (if BlockingCallbacks=0).
     * In the latter case images can be dropped if the queue is full.
     */
     
    drvADPvt *pPvt = drvPvt;
    NDArray_t *pImage = handle;
    epicsTimeStamp tNow;
    double minFileWriteTime, deltaTime;
    int status;
    int fileWriteMode, autoSave, capture;
    int blockingCallbacks;
    int imageCounter, droppedImages;
    char *functionName = "ADFileCallback";

    epicsMutexLock(pPvt->mutexId);

    status |= ADParam->getDouble(pPvt->params, ADFileMinWriteTime, &minFileWriteTime);
    status |= ADParam->getInteger(pPvt->params, ADImageCounter, &imageCounter);
    status |= ADParam->getInteger(pPvt->params, ADFileDroppedImages, &droppedImages);
    status |= ADParam->getInteger(pPvt->params, ADFileBlockingCallbacks, &blockingCallbacks);
    status |= ADParam->getInteger(pPvt->params, ADAutoSave, &autoSave);
    status |= ADParam->getInteger(pPvt->params, ADFileCapture, &capture);    
    status |= ADParam->getInteger(pPvt->params, ADFileWriteMode, &fileWriteMode);    
    
    /* Quickly see if there if we need to do anything */
    if (((fileWriteMode == ADFileModeSingle) && !autoSave) ||
        ((fileWriteMode == ADFileModeCapture) && !capture) ||
        ((fileWriteMode == ADFileModeCapture) && !capture)) goto done;
        
    epicsTimeGetCurrent(&tNow);
    deltaTime = epicsTimeDiffInSeconds(&tNow, &pPvt->lastFileWriteTime);

    if (deltaTime > minFileWriteTime) {  
        /* Time to write the next file */
        
        /* The callbacks can operate in 2 modes: blocking or non-blocking.
         * If blocking we call ADImageDoCallbacks directly, executing them
         * in the detector callback thread.
         * If non-blocking we put the image on the queue and it executes
         * in our background thread. */
        /* We always keep the last image so read() can use it.  Release it now */
        /* The callbacks can operate in 2 modes: blocking or non-blocking.
         * If blocking we call ADImageDoCallbacks directly, executing them
         * in the detector callback thread.
         * If non-blocking we put the image on the queue and it executes
         * in our background thread. */
        NDArrayBuff->reserve(pImage);
        /* Update the time we last posted an image */
        epicsTimeGetCurrent(&tNow);
        memcpy(&pPvt->lastFileWriteTime, &tNow, sizeof(tNow));
        if (blockingCallbacks) {
            ADProcessFile(pPvt, pImage);
        } else {
            /* Increase the reference count again on this image
             * It will be released in the background task when processing is done */
            NDArrayBuff->reserve(pImage);
            /* Try to put this image on the message queue.  If there is no room then return
             * immediately. */
            status = epicsMessageQueueTrySend(pPvt->msgQId, &pImage, sizeof(&pImage));
            if (status) {
                asynPrint(pasynUser, ASYN_TRACE_FLOW, 
                    "%s:%s message queue full, dropped image %d\n",
                    driverName, functionName, imageCounter);
                droppedImages++;
                status |= ADParam->setInteger(pPvt->params, ADFileDroppedImages, droppedImages);
                /* This buffer needs to be released twice, because it never made it onto the queue where
                 * it would be released later */
                NDArrayBuff->release(pImage);
                NDArrayBuff->release(pImage);
            }
        }
    }
    ADParam->callCallbacks(pPvt->params);
    done:
    epicsMutexUnlock(pPvt->mutexId);
}



static void ADFileTask(drvADPvt *pPvt)
{
    /* This thread writes the data when a new image arrives */

    /* Loop forever */
    NDArray_t *pImage;
    
    while (1) {
        /* Wait for an image to arrive from the queue */    
        epicsMessageQueueReceive(pPvt->msgQId, &pImage, sizeof(&pImage));
        
        /* Take the lock.  The function we are calling must release the lock
         * during time-consuming operations when it does not need it. */
        epicsMutexLock(pPvt->mutexId);
        /* Call the function that does the callbacks to standard asyn interfaces */
        ADProcessFile(pPvt, pImage); 
        epicsMutexUnlock(pPvt->mutexId); 
        
        /* We are done with this image buffer */       
        NDArrayBuff->release(pImage);
    }
}

static int setImageInterrupt(drvADPvt *pPvt, int connect)
{
    int status = asynSuccess;
    const char *functionName = "setImageInterrupt";
    
    /* Lock the port.  May not be necessary to do this. */
    status = pasynManager->lockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock image port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        return(status);
    }
    if (connect) {
        status = pPvt->pasynHandle->registerInterruptUser(
                    pPvt->asynHandlePvt, pPvt->pasynUserHandle,
                    ADFileCallback, pPvt, &pPvt->asynHandleInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't register for interrupt callbacks on detector port: %s\n",
                driverName, functionName, pPvt->pasynUserHandle->errorMessage);
            return(status);
        }
    } else {
        status = pPvt->pasynHandle->cancelInterruptUser(pPvt->asynHandlePvt, 
                        pPvt->pasynUserHandle, pPvt->asynHandleInterruptPvt);
        if (status != asynSuccess) {
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Can't unregister for interrupt callbacks on detector port: %s\n",
                driverName, functionName, pPvt->pasynUserHandle->errorMessage);
            return(status);
        }
    }
    /* Unlock the port.  May not be necessary to do this. */
    status = pasynManager->unlockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock image port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        return(status);
    }
    return(asynSuccess);
}

static int connectToImagePort(drvADPvt *pPvt)
{
    asynStatus status;
    asynInterface *pasynInterface;
    NDArray_t image;
    int isConnected;
    char imagePort[20];
    int imageAddr;
    const char *functionName = "connectToImagePort";

    ADParam->getString(pPvt->params, ADFileImagePort, sizeof(imagePort), imagePort);
    ADParam->getInteger(pPvt->params, ADFileImageAddr, &imageAddr);
    status = pasynManager->isConnected(pPvt->pasynUserHandle, &isConnected);
    if (status) isConnected=0;

    /* If we are currently connected cancel interrupt request */    
    if (isConnected) {
        status = setImageInterrupt(pPvt, 0);
    }
    
    /* Disconnect the image port from our asynUser.  Ignore error if there is no device
     * currently connected. */
    pasynManager->exceptionCallbackRemove(pPvt->pasynUserHandle);
    pasynManager->disconnect(pPvt->pasynUserHandle);

    /* Connect to the image port driver */
    status = pasynManager->connectDevice(pPvt->pasynUserHandle, imagePort, imageAddr);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::%s ERROR: Can't connect to image port %s address %d: %s\n",
                  driverName, functionName, imagePort, imageAddr, pPvt->pasynUserHandle->errorMessage);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return (status);
    }

    /* Find the asynHandle interface in that driver */
    pasynInterface = pasynManager->findInterface(pPvt->pasynUserHandle, asynHandleType, 1);
    if (!pasynInterface) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::connectToPort ERROR: Can't find asynHandle interface on image port %s address %d\n",
                  driverName, imagePort, imageAddr);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return(asynError);
    }
    pPvt->pasynHandle = pasynInterface->pinterface;
    pPvt->asynHandlePvt = pasynInterface->drvPvt;
    pasynManager->exceptionConnect(pPvt->pasynUser);

    /* Read the current image parameters from the image driver */
    /* Lock the port. Defintitely necessary to do this. */
    status = pasynManager->lockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock image port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        return(status);
    }
    /* Read the current image, but only request 0 bytes so no data are actually transferred */
    image.dataSize = 0;
    status = pPvt->pasynHandle->read(pPvt->asynHandlePvt,pPvt->pasynUserHandle, &image);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: reading image data:%s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
    } else {
        ADParam->setInteger(pPvt->params, ADImageSizeX, image.dims[0].size);
        ADParam->setInteger(pPvt->params, ADImageSizeY, image.dims[1].size);
        ADParam->setInteger(pPvt->params, ADDataType, image.dataType);
        ADParam->callCallbacks(pPvt->params);
    }
    /* Unlock the port.  Definitely necessary to do this. */
    status = pasynManager->unlockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock image port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
    }
    
    /* Enable interrupt callbacks */
    status = setImageInterrupt(pPvt, 1);

    return(status);
}   



/* asynInt32 interface methods */
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
        asynPrint(pPvt->pasynUser, ASYN_TRACEIO_DRIVER, 
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
    const char* functionName = "writeInt32";

    epicsMutexLock(pPvt->mutexId);

    /* Set the parameter in the parameter library. */
    status |= ADParam->setInteger(pPvt->params, function, value);

    switch(function) {
        case ADFileImageAddr:
            connectToImagePort(pPvt);
            break;
        case ADWriteFile:
            if (pPvt->pImage) {
                status = ADFileWriteFile(pPvt);
            } else {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                    "%s:%s: ERROR, no valid image to write",
                    driverName, functionName);
                status = asynError;
            }
            break;
        case ADReadFile:
            status = ADFileReadFile(pPvt);
            break;
        case ADFileCapture:
            status = ADFileDoCapture(pPvt);
            break;
        default:
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%d\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
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
              "%s::getBounds,low=%d, high=%d\n", 
              driverName, *low, *high);
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
    const char* functionName = "writeFloat64";

    epicsMutexLock(pPvt->mutexId);

    /* Set the parameter in the parameter library. */
    status |= ADParam->setDouble(pPvt->params, function, value);

    switch(function) {
        /* We don't currently need to do anything special when these functions are received */
        default:
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
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
    /* Set the parameter in the parameter library. */
    status |= ADParam->setString(pPvt->params, function, (char *)value);

    switch(function) {
        case ADFileImagePort:
            connectToImagePort(pPvt);
        default:
            break;
    }
    
     /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);

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

/* asynHandle interface methods */
static asynStatus readADImage(void *drvPvt, asynUser *pasynUser, void *handle)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    NDArray_t *pImage = handle;
    NDArrayInfo_t arrayInfo;
    int status = asynSuccess;
    const char* functionName = "readADImage";
    
    epicsMutexLock(pPvt->mutexId);
    if (!pPvt->pImage) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:functionName error, no valid image available\n", 
              driverName, functionName);
        status = asynError;
    } else {
        pImage->ndims = pPvt->pImage->ndims;
        memcpy(pImage->dims, pPvt->pImage->dims, sizeof(pImage->dims));
        pImage->dataType = pPvt->pImage->dataType;
        NDArrayBuff->getInfo(pPvt->pImage, &arrayInfo);
        if (arrayInfo.totalBytes > pImage->dataSize) arrayInfo.totalBytes = pImage->dataSize;
        memcpy(pImage->pData, pPvt->pImage->pData, arrayInfo.totalBytes);
    }
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d pData=%p\n", 
              driverName, functionName, status, pImage->pData);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s error, maxBytes=%d, data=%p\n", 
              driverName, functionName, arrayInfo.totalBytes, pImage->pData);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

static asynStatus writeADImage(void *drvPvt, asynUser *pasynUser, void *handle)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    NDArray_t *pImage = handle;
    int status = asynSuccess;
    
    if (pPvt == NULL) return asynError;
    epicsMutexLock(pPvt->mutexId);
    
    pPvt->pImage = pImage;
    ADParam->setInteger(pPvt->params, ADFileWriteMode, ADFileModeSingle);

    status = ADFileWriteFile(pPvt);

    /* Do callbacks so higher layers see any changes */
    status |= ADParam->callCallbacks(pPvt->params);
    
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}




/* asynDrvUser interface methods */
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
    if (status) status = ADUtils->findParam(ADFileParamString, NUM_AD_FILE_PARAMS, 
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
              "%s::drvUserGetType entered", driverName);

    *pptypeName = NULL;
    *psize = 0;
    return(asynError);
}

static asynStatus drvUserDestroy(void *drvPvt, asynUser *pasynUser)
{
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s::drvUserDestroy, drvPvt=%p, pasynUser=%p\n",
              driverName, drvPvt, pasynUser);

    return(asynSuccess);
}


/* asynCommon interface methods */

static void report(void *drvPvt, FILE *fp, int details)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    interruptNode *pnode;
    ELLLIST *pclientList;
    asynStandardInterfaces *pInterfaces = &pPvt->asynStdInterfaces;

    fprintf(fp, "Port: %s\n", pPvt->portName);
    if (details >= 1) {
        /* Report int32 interrupts */
        pasynManager->interruptStart(pInterfaces->int32InterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynInt32Interrupt *pInterrupt = pnode->drvPvt;
            fprintf(fp, "    int32 callback client address=%p, addr=%d, reason=%d\n",
                    pInterrupt->callback, pInterrupt->addr, 
                    pInterrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pInterfaces->int32InterruptPvt);

        /* Report float64 interrupts */
        pasynManager->interruptStart(pInterfaces->float64InterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynFloat64Interrupt *pInterrupt = pnode->drvPvt;
            fprintf(fp, "    float64 callback client address=%p, addr=%d, reason=%d\n",
                    pInterrupt->callback, pInterrupt->addr, 
                    pInterrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pInterfaces->float64InterruptPvt);

        /* Report asynOctet interrupts */
        pasynManager->interruptStart(pInterfaces->octetInterruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            asynOctetInterrupt *pInterrupt = pnode->drvPvt;
            fprintf(fp, "    octet callback client address=%p, addr=%d, reason=%d\n",
                    pInterrupt->callback, pInterrupt->addr, 
                    pInterrupt->pasynUser->reason);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(pInterfaces->octetInterruptPvt);

    }
    if (details > 5) {
        NDArrayBuff->report(details);
        ADParam->dump(pPvt->params);
    }
}

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

static asynHandle ifaceHandle = {
    writeADImage,
    readADImage
};

static asynDrvUser ifaceDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};


/* Configuration routine.  Called directly, or from the iocsh function in drvADFileEpics */

int drvADFileConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                       const char *imagePort, int imageAddr)
{
    drvADPvt *pPvt;
    asynStatus status;
    char *functionName = "drvADFileConfigure";
    asynStandardInterfaces *pInterfaces;
    asynUser *pasynUser;

    pPvt = callocMustSucceed(1, sizeof(*pPvt), functionName);
    pPvt->portName = epicsStrDup(portName);

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register port\n", functionName);
        return -1;
    }

    /* Create asynUser for debugging and for standardInterfacesBase */
    pPvt->pasynUser = pasynManager->createAsynUser(0, 0);

    /* Set addresses of asyn interfaces */
    pInterfaces = &pPvt->asynStdInterfaces;
    
    pInterfaces->common.pinterface        = (void *)&ifaceCommon;
    pInterfaces->drvUser.pinterface       = (void *)&ifaceDrvUser;
    pInterfaces->octet.pinterface         = (void *)&ifaceOctet;
    pInterfaces->int32.pinterface         = (void *)&ifaceInt32;
    pInterfaces->float64.pinterface       = (void *)&ifaceFloat64;
    pInterfaces->handle.pinterface        = (void *)&ifaceHandle;

    /* Define which interfaces can generate interrupts */
    pInterfaces->octetCanInterrupt        = 1;
    pInterfaces->int32CanInterrupt        = 1;
    pInterfaces->float64CanInterrupt      = 1;
    pInterfaces->handleCanInterrupt       = 1;

    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     pPvt->pasynUser, pPvt);
    if (status != asynSuccess) {
        printf("%s ERROR: Can't register interfaces: %s.\n",
                functionName, pPvt->pasynUser->errorMessage);
        return -1;
    }
    
    /* Connect to our device for asynTrace */
    status = pasynManager->connectDevice(pPvt->pasynUser, portName, 0);
    if (status != asynSuccess) {
        printf("%s, connectDevice failed\n", functionName);
        return -1;
    }

    /* Create asynUser for communicating with image port */
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->userPvt = pPvt;
    pPvt->pasynUserHandle = pasynUser;

    /* Create the epicsMutex for locking access to data structures from other threads */
    pPvt->mutexId = epicsMutexCreate();
    if (!pPvt->mutexId) {
        printf("%s: epicsMutexCreate failure\n", functionName);
        return asynError;
    }

    /* Create the message queue for the input images */
    pPvt->msgQId = epicsMessageQueueCreate(queueSize, sizeof(NDArray_t*));
    if (!pPvt->msgQId) {
        printf("%s: epicsMessageQueueCreate failure\n", functionName);
        return asynError;
    }
    
    /* Initialize the parameter library */
    pPvt->params = ADParam->create(0, ADFileLastDriverParam, &pPvt->asynStdInterfaces);
    if (!pPvt->params) {
        printf("%s: unable to create parameter library\n", functionName);
        return asynError;
    }
    
   /* Create the thread that does the image callbacks */
    status = (epicsThreadCreate("ADFileTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)ADFileTask,
                                pPvt) == NULL);
    if (status) {
        printf("%s: epicsThreadCreate failure\n", functionName);
        return asynError;
    }

    /* Set the initial values of some parameters */
    ADParam->setInteger(pPvt->params, ADImageCounter, 0);
    ADParam->setInteger(pPvt->params, ADFileDroppedImages, 0);
    ADParam->setString (pPvt->params, ADFileImagePort, imagePort);
    ADParam->setInteger(pPvt->params, ADFileImageAddr, imageAddr);
    ADParam->setInteger(pPvt->params, ADFileBlockingCallbacks, 0);
    ADParam->setInteger(pPvt->params, ADFileCapture, 0);
    
    /* Try to connect to the image port */
    status = connectToImagePort(pPvt);
    
    return asynSuccess;
}

