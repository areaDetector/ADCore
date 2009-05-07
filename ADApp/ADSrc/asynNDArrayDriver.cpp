/**
 * asynNDArrayDriver.cpp
 *
 * Base class that implements methods for asynStandardInterfaces with a parameter library.
 *
 * Author: Mark Rivers
 *
 * Created May 11, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <cantProceed.h>

#include "asynNDArrayDriver.h"
#define DEFINE_ND_STANDARD_PARAMS 1
#include "NDStdDriverParams.h"

static const char *driverName = "asynNDArrayDriver";

/** drvInfo strings for NDStdDriverParams */
static asynParamString_t NDStdDriverParamString[] = {
    {NDPortNameSelf,   "PORT_NAME_SELF"},
    {NDArraySizeX,     "ARRAY_SIZE_X"},
    {NDArraySizeY,     "ARRAY_SIZE_Y"},
    {NDArraySizeZ,     "ARRAY_SIZE_Z"},
    {NDArraySize,      "ARRAY_SIZE"  },
    {NDDataType,       "DATA_TYPE"   },
    {NDColorMode,      "COLOR_MODE"  },

    {NDArrayCounter,   "ARRAY_COUNTER" },

    {NDFilePath,       "FILE_PATH"     },
    {NDFileName,       "FILE_NAME"     },
    {NDFileNumber,     "FILE_NUMBER"   },
    {NDFileTemplate,   "FILE_TEMPLATE" },
    {NDAutoIncrement,  "AUTO_INCREMENT"},
    {NDFullFileName,   "FULL_FILE_NAME"},
    {NDFileFormat,     "FILE_FORMAT"   },
    {NDAutoSave,       "AUTO_SAVE"     },
    {NDWriteFile,      "WRITE_FILE"    },
    {NDReadFile,       "READ_FILE"     },
    {NDFileWriteMode,  "WRITE_MODE"    },
    {NDFileNumCapture, "NUM_CAPTURE"   },
    {NDFileNumCaptured,"NUM_CAPTURED"  },
    {NDFileCapture,    "CAPTURE"       },

    {NDArrayData,      "NDARRAY_DATA"  },
    {NDArrayCallbacks, "ARRAY_CALLBACKS"  }
};


/** Build a file name from component parts.
  * \param[in] maxChars  The size of the fullFileName string.
  * \param[out] fullFileName The constructed file name including the file path.
  * 
  * This is a convenience function that constructs a complete file name
  * from the NDFilePath, NDFileName, NDFileNumber, and
  * NDFileTemplate parameters. If NDAutoIncrement is true then it increments the
  * NDFileNumber after creating the file name.
  */
int asynNDArrayDriver::createFileName(int maxChars, char *fullFileName)
{
    /* Formats a complete file name from the components defined in NDStdDriverParams.h */
    int status = asynSuccess;
    char filePath[MAX_FILENAME_LEN];
    char fileName[MAX_FILENAME_LEN];
    char fileTemplate[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;
    
    status |= getStringParam(NDFilePath, sizeof(filePath), filePath); 
    status |= getStringParam(NDFileName, sizeof(fileName), fileName); 
    status |= getStringParam(NDFileTemplate, sizeof(fileTemplate), fileTemplate); 
    status |= getIntegerParam(NDFileNumber, &fileNumber);
    status |= getIntegerParam(NDAutoIncrement, &autoIncrement);
    if (status) return(status);
    len = epicsSnprintf(fullFileName, maxChars, fileTemplate, 
                        filePath, fileName, fileNumber);
    if (len < 0) {
        status |= asynError;
        return(status);
    }
    if (autoIncrement) {
        fileNumber++;
        status |= setIntegerParam(NDFileNumber, fileNumber);
    }
    return(status);   
}

/** Build a file name from component parts.
  * \param[in] maxChars  The size of the fullFileName string.
  * \param[out] filePath The file path.
  * \param[out] fileName The constructed file name without file file path.
  * 
  * This is a convenience function that constructs a file path and file name
  * from the NDFilePath, NDFileName, NDFileNumber, and
  * NDFileTemplate parameters. If NDAutoIncrement is true then it increments the
  * NDFileNumber after creating the file name.
  */
int asynNDArrayDriver::createFileName(int maxChars, char *filePath, char *fileName)
{
    /* Formats a complete file name from the components defined in NDStdDriverParams.h */
    int status = asynSuccess;
    char fileTemplate[MAX_FILENAME_LEN];
    char name[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;
    
    status |= getStringParam(NDFilePath, maxChars, filePath); 
    status |= getStringParam(NDFileName, sizeof(name), name); 
    status |= getStringParam(NDFileTemplate, sizeof(fileTemplate), fileTemplate); 
    status |= getIntegerParam(NDFileNumber, &fileNumber);
    status |= getIntegerParam(NDAutoIncrement, &autoIncrement);
    if (status) return(status);
    len = epicsSnprintf(fileName, maxChars, fileTemplate, 
                        name, fileNumber);
    if (len < 0) {
        status |= asynError;
        return(status);
    }
    if (autoIncrement) {
        fileNumber++;
        status |= setIntegerParam(NDFileNumber, fileNumber);
    }
    return(status);   
}

int asynNDArrayDriver::readPVAttributesFile(const char *fileName)
{
    //const char *functionName = "readAttributesFile";
    
    /* If the PVAttributes object does not yet exist then create it */
    if (!this->pPVAttributes) this->pPVAttributes = new PVAttributes;
    /* Clear any existing PVs */
    return this->pPVAttributes->readPVAttributesFile(fileName);
}

/** Called when asyn clients call pasynOctet->write().
  * This function performs actions for some parameters, including NDAttributesFile.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus asynNDArrayDriver::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
    int addr=0;
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "writeOctet";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(addr, function, (char *)value);

    switch(function) {
        case NDAttributesFile:
            this->readPVAttributesFile(value);
            break;
        default:
            break;
    }
    
     /* Do callbacks so higher layers see any changes */
    status = (asynStatus)callParamCallbacks(addr, addr);

    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%s", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *nActual = nChars;
    return status;
}

/* asynGenericPointer interface methods */
/** This method copies an NDArray object from the asynNDArrayDriver to an NDArray pointer passed in by the caller.
  * The destination NDArray address is passed by the caller in the genericPointer argument. The caller
  * must allocate the memory for the array, and pass the size in NDArray->dataSize.
  * The method will limit the amount of data copied to the actual array size or the
  * input dataSize, whichever is smaller.
  * \param[in] pasynUser Used to obtain the addr for the NDArray to be copied from, and for asynTrace output.
  * \param[out] genericPointer Pointer to an NDArray. The NDArray must have been previously allocated by the caller.
  * The NDArray from the asynNDArrayDriver will be copied into the NDArray pointed to by genericPointer.
  */
asynStatus asynNDArrayDriver::readGenericPointer(asynUser *pasynUser, void *genericPointer)
{
    NDArray *pArray = (NDArray *)genericPointer;
    NDArray *myArray;
    NDArrayInfo_t arrayInfo;
    int addr;
    asynStatus status = asynSuccess;
    const char* functionName = "readNDArray";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    this->lock();
    myArray = this->pArrays[addr];
    if (!myArray) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                    "%s:%s: error, no valid array available, pData=%p",
                    driverName, functionName, pArray->pData);
        status = asynError;
    } else {
        this->pNDArrayPool->copy(myArray, pArray, 0);
        myArray->getInfo(&arrayInfo);
        if (arrayInfo.totalBytes > pArray->dataSize) arrayInfo.totalBytes = pArray->dataSize;
        memcpy(pArray->pData, myArray->pData, arrayInfo.totalBytes);
    }
    if (!status)
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: error, maxBytes=%d, data=%p\n",
              driverName, functionName, arrayInfo.totalBytes, pArray->pData);
    this->unlock();
    return status;
}

/** This method currently does nothing, but it should be implemented in this base class.
  * Derived classes can implement this method as required.
  * \param[in] pasynUser Used to obtain the addr for the NDArray to be copied to, and for asynTrace output.
  * \param[in] genericPointer Pointer to an NDArray. 
  * The NDArray pointed to by genericPointer will be copied into the NDArray in asynNDArrayDriver .
  */
asynStatus asynNDArrayDriver::writeGenericPointer(asynUser *pasynUser, void *genericPointer)
{
    asynStatus status = asynSuccess;

    this->lock();

    this->unlock();
    return status;
}

/** Sets pasynUser->reason to one of the enum values for the parameters defined in ADStdDriverParams.h
  * if the drvInfo field matches one the strings defined in that file.
  * Simply calls asynPortDriver::drvUserCreateParam with the parameter table for this driver.
  * \param[in] pasynUser pasynUser structure that driver modifies
  * \param[in] drvInfo String containing information about what driver function is being referenced
  * \param[out] pptypeName Location in which driver puts a copy of drvInfo.
  * \param[out] psize Location where driver puts size of param 
  * \return Returns asynSuccess if a matching string was found, asynError if not found. */
asynStatus asynNDArrayDriver::drvUserCreate(asynUser *pasynUser,
                                            const char *drvInfo, 
                                            const char **pptypeName, size_t *psize)
{
    asynStatus status;
    //const char *functionName = "drvUserCreate";
    status = this->drvUserCreateParam(pasynUser, drvInfo, pptypeName, psize, 
                                      NDStdDriverParamString, NUM_ND_STANDARD_PARAMS);
    return(status);    
}



/** Report status of the driver.
  * This method calls the report function in the asynPortDriver base class. It then
  * calls the NDArrayPool::report() method if details >5.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >5 then NDArrayPool::report is called.
  */
void asynNDArrayDriver::report(FILE *fp, int details)
{
    asynPortDriver::report(fp, details);
    if (details > 5) {
        if (this->pNDArrayPool) this->pNDArrayPool->report(details);
    }
}


/** This is the constructor for the asynNDArrayDriver class.
  * portName, maxAddr, paramTableSize, interfaceMask, interruptMask, asynFlags, autoConnect, priority and stackSize
  * are simply passed to asynPortDriver::asynPortDriver. 
  * asynNDArray creates an NDArrayPool object to allocate NDArray
  * objects. maxBuffers and maxMemory are passed to NDArrayPool::NDArrayPool.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxAddr The maximum  number of asyn addr addresses this driver supports. 1 is minimum.
  * \param[in] paramTableSize The number of parameters that this driver supports.
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

asynNDArrayDriver::asynNDArrayDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers,
                                     size_t maxMemory, int interfaceMask, int interruptMask,
                                     int asynFlags, int autoConnect, int priority, int stackSize)
    : asynPortDriver(portName, maxAddr, paramTableSize, interfaceMask, interruptMask,
                     asynFlags, autoConnect, priority, stackSize),
      pNDArrayPool(NULL)
{
    if ((maxBuffers != 0) || (maxMemory != 0)) this->pNDArrayPool = new NDArrayPool(maxBuffers, maxMemory);

    /* Allocate pArray pointer array */
    this->pArrays = (NDArray **)calloc(maxAddr, sizeof(NDArray *));

    setStringParam (NDPortNameSelf, portName);
    setIntegerParam(NDArraySizeX,   0);
    setIntegerParam(NDArraySizeY,   0);
    setIntegerParam(NDArraySizeZ,   0);
    setIntegerParam(NDArraySize,    0);
    setIntegerParam(NDDataType,     NDUInt8);
    setIntegerParam(NDColorMode,    NDColorModeMono);
    setIntegerParam(NDArrayCounter, 0);
    setStringParam (NDFilePath,     "");
    setStringParam (NDFileName,     "");
    setIntegerParam(NDFileNumber,   0);
    setStringParam (NDFileTemplate, "");
    setIntegerParam(NDAutoIncrement, 0);
    setStringParam (NDFullFileName, "");
    setIntegerParam(NDFileFormat,   0);
    setIntegerParam(NDAutoSave,     0);
    setIntegerParam(NDWriteFile,    0);
    setIntegerParam(NDReadFile,     0);
    setIntegerParam(NDFileWriteMode,   0);
    setIntegerParam(NDFileNumCapture,  0);
    setIntegerParam(NDFileNumCaptured, 0);
    setIntegerParam(NDFileCapture,     0);
    setIntegerParam(NDArrayCallbacks, 1);

}

