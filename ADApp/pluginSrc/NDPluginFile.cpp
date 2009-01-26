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

#include "ADStdDriverParams.h"
#include "NDArray.h"
#include "NDPluginFile.h"
#include "drvNDFile.h"


/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static asynParamString_t NDPluginFileParamString[] = {
    {0, "unused"} /* We don't have currently any parameters ourselves but compiler does not like empty array */
};

#define NUM_ND_PLUGIN_FILE_PARAMS (sizeof(NDPluginFileParamString)/sizeof(NDPluginFileParamString[0]))

static const char *driverName="NDPluginFile";


/* Local functions, not in any interface */

asynStatus NDPluginFile::readFile(void)
{
    /* Reads a file written by NDFileWriteFile from disk in either binary or ASCII format. */
    asynStatus status = asynSuccess;
    char fullFileName[MAX_FILENAME_LEN];
    int fileFormat, fileNumber;
    int dataType=0;
    int autoIncrement;
    NDArray *pArray=NULL;
    const char* functionName = "NDFileReadFile";

    /* Get the current parameters */
    getIntegerParam(ADAutoIncrement, &autoIncrement);
    getIntegerParam(ADFileNumber,    &fileNumber);

    status = (asynStatus)createFileName(MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s:%s error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, functionName, fullFileName, status);
        return(status);
    }
    status = (asynStatus)getIntegerParam(ADFileFormat, &fileFormat);
    switch (fileFormat) {
    case NDFileFormatNetCDF:
        break;
    }

    /* If we got an error then return */
    if (status) return(status);
    
    /* Update the full file name */
    setStringParam(ADFullFileName, fullFileName);

    /* If autoincrement is set then increment file number */
    if (autoIncrement) {
        fileNumber++;
        setIntegerParam(ADFileNumber, fileNumber);
    }
    
    /* Update the new values of dimensions and the array data */
    setIntegerParam(ADDataType, dataType);
    
    /* Call any registered clients */
    doCallbacksGenericPointer(pArray, NDArrayData, 0);

    /* Set the last array to be this one */
    this->pArrays[0]->release();
    this->pArrays[0] = pArray;    
    
    return(status);
}

asynStatus NDPluginFile::writeFile() 
{
    asynStatus status = asynSuccess;
    int fileWriteMode, capture;
    int fileNumber, autoIncrement, fileFormat;
    char fullFileName[MAX_FILENAME_LEN];
    int numCapture, numCaptured;
    int i, numArrays, append, close;
    int fileOpenComplete = 0;
    const char* functionName = "NDFileWriteFile";

    /* Make sure there is a valid array */
    if (!this->pArrays[0]) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must collect an array to get dimensions first\n",
            driverName, functionName);
        return(asynError);
    }
    
    getIntegerParam(ADFileWriteMode, &fileWriteMode);    
    getIntegerParam(ADFileNumber, &fileNumber);
    getIntegerParam(ADAutoIncrement, &autoIncrement);
    getIntegerParam(ADFileFormat, &fileFormat);
    getIntegerParam(ADFileCapture, &capture);    
    getIntegerParam(ADFileNumCapture, &numCapture);    
    getIntegerParam(ADFileNumCaptured, &numCaptured);

    /* We unlock the overall mutex here because we want the callbacks to be able to queue new
     * frames without waiting while we write files here.  The only restriction is that the
     * callbacks must not modify any part of the pPvt structure that we use here.
     * However, we need to take a mutex on file I/O because manually stopping stream can
     * result in a call to this function, and that needs to block. */
    epicsMutexLock(this->fileMutexId);
    epicsMutexUnlock(this->mutexId);
    
    switch(fileWriteMode) {
        case ADFileModeSingle:
            status = (asynStatus)createFileName(sizeof(fullFileName), fullFileName);
            switch(fileFormat) {
                case NDFileFormatNetCDF:
                    close = 1;
                    numArrays = 1;
                    append = 0;
                    status = (asynStatus)NDFileWriteNetCDF(fullFileName, &this->netCDFState, 
                                                           this->pArrays[0], numArrays, append, close);
                    if (status == asynSuccess) fileOpenComplete = 1;
                    break;
            }
            break;
        case ADFileModeCapture:
            if (numCaptured > 0) {
                status = (asynStatus)createFileName(sizeof(fullFileName), fullFileName);
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
                setIntegerParam(ADFileNumCaptured, 0);
            }
            break;
        case ADFileModeStream:
            if (capture) {
                if (numCaptured == 1) {
                    switch(fileFormat) {
                        case NDFileFormatNetCDF:
                            /* Streaming was just started, write the header plus the first frame */
                            status = (asynStatus)createFileName(sizeof(fullFileName), fullFileName);
                            close = 0;
                            append = 0;
                            numArrays = -1;
                            status = (asynStatus)NDFileWriteNetCDF(fullFileName, &this->netCDFState, 
                                                                   this->pArrays[0], numArrays, append, close);
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
                                                                   this->pArrays[0], numArrays, append, close);
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
                                                                   this->pArrays[0], numArrays, append, close);
                            break;
                    }
                }
            }
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: ERROR, unknown fileWriteMode %d\n", 
                driverName, functionName, fileWriteMode);
            break;
    }
    epicsMutexUnlock(this->fileMutexId);
    epicsMutexLock(this->mutexId);
    
    if (fileOpenComplete) {
        setStringParam(ADFullFileName, fullFileName);
        /* If autoincrement is set then increment file number */
        if (autoIncrement) {
            fileNumber++;
            setIntegerParam(ADFileNumber, fileNumber);
        }
    }
    return(status);
}

asynStatus NDPluginFile::doCapture() 
{
    /* This function is called from write32 whenever capture is started or stopped */
    asynStatus status = asynSuccess;
    int fileWriteMode, capture;
    NDArray array;
    NDArrayInfo_t arrayInfo;
    int i;
    int numCapture;
    const char* functionName = "NDFileDoCapture";
    
    getIntegerParam(ADFileCapture, &capture);    
    getIntegerParam(ADFileWriteMode, &fileWriteMode);    
    getIntegerParam(ADFileNumCapture, &numCapture);

    switch(fileWriteMode) {
        case ADFileModeSingle:
            /* It is an error to set capture=1 in this mode, set to 0 */
            setIntegerParam(ADFileCapture, 0);
            break;
        case ADFileModeCapture:
            if (capture) {
                /* Capturing was just started */
                /* We need to read an array from our array source to get its dimensions */
                array.dataSize = 0;
                status = this->pasynGenericPointer->read(this->asynGenericPointerPvt,this->pasynUserGenericPointer, &array);
                setIntegerParam(ADFileNumCaptured, 0);
                array.getInfo(&arrayInfo);
                this->pCapture = (NDArray *)malloc(numCapture * sizeof(NDArray));
                if (!this->pCapture) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s ERROR: cannot allocate capture buffers\n",
                        driverName, functionName);
                    status = asynError;
                }
                for (i=0; i<numCapture; i++) {
                    memcpy(&this->pCapture[i], &array, sizeof(array));
                    this->pCapture[i].dataSize = arrayInfo.totalBytes;
                    this->pCapture[i].pData = malloc(arrayInfo.totalBytes);
                    if (!this->pCapture[i].pData) {
                        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
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
        case ADFileModeStream:
            if (capture) {
                /* Streaming was just started */
                setIntegerParam(ADFileNumCaptured, 0);
            } else {
                /* Streaming was just stopped */
                status = writeFile();
                setIntegerParam(ADFileNumCaptured, 0);
            }
    }
    return(status);
}


void NDPluginFile::processCallbacks(NDArray *pArray)
{
    int fileWriteMode, autoSave, capture;
    int arrayCounter;
    int status=asynSuccess;
    int numCapture, numCaptured;

    /* Most plugins want to increment the arrayCounter each time they are called, which NDPluginDriver
     * does.  However, for this plugin we only want to increment it when we actually got a callback we were
     * supposed to save.  So we save the array counter before calling base method, increment it here */
    getIntegerParam(NDPluginDriverArrayCounter, &arrayCounter);

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    getIntegerParam(ADAutoSave, &autoSave);
    getIntegerParam(ADFileCapture, &capture);    
    getIntegerParam(ADFileWriteMode, &fileWriteMode);    
    getIntegerParam(ADFileNumCapture, &numCapture);    
    getIntegerParam(ADFileNumCaptured, &numCaptured); 

    /* We always keep the last array so read() can use it.  
     * Release previous one, reserve new one */
    if (this->pArrays[0]) this->pArrays[0]->release();
    pArray->reserve();
    this->pArrays[0] = pArray;
    
    switch(fileWriteMode) {
        case ADFileModeSingle:
            if (autoSave) {
                arrayCounter++;
                status = writeFile();
            }
            break;
        case ADFileModeCapture:
            if (capture) {
                if (numCaptured < numCapture) {
                    this->pNDArrayPool->copy(pArray, this->pCaptureNext++, 1);
                    numCaptured++;
                    arrayCounter++;
                    setIntegerParam(ADFileNumCaptured, numCaptured);
                } 
                if (numCaptured == numCapture) {
                    if (autoSave) {
                        status = writeFile();
                    }
                    capture = 0;
                    setIntegerParam(ADFileCapture, capture);
                }
            }
            break;
        case ADFileModeStream:
            if (capture) {
                numCaptured++;
                arrayCounter++;
                setIntegerParam(ADFileNumCaptured, numCaptured);
                status = writeFile();
                if (numCaptured == numCapture) {
                    capture = 0;
                    setIntegerParam(ADFileCapture, capture);
                    status = writeFile();
                }
            }
            break;
    }

    /* Update the parameters.  */
    setIntegerParam(NDPluginDriverArrayCounter, arrayCounter);
    callParamCallbacks();
}

asynStatus NDPluginFile::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char* functionName = "writeInt32";

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    switch(function) {
        case ADWriteFile:
            if (value) {
                /* Call the callbacks so the status changes */
                callParamCallbacks();
                if (this->pArrays[0]) {
                    status = writeFile();
                } else {
                    asynPrint(pasynUser, ASYN_TRACE_ERROR,
                        "%s:%s: ERROR, no valid array to write",
                        driverName, functionName);
                    status = asynError;
                }
                /* Set the flag back to 0, since this could be a busy record */
                setIntegerParam(ADWriteFile, 0);
            }
            break;
        case ADReadFile:
            if (value) {
                /* Call the callbacks so the status changes */
                callParamCallbacks();
                status = readFile();
                /* Set the flag back to 0, since this could be a busy record */
                setIntegerParam(ADReadFile, 0);
            }
            break;
        case ADFileCapture:
            if (value) {
                status = doCapture();
            }
            break;
        default:
            /* This was not a parameter that this driver understands, try the base class */
            status = NDPluginDriver::writeInt32(pasynUser, value);
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status = callParamCallbacks();
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%d\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    return status;
}


asynStatus NDPluginFile::writeNDArray(asynUser *pasynUser, void *genericPointer)
{
    NDArray *pArray = (NDArray *)genericPointer;
    asynStatus status = asynSuccess;
    const char *functionName = "writeNDArray";
        
    this->pArrays[0] = pArray;
    setIntegerParam(ADFileWriteMode, ADFileModeSingle);

    status = writeFile();

    /* Do callbacks so higher layers see any changes */
    status = callParamCallbacks();
    
    return status;
}

/* asynDrvUser interface methods */
asynStatus NDPluginFile::drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    asynStatus status;
    int param;
    const char *functionName = "drvUserCreate";

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
    status = NDPluginDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
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
    : NDPluginDriver(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, 1, NDPluginFileLastParam, 0, 0, 
                   asynGenericPointerMask, asynGenericPointerMask)
{
    const char *functionName = "NDPluginFile";
    asynStatus status;

    /* Create the epicsMutex for locking access to file I/O from other threads */
    this->fileMutexId = epicsMutexCreate();
    if (!this->fileMutexId) {
        printf("%s:%s: epicsMutexCreate failure for file mutex\n", driverName, functionName);
        return;
    }

    /* Set the initial values of some parameters */
    setStringParam (ADFilePath,             "");
    setStringParam (ADFileName,             "");
    setIntegerParam(ADFileNumber,            0);
    setStringParam (ADFileTemplate,         "");
    setIntegerParam(ADAutoIncrement,         0);
    setStringParam (ADFullFileName,         "");
    setIntegerParam(ADFileFormat,            0);
    setIntegerParam(ADAutoSave,              0);
    setIntegerParam(ADWriteFile,             0);
    setIntegerParam(ADReadFile,              0);
    setIntegerParam(ADFileWriteMode,         0);
    setIntegerParam(ADFileNumCapture,        0);
    setIntegerParam(ADFileNumCaptured,       0);
    setIntegerParam(ADFileCapture,           0);
    
    /* Try to connect to the NDArray port */
    status = connectToArrayPort();
}

