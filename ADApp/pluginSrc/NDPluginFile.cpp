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
    int addr=0;
    char fullFileName[MAX_FILENAME_LEN];
    int fileFormat, fileNumber;
    int dataType=0;
    int autoIncrement;
    NDArray *pArray=NULL;
    const char* functionName = "NDFileReadFile";

    /* Get the current parameters */
    getIntegerParam(addr, ADAutoIncrement, &autoIncrement);
    getIntegerParam(addr, ADFileNumber,    &fileNumber);

    status = (asynStatus)createFileName(MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s:%s error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, functionName, fullFileName, status);
        return(status);
    }
    status = (asynStatus)getIntegerParam(addr, ADFileFormat, &fileFormat);
    switch (fileFormat) {
    case NDFileFormatNetCDF:
        break;
    }

    /* If we got an error then return */
    if (status) return(status);
    
    /* Update the full file name */
    setStringParam(addr, ADFullFileName, fullFileName);

    /* If autoincrement is set then increment file number */
    if (autoIncrement) {
        fileNumber++;
        setIntegerParam(addr, ADFileNumber, fileNumber);
    }
    
    /* Update the new values of dimensions and the array data */
    setIntegerParam(addr, ADDataType, dataType);
    
    /* Call any registered clients */
    doCallbacksGenericPointer(pArray, NDArrayData, addr);

    /* Set the last array to be this one */
    this->pArrays[addr]->release();
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
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must collect an array to get dimensions first\n",
            driverName, functionName);
        return(asynError);
    }
    
    getIntegerParam(addr, ADFileWriteMode, &fileWriteMode);    
    getIntegerParam(addr, ADFileNumber, &fileNumber);
    getIntegerParam(addr, ADAutoIncrement, &autoIncrement);
    getIntegerParam(addr, ADFileFormat, &fileFormat);
    getIntegerParam(addr, ADFileCapture, &capture);    
    getIntegerParam(addr, ADFileNumCapture, &numCapture);    
    getIntegerParam(addr, ADFileNumCaptured, &numCaptured);

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
                                                           this->pArrays[addr], numArrays, append, close);
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
                setIntegerParam(addr, ADFileNumCaptured, 0);
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
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: ERROR, unknown fileWriteMode %d\n", 
                driverName, functionName, fileWriteMode);
            break;
    }
    epicsMutexUnlock(this->fileMutexId);
    epicsMutexLock(this->mutexId);
    
    if (fileOpenComplete) {
        setStringParam(addr, ADFullFileName, fullFileName);
        /* If autoincrement is set then increment file number */
        if (autoIncrement) {
            fileNumber++;
            setIntegerParam(addr, ADFileNumber, fileNumber);
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
    NDArray array;
    NDArrayInfo_t arrayInfo;
    int i;
    int numCapture;
    const char* functionName = "NDFileDoCapture";
    
    getIntegerParam(addr, ADFileCapture, &capture);    
    getIntegerParam(addr, ADFileWriteMode, &fileWriteMode);    
    getIntegerParam(addr, ADFileNumCapture, &numCapture);

    switch(fileWriteMode) {
        case ADFileModeSingle:
            /* It is an error to set capture=1 in this mode, set to 0 */
            setIntegerParam(addr, ADFileCapture, 0);
            break;
        case ADFileModeCapture:
            if (capture) {
                /* Capturing was just started */
                /* We need to read an array from our array source to get its dimensions */
                array.dataSize = 0;
                status = this->pasynGenericPointer->read(this->asynGenericPointerPvt,this->pasynUserGenericPointer, &array);
                setIntegerParam(addr, ADFileNumCaptured, 0);
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
                setIntegerParam(addr, ADFileNumCaptured, 0);
            } else {
                /* Streaming was just stopped */
                status = writeFile();
                setIntegerParam(addr, ADFileNumCaptured, 0);
            }
    }
    return(status);
}


void NDPluginFile::processCallbacks(NDArray *pArray)
{
    int fileWriteMode, autoSave, capture;
    int addr=0;
    int arrayCounter;
    int status=asynSuccess;
    int numCapture, numCaptured;

    /* Most plugins want to increment the arrayCounter each time they are called, which NDPluginDriver
     * does.  However, for this plugin we only want to increment it when we actually got a callback we were
     * supposed to save.  So we save the array counter before calling base method, increment it here */
    getIntegerParam(addr, NDPluginDriverArrayCounter, &arrayCounter);

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    getIntegerParam(addr, ADAutoSave, &autoSave);
    getIntegerParam(addr, ADFileCapture, &capture);    
    getIntegerParam(addr, ADFileWriteMode, &fileWriteMode);    
    getIntegerParam(addr, ADFileNumCapture, &numCapture);    
    getIntegerParam(addr, ADFileNumCaptured, &numCaptured); 

    /* We always keep the last array so read() can use it.  
     * Release previous one, reserve new one */
    if (this->pArrays[addr]) this->pArrays[addr]->release();
    pArray->reserve();
    this->pArrays[addr] = pArray;
    
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
                    pArray->copy(this->pCaptureNext++);
                    numCaptured++;
                    arrayCounter++;
                    setIntegerParam(addr, ADFileNumCaptured, numCaptured);
                } 
                if (numCaptured == numCapture) {
                    if (autoSave) {
                        status = writeFile();
                    }
                    capture = 0;
                    setIntegerParam(addr, ADFileCapture, capture);
                }
            }
            break;
        case ADFileModeStream:
            if (capture) {
                numCaptured++;
                arrayCounter++;
                setIntegerParam(addr, ADFileNumCaptured, numCaptured);
                status = writeFile();
                if (numCaptured == numCapture) {
                    capture = 0;
                    setIntegerParam(addr, ADFileCapture, capture);
                    status = writeFile();
                }
            }
            break;
    }

    /* Update the parameters.  */
    setIntegerParam(addr, NDPluginDriverArrayCounter, arrayCounter);
    callParamCallbacks(addr, addr);
}

asynStatus NDPluginFile::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(addr, function, value);

    switch(function) {
        case ADWriteFile:
            if (value) {
                /* Call the callbacks so the status changes */
                callParamCallbacks(addr, addr);
                if (this->pArrays[addr]) {
                    status = writeFile();
                } else {
                    asynPrint(pasynUser, ASYN_TRACE_ERROR,
                        "%s:%s: ERROR, no valid array to write",
                        driverName, functionName);
                    status = asynError;
                }
                /* Set the flag back to 0, since this could be a busy record */
                setIntegerParam(addr, ADWriteFile, 0);
            }
            break;
        case ADReadFile:
            if (value) {
                /* Call the callbacks so the status changes */
                callParamCallbacks(addr, addr);
                status = readFile();
                /* Set the flag back to 0, since this could be a busy record */
                setIntegerParam(addr, ADReadFile, 0);
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
    status = callParamCallbacks(addr, addr);
    
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
    int addr=0;
    asynStatus status = asynSuccess;
    const char *functionName = "writeNDArray";
    
    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    
    this->pArrays[addr] = pArray;
    setIntegerParam(addr, ADFileWriteMode, ADFileModeSingle);

    status = writeFile();

    /* Do callbacks so higher layers see any changes */
    status = callParamCallbacks(addr, addr);
    
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
    int addr=0;

    /* Create the epicsMutex for locking access to file I/O from other threads */
    this->fileMutexId = epicsMutexCreate();
    if (!this->fileMutexId) {
        printf("%s:%s: epicsMutexCreate failure for file mutex\n", driverName, functionName);
        return;
    }

    /* Set the initial values of some parameters */
    setStringParam (addr, ADFilePath,             "");
    setStringParam (addr, ADFileName,             "");
    setIntegerParam(addr, ADFileNumber,            0);
    setStringParam (addr, ADFileTemplate,         "");
    setIntegerParam(addr, ADAutoIncrement,         0);
    setStringParam (addr, ADFullFileName,         "");
    setIntegerParam(addr, ADFileFormat,            0);
    setIntegerParam(addr, ADAutoSave,              0);
    setIntegerParam(addr, ADWriteFile,             0);
    setIntegerParam(addr, ADReadFile,              0);
    setIntegerParam(addr, ADFileWriteMode,         0);
    setIntegerParam(addr, ADFileNumCapture,        0);
    setIntegerParam(addr, ADFileNumCaptured,       0);
    setIntegerParam(addr, ADFileCapture,           0);
    
    /* Try to connect to the NDArray port */
    status = connectToArrayPort();
}

