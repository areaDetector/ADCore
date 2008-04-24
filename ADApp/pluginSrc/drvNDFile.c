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
    NDFileArrayPort           /* (asynOctet,    r/w) The port for the NDArray interface */
      = ADFirstDriverParam,
    NDFileArrayAddr,          /* (asynInt32,    r/w) The address on the port */
    NDFileMinWriteTime,       /* (asynFloat64,  r/w) Minimum time between file writes */
    NDFileDroppedArrays,      /* (asynInt32,    r/w) Number of dropped arrays */
    NDFileArrayCounter,       /* (asynInt32,    r/w) Number of arrays processed */
    NDFileBlockingCallbacks,  /* (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
    NDFileWriteMode,          /* (asynInt32,    r/w) File saving mode (NDFileMode_t) */
    NDFileNumCapture,         /* (asynInt32,    r/w) Number of arrays to capture */
    NDFileNumCaptured,        /* (asynInt32,    r/w) Number of arrays already captured */
    NDFileCapture,            /* (asynInt32,    r/w) Start or stop capturing arrays */
    NDFileLastDriverParam
} NDFileParam_t;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t NDFileParamString[] = {
    {NDFileArrayPort,         "NDARRAY_PORT" },
    {NDFileArrayAddr,         "NDARRAY_ADDR" },
    {NDFileMinWriteTime,      "FILE_MIN_TIME" },
    {NDFileDroppedArrays,     "DROPPED_ARRAYS" },
    {NDFileArrayCounter,      "ARRAY_COUNTER"},
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
    NDArray_t *pArray;

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
    int dataType=0;
    int autoIncrement;
    NDArray_t *pArray=NULL;
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
    
    /* Update the new values of dimensions and the array data */
    ADParam->setInteger(pPvt->params, ADDataType, dataType);
    
    /* Call any registered clients */
    ADUtils->handleCallback(pPvt->asynStdInterfaces.handleInterruptPvt, pArray, NDArrayData, 0);

    /* Set the last arrat to be this one */
    NDArrayBuff->release(pPvt->pArray);
    pPvt->pArray = pArray;    
    
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

    /* Make sure there is a valid array */
    if (!pPvt->pArray) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must collect an array to get dimensions first\n",
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
                    status = NDFileWriteNetCDF(fullFileName, &pPvt->netCDFState, pPvt->pArray, numArrays, append, close);
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
                            status = NDFileWriteNetCDF(fullFileName, &pPvt->netCDFState, pPvt->pArray, numArrays, append, close);
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
                            status = NDFileWriteNetCDF(NULL, &pPvt->netCDFState, pPvt->pArray, numArrays, append, close);
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
                            status = NDFileWriteNetCDF(NULL, &pPvt->netCDFState, pPvt->pArray, numArrays, append, close);
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
                /* We need to read an array from our array source to get its dimensions */
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

static int NDProcessFile(drvADPvt *pPvt, NDArray_t *pArray)
{
    int fileWriteMode, autoSave, capture;
    int arrayCounter;
    int status=asynSuccess;
    int numCapture, numCaptured;

    ADParam->getInteger(pPvt->params, ADAutoSave, &autoSave);
    ADParam->getInteger(pPvt->params, NDFileCapture, &capture);    
    ADParam->getInteger(pPvt->params, NDFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(pPvt->params, NDFileArrayCounter, &arrayCounter);    
    ADParam->getInteger(pPvt->params, NDFileNumCapture, &numCapture);    
    ADParam->getInteger(pPvt->params, NDFileNumCaptured, &numCaptured); 

    /* We always keep the last array so read() can use it.  Release it now */
    if (pPvt->pArray) NDArrayBuff->release(pPvt->pArray);
    pPvt->pArray = pArray;
    
    switch(fileWriteMode) {
        case NDFileModeSingle:
            if (autoSave) {
                arrayCounter++;
                status = NDFileWriteFile(pPvt);
            }
            break;
        case NDFileModeCapture:
            if (capture) {
                arrayCounter++;
                if (numCaptured < numCapture) {
                    NDArrayBuff->copy(pPvt->pCaptureNext++, pArray);
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
                arrayCounter++;
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
    ADParam->setInteger(pPvt->params, NDFileArrayCounter, arrayCounter);    
    ADParam->callCallbacks(pPvt->params);
    return(status);
}


static void NDFileCallback(void *drvPvt, asynUser *pasynUser, void *handle)
{
    /* This callback function is called from the detector driver when a new array arrives.
     * It writes the array in a disk file.
     * It can either do the callbacks directly (if BlockingCallbacks=1) or by queueing
     * the arrays to be processed by a background task (if BlockingCallbacks=0).
     * In the latter case arrays can be dropped if the queue is full.
     */
     
    drvADPvt *pPvt = drvPvt;
    NDArray_t *pArray = handle;
    epicsTimeStamp tNow;
    double minFileWriteTime, deltaTime;
    int status;
    int fileWriteMode, autoSave, capture;
    int blockingCallbacks;
    int arrayCounter, droppedArrays;
    char *functionName = "NDFileCallback";

    epicsMutexLock(pPvt->mutexId);

    status |= ADParam->getDouble(pPvt->params, NDFileMinWriteTime, &minFileWriteTime);
    status |= ADParam->getInteger(pPvt->params, NDFileArrayCounter, &arrayCounter);
    status |= ADParam->getInteger(pPvt->params, NDFileDroppedArrays, &droppedArrays);
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
         * If blocking we call NDProcessFile directly, executing them
         * in the detector callback thread.
         * If non-blocking we put the array on the queue and it executes
         * in our background thread. */
        /* We always keep the last array so read() can use it. Reserve it. */
         NDArrayBuff->reserve(pArray);
        /* Update the time we last posted an array */
        epicsTimeGetCurrent(&tNow);
        memcpy(&pPvt->lastFileWriteTime, &tNow, sizeof(tNow));
        if (blockingCallbacks) {
            NDProcessFile(pPvt, pArray);
        } else {
            /* Increase the reference count again on this array
             * It will be released in the background task when processing is done */
            NDArrayBuff->reserve(pArray);
            /* Try to put this array on the message queue.  If there is no room then return
             * immediately. */
            status = epicsMessageQueueTrySend(pPvt->msgQId, &pArray, sizeof(&pArray));
            if (status) {
                asynPrint(pasynUser, ASYN_TRACE_FLOW, 
                    "%s:%s message queue full, dropped array %d\n",
                    driverName, functionName, arrayCounter);
                droppedArrays++;
                status |= ADParam->setInteger(pPvt->params, NDFileDroppedArrays, droppedArrays);
                /* This buffer needs to be released twice, because it never made it onto the queue where
                 * it would be released later */
                NDArrayBuff->release(pArray);
                NDArrayBuff->release(pArray);
            }
        }
    }
    ADParam->callCallbacks(pPvt->params);
    done:
    epicsMutexUnlock(pPvt->mutexId);
}



static void NDFileTask(drvADPvt *pPvt)
{
    /* This thread writes the data when a new array arrives */

    /* Loop forever */
    NDArray_t *pArray;
    
    while (1) {
        /* Wait for an array to arrive from the queue */    
        epicsMessageQueueReceive(pPvt->msgQId, &pArray, sizeof(&pArray));
        
        /* Take the lock.  The function we are calling must release the lock
         * during time-consuming operations when it does not need it. */
        epicsMutexLock(pPvt->mutexId);
        /* Call the function that does the callbacks to standard asyn interfaces */
        NDProcessFile(pPvt, pArray); 
        epicsMutexUnlock(pPvt->mutexId); 
        
        /* We are done with this array buffer */       
        NDArrayBuff->release(pArray);
    }
}

static int setArrayInterrupt(drvADPvt *pPvt, int connect)
{
    int status = asynSuccess;
    const char *functionName = "setArrayInterrupt";
    
    /* Lock the port.  May not be necessary to do this. */
    status = pasynManager->lockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock array port: %s\n",
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
            "%s::%s ERROR: Can't unlock array port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        return(status);
    }
    return(asynSuccess);
}

static int connectToArrayPort(drvADPvt *pPvt)
{
    asynStatus status;
    asynInterface *pasynInterface;
    NDArray_t array;
    int isConnected;
    char arrayPort[20];
    int arrayAddr;
    const char *functionName = "connectToArrayPort";

    ADParam->getString(pPvt->params, NDFileArrayPort, sizeof(arrayPort), arrayPort);
    ADParam->getInteger(pPvt->params, NDFileArrayAddr, &arrayAddr);
    status = pasynManager->isConnected(pPvt->pasynUserHandle, &isConnected);
    if (status) isConnected=0;

    /* If we are currently connected cancel interrupt request */    
    if (isConnected) {
        status = setArrayInterrupt(pPvt, 0);
    }
    
    /* Disconnect the array port from our asynUser.  Ignore error if there is no device
     * currently connected. */
    pasynManager->exceptionCallbackRemove(pPvt->pasynUserHandle);
    pasynManager->disconnect(pPvt->pasynUserHandle);

    /* Connect to the array port driver */
    status = pasynManager->connectDevice(pPvt->pasynUserHandle, arrayPort, arrayAddr);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::%s ERROR: Can't connect to array port %s address %d: %s\n",
                  driverName, functionName, arrayPort, arrayAddr, pPvt->pasynUserHandle->errorMessage);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return (status);
    }

    /* Find the asynHandle interface in that driver */
    pasynInterface = pasynManager->findInterface(pPvt->pasynUserHandle, asynHandleType, 1);
    if (!pasynInterface) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
                  "%s::connectToPort ERROR: Can't find asynHandle interface on array port %s address %d\n",
                  driverName, arrayPort, arrayAddr);
        pasynManager->exceptionDisconnect(pPvt->pasynUser);
        return(asynError);
    }
    pPvt->pasynHandle = pasynInterface->pinterface;
    pPvt->asynHandlePvt = pasynInterface->drvPvt;
    pasynManager->exceptionConnect(pPvt->pasynUser);

    /* Read the current array parameters from the array driver */
    /* Lock the port. Defintitely necessary to do this. */
    status = pasynManager->lockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't lock array port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
        return(status);
    }
    /* Read the current array, but only request 0 bytes so no data are actually transferred */
    array.dataSize = 0;
    status = pPvt->pasynHandle->read(pPvt->asynHandlePvt,pPvt->pasynUserHandle, &array);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: reading array data:%s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
    } else {
        ADParam->callCallbacks(pPvt->params);
    }
    /* Unlock the port.  Definitely necessary to do this. */
    status = pasynManager->unlockPort(pPvt->pasynUserHandle);
    if (status != asynSuccess) {
        asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,
            "%s::%s ERROR: Can't unlock array port: %s\n",
            driverName, functionName, pPvt->pasynUserHandle->errorMessage);
    }
    
    /* Enable interrupt callbacks */
    status = setArrayInterrupt(pPvt, 1);

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
        case NDFileArrayAddr:
            connectToArrayPort(pPvt);
            break;
        case ADWriteFile:
            if (pPvt->pArray) {
                status = NDFileWriteFile(pPvt);
            } else {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                    "%s:%s: ERROR, no valid array to write",
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
        case NDFileArrayPort:
            connectToArrayPort(pPvt);
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
static asynStatus readNDArray(void *drvPvt, asynUser *pasynUser, void *handle)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    NDArray_t *pArray = handle;
    NDArrayInfo_t arrayInfo;
    int status = asynSuccess;
    const char* functionName = "readNDArray";
    
    epicsMutexLock(pPvt->mutexId);
    if (!pPvt->pArray) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:functionName error, no valid array available\n", 
              driverName, functionName);
        status = asynError;
    } else {
        pArray->ndims = pPvt->pArray->ndims;
        memcpy(pArray->dims, pPvt->pArray->dims, sizeof(pArray->dims));
        pArray->dataType = pPvt->pArray->dataType;
        NDArrayBuff->getInfo(pPvt->pArray, &arrayInfo);
        if (arrayInfo.totalBytes > pArray->dataSize) arrayInfo.totalBytes = pArray->dataSize;
        memcpy(pArray->pData, pPvt->pArray->pData, arrayInfo.totalBytes);
    }
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d pData=%p\n", 
              driverName, functionName, status, pArray->pData);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s error, maxBytes=%d, data=%p\n", 
              driverName, functionName, arrayInfo.totalBytes, pArray->pData);
    epicsMutexUnlock(pPvt->mutexId);
    return status;
}

static asynStatus writeNDArray(void *drvPvt, asynUser *pasynUser, void *handle)
{
    drvADPvt *pPvt = (drvADPvt *)drvPvt;
    NDArray_t *pArray = handle;
    int status = asynSuccess;
    
    if (pPvt == NULL) return asynError;
    epicsMutexLock(pPvt->mutexId);
    
    pPvt->pArray = pArray;
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
    writeNDArray,
    readNDArray
};

static asynDrvUser ifaceDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};


/* Configuration routine.  Called directly, or from the iocsh function in drvNDFileEpics */

int drvNDFileConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                       const char *NDArrayPort, int NDArrayAddr)
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

    /* Create asynUser for communicating with NDArray port */
    pasynUser = pasynManager->createAsynUser(0, 0);
    pasynUser->userPvt = pPvt;
    pPvt->pasynUserHandle = pasynUser;
    pPvt->pasynUserHandle->reason = NDArrayData;

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

    /* Create the message queue for the input arrays */
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
    
   /* Create the thread that handles the NDArray callbacks */
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
    ADParam->setInteger(pPvt->params, NDFileArrayCounter, 0);
    ADParam->setInteger(pPvt->params, NDFileDroppedArrays, 0);
    ADParam->setInteger(pPvt->params, NDFileNumCaptured, 0);
    ADParam->setInteger(pPvt->params, NDFileCapture, 0);
    ADParam->setString (pPvt->params, NDFileArrayPort, NDArrayPort);
    ADParam->setInteger(pPvt->params, NDFileArrayAddr, NDArrayAddr);
    
    /* Try to connect to the NDArray port */
    status = connectToArrayPort(pPvt);
    
    return asynSuccess;
}

