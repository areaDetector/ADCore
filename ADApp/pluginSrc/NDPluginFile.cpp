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

/* This will define the parameter strings in ADInterface.h */
#define DEFINE_STANDARD_PARAM_STRINGS 1

#include "ADInterface.h"
#include "NDArrayBuff.h"
#include "ADParamLib.h"
#include "ADUtils.h"
#include "NDPluginFile.h"
#include "drvNDFile.h"

#define driverName "NDPluginFile"


/* Local functions, not in any interface */

asynStatus NDPluginFile::readFile(void)
{
    /* Reads a file written by NDFileWriteFile from disk in either binary or ASCII format. */
    asynStatus status = asynSuccess;
    char fullFileName[MAX_FILENAME_LEN];
    int fileFormat, fileNumber;
    int dataType=0;
    int autoIncrement;
    NDArray_t *pArray=NULL;
    const char* functionName = "NDFileReadFile";

    /* Get the current parameters */
    ADParam->getInteger(this->params, ADAutoIncrement, &autoIncrement);
    ADParam->getInteger(this->params, ADFileNumber,    &fileNumber);

    status = (asynStatus)ADUtils->createFileName(this->params, MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, functionName, fullFileName, status);
        return(status);
    }
    status = (asynStatus)ADParam->getInteger(this->params, ADFileFormat, &fileFormat);
    switch (fileFormat) {
    case NDFileFormatNetCDF:
        break;
    }

    /* If we got an error then return */
    if (status) return(status);
    
    /* Update the full file name */
    ADParam->setString(this->params, ADFullFileName, fullFileName);

    /* If autoincrement is set then increment file number */
    if (autoIncrement) {
        fileNumber++;
        ADParam->setInteger(this->params, ADFileNumber, fileNumber);
    }
    
    /* Update the new values of dimensions and the array data */
    ADParam->setInteger(this->params, ADDataType, dataType);
    
    /* Call any registered clients */
    ADUtils->handleCallback(this->asynStdInterfaces.handleInterruptPvt, pArray, NDArrayData, 0);

    /* Set the last arrat to be this one */
    NDArrayBuff->release(this->pArray);
    this->pArray = pArray;    
    
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
    if (!this->pArray) {
        asynPrint(this->pasynUser, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must collect an array to get dimensions first\n",
            driverName, functionName);
        return(asynError);
    }
    
    ADParam->getInteger(this->params, NDPluginFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(this->params, ADFileNumber, &fileNumber);
    ADParam->getInteger(this->params, ADAutoIncrement, &autoIncrement);
    ADParam->getInteger(this->params, ADFileFormat, &fileFormat);
    ADParam->getInteger(this->params, NDPluginFileCapture, &capture);    
    ADParam->getInteger(this->params, NDPluginFileCapture, &numCapture);    
    ADParam->getInteger(this->params, NDPluginFileNumCaptured, &numCaptured);

    /* We unlock the overall mutex here because we want the callbacks to be able to queue new
     * frames without waiting while we write files here.  The only restriction is that the
     * callbacks must not modify any part of the pPvt structure that we use here.
     * However, we need to take a mutex on file I/O because manually stopping stream can
     * result in a call to this function, and that needs to block. */
    epicsMutexLock(this->fileMutexId);
    epicsMutexUnlock(this->mutexId);
    
    switch(fileWriteMode) {
        case NDPluginFileModeSingle:
            status = (asynStatus)ADUtils->createFileName(this->params, sizeof(fullFileName), fullFileName);
            switch(fileFormat) {
                case NDFileFormatNetCDF:
                    close = 1;
                    numArrays = 1;
                    append = 0;
                    status = (asynStatus)NDFileWriteNetCDF(fullFileName, &this->netCDFState, this->pArray, numArrays, append, close);
                    if (status == asynSuccess) fileOpenComplete = 1;
                    break;
            }
            break;
        case NDPluginFileModeCapture:
            if (numCaptured > 0) {
            status = (asynStatus)ADUtils->createFileName(this->params, sizeof(fullFileName), fullFileName);
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
                ADParam->setInteger(this->params, NDPluginFileNumCaptured, 0);
            }
            break;
        case NDPluginFileModeStream:
            if (capture) {
                if (numCaptured == 1) {
                    switch(fileFormat) {
                        case NDFileFormatNetCDF:
                            /* Streaming was just started, write the header plus the first frame */
                            status = (asynStatus)ADUtils->createFileName(this->params, sizeof(fullFileName), fullFileName);
                            close = 0;
                            append = 0;
                            numArrays = -1;
                            status = (asynStatus)NDFileWriteNetCDF(fullFileName, &this->netCDFState, this->pArray, numArrays, append, close);
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
                            status = (asynStatus)NDFileWriteNetCDF(NULL, &this->netCDFState, this->pArray, numArrays, append, close);
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
                            status = (asynStatus)NDFileWriteNetCDF(NULL, &this->netCDFState, this->pArray, numArrays, append, close);
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
        ADParam->setString(this->params, ADFullFileName, fullFileName);
        /* If autoincrement is set then increment file number */
        if (autoIncrement) {
            fileNumber++;
            ADParam->setInteger(this->params, ADFileNumber, fileNumber);
        }
    }
    return(status);
}

asynStatus NDPluginFile::doCapture() 
{
    /* This function is called from write32 whenever capture is started or stopped */
    asynStatus status = asynSuccess;
    int fileWriteMode, capture;
    NDArray_t array;
    NDArrayInfo_t arrayInfo;
    int i;
    int numCapture;
    const char* functionName = "NDFileDoCapture";
    
    ADParam->getInteger(this->params, NDPluginFileCapture, &capture);    
    ADParam->getInteger(this->params, NDPluginFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(this->params, NDPluginFileNumCapture, &numCapture);

    switch(fileWriteMode) {
        case NDPluginFileModeSingle:
            /* It is an error to set capture=1 in this mode, set to 0 */
            ADParam->setInteger(this->params, NDPluginFileCapture, 0);
            break;
        case NDPluginFileModeCapture:
            if (capture) {
                /* Capturing was just started */
                /* We need to read an array from our array source to get its dimensions */
                array.dataSize = 0;
                status = this->pasynHandle->read(this->asynHandlePvt,this->pasynUserHandle, &array);
                ADParam->setInteger(this->params, NDPluginFileNumCaptured, 0);
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
                ADParam->setInteger(this->params, NDPluginFileNumCaptured, 0);
            } else {
                /* Streaming was just stopped */
                status = writeFile();
                ADParam->setInteger(this->params, NDPluginFileNumCaptured, 0);
            }
    }
    return(status);
}

void NDPluginFile::processCallbacks(NDArray_t *pArray)
{
    int fileWriteMode, autoSave, capture;
    int arrayCounter;
    int status=asynSuccess;
    int numCapture, numCaptured;

    ADParam->getInteger(this->params, ADAutoSave, &autoSave);
    ADParam->getInteger(this->params, NDPluginFileCapture, &capture);    
    ADParam->getInteger(this->params, NDPluginFileWriteMode, &fileWriteMode);    
    ADParam->getInteger(this->params, NDPluginBaseArrayCounter, &arrayCounter);    
    ADParam->getInteger(this->params, NDPluginFileNumCapture, &numCapture);    
    ADParam->getInteger(this->params, NDPluginFileNumCaptured, &numCaptured); 

    /* We always keep the last array so read() can use it.  Release it now */
    if (this->pArray) NDArrayBuff->release(this->pArray);
    this->pArray = pArray;
    
    switch(fileWriteMode) {
        case NDPluginFileModeSingle:
            if (autoSave) {
                arrayCounter++;
                status = writeFile();
            }
            break;
        case NDPluginFileModeCapture:
            if (capture) {
                arrayCounter++;
                if (numCaptured < numCapture) {
                    NDArrayBuff->copy(this->pCaptureNext++, pArray);
                    numCaptured++;
                    ADParam->setInteger(this->params, NDPluginFileNumCaptured, numCaptured);
                } 
                if (numCaptured == numCapture) {
                    capture = 0;
                    ADParam->setInteger(this->params, NDPluginFileCapture, capture);
                    if (autoSave) {
                        status = writeFile();
                    }
                }
            }
            break;
        case NDPluginFileModeStream:
            if (capture) {
                arrayCounter++;
                numCaptured++;
                ADParam->setInteger(this->params, NDPluginFileNumCaptured, numCaptured);
                status = writeFile();
                if (numCaptured == numCapture) {
                    capture = 0;
                    ADParam->setInteger(this->params, NDPluginFileCapture, capture);
                    status = writeFile();
                }
            }
            break;
    }

    /* Update the parameters.  */
    ADParam->setInteger(this->params, NDPluginBaseArrayCounter, arrayCounter);    
    ADParam->callCallbacks(this->params);
}

asynStatus NDPluginFile::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char* functionName = "writeInt32";

    epicsMutexLock(this->mutexId);

    /* Set the parameter in the parameter library. */
    status = (asynStatus) ADParam->setInteger(this->params, function, value);

    switch(function) {
        case ADWriteFile:
            if (this->pArray) {
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
    status = (asynStatus)ADParam->callCallbacks(this->params);
    
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

asynStatus NDPluginFile::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char* functionName = "writeFloat64";

    epicsMutexLock(this->mutexId);

    /* Set the parameter in the parameter library. */
    status = (asynStatus)ADParam->setDouble(this->params, function, value);

    switch(function) {
        /* We don't currently need to do anything special when these functions are received */
        default:
            /* This was not a parameter that this driver understands, try the base class */
            epicsMutexUnlock(this->mutexId);
            status = NDPluginBase::writeFloat64(pasynUser, value);
            epicsMutexLock(this->mutexId);
            break;
            break;
    }
    
    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacks(this->params);
    
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}

asynStatus NDPluginFile::writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    epicsMutexLock(this->mutexId);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)ADParam->setString(this->params, function, (char *)value);

    switch(function) {
        default:
            /* This was not a parameter that this driver understands, try the base class */
            epicsMutexUnlock(this->mutexId);
            status = NDPluginBase::writeOctet(pasynUser, value, nChars, nActual);
            epicsMutexLock(this->mutexId);
            break;
            break;
    }
    
     /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacks(this->params);

    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:writeOctet error, status=%d function=%d, value=%s\n", 
              driverName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeOctet: function=%d, value=%s\n", 
              driverName, function, value);
    *nActual = nChars;
    epicsMutexUnlock(this->mutexId);
    return status;
}

asynStatus NDPluginFile::writeNDArray(asynUser *pasynUser, void *handle)
{
    NDArray_t *pArray = (NDArray_t *)handle;
    asynStatus status = asynSuccess;
    
    epicsMutexLock(this->mutexId);
    
    this->pArray = pArray;
    ADParam->setInteger(this->params, NDPluginFileWriteMode, NDPluginFileModeSingle);

    status = writeFile();

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacks(this->params);
    
    epicsMutexUnlock(this->mutexId);
    return status;
}

/* asynDrvUser interface methods */
asynStatus NDPluginFile::drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    int status;
    int param;
    static char *functionName = "drvUserCreate";

    /* First see if this is a parameter specific to this plugin */
        status = ADUtils->findParam(NDPluginFileParamString, NUM_ND_PLUGIN_FILE_PARAMS, 
                                    drvInfo, &param);
                                    
    /* If not, then see if this is a base plugin parameter */
    if (status != asynSuccess) 
        status = ADUtils->findParam(NDPluginBaseParamString, NUM_ND_PLUGIN_BASE_PARAMS, 
                                drvInfo, &param);

    /* If not, then is it a driver parameter defined in ADInterface ? */
    if (status != asynSuccess) 
        status = ADUtils->findParam(ADStandardParamString, NUM_AD_STANDARD_PARAMS, 
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
    } else {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                     "%s:%s:, unknown drvInfo=%s", 
                     driverName, functionName, drvInfo);
        return(asynError);
    }
}




/* Configuration routine.  Called directly, or from the iocsh function in drvNDFileEpics */

extern "C" int drvNDFileConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                                  const char *NDArrayPort, int NDArrayAddr)
{
    new NDPluginFile(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr);
}

/* The constructure for this class */
NDPluginFile::NDPluginFile(const char *portName, int queueSize, int blockingCallbacks, 
                           const char *NDArrayPort, int NDArrayAddr)
    /* Invoke the base class constructor */
    : NDPluginBase(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, NDPluginFileLastParam)
{
    char *functionName = "NDPluginFile";
    asynStatus status;
    asynStandardInterfaces *pInterfaces;

    /* Set addresses of asyn interfaces */
    pInterfaces = &this->asynStdInterfaces;
    
    /* Define which interfaces can generate interrupts if more than ones defined in base class */
    
    /* Initialize asynStandardInterfaces.  Base class constructor cannot do this, because we may
     * be modifying the structure after calling base class constructor. */
    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     this->pasynUser, this);
    if (status != asynSuccess) {
        printf("%s:%s: ERROR: Can't register interfaces: %s.\n",
                driverName, functionName, this->pasynUser->errorMessage);
        return;
    }
    
    /* Connect to our device for asynTrace */
    status = pasynManager->connectDevice(this->pasynUser, portName, 0);
    if (status != asynSuccess) {
        printf("%s:%s:, connectDevice failed\n", driverName, functionName);
        return;
    }

    /* Create the epicsMutex for locking access to file I/O from other threads */
    this->fileMutexId = epicsMutexCreate();
    if (!this->fileMutexId) {
        printf("%s:%s: epicsMutexCreate failure for file mutex\n", driverName, functionName);
        return;
    }

    /* Set the initial values of some parameters */
    ADParam->setInteger(this->params, NDPluginFileNumCaptured, 0);
    ADParam->setInteger(this->params, NDPluginFileCapture, 0);
    
    /* Try to connect to the NDArray port */
    status = connectToArrayPort();
printf("%s:%s:, connectToArrayPort status=%d\n", driverName, functionName, status);
}

