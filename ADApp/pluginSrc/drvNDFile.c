/*
 * drvNDFile.c
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
#include "drvNDFile.h"

#include "NDFileNetCDF.h"

#define driverName "drvNDFile"

/* Note that the file format enum must agree with the mbbo/mbbi records in the NDFile.template file */
typedef enum {
    NDFileFormatNetCDF,
} NDFileFormat_t;

typedef enum {
    NDFileModeSingle,
    NDFileModeCapture,
    NDFileModeStream
} NDFileMode_t;

typedef enum
{
    NDFileImagePort           /* (asynOctet,    r/w) The port for the ADImage interface */
      = ADFirstDriverParam,
    NDFileImageAddr,          /* (asynInt32,    r/w) The address on the port */
    NDFileMinWriteTime,       /* (asynFloat64,  r/w) Minimum time between file writes */
    NDFileDroppedImages,      /* (asynInt32,    r/w) Number of dropped images */
    NDFileBlockingCallbacks,  /* (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
    NDFileWriteMode,           /* (asynInt32,    r/w) File saving mode (NDFileMode_t) */
    NDFileNumCapture,         /* (asynInt32,    r/w) Number of images to capture */
    NDFileNumCaptured,        /* (asynInt32,    r/w) Number of images already captured */
    NDFileCapture,            /* (asynInt32,    r/w) Start or stop capturing images */
    NDFileLastDriverParam
} NDFileParam_t;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t NDFileParamString[] = {
    {NDFileImagePort,         "IMAGE_PORT" },
    {NDFileImageAddr,         "IMAGE_ADDR" },
    {NDFileMinWriteTime,      "FILE_MIN_TIME" },
    {NDFileDroppedImages,     "DROPPED_IMAGES" },
    {NDFileBlockingCallbacks, "BLOCKING_CALLBACKS" },
    {NDFileWriteMode,         "WRITE_MODE" },
    {NDFileNumCapture,        "NUM_CAPTURE" },
    {NDFileNumCaptured,       "NUM_CAPTURED" },
    {NDFileCapture,           "CAPTURE" },
};

#define NUM_AD_FILE_PARAMS (sizeof(NDFileParamString)/sizeof(NDFileParamString[0]))

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
    
    /* These fields are specific to the NDFile driver */
    epicsTimeStamp lastFileWriteTime;
    NDArray_t *pCaptureNext;
    NDArray_t *pCapture;
    NDFileNetCDFState_t netCDFState;
    epicsMutexId fileMutexId;
} drvADPvt;


/* Local functions, not in any interface */

static int NDFileReadFile(drvADPvt *pPvt)
{
    /* Reads a file written by NDFileWriteFile from disk in either binary or ASCII format. */
    int status = asynSuccess;
    char fullFileName[MAX_FILENAME_LEN];
    int fileFormat, fileNumber;
    int imageSizeX, imageSizeY, dataType;
    int autoIncrement;
    NDArray_t *pImage=NULL;
    const char* functionName = "NDFileReadFile";

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
    case NDFileFormatNetCDF:
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

static int NDFileWriteFile(drvADPvt *pPvt) 
{
    int status = asynSuccess;
    int fileWriteMode, capture;
    int fileNumber, autoIncrement, fileFormat;
    char fullFileName[MAX_FILENAME_LEN];
    int numCapture, numCaptured;
    int i, numArrays, append, close;
    int fileOpenComplete = 0;
    const char* functionName = "NDFileWriteFile";

    /* Make sure there is a valid image */
    if (!pPvt->pImage) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must collect an image to get dimensions first\n",
            driverName, functionName);
        return(asynError);
    }
    
    ADParam->getInteger(pPvt->params, NDFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(pPvt->params, ADFileNumber, &fileNumber);
    ADParam->getInteger(pPvt->params, ADAutoIncrement, &autoIncrement);
    ADParam->getInteger(pPvt->params, ADFileFormat, &fileFormat);
    ADParam->getInteger(pPvt->params, NDFileCapture, &capture);    
    ADParam->getInteger(pPvt->params, NDFileCapture, &numCapture);    
    ADParam->getInteger(pPvt->params, NDFileNumCaptured, &numCaptured);

    /* We unlock the overall mutex here because we want the callbacks to be able to queue new
     * frames without waiting while we write files here.  The only restriction is that the
     * callbacks must not modify any part of the pPvt structure that we use here.
     * However, we need to take a mutex on file I/O because manually stopping stream can
     * result in a call to this function, and that needs to block. */
    epicsMutexLock(pPvt->fileMutexId);
    epicsMutexUnlock(pPvt->mutexId);
    
    switch(fileWriteMode) {
        case NDFileModeSingle:
            status = ADUtils->createFileName(pPvt->params, sizeof(fullFileName), fullFileName);
            switch(fileFormat) {
                case NDFileFormatNetCDF:
                    close = 1;
                    numArrays = 1;
                    append = 0;
                    status = NDFileWriteNetCDF(fullFileName, &pPvt->netCDFState, pPvt->pImage, numArrays, append, close);
                    if (status == asynSuccess) fileOpenComplete = 1;
                    break;
            }
            break;
        case NDFileModeCapture:
            if (numCaptured > 0) {
            status = ADUtils->createFileName(pPvt->params, sizeof(fullFileName), fullFileName);
               switch(fileFormat) {
                    case NDFileFormatNetCDF:
                        close = 1;
                        append = 0;
                        numArrays = numCaptured;
                        status = NDFileWriteNetCDF(fullFileName, &pPvt->netCDFState, pPvt->pCapture, numArrays, append, close);
                        if (status == asynSuccess) fileOpenComplete = 1;
                        break;
                }
                /* Free all the buffer memory we allocated */
                for (i=0; i<numCapture; i++) free(pPvt->pCapture[i].pData);
                free(pPvt->pCapture);
                pPvt->pCapture = NULL;
                ADParam->setInteger(pPvt->params, NDFileNumCaptured, 0);
            }
            break;
        case NDFileModeStream:
            if (capture) {
                if (numCaptured == 1) {
                    switch(fileFormat) {
                        case NDFileFormatNetCDF:
                            /* Streaming was just started, write the header plus the first frame */
                            status = ADUtils->createFileName(pPvt->params, sizeof(fullFileName), fullFileName);
                            close = 0;
                            append = 0;
                            numArrays = -1;
                            status = NDFileWriteNetCDF(fullFileName, &pPvt->netCDFState, pPvt->pImage, numArrays, append, close);
                            if (status == asynSuccess) fileOpenComplete = 1;
                            break;
                    }
                } else {
                    switch(fileFormat) {
                        case NDFileFormatNetCDF:
                            /* Streaming  is in progress */
                            close = 0;
                            append = 1;
                            numArrays = 1;
                            status = NDFileWriteNetCDF(NULL, &pPvt->netCDFState, pPvt->pImage, numArrays, append, close);
                            break;
                    }
                }
            } else {
                if (numCaptured > 0) {
                    /* Capture is complete, close the file */
                    switch(fileFormat) {
                        case NDFileFormatNetCDF:
                            close = 1;
                            append = 1;
                            numArrays = 0;
                            status = NDFileWriteNetCDF(NULL, &pPvt->netCDFState, pPvt->pImage, numArrays, append, close);
                            break;
                    }
                }
            }
            break;
        default:
            asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                "%s:%s: ERROR, unknown fileWriteMode %d\n", 
                driverName, functionName, fileWriteMode);
            break;
    }
    epicsMutexUnlock(pPvt->fileMutexId);
    epicsMutexLock(pPvt->mutexId);
    
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

static int NDFileDoCapture(drvADPvt *pPvt) 
{
    /* This function is called from write32 whenever capture is started or stopped */
    int status = asynSuccess;
    int fileWriteMode, capture;
    NDArray_t array;
    NDArrayInfo_t arrayInfo;
    int i;
    int numCapture;
    const char* functionName = "NDFileDoCapture";
    
    ADParam->getInteger(pPvt->params, NDFileCapture, &capture);    
    ADParam->getInteger(pPvt->params, NDFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(pPvt->params, NDFileNumCapture, &numCapture);

    switch(fileWriteMode) {
        case NDFileModeSingle:
            /* It is an error to set capture=1 in this mode, set to 0 */
            ADParam->setInteger(pPvt->params, NDFileCapture, 0);
            break;
        case NDFileModeCapture:
            if (capture) {
                /* Capturing was just started */
                /* We need to read an image from our image source to get its dimensions */
                array.dataSize = 0;
                status = pPvt->pasynHandle->read(pPvt->asynHandlePvt,pPvt->pasynUserHandle, &array);
                ADParam->setInteger(pPvt->params, NDFileNumCaptured, 0);
                NDArrayBuff->getInfo(&array, &arrayInfo);
                pPvt->pCapture = malloc(numCapture * sizeof(NDArray_t));
                if (!pPvt->pCapture) {
                    asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                        "%s:%s ERROR: cannot allocate capture buffers\n",
                        driverName, functionName);
                    status = asynError;
                }
                for (i=0; i<numCapture; i++) {
                    memcpy(&pPvt->pCapture[i], &array, sizeof(array));
                    pPvt->pCapture[i].dataSize = arrayInfo.totalBytes;
                    pPvt->pCapture[i].pData = malloc(arrayInfo.totalBytes);
                    if (!pPvt->pCapture[i].pData) {
                        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                            "%s:%s ERROR: cannot allocate capture array for buffer %s\n",
                            driverName, functionName, i);
                        status = asynError;
                    }
                }
                pPvt->pCaptureNext = pPvt->pCapture;
            } else {
                /* Stop capturing, nothing to do, setting the parameter is all that is needed */
            }
            break;
        case NDFileModeStream:
            if (capture) {
                /* Streaming was just started */
                ADParam->setInteger(pPvt->params, NDFileNumCaptured, 0);
            } else {
                /* Streaming was just stopped */
                status = NDFileWriteFile(pPvt);
                ADParam->setInteger(pPvt->params, NDFileNumCaptured, 0);
            }
    }
    return(status);
}

static int NDProcessFile(drvADPvt *pPvt, NDArray_t *pImage)
{
    int fileWriteMode, autoSave, capture;
    int imageCounter;
    int status=asynSuccess;
    int numCapture, numCaptured;

    ADParam->getInteger(pPvt->params, ADAutoSave, &autoSave);
    ADParam->getInteger(pPvt->params, NDFileCapture, &capture);    
    ADParam->getInteger(pPvt->params, NDFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(pPvt->params, ADImageCounter, &imageCounter);    
    ADParam->getInteger(pPvt->params, NDFileNumCapture, &numCapture);    
    ADParam->getInteger(pPvt->params, NDFileNumCaptured, &numCaptured); 

    /* We always keep the last image so read() can use it.  Release it now */
    if (pPvt->pImage) NDArrayBuff->release(pPvt->pImage);
    pPvt->pImage = pImage;
    
    switch(fileWriteMode) {
        case NDFileModeSingle:
            if (autoSave) {
                imageCounter++;
                status = NDFileWriteFile(pPvt);
            }
            break;
        case NDFileModeCapture:
            if (capture) {
                imageCounter++;
                if (numCaptured < numCapture) {
                    NDArrayBuff->copy(pPvt->pCaptureNext++, pImage);
                    numCaptured++;
                    ADParam->setInteger(pPvt->params, NDFileNumCaptured, numCaptured);
                } 
                if (numCaptured == numCapture) {
                    capture = 0;
                    ADParam->setInteger(pPvt->params, NDFileCapture, capture);
                    if (autoSave) {
                        status = NDFileWriteFile(pPvt);
                    }
                }
            }
            break;
        case NDFileModeStream:
            if (capture) {
                imageCounter++;
                numCaptured++;
                ADParam->setInteger(pPvt->params, NDFileNumCaptured, numCaptured);
                status = NDFileWriteFile(pPvt);
                if (numCaptured == numCapture) {
                    capture = 0;
                    ADParam->setInteger(pPvt->params, NDFileCapture, capture);
                    status = NDFileWriteFile(pPvt);
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


static void NDFileCallback(void *drvPvt, asynUser *pasynUser, void *handle)
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
    char *functionName = "NDFileCallback";

    epicsMutexLock(pPvt->mutexId);

    status |= ADParam->getDouble(pPvt->params, NDFileMinWriteTime, &minFileWriteTime);
    status |= ADParam->getInteger(pPvt->params, ADImageCounter, &imageCounter);
    status |= ADParam->getInteger(pPvt->params, NDFileDroppedImages, &droppedImages);
    status |= ADParam->getInteger(pPvt->params, NDFileBlockingCallbacks, &blockingCallbacks);
    status |= ADParam->getInteger(pPvt->params, ADAutoSave, &autoSave);
    status |= ADParam->getInteger(pPvt->params, NDFileCapture, &capture);    
    status |= ADParam->getInteger(pPvt->params, NDFileWriteMode, &fileWriteMode);    
    
    /* Quickly see if there if we need to do anything */
    if (((fileWriteMode == NDFileModeSingle) && !autoSave) ||
        ((fileWriteMode == NDFileModeCapture) && !capture) ||
        ((fileWriteMode == NDFileModeCapture) && !capture)) goto done;
        
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
            NDProcessFile(pPvt, pImage);
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
                status |= ADParam->setInteger(pPvt->params, NDFileDroppedImages, droppedImages);
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



static void NDFileTask(drvADPvt *pPvt)
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
        NDProcessFile(pPvt, pImage); 
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
                    NDFileCallback, pPvt, &pPvt->asynHandleInterruptPvt);
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

    ADParam->getString(pPvt->params, NDFileImagePort, sizeof(imagePort), imagePort);
    ADParam->getInteger(pPvt->params, NDFileImageAddr, &imageAddr);
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
        case NDFileImageAddr:
            connectToImagePort(pPvt);
            break;
        case ADWriteFile:
            if (pPvt->pImage) {
                status = NDFileWriteFile(pPvt);
            } else {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                    "%s:%s: ERROR, no valid image to write",
                    driverName, functionName);
                status = asynError;
            }
            break;
        case ADReadFile:
            status = NDFileReadFile(pPvt);
            break;
        case NDFileCapture:
            status = NDFileDoCapture(pPvt);
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
        case NDFileImagePort:
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
    ADParam->setInteger(pPvt->params, NDFileWriteMode, NDFileModeSingle);

    status = NDFileWriteFile(pPvt);

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
    if (status) status = ADUtils->findParam(NDFileParamString, NUM_AD_FILE_PARAMS, 
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


/* Configuration routine.  Called directly, or from the iocsh function in drvNDFileEpics */

int drvNDFileConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                       const char *imagePort, int imageAddr)
{
    drvADPvt *pPvt;
    asynStatus status;
    char *functionName = "drvNDFileConfigure";
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

    /* Create the epicsMutex for locking access to file I/O from other threads */
    pPvt->fileMutexId = epicsMutexCreate();
    if (!pPvt->fileMutexId) {
        printf("%s: epicsMutexCreate failure for file mutex\n", functionName);
        return asynError;
    }

    /* Create the message queue for the input images */
    pPvt->msgQId = epicsMessageQueueCreate(queueSize, sizeof(NDArray_t*));
    if (!pPvt->msgQId) {
        printf("%s: epicsMessageQueueCreate failure\n", functionName);
        return asynError;
    }
    
    /* Initialize the parameter library */
    pPvt->params = ADParam->create(0, NDFileLastDriverParam, &pPvt->asynStdInterfaces);
    if (!pPvt->params) {
        printf("%s: unable to create parameter library\n", functionName);
        return asynError;
    }
    
   /* Create the thread that does the image callbacks */
    status = (epicsThreadCreate("NDFileTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC)NDFileTask,
                                pPvt) == NULL);
    if (status) {
        printf("%s: epicsThreadCreate failure\n", functionName);
        return asynError;
    }

    /* Set the initial values of some parameters */
    ADParam->setInteger(pPvt->params, ADImageCounter, 0);
    ADParam->setInteger(pPvt->params, NDFileDroppedImages, 0);
    ADParam->setString (pPvt->params, NDFileImagePort, imagePort);
    ADParam->setInteger(pPvt->params, NDFileImageAddr, imageAddr);
    ADParam->setInteger(pPvt->params, NDFileBlockingCallbacks, 0);
    ADParam->setInteger(pPvt->params, NDFileCapture, 0);
    
    /* Try to connect to the image port */
    status = connectToImagePort(pPvt);
    
    return asynSuccess;
}

