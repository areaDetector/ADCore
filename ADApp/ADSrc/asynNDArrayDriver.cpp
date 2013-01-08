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
#include <sys/types.h>
#include <sys/stat.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <cantProceed.h>

#include "tinyxml.h"
#include "PVAttribute.h"
#include "paramAttribute.h"
#include <asynDriver.h>
#define epicsExportSharedSymbols
#include <shareLib.h>
#include "asynNDArrayDriver.h"

static const char *driverName = "asynNDArrayDriver";

/** Checks whether the directory specified NDFilePath parameter exists.
  * 
  * This is a convenience function that determinesthe directory specified NDFilePath parameter exists.
  * It sets the value of NDFilePathExists to 0 (does not exist) or 1 (exists).  
  * It also adds a trailing '/' character to the path if one is not present.
  * Returns a error status if the directory does not exist.
  */
int asynNDArrayDriver::checkPath()
{
    /* Formats a complete file name from the components defined in NDStdDriverParams */
    int status = asynError;
    char filePath[MAX_FILENAME_LEN];
    int hasTerminator=0;
    struct stat buff;
    size_t len;
    int isDir=0;
    int pathExists=0;
    
    status = getStringParam(NDFilePath, sizeof(filePath), filePath);
    len = strlen(filePath);
    if (len == 0) return(asynSuccess);
    /* If the path contains a trailing '/' or '\' remove it, because Windows won't find
     * the directory if it has that trailing character */
    if ((filePath[len-1] == '/') || (filePath[len-1] == '\\')) {
        filePath[len-1] = 0;
        len--;
        hasTerminator=1;
    }
    status = stat(filePath, &buff);
    if (!status) isDir = (S_IFDIR & buff.st_mode);
    if (!status && isDir) {
        pathExists = 1;
        status = asynSuccess;
    }
    /* If the path did not have a trailing terminator then add it if there is room */
    if (!hasTerminator) {
        if (len < MAX_FILENAME_LEN-2) strcat(filePath, "/");
        setStringParam(NDFilePath, filePath);
    }
    setIntegerParam(NDFilePathExists, pathExists);
    return(status);   
}

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
    /* Formats a complete file name from the components defined in NDStdDriverParams */
    int status = asynSuccess;
    char filePath[MAX_FILENAME_LEN];
    char fileName[MAX_FILENAME_LEN];
    char fileTemplate[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;
    
    this->checkPath();
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
    /* Formats a complete file name from the components defined in NDStdDriverParams */
    int status = asynSuccess;
    char fileTemplate[MAX_FILENAME_LEN];
    char name[MAX_FILENAME_LEN];
    int fileNumber;
    int autoIncrement;
    int len;
    
    this->checkPath();
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

/** Create this driver's NDAttributeList (pAttributeList) by reading an XML file
  * \param[in] fileName  The name of the XML file to read.
  * 
  * This clears any existing attributes from this drivers' NDAttributeList and then creates a new list
  * based on the XML file.  These attributes can then be associated with an NDArray by calling asynNDArrayDriver::getAttributes()
  * passing it pNDArray->pAttributeList.
  * 
  * The following simple example XML file illustrates the way that both PVAttribute and paramAttribute attributes are defined.
  * <pre>
  * <?xml version="1.0" standalone="no" ?>
  * \<Attributes>
  * \<Attribute name="AcquireTime"         type="EPICS_PV" source="13SIM1:cam1:AcquireTime"      dbrtype="DBR_NATIVE"  description="Camera acquire time"/>
  * \<Attribute name="CameraManufacturer"  type="PARAM"    source="MANUFACTURER"                 datatype="STRING"     description="Camera manufacturer"/>
  * \</Attributes>
  * </pre>
  * Each NDAttribute (currently either an PVAttribute or paramAttribute, but other types may be added in the future) 
  * is defined with an XML <b>Attribute</b> tag.  For each attribute there are a number of XML attributes
  * (unfortunately there are 2 meanings of attribute here: the NDAttribute and the XML attribute).  
  * XML attributes have the syntax name="value".  The XML attribute names are case-sensitive and must be lower case, i.e. name="xxx", not NAME="xxx".  
  * The XML attribute values are specified by the XML Schema and are always uppercase for <b>datatype</b> and <b>dbrtype</b> attributes.
  * The XML attribute names are listed here:
  *
  * <b>name</b> determines the name of the NDAttribute.  It is required, must be unique, is case-insensitive, 
  * and must start with a letter.  It can include only letters, numbers and underscore. (No whitespace or other punctuation.)
  *
  * <b>type</b> determines the type of the NDAttribute.  "EPICS_PV" creates a PVAttribute, while "PARAM" creates a paramAttribute.
  * The default is EPICS_PV if this XML attribute is absent.
  *
  * <b>source</b> determines the source of the NDAttribute.  It is required. If type="EPICS_PV" then this is the name of the EPICS PV, which is
  * case-sensitive. If type="PARAM" then this is the drvInfo string that is used in EPICS database files (e.g. ADBase.template) to identify
  * this parameter.
  *
  * <b>dbrtype</b> determines the data type that will be used to read an EPICS_PV value with channel access.  It can be one of the standard EPICS
  * DBR types (e.g. "DBR_DOUBLE", "DBR_STRING", ...) or it can be the special type "DBR_NATIVE" which means to use the native channel access
  * data type for this PV.  The default is DBR_NATIVE if this XML attribute is absent.  Always use uppercase.
  *
  * <b>datatype</b> determines the parameter data type for type="PARAM".  It must match the actual data type in the driver or plugin
  * parameter library, and must be "INT", "DOUBLE", or "STRING".  The default is "INT" if this XML attribute is absent.   Always use uppercase.
  * 
  * <b>addr</b> determines the asyn addr (address) for type="PARAM".  The default is 0 if the XML attribute is absent.
  * 
  * <b>description</b> determines the description for this attribute.  It is not required, and the default is a NULL string.
  *
  */
int asynNDArrayDriver::readNDAttributesFile(const char *fileName)
{
    static const char *functionName = "readNDAttributesFile";
    
    const char *pName, *pDBRType, *pSource, *pType, *pDataType, *pAddr, *pDescription=NULL;
    int dbrType;
    int addr=0;
    PVAttribute *pPVAttribute=NULL;
    paramAttribute *pParamAttribute=NULL;
    TiXmlDocument doc(fileName);
    TiXmlElement *Attr, *Attrs;
    
    /* Clear any existing attributes */
    this->pAttributeList->clear();
    if (!fileName || (strlen(fileName) == 0)) return(asynSuccess);

    if (!doc.LoadFile()) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: cannot open file %s error=%s\n", 
            driverName, functionName, fileName, doc.ErrorDesc());
        return(asynError);
    }
    Attrs = doc.FirstChildElement( "Attributes" );
    if (!Attrs) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: cannot find Attributes element\n", 
            driverName, functionName);
        return(asynError);
    }
    for (Attr = Attrs->FirstChildElement(); Attr; Attr = Attr->NextSiblingElement()) {
        pName = Attr->Attribute("name");
        if (!pName) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: name attribute not found\n", 
                driverName, functionName);
            return(asynError);
        }
        pDescription = Attr->Attribute("description");
        pSource = Attr->Attribute("source");
        pType = Attr->Attribute("type");
        if (!pType) pType = "EPICS_PV";
        if (epicsStrCaseCmp(pType, "EPICS_PV") == 0) {
            pDBRType = Attr->Attribute("dbrtype");
            dbrType = DBR_NATIVE;
            // TODO: do not enforce new uppercase rule yet on value of dbrtype attribute
            if (pDBRType) {
                if      (!epicsStrCaseCmp(pDBRType, "DBR_CHAR"))   dbrType = DBR_CHAR;
                else if (!epicsStrCaseCmp(pDBRType, "DBR_SHORT"))  dbrType = DBR_SHORT;
                else if (!epicsStrCaseCmp(pDBRType, "DBR_ENUM"))   dbrType = DBR_ENUM;
                else if (!epicsStrCaseCmp(pDBRType, "DBR_INT"))    dbrType = DBR_INT;
                else if (!epicsStrCaseCmp(pDBRType, "DBR_LONG"))   dbrType = DBR_LONG;
                else if (!epicsStrCaseCmp(pDBRType, "DBR_FLOAT"))  dbrType = DBR_FLOAT;
                else if (!epicsStrCaseCmp(pDBRType, "DBR_DOUBLE")) dbrType = DBR_DOUBLE;
                else if (!epicsStrCaseCmp(pDBRType, "DBR_STRING")) dbrType = DBR_STRING;
                else if (!epicsStrCaseCmp(pDBRType, "DBR_NATIVE")) dbrType = DBR_NATIVE;
                else {
                    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: unknown dbrType = %s\n", 
                        driverName, functionName, pDBRType);
                    return(asynError);
                }
            }
            asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
                "%s:%s: Name=%s, PVName=%s, pDBRType=%s, dbrType=%d, pDescription=%s\n",
                driverName, functionName, pName, pSource, pDBRType, dbrType, pDescription);
            pPVAttribute = new PVAttribute(pName, pDescription, pSource, dbrType);
            this->pAttributeList->add(pPVAttribute);
        } else if (epicsStrCaseCmp(pType, "PARAM") == 0) {
            // TODO: do not enforce new uppercase rule yet on value of datatype attribute
            pDataType = Attr->Attribute("datatype");
            if (!pDataType) pDataType = "int";
            pAddr = Attr->Attribute("addr");
            if (pAddr) addr = strtol(pAddr, NULL, 0);
            else addr = 0;
            asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
                "%s:%s: Name=%s, drvInfo=%s, dataType=%s,pDescription=%s\n",
                driverName, functionName, pName, pSource, pDataType, pDescription); 
            pParamAttribute = new paramAttribute(pName, pDescription, pSource, addr, this, pDataType);
            this->pAttributeList->add(pParamAttribute);
        }
    }
    return(asynSuccess);
}


/** Get the current values of attributes from this driver and appends them to an output attribute list.
  * Calls NDAttributeList::updateValues for this driver's attribute list, 
  * and then NDAttributeList::copy, to copy this driver's attribute 
  * list to pList, appending the values to that output attribute list.
  * \param[out] pList  The NDAttributeList to copy the attributes to.
  *
  * NOTE: Plugins must never call this function with a pointer to the attribute
  * list from the NDArray they were passed in NDPluginDriver::processCallbacks, because
  * that modifies the original NDArray which is forbidden.
  */
int asynNDArrayDriver::getAttributes(NDAttributeList *pList)
{
    //const char *functionName = "getAttributes";
    int status = asynSuccess;
    
    status = this->pAttributeList->updateValues();
    status = this->pAttributeList->copy(pList);
    return(status);
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

    status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(addr, function, (char *)value);

    if (function == NDAttributesFile) {
        this->readNDAttributesFile(value);
    } else if (function == NDFilePath) {
        this->checkPath();
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

    status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
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
              "%s:%s: error, maxBytes=%lu, data=%p\n",
              driverName, functionName, (unsigned long)arrayInfo.totalBytes, pArray->pData);
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

asynStatus asynNDArrayDriver::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    // Just read the status of the NDArrayPool
    if (function == NDPoolMaxBuffers) {
        setIntegerParam(function, this->pNDArrayPool->maxBuffers());
    } else if (function == NDPoolAllocBuffers) {
        setIntegerParam(function, this->pNDArrayPool->numBuffers());
    } else if (function == NDPoolFreeBuffers) {
        setIntegerParam(function, this->pNDArrayPool->numFree());
    }

    // Call base class
    status = asynPortDriver::readInt32(pasynUser, value);
    return status;
}

#define MEGABYTE_DBL 1048576.
asynStatus asynNDArrayDriver::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    // Just read the status of the NDArrayPool
    if (function == NDPoolMaxMemory) {
        setDoubleParam(function, this->pNDArrayPool->maxMemory() / MEGABYTE_DBL);
    } else if (function == NDPoolUsedMemory) {
        setDoubleParam(function, this->pNDArrayPool->memorySize() / MEGABYTE_DBL);
    }

    // Call base class
    status = asynPortDriver::readFloat64(pasynUser, value);
    return status;
}


/** Report status of the driver.
  * This method calls the report function in the asynPortDriver base class. It then
  * calls the NDArrayPool::report() method if details >5.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >5 then NDArrayPool::report and NDAttributeList::report are both called.
  */
void asynNDArrayDriver::report(FILE *fp, int details)
{
    asynPortDriver::report(fp, details);
    if (details > 5) {
        if (this->pNDArrayPool) this->pNDArrayPool->report(details);
        this->pAttributeList->report(details);
    }
}


/** This is the constructor for the asynNDArrayDriver class.
  * portName, maxAddr, paramTableSize, interfaceMask, interruptMask, asynFlags, autoConnect, priority and stackSize
  * are simply passed to asynPortDriver::asynPortDriver. 
  * asynNDArrayDriver creates an NDArrayPool object to allocate NDArray
  * objects. maxBuffers and maxMemory are passed to NDArrayPool::NDArrayPool.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxAddr The maximum  number of asyn addr addresses this driver supports. 1 is minimum.
  * \param[in] numParams The number of parameters in the derived class.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to 0 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to 0 to allow an unlimited amount of memory.
  * \param[in] interfaceMask Bit mask defining the asyn interfaces that this driver supports.
  * \param[in] interruptMask Bit mask definining the asyn interfaces that can generate interrupts (callbacks)
  * \param[in] asynFlags Flags when creating the asyn port driver; includes ASYN_CANBLOCK and ASYN_MULTIDEVICE.
  * \param[in] autoConnect The autoConnect flag for the asyn port driver.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */

asynNDArrayDriver::asynNDArrayDriver(const char *portName, int maxAddr, int numParams, int maxBuffers,
                                     size_t maxMemory, int interfaceMask, int interruptMask,
                                     int asynFlags, int autoConnect, int priority, int stackSize)
    : asynPortDriver(portName, maxAddr, numParams+NUM_NDARRAY_PARAMS, 
                     interfaceMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynGenericPointerMask, 
                     interruptMask | asynInt32Mask | asynFloat64Mask | asynOctetMask,
                     asynFlags, autoConnect, priority, stackSize),
      pNDArrayPool(NULL)
{
    this->pNDArrayPool = new NDArrayPool(maxBuffers, maxMemory);

    /* Allocate pArray pointer array */
    this->pArrays = (NDArray **)calloc(maxAddr, sizeof(NDArray *));
    this->pAttributeList = new NDAttributeList();
    
    createParam(NDPortNameSelfString,         asynParamOctet,           &NDPortNameSelf);
    createParam(NDArraySizeXString,           asynParamInt32,           &NDArraySizeX);
    createParam(NDArraySizeYString,           asynParamInt32,           &NDArraySizeY);
    createParam(NDArraySizeZString,           asynParamInt32,           &NDArraySizeZ);
    createParam(NDArraySizeString,            asynParamInt32,           &NDArraySize);
    createParam(NDNDimensionsString,          asynParamInt32,           &NDNDimensions);
    createParam(NDDimensionsString,           asynParamInt32,           &NDDimensions);
    createParam(NDDataTypeString,             asynParamInt32,           &NDDataType);
    createParam(NDColorModeString,            asynParamInt32,           &NDColorMode);
    createParam(NDUniqueIdString,             asynParamInt32,           &NDUniqueId);
    createParam(NDTimeStampString,            asynParamFloat64,         &NDTimeStamp);
    createParam(NDBayerPatternString,         asynParamInt32,           &NDBayerPattern);
    createParam(NDArrayCounterString,         asynParamInt32,           &NDArrayCounter);
    createParam(NDFilePathString,             asynParamOctet,           &NDFilePath);
    createParam(NDFilePathExistsString,       asynParamInt32,           &NDFilePathExists);
    createParam(NDFileNameString,             asynParamOctet,           &NDFileName);
    createParam(NDFileNumberString,           asynParamInt32,           &NDFileNumber);
    createParam(NDFileTemplateString,         asynParamOctet,           &NDFileTemplate);
    createParam(NDAutoIncrementString,        asynParamInt32,           &NDAutoIncrement);
    createParam(NDFullFileNameString,         asynParamOctet,           &NDFullFileName);
    createParam(NDFileFormatString,           asynParamInt32,           &NDFileFormat);
    createParam(NDAutoSaveString,             asynParamInt32,           &NDAutoSave);
    createParam(NDWriteFileString,            asynParamInt32,           &NDWriteFile);
    createParam(NDReadFileString,             asynParamInt32,           &NDReadFile);
    createParam(NDFileWriteModeString,        asynParamInt32,           &NDFileWriteMode);
    createParam(NDFileWriteStatusString,      asynParamInt32,           &NDFileWriteStatus);
    createParam(NDFileWriteMessageString,     asynParamOctet,           &NDFileWriteMessage);
    createParam(NDFileNumCaptureString,       asynParamInt32,           &NDFileNumCapture);
    createParam(NDFileNumCapturedString,      asynParamInt32,           &NDFileNumCaptured);
    createParam(NDFileCaptureString,          asynParamInt32,           &NDFileCapture);   
    createParam(NDFileDeleteDriverFileString, asynParamInt32,           &NDFileDeleteDriverFile);
    createParam(NDAttributesFileString,       asynParamOctet,           &NDAttributesFile);
    createParam(NDArrayDataString,            asynParamGenericPointer,  &NDArrayData);
    createParam(NDArrayCallbacksString,       asynParamInt32,           &NDArrayCallbacks);
    createParam(NDPoolMaxBuffersString,       asynParamInt32,           &NDPoolMaxBuffers);
    createParam(NDPoolAllocBuffersString,     asynParamInt32,           &NDPoolAllocBuffers);
    createParam(NDPoolFreeBuffersString,      asynParamInt32,           &NDPoolFreeBuffers);
    createParam(NDPoolMaxMemoryString,        asynParamFloat64,         &NDPoolMaxMemory);
    createParam(NDPoolUsedMemoryString,       asynParamFloat64,         &NDPoolUsedMemory);

    /* Here we set the values of read-only parameters and of read/write parameters that cannot
     * or should not get their values from the database.  Note that values set here will override
     * those in the database for output records because if asyn device support reads a value from 
     * the driver with no error during initialization then it sets the output record to that value.  
     * If a value is not set here then the read request will return an error (uninitialized).
     * Values set here will be overridden by values from save/restore if they exist. */
    setStringParam (NDPortNameSelf, portName);
    setIntegerParam(NDArraySizeX,   0);
    setIntegerParam(NDArraySizeY,   0);
    setIntegerParam(NDArraySizeZ,   0);
    setIntegerParam(NDArraySize,    0);
    setIntegerParam(NDNDimensions,  0);
    setIntegerParam(NDDataType,     NDUInt8);
    setIntegerParam(NDColorMode,    NDColorModeMono);
    setIntegerParam(NDUniqueId,     0);
    setDoubleParam (NDTimeStamp,    0.);
    setIntegerParam(NDBayerPattern, 0);
    setIntegerParam(NDArrayCounter, 0);
    /* Set the initial values of FileSave, FileRead, and FileCapture so the readbacks are initialized */
    setIntegerParam(NDWriteFile, 0);
    setIntegerParam(NDReadFile, 0);
    setIntegerParam(NDFileCapture, 0);
    setIntegerParam(NDFileWriteStatus, 0);
    setStringParam (NDFileWriteMessage, "");
    /* We set FileTemplate to a reasonable value because it cannot be defined in the database, since it is a
     * waveform record. However, the waveform record does not currently read the driver value for initialization! */
    setStringParam (NDFileTemplate, "%s%s_%3.3d.dat");
    setIntegerParam(NDFileNumCaptured, 0);

    setIntegerParam(NDPoolMaxBuffers, this->pNDArrayPool->maxBuffers());
    setIntegerParam(NDPoolAllocBuffers, this->pNDArrayPool->numBuffers());
    setIntegerParam(NDPoolFreeBuffers, this->pNDArrayPool->numFree());

}

