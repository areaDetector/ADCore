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


/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static asynParamString_t NDPluginFileParamString[] = {
    {0, "unused"} /* We don't have currently any parameters ourselves but compiler does not like empty array */
};

#define NUM_ND_PLUGIN_FILE_PARAMS (sizeof(NDPluginFileParamString)/sizeof(NDPluginFileParamString[0]))

static const char *driverName="NDPluginFile";


/** Base method for opening a file
  * Creates the file name with NDPluginBase::createFileName, then calls the pure virtual function openFile
  * in the derived class. */
asynStatus NDPluginFile::openFileBase(NDFileOpenMode_t openMode, NDArray *pArray)
{
    /* Opens a file for reading or writing */
    asynStatus status = asynSuccess;
    char fullFileName[MAX_FILENAME_LEN];
    const char* functionName = "openFileBase";

    status = (asynStatus)createFileName(MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s:%s error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, functionName, fullFileName, status);
        return(status);
    }
    setStringParam(ADFullFileName, fullFileName);
    
    /* Call the openFile method in the derived class */
    epicsMutexLock(this->fileMutexId);
    status = this->openFile(fullFileName, openMode, pArray);
    epicsMutexUnlock(this->fileMutexId);
    
    return(status);
}

/** Base method for closing a file
  * Calls the pure virtual function closeFile in the derived class. */
asynStatus NDPluginFile::closeFileBase()
{
    /* Closes a file */
    asynStatus status = asynSuccess;
    //const char* functionName = "closeFileBase";

     /* Call the closeFile method in the derived class */
    epicsMutexLock(this->fileMutexId);
    status = this->closeFile();
    epicsMutexUnlock(this->fileMutexId);
    
    return(status);
}

/** Base method for reading a file
  * Creates the file name with NDPluginBase::createFileName, then calls the pure virtual functions openFile,
  * readFile and closeFile in the derived class.  Does callbacks with the NDArray that was read in. */
asynStatus NDPluginFile::readFileBase(void)
{
    asynStatus status = asynSuccess;
    char fullFileName[MAX_FILENAME_LEN];
    int dataType=0;
    NDArray *pArray=NULL;
    const char* functionName = "readFileBase";

    status = (asynStatus)createFileName(MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s:%s error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, functionName, fullFileName, status);
        return(status);
    }
    
    /* Call the readFile method in the derived class */
    epicsMutexLock(this->fileMutexId);
    status = this->openFile(fullFileName, NDFileModeRead, pArray);
    status = this->readFile(&pArray);
    status = this->closeFile();
    epicsMutexUnlock(this->fileMutexId);
    
    /* If we got an error then return */
    if (status) return(status);
    
    /* Update the new values of dimensions and the array data */
    setIntegerParam(ADDataType, dataType);
    
    /* Call any registered clients */
    doCallbacksGenericPointer(pArray, NDArrayData, 0);

    /* Set the last array to be this one */
    this->pArrays[0]->release();
    this->pArrays[0] = pArray;    
    
    return(status);
}

/** Base method for writing a file
  * Handles logic for ADFileModeSingle, ADFileModeCapture and ADFileModeStream when the derived class does or
  * does not support NDPulginFileMultiple. Calls writeFile in the derived class. */
asynStatus NDPluginFile::writeFileBase() 
{
    asynStatus status = asynSuccess;
    int fileWriteMode;
    int numCapture, numCaptured;
    int i;
    NDArray *pArray;
    const char* functionName = "writeFileBase";

    /* Make sure there is a valid array */
    if (!this->pArrays[0]) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must collect an array to get dimensions first\n",
            driverName, functionName);
        return(asynError);
    }
    
    getIntegerParam(ADFileWriteMode, &fileWriteMode);    
    getIntegerParam(ADFileNumCapture, &numCapture);    
    getIntegerParam(ADFileNumCaptured, &numCaptured);

    /* We unlock the overall mutex here because we want the callbacks to be able to queue new
     * frames without waiting while we write files here.  The only restriction is that the
     * callbacks must not modify any part of the class structure that we use here. */

    
    switch(fileWriteMode) {
        case ADFileModeSingle:
            this->unlock();
            epicsMutexLock(this->fileMutexId);
            status = this->writeFile(this->pArrays[0]);
            epicsMutexUnlock(this->fileMutexId);
            this->lock();
            break;
        case ADFileModeCapture:
            /* Write the file */
            if (this->supportsMultipleArrays) this->openFileBase(NDFileModeWrite | NDFileModeMultiple, this->pArrays[0]);
            for (i=0; i<numCaptured; i++) {
                pArray = this->pCapture[i];
                if (!this->supportsMultipleArrays) this->openFileBase(NDFileModeWrite, pArray);
                epicsMutexLock(this->fileMutexId);
                this->writeFile(pArray);
                epicsMutexUnlock(this->fileMutexId);
                if (!this->supportsMultipleArrays) this->closeFileBase();
            }
            /* Free the capture buffer */
            for (i=0; i<numCapture; i++) {
                pArray = this->pCapture[i];
                delete pArray;
            }
            if (this->supportsMultipleArrays) this->closeFileBase();
            free(this->pCapture);
            this->pCapture = NULL;
            setIntegerParam(ADFileNumCaptured, 0);
            break;
        case ADFileModeStream:
            this->unlock();
            epicsMutexLock(this->fileMutexId);
            status = this->writeFile(this->pArrays[0]);
            epicsMutexUnlock(this->fileMutexId);
            this->lock();
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: ERROR, unknown fileWriteMode %d\n", 
                driverName, functionName, fileWriteMode);
            break;
    }
    
    return(status);
}

/** Handles the logic for when ADFileCapture changes state, starting or stopping capturing or streaming NDArrays
  * to a file. */
asynStatus NDPluginFile::doCapture() 
{
    /* This function is called from write32 whenever capture is started or stopped */
    asynStatus status = asynSuccess;
    int fileWriteMode, capture;
    NDArray array;
    NDArrayInfo_t arrayInfo;
    int i;
    int numCapture;
    const char* functionName = "doCapture";
    
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
                this->pCapture = (NDArray **)malloc(numCapture * sizeof(NDArray *));
                if (!this->pCapture) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s ERROR: cannot allocate capture buffer\n",
                        driverName, functionName);
                    return(asynError);
                }
                for (i=0; i<numCapture; i++) {
                    pCapture[i] = new NDArray;
                    if (!this->pCapture[i]) {
                        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                            "%s:%s ERROR: cannot allocate capture buffer %d\n",
                            driverName, functionName, i);
                        return(asynError);
                    }
                    this->pCapture[i]->dataSize = arrayInfo.totalBytes;
                    this->pCapture[i]->pData = malloc(arrayInfo.totalBytes);
                    if (!this->pCapture[i]->pData) {
                        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                            "%s:%s ERROR: cannot allocate capture array for buffer %s\n",
                            driverName, functionName, i);
                        return(asynError);
                    }
                }
            } else {
                /* Stop capturing, nothing to do, setting the parameter is all that is needed */
            }
            break;
        case ADFileModeStream:
            if (capture) {
                /* Streaming was just started */
                /* We need to read an array from our array source to get its dimensions */
                array.dataSize = 0;
                status = this->pasynGenericPointer->read(this->asynGenericPointerPvt,this->pasynUserGenericPointer, &array);
                status = this->openFileBase(NDFileModeWrite | NDFileModeMultiple, &array);
                setIntegerParam(ADFileNumCaptured, 0);
                setIntegerParam(ADWriteFile, 1);
            } else {
                /* Streaming was just stopped */
                status = this->closeFileBase();
                setIntegerParam(ADWriteFile, 0);
                setIntegerParam(ADFileNumCaptured, 0);
            }
    }
    return(status);
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Saves a single file if ADFileWriteMode=ADFileModeSingle and ADAutoSave=1.
  * Stores array in a capture buffer if ADFileWriteMode=ADFileModeCapture and ADFileCapture=1.
  * Appends data to an open file if ADFileWriteMode=ADFileModeStream and ADFileCapture=1.
  * In capture or stream mode if the desired number of arrays has been saved (ADFileNumCaptured=ADFileNumCapture)
  * then it stops capture or streaming.
  * \param[in] pArray  The NDArray from the callback.
  */ 
void NDPluginFile::processCallbacks(NDArray *pArray)
{
    int fileWriteMode, autoSave, capture;
    int arrayCounter;
    int status=asynSuccess;
    int numCapture, numCaptured;
    //const char* functionName = "processCallbacks";

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
                status = openFileBase(NDFileModeWrite, this->pArrays[0]);
                status = writeFileBase();
                status = closeFileBase();
            }
            break;
        case ADFileModeCapture:
            if (capture) {
                if (numCaptured < numCapture) {
                    this->pNDArrayPool->copy(pArray, this->pCapture[numCaptured++], 1);
                    arrayCounter++;
                    setIntegerParam(ADFileNumCaptured, numCaptured);
                } 
                if (numCaptured == numCapture) {
                    if (autoSave) {
                        status = writeFileBase();
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
                status = writeFileBase();
                if (numCaptured == numCapture) {
                    status = closeFileBase();
                    capture = 0;
                    setIntegerParam(ADFileCapture, capture);
                    setIntegerParam(ADWriteFile, 0);
                }
            }
            break;
    }

    /* Update the parameters.  */
    setIntegerParam(NDPluginDriverArrayCounter, arrayCounter);
    callParamCallbacks();
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including ADReadFile, ADWriteFile and ADFileCapture.
  * For other parameters it calls NDPluginDriver::writeInt32 to see if that method understands the parameter. 
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
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
                    status = writeFileBase();
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
                status = readFileBase();
                /* Set the flag back to 0, since this could be a busy record */
                setIntegerParam(ADReadFile, 0);
            }
            break;
        case ADFileCapture:
            /* Must call doCapture if capturing was just started or stopped */
            status = doCapture();
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
    //const char *functionName = "writeNDArray";
        
    this->pArrays[0] = pArray;
    setIntegerParam(ADFileWriteMode, ADFileModeSingle);

    status = writeFileBase();

    /* Do callbacks so higher layers see any changes */
    status = callParamCallbacks();
    
    return status;
}

/** Sets pasynUser->reason to one of the enum values for the parameters defined for
  * the NDPluginFile class if the drvInfo field matches one the strings defined for it.
  * If the parameter is not recognized by this class then calls NDPluginDriver::drvUserCreate.
  * Uses asynPortDriver::drvUserCreateParam.
  * \param[in] pasynUser pasynUser structure that driver modifies
  * \param[in] drvInfo String containing information about what driver function is being referenced
  * \param[out] pptypeName Location in which driver puts a copy of drvInfo.
  * \param[out] psize Location where driver puts size of param 
  * \return Returns asynSuccess if a matching string was found, asynError if not found. */
asynStatus NDPluginFile::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    asynStatus status;
    //const char *functionName = "drvUserCreate";
    
    status = this->drvUserCreateParam(pasynUser, drvInfo, pptypeName, psize, 
                                      NDPluginFileParamString, NUM_ND_PLUGIN_FILE_PARAMS);

    /* If not, then call the base class method, see if it is known there */
    if (status) status = NDPluginDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return(status);
}


/** Constructor for NDPluginFile; all parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for many file-related 
  *  parameters defined in ADStdDriverParams.h.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginFile::NDPluginFile(const char *portName, int queueSize, int blockingCallbacks, 
                           const char *NDArrayPort, int NDArrayAddr,
                           int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 1 NDArray of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginDriver(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, 1, NDPluginFileLastParam, 1, -1, 
                   asynGenericPointerMask, asynGenericPointerMask,
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    //const char *functionName = "NDPluginFile";
    asynStatus status;

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
    
    this->fileMutexId = epicsMutexCreate();
    
    /* Try to connect to the NDArray port */
    status = connectToArrayPort();
}

