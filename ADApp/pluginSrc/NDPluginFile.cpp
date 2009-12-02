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

#include "NDPluginFile.h"


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
    setStringParam(NDFullFileName, fullFileName);
    
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
    setIntegerParam(NDDataType, dataType);
    
    /* Call any registered clients */
    doCallbacksGenericPointer(pArray, NDArrayData, 0);

    /* Set the last array to be this one */
    this->pArrays[0]->release();
    this->pArrays[0] = pArray;    
    
    return(status);
}

/** Base method for writing a file
  * Handles logic for NDFileModeSingle, NDFileModeCapture and NDFileModeStream when the derived class does or
  * does not support NDPulginFileMultiple. Calls writeFile in the derived class. */
asynStatus NDPluginFile::writeFileBase() 
{
    int status = asynSuccess;
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
    
    getIntegerParam(NDFileWriteMode, &fileWriteMode);    
    getIntegerParam(NDFileNumCapture, &numCapture);    
    getIntegerParam(NDFileNumCaptured, &numCaptured);

    /* We unlock the overall mutex here because we want the callbacks to be able to queue new
     * frames without waiting while we write files here.  The only restriction is that the
     * callbacks must not modify any part of the class structure that we use here. */

    
    switch(fileWriteMode) {
        case NDFileModeSingle:
            setIntegerParam(NDWriteFile, 1);
            callParamCallbacks();
            status = this->openFileBase(NDFileModeWrite, this->pArrays[0]);
            if (status == asynSuccess) {
                this->unlock();
                epicsMutexLock(this->fileMutexId);
                status = this->writeFile(this->pArrays[0]);
                epicsMutexUnlock(this->fileMutexId);
                this->lock();
                status += this->closeFileBase();
                setIntegerParam(NDWriteFile, 0);
                callParamCallbacks();
            }
            break;
        case NDFileModeCapture:
            /* Write the file */
            if (!this->pCapture) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: ERROR, no capture buffer present\n", 
                    driverName, functionName);
                break;
            }
            setIntegerParam(NDWriteFile, 1);
            callParamCallbacks();
            if (this->supportsMultipleArrays)
                status = this->openFileBase(NDFileModeWrite | NDFileModeMultiple, this->pArrays[0]);
            if (status == asynSuccess) {
                for (i=0; i<numCaptured; i++) {
                    pArray = this->pCapture[i];
                    if (!this->supportsMultipleArrays)
                        status = this->openFileBase(NDFileModeWrite, pArray);
                    if (status == asynSuccess) {
                        epicsMutexLock(this->fileMutexId);
                        status = this->writeFile(pArray);
                        epicsMutexUnlock(this->fileMutexId);
                        if (!this->supportsMultipleArrays)
                            status += this->closeFileBase();
                    }
                }
            }
            /* Free the capture buffer */
            for (i=0; i<numCapture; i++) {
                pArray = this->pCapture[i];
                delete pArray;
            }
            if (this->supportsMultipleArrays) 
                status = this->closeFileBase();
            free(this->pCapture);
            this->pCapture = NULL;
            setIntegerParam(NDFileNumCaptured, 0);
            setIntegerParam(NDWriteFile, 0);
            callParamCallbacks();
            break;
        case NDFileModeStream:
            if (!this->supportsMultipleArrays)
                status = this->openFileBase(NDFileModeWrite | NDFileModeMultiple, this->pArrays[0]);
            this->unlock();
            epicsMutexLock(this->fileMutexId);
            status = this->writeFile(this->pArrays[0]);
            epicsMutexUnlock(this->fileMutexId);
            this->lock();
            if (!this->supportsMultipleArrays) 
                status = this->closeFileBase();
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: ERROR, unknown fileWriteMode %d\n", 
                driverName, functionName, fileWriteMode);
            break;
    }
    
    return((asynStatus)status);
}

/** Handles the logic for when NDFileCapture changes state, starting or stopping capturing or streaming NDArrays
  * to a file.
  * \param[in] capture Flag to start or stop capture; 1=start capture, 0=stop capture. */
asynStatus NDPluginFile::doCapture(int capture) 
{
    /* This function is called from write32 whenever capture is started or stopped */
    asynStatus status = asynSuccess;
    int fileWriteMode;
    NDArray *pArray = this->pArrays[0];
    NDArrayInfo_t arrayInfo;
    int i;
    int numCapture;
    const char* functionName = "doCapture";

    /* Make sure there is a valid array */
    if (!pArray) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: ERROR, must collect an array to get dimensions first\n",
            driverName, functionName);
        return(asynError);
    }
    
    getIntegerParam(NDFileWriteMode, &fileWriteMode);    
    getIntegerParam(NDFileNumCapture, &numCapture);

    switch(fileWriteMode) {
        case NDFileModeSingle:
            /* It is an error to set capture=1 in this mode, set to 0 */
            setIntegerParam(NDFileCapture, 0);
            break;
        case NDFileModeCapture:
            if (capture) {
                /* Capturing was just started */
                setIntegerParam(NDFileNumCaptured, 0);
                pArray->getInfo(&arrayInfo);
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
        case NDFileModeStream:
            if (capture) {
                /* Streaming was just started */
                if (this->supportsMultipleArrays)
                    status = this->openFileBase(NDFileModeWrite | NDFileModeMultiple, pArray);
                setIntegerParam(NDFileNumCaptured, 0);
                setIntegerParam(NDWriteFile, 1);
            } else {
                /* Streaming was just stopped */
                if (this->supportsMultipleArrays)
                    status = this->closeFileBase();
                setIntegerParam(NDFileCapture, 0);
                setIntegerParam(NDWriteFile, 0);
            }
    }
    return(status);
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Saves a single file if NDFileWriteMode=NDFileModeSingle and NDAutoSave=1.
  * Stores array in a capture buffer if NDFileWriteMode=NDFileModeCapture and NDFileCapture=1.
  * Appends data to an open file if NDFileWriteMode=NDFileModeStream and NDFileCapture=1.
  * In capture or stream mode if the desired number of arrays has been saved (NDFileNumCaptured=NDFileNumCapture)
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
    getIntegerParam(NDArrayCounter, &arrayCounter);

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    getIntegerParam(NDAutoSave, &autoSave);
    getIntegerParam(NDFileCapture, &capture);    
    getIntegerParam(NDFileWriteMode, &fileWriteMode);    
    getIntegerParam(NDFileNumCapture, &numCapture);    
    getIntegerParam(NDFileNumCaptured, &numCaptured);

    /* We always keep the last array so read() can use it.  
     * Release previous one, reserve new one */
    if (this->pArrays[0]) this->pArrays[0]->release();
    pArray->reserve();
    this->pArrays[0] = pArray;
    
    switch(fileWriteMode) {
        case NDFileModeSingle:
            if (autoSave) {
                arrayCounter++;
                status = writeFileBase();
            }
            break;
        case NDFileModeCapture:
            if (capture) {
                if (numCaptured < numCapture) {
                    this->pNDArrayPool->copy(pArray, this->pCapture[numCaptured++], 1);
                    arrayCounter++;
                    setIntegerParam(NDFileNumCaptured, numCaptured);
                } 
                if (numCaptured == numCapture) {
                    if (autoSave) {
                        status = writeFileBase();
                    }
                    capture = 0;
                    setIntegerParam(NDFileCapture, capture);
                }
            }
            break;
        case NDFileModeStream:
            if (capture) {
                numCaptured++;
                arrayCounter++;
                setIntegerParam(NDFileNumCaptured, numCaptured);
                status = writeFileBase();
                if (numCaptured == numCapture) {
                    doCapture(0);
                }
            }
            break;
    }

    /* Update the parameters.  */
    setIntegerParam(NDArrayCounter, arrayCounter);
    callParamCallbacks();
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including NDReadFile, NDWriteFile and NDFileCapture.
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

    if (function == NDWriteFile) {
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
            setIntegerParam(NDWriteFile, 0);
        }
    } else if (function == NDReadFile) {
        if (value) {
            /* Call the callbacks so the status changes */
            callParamCallbacks();
            status = readFileBase();
            /* Set the flag back to 0, since this could be a busy record */
            setIntegerParam(NDReadFile, 0);
        }
    } else if (function == NDFileCapture) {
        /* Must call doCapture if capturing was just started or stopped */
        status = doCapture(value);
    } else {
        /* This was not a parameter that this driver understands, try the base class */
        if (function <= LAST_NDPLUGIN_PARAM) status = NDPluginDriver::writeInt32(pasynUser, value);
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
    setIntegerParam(NDFileWriteMode, NDFileModeSingle);

    status = writeFileBase();

    /* Do callbacks so higher layers see any changes */
    status = callParamCallbacks();
    
    return status;
}

/** Constructor for NDPluginFile; all parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxAddr The maximum  number of asyn addr addresses this driver supports. 1 is minimum.
  * \param[in] numParams The number of parameters supported by the derived class calling this constructor.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] interfaceMask Bit mask defining the asyn interfaces that this driver supports.
  * \param[in] interruptMask Bit mask definining the asyn interfaces that can generate interrupts (callbacks)
  * \param[in] asynFlags Flags when creating the asyn port driver; includes ASYN_CANBLOCK and ASYN_MULTIDEVICE.
  * \param[in] autoConnect The autoConnect flag for the asyn port driver.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginFile::NDPluginFile(const char *portName, int queueSize, int blockingCallbacks, 
                           const char *NDArrayPort, int NDArrayAddr, int maxAddr, int numParams,
                           int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask,
                           int asynFlags, int autoConnect, int priority, int stackSize)

    /* Invoke the base class constructor.
     * We allocate 1 NDArray of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginDriver(portName, queueSize, blockingCallbacks, 
                     NDArrayPort, NDArrayAddr, maxAddr, numParams+NUM_NDPLUGIN_FILE_PARAMS, maxBuffers, maxMemory, 
                     asynGenericPointerMask, asynGenericPointerMask,
                     asynFlags, autoConnect, priority, stackSize)
{
    //const char *functionName = "NDPluginFile";
    asynStatus status;
    
    this->fileMutexId = epicsMutexCreate();
    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDPluginFile");
    
    /* Try to connect to the NDArray port */
    status = connectToArrayPort();
}

