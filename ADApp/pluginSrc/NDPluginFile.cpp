/*
 * NDPluginFile.cpp
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

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsTime.h>
#include <epicsString.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include <NDPluginDriver.h>
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
    char tempSuffix[MAX_FILENAME_LEN];
    char errorMessage[256];
    static const char* functionName = "openFileBase";

    if (this->useAttrFilePrefix)
        this->attrFileNameSet();

    setIntegerParam(NDFileWriteStatus, NDFileWriteOK);
    setStringParam(NDFileWriteMessage, "");
    status = (asynStatus)createFileName(MAX_FILENAME_LEN, fullFileName);
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s:%s error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, functionName, fullFileName, status);
        setIntegerParam(NDFileWriteStatus, NDFileWriteError);
        setStringParam(NDFileWriteMessage, "Error creating full file name");
        return(status);
    }
    setStringParam(NDFullFileName, fullFileName);
    
    getStringParam(NDFileTempSuffix, sizeof(tempSuffix), tempSuffix);
    if ( *tempSuffix != 0 && 
         (strlen(fullFileName) + strlen(tempSuffix)) < sizeof(fullFileName) ) {
        strcat( fullFileName, tempSuffix );
    }

    /* Call the openFile method in the derived class */
    /* Do this with the main lock released since it is slow */
    this->unlock();
    epicsMutexLock(this->fileMutexId);
    this->registerInitFrameInfo(pArray);
    status = this->openFile(fullFileName, openMode, pArray);
    if (status) {
        epicsSnprintf(errorMessage, sizeof(errorMessage)-1, 
            "Error opening file %s, status=%d", fullFileName, status);
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s:%s %s\n", 
              driverName, functionName, errorMessage);
        setIntegerParam(NDFileWriteStatus, NDFileWriteError);
        setStringParam(NDFileWriteMessage, errorMessage);
    }
    epicsMutexUnlock(this->fileMutexId);
    this->lock();
    
    return(status);
}

/** Base method for closing a file
  * Calls the pure virtual function closeFile in the derived class. */
asynStatus NDPluginFile::closeFileBase()
{
    /* Closes a file */
    asynStatus status = asynSuccess;
    char fullFileName[MAX_FILENAME_LEN];
    char tempSuffix[MAX_FILENAME_LEN];
    char tempFileName[MAX_FILENAME_LEN];
    char errorMessage[256];
    static const char* functionName = "closeFileBase";

    setIntegerParam(NDFileWriteStatus, NDFileWriteOK);
    setStringParam(NDFileWriteMessage, "");

    getStringParam(NDFullFileName, sizeof(fullFileName), fullFileName);
    getStringParam(NDFileTempSuffix, sizeof(tempSuffix), tempSuffix);

    /* Call the closeFile method in the derived class */
    /* Do this with the main lock released since it is slow */
    this->unlock();
    epicsMutexLock(this->fileMutexId);
    status = this->closeFile();
    if (status) {
        epicsSnprintf(errorMessage, sizeof(errorMessage)-1, 
            "Error closing file, status=%d", status);
    }

    if ( *tempSuffix != 0 && 
         (strlen(fullFileName) - strlen(tempSuffix)) < sizeof(fullFileName) ) {
        strcpy( tempFileName, fullFileName );
        strcat( tempFileName, tempSuffix );
        if ( rename( tempFileName, fullFileName ) != 0 ) {
            epicsSnprintf(errorMessage, sizeof(errorMessage)-1, 
                          "Error renaming temporary file %s to %s", tempFileName, fullFileName );
            status=asynError;
        }
    }

    epicsMutexUnlock(this->fileMutexId);
    this->lock();
    if (status) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s:%s %s\n", 
              driverName, functionName, errorMessage);
        setIntegerParam(NDFileWriteStatus, NDFileWriteError);
        setStringParam(NDFileWriteMessage, errorMessage);
    }

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
    static const char* functionName = "readFileBase";

    status = (asynStatus)createFileName(MAX_FILENAME_LEN, fullFileName);
    if (status) { 
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s:%s error creating full file name, fullFileName=%s, status=%d\n", 
              driverName, functionName, fullFileName, status);
        return(status);
    }
    
    /* Call the readFile method in the derived class */
    /* Do this with the main lock released since it is slow */
    this->unlock();
    epicsMutexLock(this->fileMutexId);
    status = this->openFile(fullFileName, NDFileModeRead, pArray);
    status = this->readFile(&pArray);
    status = this->closeFile();
    epicsMutexUnlock(this->fileMutexId);
    this->lock();
    
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
    bool doLazyOpen;
    int deleteDriverFile;
    NDArray *pArray;
    NDAttribute *pAttribute;
    char driverFileName[MAX_FILENAME_LEN];
    char errorMessage[256];
    static const char* functionName = "writeFileBase";

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

    setIntegerParam(NDFileWriteStatus, NDFileWriteOK);
    setStringParam(NDFileWriteMessage, "");
    
    /* We unlock the overall mutex here because we want the callbacks to be able to queue new
     * frames without waiting while we write files here.  The only restriction is that the
     * callbacks must not modify any part of the class structure that we use here. */

    switch(fileWriteMode) {
        case NDFileModeSingle:
            setIntegerParam(NDWriteFile, 1);
            callParamCallbacks();
            // Some file writing plugins (e.g. HDF5) use the value of NDFileNumCaptured 
            // even in single mode
            setIntegerParam(NDFileNumCaptured, 1);
            status = this->openFileBase(NDFileModeWrite, this->pArrays[0]);
            if (status == asynSuccess) {
                NDArray *pArrayOut = this->pArrays[0];
                this->unlock();
                epicsMutexLock(this->fileMutexId);
                status = this->writeFile(pArrayOut);
                epicsMutexUnlock(this->fileMutexId);
                this->lock();
                doNDArrayCallbacks(pArrayOut);
                if (status) {
                    epicsSnprintf(errorMessage, sizeof(errorMessage)-1, 
                        "Error writing file, status=%d", status);
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                          "%s:%s %s\n", 
                          driverName, functionName, errorMessage);
                    setIntegerParam(NDFileWriteStatus, NDFileWriteError);
                    setStringParam(NDFileWriteMessage, errorMessage);
                } else {
                    status = this->closeFileBase(); 
                }
            }
            setIntegerParam(NDWriteFile, 0);
            callParamCallbacks();
            break;
        case NDFileModeCapture:
            /* Write the file */
            if (!this->pCapture) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: ERROR, no capture buffer present\n", 
                    driverName, functionName);
                setIntegerParam(NDFileWriteStatus, NDFileWriteError);
                setStringParam(NDFileWriteMessage, "ERROR, no capture buffer present");
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
                    else
                        this->attrFileNameCheck();
                    if (status == asynSuccess) {
                        this->unlock();
                        epicsMutexLock(this->fileMutexId);
                        status = this->writeFile(pArray);
                        epicsMutexUnlock(this->fileMutexId);
                        this->lock();
                        doNDArrayCallbacks(pArray);
                        if (status) {
                            epicsSnprintf(errorMessage, sizeof(errorMessage)-1, 
                                "Error writing file, status=%d", status);
                            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                                  "%s:%s %s\n", 
                                  driverName, functionName, errorMessage);
                            setIntegerParam(NDFileWriteStatus, NDFileWriteError);
                            setStringParam(NDFileWriteMessage, errorMessage);
                        } else {
                            if (!this->supportsMultipleArrays)
                                status = this->closeFileBase();
                        }
                    }
                }
            }
            freeCaptureBuffer(numCapture);
            if ((status == asynSuccess) && this->supportsMultipleArrays) 
                status = this->closeFileBase();
            this->registerInitFrameInfo(NULL);
            setIntegerParam(NDFileNumCaptured, 0);
            setIntegerParam(NDWriteFile, 0);
            callParamCallbacks();
            break;
        case NDFileModeStream:
            doLazyOpen = this->lazyOpen && (numCaptured == 0);
            if (!this->supportsMultipleArrays || doLazyOpen)
                status = this->openFileBase(NDFileModeWrite | NDFileModeMultiple, this->pArrays[0]);
            else
                this->attrFileNameCheck();
            if (!this->isFrameValid(this->pArrays[0])) {
                setIntegerParam(NDFileWriteStatus, NDFileWriteError);
                setStringParam(NDFileWriteMessage, "Invalid frame. Ignoring.");
                status = asynError;
            }
            if (status == asynSuccess) {
                NDArray *pArrayOut = this->pArrays[0];
                this->unlock();
                epicsMutexLock(this->fileMutexId);
                status = this->writeFile(pArrayOut);
                epicsMutexUnlock(this->fileMutexId);
                this->lock();
                doNDArrayCallbacks(pArrayOut);
                if (status) {
                    epicsSnprintf(errorMessage, sizeof(errorMessage)-1,
                            "Error writing file, status=%d", status);
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                            "%s:%s %s\n",
                            driverName, functionName, errorMessage);
                    setIntegerParam(NDFileWriteStatus, NDFileWriteError);
                    setStringParam(NDFileWriteMessage, errorMessage);
                } else {
                    status = this->attrFileCloseCheck();
                    if (!this->supportsMultipleArrays)
                        status = this->closeFileBase();
                }
            }
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: ERROR, unknown fileWriteMode %d\n", 
                driverName, functionName, fileWriteMode);
            break;
    }
    
    /* Check to see if we should delete the original file
     * Only do this if all of the following conditions are met
     *  - DeleteOriginalFile is true
     *  - There were no errors above
     *  - The NDFullFileName attribute is present and contains a non-blank string
     */
    getIntegerParam(NDFileDeleteDriverFile, &deleteDriverFile);
    if ((status == asynSuccess) && deleteDriverFile) {
        pAttribute = this->pArrays[0]->pAttributeList->find("DriverFileName");
        if (pAttribute) {
            status = pAttribute->getValue(NDAttrString, driverFileName, sizeof(driverFileName));
            if ((status == asynSuccess) && (strlen(driverFileName) > 0)) {
                status = remove(driverFileName);
                if (status != 0) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                              "%s:%s: error deleting file %s, error=%s\n",
                              driverName, functionName, driverFileName, strerror(errno));
                }
            }
        }
    }

    return((asynStatus)status);
}

void NDPluginFile::freeCaptureBuffer(int numCapture)
{
    int i;
    NDArray *pArray;
    
    if (!this->pCapture) return;
    /* Free the capture buffer */
    for (i=0; i<numCapture; i++) {
        pArray = this->pCapture[i];
        if (!pArray) break;
        delete pArray;
    }
    free(this->pCapture);
    this->pCapture = NULL;
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
    static const char* functionName = "doCapture";

    /* Make sure there is a valid array if capture is set to 1 */
    if (!pArray && !this->lazyOpen) {
        if (capture == 0){
            /* No error here, but just return straight away as stop capture is non operation */
            return(asynSuccess);
        } else {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: ERROR, must collect an array to get dimensions first\n",
                driverName, functionName);
            return(asynError);
        }
    }
    
    /* Decide whether or not to use the NDAttribute named "fileprefix" to create the filename */
    if (pArray) {
        if( pArray->pAttributeList->find(FILEPLUGIN_NAME) != NULL)
            this->useAttrFilePrefix = true;
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
                if (!pArray) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s ERROR: No arrays collected: cannot allocate capture buffer\n",
                        driverName, functionName);
                    return(asynError);
                }
                pArray->getInfo(&arrayInfo);
                this->registerInitFrameInfo(pArray);
                this->pCapture = (NDArray **)calloc(numCapture, sizeof(NDArray *));
                if (!this->pCapture) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s ERROR: cannot allocate capture buffer\n",
                        driverName, functionName);
                    setIntegerParam(NDFileCapture, 0);
                    return(asynError);
                }
                for (i=0; i<numCapture; i++) {
                    pCapture[i] = new NDArray;
                    if (!this->pCapture[i]) {
                        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                            "%s:%s ERROR: cannot allocate capture buffer %d\n",
                            driverName, functionName, i);
                        setIntegerParam(NDFileCapture, 0);
                        freeCaptureBuffer(numCapture);
                        return(asynError);
                    }
                    this->pCapture[i]->dataSize = arrayInfo.totalBytes;
                    this->pCapture[i]->pData = malloc(arrayInfo.totalBytes);
                    this->pCapture[i]->ndims = pArray->ndims;
                    if (!this->pCapture[i]->pData) {
                        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                            "%s:%s ERROR: cannot allocate capture array for buffer %d\n",
                            driverName, functionName, i);
                        setIntegerParam(NDFileCapture, 0);
                        freeCaptureBuffer(numCapture);
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
                if (this->supportsMultipleArrays && !this->useAttrFilePrefix && !this->lazyOpen)
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

/** Check whether an attribute asking the file to be closed has been set.
 *  if the value of FILEPLUGIN_CLOSE attribute is set to 1 then close the file.
 */
asynStatus NDPluginFile::attrFileCloseCheck()
{
    asynStatus status = asynSuccess;
    NDAttribute *NDattrFileClose;
    int getStatus = 0;
    int closeFile = 0;
    NDattrFileClose = this->pArrays[0]->pAttributeList->find(FILEPLUGIN_CLOSE);
    // Check for the existence of the parameter
    if (NDattrFileClose != NULL) {
        // Check NDAttribute value (0 = continue, anything else = close file)
        getStatus = NDattrFileClose->getValue(NDAttrInt32, &closeFile);
        if (getStatus == 0){
            if (closeFile != 0){
                // Force a file close
                this->closeFileBase();
                // We must also set the parameter to notify we have stopped capturing
                setIntegerParam(NDFileCapture, 0);
            }
        } else {
            status = asynError;
        }
    }
    return status;
}

/** Check whether the attributes defining the filename has changed since last write.
 * If this is the first frame (NDFileNumCaptured == 1) then the file is opened.
 * For other frames we check whether the attribute file name or number has changed
 * since last write. If this is the case then the current file is closed a new one opened.
 */
asynStatus NDPluginFile::attrFileNameCheck()
{
    asynStatus status = asynSuccess;
    NDAttribute *NDattrFileName, *NDattrFileNumber;
    int compare, attrFileNumber, ndFileNumber;
    char attrFileName[MAX_FILENAME_LEN];
    char ndFileName[MAX_FILENAME_LEN];
    int numCapture, numCaptured;
    bool reopenFile = false;

    if (!this->useAttrFilePrefix) return status;

    getIntegerParam(NDFileNumCapture, &numCapture);
    getIntegerParam(NDFileNumCaptured, &numCaptured);

    if (numCaptured == 1) {
        status = this->openFileBase(NDFileModeWrite | NDFileModeMultiple, this->pArrays[0]);
        return status;
    }

    NDattrFileName   = this->pArrays[0]->pAttributeList->find(FILEPLUGIN_NAME);
    if (NDattrFileName != NULL) {
        NDattrFileName->getValue(NDAttrString, attrFileName, MAX_FILENAME_LEN);
        getStringParam(NDFileName, MAX_FILENAME_LEN, ndFileName);
        compare = epicsStrnCaseCmp(attrFileName, ndFileName, strlen(attrFileName));
        if (compare != 0)
            reopenFile = true;
    }

    NDattrFileNumber = this->pArrays[0]->pAttributeList->find(FILEPLUGIN_NUMBER);
    if (NDattrFileNumber != NULL)
    {
        NDattrFileNumber->getValue(NDAttrInt32, (void*) &attrFileNumber, 0);
        getIntegerParam(NDFileNumber, &ndFileNumber);
        if (ndFileNumber != attrFileNumber)
        {
            reopenFile = true;
            setIntegerParam( NDFileNumber, attrFileNumber);
        }
    }
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "attrFileNameCheck: name=%s(%s) num=%d (%s) reopen=%d\n",
            attrFileName, ndFileName, attrFileNumber, NDattrFileNumber->getSource(), (int)reopenFile );
    if (reopenFile)
    {
        this->closeFileBase();
        setIntegerParam(NDFileNumCaptured, 1);
        status = this->openFileBase(NDFileModeWrite | NDFileModeMultiple, this->pArrays[0]);
    }
    return status;
}

/** Look up filename related attributes in the NDArray.
 *  If file name or number is found in the NDArray, the values are replacing the existing ones
 *  in the parameter library. If not found the existing settings remain.
 */
asynStatus NDPluginFile::attrFileNameSet()
{
    asynStatus status = asynSuccess;
    NDAttribute *ndAttr;
    char attrFileName[MAX_FILENAME_LEN];
    epicsInt32 attrFileNumber;
    size_t attrFileNameLen;
    NDAttrDataType_t attrDataType;
    NDArray *pArray = this->pArrays[0];

    if (this->useAttrFilePrefix == false)
        return status;

    /* first check if the attribute contain a fileprefix to form part of the filename. */
    ndAttr = pArray->pAttributeList->find(FILEPLUGIN_NAME);
    if (ndAttr != NULL)
    {
        ndAttr->getValueInfo(&attrDataType, &attrFileNameLen);
        if (attrDataType == NDAttrString)
        {
            if (attrFileNameLen > MAX_FILENAME_LEN) attrFileNameLen = MAX_FILENAME_LEN;
            ndAttr->getValue(NDAttrString, attrFileName, attrFileNameLen);
            setStringParam(NDFileName, attrFileName);
        }
    }

    ndAttr = pArray->pAttributeList->find(FILEPLUGIN_NUMBER);
    if (ndAttr != NULL)
    {
        ndAttr->getValueInfo(&attrDataType, &attrFileNameLen);
        if (attrDataType == NDAttrInt32)
        {
            ndAttr->getValue(NDAttrInt32, &attrFileNumber, 0);
            setIntegerParam(NDFileNumber, attrFileNumber);
            // ensure auto increment is switched off when using attribute file numbers
            setIntegerParam(NDAutoIncrement, 0);
        }
    }
    return status;
}

/** Decide whether or not this frame is intended to be processed by this plugin.
 * By default all frames are processed. The decision not to process a frame is
 * made based on the string value of the FILEPLUGIN_DESTINATION: if the value does not equal
 * either "all" or the ASYN port name of the current plugin the frame is not to be processed.
 * \param[in] pAttrList  A pointer to the current NDArray's attribute list.
 * \returns true if the frame is to be processed. false if the frame is not to be processed.
 */
bool NDPluginFile::attrIsProcessingRequired(NDAttributeList* pAttrList)
{
    char destPortName[MAX_FILENAME_LEN];
    NDAttribute *ndAttr;
    size_t destPortNameLen;
    NDAttrDataType_t attrDataType;

    ndAttr = pAttrList->find(FILEPLUGIN_DESTINATION);
    if (ndAttr != NULL)
    {
        ndAttr->getValueInfo(&attrDataType, &destPortNameLen);
        if (attrDataType == NDAttrString && destPortNameLen > 1)
        {
            if (destPortNameLen > MAX_FILENAME_LEN)
                destPortNameLen = MAX_FILENAME_LEN;
                ndAttr->getValue(NDAttrString, destPortName, destPortNameLen);
            if (epicsStrnCaseCmp(destPortName, "all", destPortNameLen>3?3:destPortNameLen) != 0 &&
                epicsStrnCaseCmp(destPortName, this->portName, destPortNameLen) != 0)
                return false;
        }
    }
    return true;
}

void NDPluginFile::registerInitFrameInfo(NDArray *pArray)
{
    if (this->ndArrayInfoInit != NULL) {
        free(this->ndArrayInfoInit);
        this->ndArrayInfoInit = NULL;
    }
    if (pArray == NULL) return;
    this->ndArrayInfoInit = (NDArrayInfo_t*)calloc(1, sizeof(NDArrayInfo_t));
    if (this->ndArrayInfoInit == NULL) return;
    pArray->getInfo(this->ndArrayInfoInit);
}

bool NDPluginFile::isFrameValid(NDArray *pArray)
{
    if (pArray == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "NDPluginFile::isFrameValid: pArray == NULL\n");
        return false;
    }
    NDArrayInfo_t info;
    NDArrayInfo_t *initInfo = this->ndArrayInfoInit;
    if (initInfo == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "NDPluginFile::isFrameValid: no init array info was registered\n");
        return false;
    }
    bool valid = true;

    if (pArray->getInfo(&info) != ND_SUCCESS) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "NDPluginFile::isFrameValid: Unable to get NDArray info from pArray=%p\n", pArray);
        return false;
    }

    // Check the number of bytes per element
    if (initInfo->bytesPerElement != info.bytesPerElement) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "NDPluginFile::isFrameValid: WARNING: Datatype bit-width has changed. Was: %dbytes now: %dbytes\n",
                initInfo->bytesPerElement, info.bytesPerElement);
        valid = false;
    }

    // Check frame size in X and Y dimensions
    if ((initInfo->xSize != info.xSize) || (initInfo->ySize != info.ySize)) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "NDPluginFile::isFrameValid: WARNING: Frame dimensions have changed X:%lu,%lu Y:%lu,%lu]\n",
                (unsigned long)initInfo->xSize, (unsigned long)info.xSize,
                (unsigned long)initInfo->ySize, (unsigned long)info.ySize);
        valid = false;
    }

    return valid;
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
    int numCapture, numCaptured;
    asynStatus status = asynSuccess;
    //static const char* functionName = "processCallbacks";

    /* First check if the callback is really for this file saving plugin */
    if (!this->attrIsProcessingRequired(pArray->pAttributeList))
        return;

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
                writeFileBase();
            }
            break;
        case NDFileModeCapture:
            if (capture) {
                if (numCaptured < numCapture && this->isFrameValid(pArray)) {
                    this->pNDArrayPool->copy(pArray, this->pCapture[numCaptured++], 1);
                    arrayCounter++;
                    setIntegerParam(NDFileNumCaptured, numCaptured);
                } 
                if (numCaptured == numCapture) {
                    if (autoSave) {
                        writeFileBase();
                    }
                    capture = 0;
                    setIntegerParam(NDFileCapture, capture);
                }
            }
            break;
        case NDFileModeStream:
            if (capture) {
                arrayCounter++;
                status = writeFileBase();
                if (status == asynSuccess) {
                    numCaptured++;
                    setIntegerParam(NDFileNumCaptured, numCaptured);
                }
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

void NDPluginFile::doNDArrayCallbacks(NDArray *pArray)
{
  int arrayCallbacks = 0;
  static const char *functionName = "doNDArrayCallbacks";

  getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
  if (arrayCallbacks == 1) {
    NDArray *pArrayOut = this->pNDArrayPool->copy(pArray, NULL, 1);
    if (pArrayOut != NULL) {
      this->getAttributes(pArrayOut->pAttributeList);
      this->unlock();
      doCallbacksGenericPointer(pArrayOut, NDArrayData, 0);
      this->lock();
      pArrayOut->release();
    }
    else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s: Couldn't allocate output array. Callbacks failed.\n", 
        functionName);
    }
  }
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
    static const char* functionName = "writeInt32";

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
        if (value) {  // Started capture or stream
            // Reset the value temporarily until the doCapture() has called the
            // inherited openFile() method and the writer is in a good state to
            // start writing frames.
            // See comments on: https://github.com/areaDetector/ADCore/pull/100
            setIntegerParam(NDFileCapture, 0);

            /* Latch the NDFileLazyOpen parameter so that we don't need to care
             * if the user modifies this parameter before first frame has arrived. */
            int paramFileLazyOpen = 0;
            getIntegerParam(NDFileLazyOpen, &paramFileLazyOpen);
            this->lazyOpen = (paramFileLazyOpen != 0);
            /* So far everything is OK, so we just clear the FileWriteStatus parameters */
            setIntegerParam(NDFileWriteStatus, NDFileWriteOK);
            setStringParam(NDFileWriteMessage, "");
            setStringParam(NDFullFileName, "");
        }
        /* Must call doCapture if capturing was just started or stopped */
        status = doCapture(value);
        if (status == asynSuccess) {
            if (this->lazyOpen) setStringParam(NDFileWriteMessage, "Lazy Open...");
            setIntegerParam(NDFileCapture, value);
        } else {
            setIntegerParam(NDFileCapture, 0);
        }
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
    //static const char *functionName = "writeNDArray";
        
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
                     asynFlags, autoConnect, priority, stackSize),
    pCapture(NULL), captureBufferSize(0)
{
    //static const char *functionName = "NDPluginFile";

    this->ndArrayInfoInit = NULL;
    this->lazyOpen = false;

    this->useAttrFilePrefix = false;
    this->fileMutexId = epicsMutexCreate();
    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDPluginFile");

    // Disable ArrayCallbacks.  
    // This plugin currently does not do array callbacks, so make the setting reflect the behavior
    setIntegerParam(NDArrayCallbacks, 0);

    /* Try to connect to the NDArray port */
    connectToArrayPort();
}

