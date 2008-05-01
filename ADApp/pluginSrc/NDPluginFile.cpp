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

#include "ADInterface.h"
#include "NDArrayBuff.h"
#include "ADParamLib.h"
#include "ADUtils.h"
#include "NDPluginFile.h"
#include "drvNDFile.h"


/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static asynParamString_t NDPluginFileParamString[] = {
    {NDPluginFileWriteMode,         "WRITE_MODE" },
    {NDPluginFileNumCapture,        "NUM_CAPTURE" },
    {NDPluginFileNumCaptured,       "NUM_CAPTURED" },
    {NDPluginFileCapture,           "CAPTURE" },
};

#define NUM_ND_PLUGIN_FILE_PARAMS (sizeof(NDPluginFileParamString)/sizeof(NDPluginFileParamString[0]))

static const char *driverName="NDPluginFile";


/* Local functions, not in any interface */

asynStatus NDPluginFile::readFile(void)
{
    /* Reads a file written by NDFileWriteFile from disk in either binary or ASCII format. */
    asynStatus status = asynSuccess;
    int addr=0;
    char fullFileName[MAX_FILENAME_LEN];
    int fileFormat, fileNumber;
    int dataType=0;
    int autoIncrement;
    NDArray_t *pArray=NULL;
    const char* functionName = "NDFileReadFile";

    /* Get the current parameters */
    ADParam->getInteger(this->params[addr], ADAutoIncrement, &autoIncrement);
    ADParam->getInteger(this->params[addr], ADFileNumber,    &fileNumber);

    status = (asynStatus)ADUtils->createFileName(this->params[addr], MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, functionName, fullFileName, status);
        return(status);
    }
    status = (asynStatus)ADParam->getInteger(this->params[addr], ADFileFormat, &fileFormat);
    switch (fileFormat) {
    case NDFileFormatNetCDF:
        break;
    }

    /* If we got an error then return */
    if (status) return(status);
    
    /* Update the full file name */
    ADParam->setString(this->params[addr], ADFullFileName, fullFileName);

    /* If autoincrement is set then increment file number */
    if (autoIncrement) {
        fileNumber++;
        ADParam->setInteger(this->params[addr], ADFileNumber, fileNumber);
    }
    
    /* Update the new values of dimensions and the array data */
    ADParam->setInteger(this->params[addr], ADDataType, dataType);
    
    /* Call any registered clients */
    doCallbacksNDArray(pArray, NDArrayData, addr);

    /* Set the last array to be this one */
    NDArrayBuff->release(this->pArrays[addr]);
    this->pArrays[addr] = pArray;    
    
    return(status);
}

asynStatus NDPluginFile::writeFile() 
{
    asynStatus status = asynSuccess;
    int addr=0;
    int fileWriteMode, capture;
    int fileNumber, autoIncrement, fileFormat;
    char fullFileName[MAX_FILENAME_LEN];
    int numCapture, numCaptured;
    int i, numArrays, append, close;
    int fileOpenComplete = 0;
    const char* functionName = "NDFileWriteFile";

    /* Make sure there is a valid array */
    if (!this->pArrays[addr]) {
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must collect an array to get dimensions first\n",
            driverName, functionName);
        return(asynError);
    }
    
    ADParam->getInteger(this->params[addr], NDPluginFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(this->params[addr], ADFileNumber, &fileNumber);
    ADParam->getInteger(this->params[addr], ADAutoIncrement, &autoIncrement);
    ADParam->getInteger(this->params[addr], ADFileFormat, &fileFormat);
    ADParam->getInteger(this->params[addr], NDPluginFileCapture, &capture);    
    ADParam->getInteger(this->params[addr], NDPluginFileCapture, &numCapture);    
    ADParam->getInteger(this->params[addr], NDPluginFileNumCaptured, &numCaptured);

    /* We unlock the overall mutex here because we want the callbacks to be able to queue new
     * frames without waiting while we write files here.  The only restriction is that the
     * callbacks must not modify any part of the pPvt structure that we use here.
     * However, we need to take a mutex on file I/O because manually stopping stream can
     * result in a call to this function, and that needs to block. */
    epicsMutexLock(this->fileMutexId);
    epicsMutexUnlock(this->mutexId);
    
    switch(fileWriteMode) {
        case NDPluginFileModeSingle:
            status = (asynStatus)ADUtils->createFileName(this->params[addr], sizeof(fullFileName), fullFileName);
            switch(fileFormat) {
                case NDFileFormatNetCDF:
                    close = 1;
                    numArrays = 1;
                    append = 0;
                    status = (asynStatus)NDFileWriteNetCDF(fullFileName, &this->netCDFState, 
                                                           this->pArrays[addr], numArrays, append, close);
                    if (status == asynSuccess) fileOpenComplete = 1;
                    break;
            }
            break;
        case NDPluginFileModeCapture:
            if (numCaptured > 0) {
            status = (asynStatus)ADUtils->createFileName(this->params[addr], sizeof(fullFileName), fullFileName);
               switch(fileFormat) {
                    case NDFileFormatNetCDF:
                        close = 1;
                        append = 0;
                        numArrays = numCaptured;
                        status = (asynStatus)NDFileWriteNetCDF(fullFileName, &this->netCDFState, this->pCapture, numArrays, append, close);
                        if (status == asynSuccess) fileOpenComplete = 1;
                        break;
                }
                /* Free all the buffer memory we allocated */
                for (i=0; i<numCapture; i++) free(this->pCapture[i].pData);
                free(this->pCapture);
                this->pCapture = NULL;
                ADParam->setInteger(this->params[addr], NDPluginFileNumCaptured, 0);
            }
            break;
        case NDPluginFileModeStream:
            if (capture) {
                if (numCaptured == 1) {
                    switch(fileFormat) {
                        case NDFileFormatNetCDF:
                            /* Streaming was just started, write the header plus the first frame */
                            status = (asynStatus)ADUtils->createFileName(this->params[addr], sizeof(fullFileName), fullFileName);
                            close = 0;
                            append = 0;
                            numArrays = -1;
                            status = (asynStatus)NDFileWriteNetCDF(fullFileName, &this->netCDFState, 
                                                                   this->pArrays[addr], numArrays, append, close);
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
                            status = (asynStatus)NDFileWriteNetCDF(NULL, &this->netCDFState, 
                                                                   this->pArrays[addr], numArrays, append, close);
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
                            status = (asynStatus)NDFileWriteNetCDF(NULL, &this->netCDFState, 
                                                                   this->pArrays[addr], numArrays, append, close);
                            break;
                    }
                }
            }
            break;
        default:
            asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                "%s:%s: ERROR, unknown fileWriteMode %d\n", 
                driverName, functionName, fileWriteMode);
            break;
    }
    epicsMutexUnlock(this->fileMutexId);
    epicsMutexLock(this->mutexId);
    
    if (fileOpenComplete) {
        ADParam->setString(this->params[addr], ADFullFileName, fullFileName);
        /* If autoincrement is set then increment file number */
        if (autoIncrement) {
            fileNumber++;
            ADParam->setInteger(this->params[addr], ADFileNumber, fileNumber);
        }
    }
    return(status);
}

asynStatus NDPluginFile::doCapture() 
{
    /* This function is called from write32 whenever capture is started or stopped */
    asynStatus status = asynSuccess;
    int addr=0;
    int fileWriteMode, capture;
    NDArray_t array;
    NDArrayInfo_t arrayInfo;
    int i;
    int numCapture;
    const char* functionName = "NDFileDoCapture";
    
    ADParam->getInteger(this->params[addr], NDPluginFileCapture, &capture);    
    ADParam->getInteger(this->params[addr], NDPluginFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(this->params[addr], NDPluginFileNumCapture, &numCapture);

    switch(fileWriteMode) {
        case NDPluginFileModeSingle:
            /* It is an error to set capture=1 in this mode, set to 0 */
            ADParam->setInteger(this->params[addr], NDPluginFileCapture, 0);
            break;
        case NDPluginFileModeCapture:
            if (capture) {
                /* Capturing was just started */
                /* We need to read an array from our array source to get its dimensions */
                array.dataSize = 0;
                status = this->pasynHandle->read(this->asynHandlePvt,this->pasynUserHandle, &array);
                ADParam->setInteger(this->params[addr], NDPluginFileNumCaptured, 0);
                NDArrayBuff->getInfo(&array, &arrayInfo);
                this->pCapture = (NDArray_t *)malloc(numCapture * sizeof(NDArray_t));
                if (!this->pCapture) {
                    asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                        "%s:%s ERROR: cannot allocate capture buffers\n",
                        driverName, functionName);
                    status = asynError;
                }
                for (i=0; i<numCapture; i++) {
                    memcpy(&this->pCapture[i], &array, sizeof(array));
                    this->pCapture[i].dataSize = arrayInfo.totalBytes;
                    this->pCapture[i].pData = malloc(arrayInfo.totalBytes);
                    if (!this->pCapture[i].pData) {
                        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
                            "%s:%s ERROR: cannot allocate capture array for buffer %s\n",
                            driverName, functionName, i);
                        status = asynError;
                    }
                }
                this->pCaptureNext = this->pCapture;
            } else {
                /* Stop capturing, nothing to do, setting the parameter is all that is needed */
            }
            break;
        case NDPluginFileModeStream:
            if (capture) {
                /* Streaming was just started */
                ADParam->setInteger(this->params[addr], NDPluginFileNumCaptured, 0);
            } else {
                /* Streaming was just stopped */
                status = writeFile();
                ADParam->setInteger(this->params[addr], NDPluginFileNumCaptured, 0);
            }
    }
    return(status);
}


void NDPluginFile::processCallbacks(NDArray_t *pArray)
{
    int fileWriteMode, autoSave, capture;
    int addr=0;
    int arrayCounter;
    int status=asynSuccess;
    int numCapture, numCaptured;

    /* Most plugins want to increment the arrayCounter each time they are called, which NDPluginBase
     * does.  However, for this plugin we only want to increment it when we actually got a callback we were
     * supposed to save.  So we save the array counter before calling base method, increment it here */
    ADParam->getInteger(this->params[addr], NDPluginBaseArrayCounter, &arrayCounter);

    /* Call the base class method */
    NDPluginBase::processCallbacks(pArray);
    
    ADParam->getInteger(this->params[addr], ADAutoSave, &autoSave);
    ADParam->getInteger(this->params[addr], NDPluginFileCapture, &capture);    
    ADParam->getInteger(this->params[addr], NDPluginFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(this->params[addr], NDPluginFileNumCapture, &numCapture);    
    ADParam->getInteger(this->params[addr], NDPluginFileNumCaptured, &numCaptured); 

    /* We always keep the last array so read() can use it.  
     * Release previous one, reserve new one */
    if (this->pArrays[addr]) NDArrayBuff->release(this->pArrays[addr]);
    NDArrayBuff->reserve(pArray);
    this->pArrays[addr] = pArray;
    
    switch(fileWriteMode) {
        case NDPluginFileModeSingle:
            if (autoSave) {
                arrayCounter++;
                status = writeFile();
            }
            break;
        case NDPluginFileModeCapture:
            if (capture) {
                if (numCaptured < numCapture) {
                    NDArrayBuff->copy(this->pCaptureNext++, pArray);
                    numCaptured++;
                    arrayCounter++;
                    ADParam->setInteger(this->params[addr], NDPluginFileNumCaptured, numCaptured);
                } 
                if (numCaptured == numCapture) {
                    capture = 0;
                    ADParam->setInteger(this->params[addr], NDPluginFileCapture, capture);
                    if (autoSave) {
                        status = writeFile();
                    }
                }
            }
            break;
        case NDPluginFileModeStream:
            if (capture) {
                numCaptured++;
                arrayCounter++;
                ADParam->setInteger(this->params[addr], NDPluginFileNumCaptured, numCaptured);
                status = writeFile();
                if (numCaptured == numCapture) {
                    capture = 0;
                    ADParam->setInteger(this->params[addr], NDPluginFileCapture, capture);
                    status = writeFile();
                }
            }
            break;
    }

    /* Update the parameters.  */
    ADParam->setInteger(this->params[addr], NDPluginBaseArrayCounter, arrayCounter);
    ADParam->callCallbacksAddr(this->params[addr], addr);
}

asynStatus NDPluginFile::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);

    /* Set the parameter in the parameter library. */
    status = (asynStatus) ADParam->setInteger(this->params[addr], function, value);

    switch(function) {
        case ADWriteFile:
            if (this->pArrays[addr]) {
                status = writeFile();
            } else {
                asynPrint(pasynUser, ASYN_TRACE_ERROR,
                    "%s:%s: ERROR, no valid array to write",
                    driverName, functionName);
                status = asynError;
            }
            break;
        case ADReadFile:
            status = readFile();
            break;
        case NDPluginFileCapture:
            status = doCapture();
            break;
        default:
            /* This was not a parameter that this driver understands, try the base class */
            epicsMutexUnlock(this->mutexId);
            status = NDPluginBase::writeInt32(pasynUser, value);
            epicsMutexLock(this->mutexId);
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacksAddr(this->params[addr], addr);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%d\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}


asynStatus NDPluginFile::writeNDArray(asynUser *pasynUser, void *handle)
{
    NDArray_t *pArray = (NDArray_t *)handle;
    int addr=0;
    asynStatus status = asynSuccess;
    const char *functionName = "writeNDArray";
    
    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);
    
    this->pArrays[addr] = pArray;
    ADParam->setInteger(this->params[addr], NDPluginFileWriteMode, NDPluginFileModeSingle);

    status = writeFile();

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacksAddr(this->params[addr], addr);
    
    epicsMutexUnlock(this->mutexId);
    return status;
}

/* asynDrvUser interface methods */
asynStatus NDPluginFile::drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    asynStatus status;
    int param;
    static char *functionName = "drvUserCreate";

    /* First see if this is a parameter specific to this plugin */
    status = findParam(NDPluginFileParamString, NUM_ND_PLUGIN_FILE_PARAMS, 
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
                                    
    /* If not, then call the base class method, see if it is known there */
    status = NDPluginBase::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return(status);
}




/* Configuration routine.  Called directly, or from the iocsh function in drvNDFileEpics */

extern "C" int drvNDFileConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                                  const char *NDArrayPort, int NDArrayAddr)
{
    new NDPluginFile(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr);
    return(asynSuccess);
}

/* The constructor for this class */
NDPluginFile::NDPluginFile(const char *portName, int queueSize, int blockingCallbacks, 
                           const char *NDArrayPort, int NDArrayAddr)
    /* Invoke the base class constructor */
    : NDPluginBase(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, 1, NDPluginFileLastParam)
{
    char *functionName = "NDPluginFile";
    asynStatus status;
    int addr=0;

    /* Create the epicsMutex for locking access to file I/O from other threads */
    this->fileMutexId = epicsMutexCreate();
    if (!this->fileMutexId) {
        printf("%s:%s: epicsMutexCreate failure for file mutex\n", driverName, functionName);
        return;
    }

    /* Set the initial values of some parameters */
    ADParam->setInteger(this->params[addr], NDPluginFileNumCaptured, 0);
    ADParam->setInteger(this->params[addr], NDPluginFileCapture, 0);
    
    /* Try to connect to the NDArray port */
    status = connectToArrayPort();
}

