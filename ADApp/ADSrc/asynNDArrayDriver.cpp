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
#include <sstream>
#include <fstream>

#include <libxml/parser.h>

#include <epicsString.h>
#include <epicsThread.h>
#include <macLib.h>
#include <cantProceed.h>

#include "PVAttribute.h"
#include "paramAttribute.h"
#include "functAttribute.h"
#include "asynNDArrayDriver.h"

#define MAX_PATH_PARTS 32

#if defined(_WIN32)              // Windows
  #include <direct.h>
  #define strtok_r(a,b,c) strtok(a,b)
  #define MKDIR(a,b) _mkdir(a)
  #define delim "\\"
#elif defined(vxWorks)           // VxWorks
  #include <sys/stat.h>
  #define MKDIR(a,b) mkdir(a)
  #define delim "/"
#else                            // Linux
  #include <sys/stat.h>
  #include <sys/types.h>
  #define delim "/"
  #define MKDIR(a,b) mkdir(a,b)
#endif

static const char *driverName = "asynNDArrayDriver";

/** Checks whether the directory specified exists.
  *
  * This is a convenience function that determines the directory specified exists.
  * It adds a trailing '/' or '\' character to the path if one is not present.
  * It returns true if the directory exists and false if it does not
  */
bool asynNDArrayDriver::checkPath(std::string &filePath)
{
    char lastChar;
    struct stat buff;
    int istat;
    size_t len;
    int isDir=0;
    bool pathExists=false;

    len = filePath.size();
    if (len == 0) return false;
    /* If the path contains a trailing '/' or '\' remove it, because Windows won't find
     * the directory if it has that trailing character */
    lastChar = filePath[len-1];
#ifdef _WIN32
    if ((lastChar == '/') || (lastChar == '\\'))
#else
    if (lastChar == '/')
#endif
    {
        filePath.resize(len-1);
    }
    istat = stat(filePath.c_str(), &buff);
    if (!istat) isDir = (S_IFDIR & buff.st_mode);
    if (!istat && isDir) {
        pathExists = true;
    }
    /* Add a terminator even if it did not have one originally */
    filePath.append(delim);
    return pathExists;
}


/** Checks whether the directory specified NDFilePath parameter exists.
  *
  * This is a convenience function that determines the directory specified NDFilePath parameter exists.
  * It sets the value of NDFilePathExists to 0 (does not exist) or 1 (exists).
  * It also adds a trailing '/' character to the path if one is not present.
  * Returns a error status if the directory does not exist.
  */
asynStatus asynNDArrayDriver::checkPath()
{
    asynStatus status;
    std::string filePath;
    int pathExists;

    getStringParam(NDFilePath, filePath);
    if (filePath.size() == 0) return asynSuccess;
    pathExists = checkPath(filePath);
    status = pathExists ? asynSuccess : asynError;
    setStringParam(NDFilePath, filePath);
    setIntegerParam(NDFilePathExists, pathExists);
    return status;
}

/** Function to create a directory path for a file.
  \param[in] path  Path to create. The final part is the file name and is not created.
  \param[in] pathDepth  This determines how much of the path to assume exists before attempting
                      to create directories:
                      pathDepth = 0 create no directories
                      pathDepth = 1 create all directories needed (i.e. only assume root directory exists).
                      pathDepth = 2  Assume 1 directory below the root directory exists
                      pathDepth = -1 Assume all but one directory exists
                      pathDepth = -2 Assume all but two directories exist.
*/
asynStatus asynNDArrayDriver::createFilePath(const char *path, int pathDepth)
{
    asynStatus result = asynSuccess;
    char *parts[MAX_PATH_PARTS];
    int num_parts;
    char directory[MAX_FILENAME_LEN];

    // Initialise the directory to create
    char nextDir[MAX_FILENAME_LEN];

    // Extract the next name from the directory
    char* saveptr;
    int i=0;

    // Check for trivial case.
    if (pathDepth == 0) return asynSuccess;

    // Check for Windows disk designator
    if (path[1] == ':') {
        nextDir[0]=path[0];
        nextDir[1]=':';
        i+=2;
    }

    // Skip over any more delimiters
    while ((path[i] == '/' || path[i] == '\\') && i < MAX_FILENAME_LEN) {
        nextDir[i] = path[i];
        ++i;
    }
    nextDir[i] = 0;

    // Now, tokenise the path - first making a copy because strtok is destructive
    strcpy(directory, &path[i] );
    num_parts = 0;
    parts[num_parts] = strtok_r( directory, "\\/", &saveptr);
    while ( parts[num_parts] != NULL ) {
        parts[++num_parts] = strtok_r(NULL, "\\/", &saveptr);
    }

    // Handle the case if the path depth is negative
    if (pathDepth < 0) {
        pathDepth = num_parts + pathDepth;
        if (pathDepth < 1) pathDepth = 1;
    }

    // Loop through parts creating directories
    for ( i = 0; i < num_parts && result != asynError; i++ ) {
        strcat(nextDir, parts[i]);
        if ( i >= pathDepth ) {
            if(MKDIR(nextDir, 0777) != 0 && errno != EEXIST) {
                result = asynError;
            }
        }
        strcat(nextDir, delim);
   }

    return result;
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
asynStatus asynNDArrayDriver::createFileName(int maxChars, char *fullFileName)
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
    if (status) return (asynStatus) status;
    len = epicsSnprintf(fullFileName, maxChars, fileTemplate,
                        filePath, fileName, fileNumber);
    if (len < 0) {
        status |= asynError;
        return (asynStatus) status;
    }
    if (autoIncrement) {
        fileNumber++;
        status |= setIntegerParam(NDFileNumber, fileNumber);
    }
    return (asynStatus) status;
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
asynStatus asynNDArrayDriver::createFileName(int maxChars, char *filePath, char *fileName)
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
    if (status) return (asynStatus) status;
    len = epicsSnprintf(fileName, maxChars, fileTemplate,
                        name, fileNumber);
    if (len < 0) {
        status |= asynError;
        return (asynStatus) status;
    }
    if (autoIncrement) {
        fileNumber++;
        status |= setIntegerParam(NDFileNumber, fileNumber);
    }
    return (asynStatus) status;
}

/** Create this driver's NDAttributeList (pAttributeList) by reading an XML file
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
asynStatus asynNDArrayDriver::readNDAttributesFile()
{
    const char *pName, *pSource, *pAttrType, *pDescription;
    xmlDocPtr doc;
    xmlNode *Attr, *Attrs;
    std::ostringstream buff;
    std::string buffer;
    std::ifstream infile;
    std::string attributesMacros;
    std::string fileName;
    MAC_HANDLE *macHandle;
    char **macPairs;
    int bufferSize;
    char *tmpBuffer = 0;
    int status;
    static const char *functionName = "readNDAttributesFile";

    getStringParam(NDAttributesFile, fileName);
    getStringParam(NDAttributesMacros, attributesMacros);

    /* Clear any existing attributes */
    this->pAttributeList->clear();
    if (fileName.length() == 0) return asynSuccess;

    // Fill input buffer by XML string or reading from a file
    if (fileName.find("<Attributes>") != std::string::npos){
        buffer = fileName;
    } else {
        infile.open(fileName.c_str());
        if (infile.fail()) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s error opening file %s\n",
                driverName, functionName, fileName.c_str());
            setIntegerParam(NDAttributesStatus, NDAttributesFileNotFound);
            return asynError;
        }
        buff << infile.rdbuf();
        buffer = buff.str();
    }

    // We now have file in memory.  Do macro substitution if required
    if (attributesMacros.length() > 0) {
        macCreateHandle(&macHandle, 0);
        status = macParseDefns(macHandle, attributesMacros.c_str(), &macPairs);
        if (status < 0) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s, error parsing macros\n", driverName, functionName);
            goto done_macros;
        }
        status = macInstallMacros(macHandle, macPairs);
        if (status < 0) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s, error installed macros\n", driverName, functionName);
            goto done_macros;
        }
        // Create a temporary buffer 10 times larger than input buffer
        bufferSize = (int)(buffer.length() * 10);
        tmpBuffer = (char *)malloc(bufferSize);
        status = macExpandString(macHandle, buffer.c_str(), tmpBuffer, bufferSize);
        // NOTE: There is a bug in macExpandString up to 3.14.12.6 and 3.15.5 so that it does not return <0
        // if there is an undefined macro which is not the last macro in the string.
        // We work around this by testing also if the returned string contains ",undefined)".  This is
        // unlikely to occur otherwise.  Eventually we can remove this test.
        if ((status < 0)  || strstr(tmpBuffer, ",undefined)")) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s, error expanding macros\n", driverName, functionName);
            goto done_macros;
        }
        if (status >= bufferSize) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s, error macro buffer too small\n", driverName, functionName);
            goto done_macros;
        }
        buffer = tmpBuffer;
done_macros:
        macDeleteHandle(macHandle);
        free(tmpBuffer);
        if (status < 0) {
            setIntegerParam(NDAttributesStatus, NDAttributesMacroError);
            return asynError;
        }
    }
    // Assume failure
    setIntegerParam(NDAttributesStatus, NDAttributesXMLSyntaxError);
    doc = xmlReadMemory(buffer.c_str(), (int)buffer.length(), "noname.xml", NULL, 0);
    if (doc == NULL) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: error creating doc\n", driverName, functionName);
        return asynError;
    }
    Attrs = xmlDocGetRootElement(doc);
    if ((!xmlStrEqual(Attrs->name, (const xmlChar *)"Attributes"))) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: cannot find Attributes element\n", driverName, functionName);
        return asynError;
    }
    for (Attr = xmlFirstElementChild(Attrs); Attr; Attr = xmlNextElementSibling(Attr)) {
        pName = (const char *)xmlGetProp(Attr, (const xmlChar *)"name");
        if (!pName) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: name attribute not found\n", driverName, functionName);
            return asynError;
        }
        pDescription = (const char *)xmlGetProp(Attr, (const xmlChar *)"description");
        if (!pDescription) pDescription = "";
        pSource = (const char *)xmlGetProp(Attr, (const xmlChar *)"source");
        if (!pSource) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: source attribute not found for attribute %s\n", driverName, functionName, pName);
            return asynError;
        }
        pAttrType = (const char *)xmlGetProp(Attr, (const xmlChar *)"type");
        if (!pAttrType) pAttrType = NDAttribute::attrSourceString(NDAttrSourceEPICSPV);
        if (strcmp(pAttrType, NDAttribute::attrSourceString(NDAttrSourceEPICSPV)) == 0) {
            const char *pDBRType = (const char *)xmlGetProp(Attr, (const xmlChar *)"dbrtype");
            int dbrType = DBR_NATIVE;
            if (pDBRType) {
                if      (!strcmp(pDBRType, "DBR_CHAR"))   dbrType = DBR_CHAR;
                else if (!strcmp(pDBRType, "DBR_SHORT"))  dbrType = DBR_SHORT;
                else if (!strcmp(pDBRType, "DBR_ENUM"))   dbrType = DBR_ENUM;
                else if (!strcmp(pDBRType, "DBR_INT"))    dbrType = DBR_INT;
                else if (!strcmp(pDBRType, "DBR_LONG"))   dbrType = DBR_LONG;
                else if (!strcmp(pDBRType, "DBR_FLOAT"))  dbrType = DBR_FLOAT;
                else if (!strcmp(pDBRType, "DBR_DOUBLE")) dbrType = DBR_DOUBLE;
                else if (!strcmp(pDBRType, "DBR_STRING")) dbrType = DBR_STRING;
                else if (!strcmp(pDBRType, "DBR_NATIVE")) dbrType = DBR_NATIVE;
                else {
                    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                        "%s:%s: unknown dbrType = %s for attribute %s\n", driverName, functionName, pDBRType, pName);
                   return asynError;
                }
            }
            asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
                "%s:%s: Name=%s, PVName=%s, pDBRType=%s, dbrType=%d, pDescription=%s\n",
                driverName, functionName, pName, pSource, pDBRType, dbrType, pDescription);
#ifndef EPICS_LIBCOM_ONLY
            PVAttribute *pPVAttribute = new PVAttribute(pName, pDescription, pSource, dbrType);
            this->pAttributeList->add(pPVAttribute);
#endif
        } else if (strcmp(pAttrType, NDAttribute::attrSourceString(NDAttrSourceParam)) == 0) {
            const char *pDataType = (const char *)xmlGetProp(Attr, (const xmlChar *)"datatype");
            if (!pDataType) pDataType = "int";
            const char *pAddr = (const char *)xmlGetProp(Attr, (const xmlChar *)"addr");
            int addr=0;
            if (pAddr) addr = strtol(pAddr, NULL, 0);
            asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
                "%s:%s: Name=%s, drvInfo=%s, dataType=%s,pDescription=%s\n",
                driverName, functionName, pName, pSource, pDataType, pDescription);
            paramAttribute *pParamAttribute = new paramAttribute(pName, pDescription, pSource, addr, this, pDataType);
            this->pAttributeList->add(pParamAttribute);
        } else if (strcmp(pAttrType, NDAttribute::attrSourceString(NDAttrSourceFunct)) == 0) {
            const char *pParam = (const char *)xmlGetProp(Attr, (const xmlChar *)"param");
            if (!pParam) pParam = epicsStrDup("");
            asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
                "%s:%s: Name=%s, function=%s, pParam=%s, pDescription=%s\n",
                driverName, functionName, pName, pSource, pParam, pDescription);
#ifndef EPICS_LIBCOM_ONLY
            functAttribute *pFunctAttribute = new functAttribute(pName, pDescription, pSource, pParam);
            this->pAttributeList->add(pFunctAttribute);
#endif
        } 
        else if (strcmp(pAttrType, NDAttribute::attrSourceString(NDAttrSourceConst)) == 0) {
            NDAttribute *pAttribute = new NDAttribute(pName, pDescription, NDAttrSourceConst, pSource, NDAttrString, (void *)pSource);
            this->pAttributeList->add(pAttribute);
        } else { 
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: unknown attribute type = %s for attribute %s\n", driverName, functionName, pAttrType, pName);
            return asynError;
        }
    }
    setIntegerParam(NDAttributesStatus, NDAttributesOK);
    // Wait a short while for channel access callbacks on EPICS PVs
    epicsThreadSleep(0.5);
    // Get the initial values
    this->pAttributeList->updateValues();
    return asynSuccess;
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
asynStatus asynNDArrayDriver::getAttributes(NDAttributeList *pList)
{
    //const char *functionName = "getAttributes";
    int status = asynSuccess;

    status = this->pAttributeList->updateValues();
    status = this->pAttributeList->copy(pList);
    return (asynStatus) status;
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
    int addr;
    int function;
    const char *paramName;
    asynStatus status = asynSuccess;
    const char *functionName = "writeOctet";

    status = parseAsynUser(pasynUser, &function, &addr, &paramName);
    if (status != asynSuccess) return status;

    /* Set the parameter in the parameter library. */
    status = (asynStatus)setStringParam(addr, function, (char *)value);

    if ((function == NDAttributesFile) ||
        (function == NDAttributesMacros)) {
        this->readNDAttributesFile();
    } else if (function == NDFilePath) {
        status = this->checkPath();
        if (status == asynError) {
            // If the directory does not exist then try to create it
            int pathDepth;
            getIntegerParam(NDFileCreateDir, &pathDepth);
            status = createFilePath(value, pathDepth);
            status = this->checkPath();
        }
    }
     /* Do callbacks so higher layers see any changes */
    status = (asynStatus)callParamCallbacks(addr, addr);

    if (status)
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, paramName=%s, value=%s",
                  driverName, functionName, status, function, paramName, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, paramName=%s, value=%s\n",
              driverName, functionName, function, paramName, value);
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
    int function;
    const char *paramName;
    asynStatus status = asynSuccess;
    const char* functionName = "readNDArray";

    status = parseAsynUser(pasynUser, &function, &addr, &paramName);
    if (status != asynSuccess) return status;

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
        pasynUser->timestamp = myArray->epicsTS;
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

/** Sets the value for an integer in the parameter library.
  * Calls setIntegerParam(0, index, value) i.e. for parameter list 0.
  * \param[in] index The parameter number
  * \param[in] value Value to set. */
asynStatus asynNDArrayDriver::setIntegerParam(int index, int value)
{
    return this->setIntegerParam(0, index, value);
}

/** Sets the value for an integer in the parameter library.
  * \param[in] list The parameter list number.  Must be < maxAddr passed to asynPortDriver::asynPortDriver.
  * \param[in] index The parameter number
  * \param[in] value Value to set.
  * This function was added to trap the driver setting ADAcquire to 0 and
  * setting NumQueuedArrays.  It implements the logic of
  * setting ADAcquireBusy to reflect whether acquisition is done.
  * If WaitForPlugins is true then this includes waiting for NumQueuedArrays to be 0.
  * When ADAcquire goes to 0 it must use getQueuedArrayCount rather then NumQueuedArrays
  * from the parameter library, because NumQueuedArrays is updated in a separate thread
  * and might not have been set yet.  getQueuedArrayCount updates immediately. */
asynStatus asynNDArrayDriver::setIntegerParam(int list, int index, int value)
{

    if (index == ADAcquire) {
        if (value == 0) {
            int waitForPlugins;
            getIntegerParam(list, ADWaitForPlugins, &waitForPlugins);
            if (waitForPlugins) {
                int count = getQueuedArrayCount();
                if (count == 0) {
                    asynPortDriver::setIntegerParam(list, ADAcquireBusy, 0);
                }
            } else {
                asynPortDriver::setIntegerParam(list, ADAcquireBusy, 0);
            }
        } else {
            asynPortDriver::setIntegerParam(list, ADAcquireBusy, 1);
        }
    }
    else if ((index == NDNumQueuedArrays) && (value == 0)) {
        int acquire;
        getIntegerParam(list, ADAcquire, &acquire);
        if (acquire == 0) {
            asynPortDriver::setIntegerParam(list, ADAcquireBusy, 0);
        }
    }
    return asynPortDriver::setIntegerParam(list, index, value);
}

/** Sets an int32 parameter.
  * \param[in] pasynUser asynUser structure that contains the function code in pasynUser->reason.
  * \param[in] value The value for this parameter
  *
  * Takes action if the function code requires it. */
#define MEGABYTE_DBL 1048576.
asynStatus asynNDArrayDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function;
    int addr;
    const char *paramName;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeInt32";

    status = parseAsynUser(pasynUser, &function, &addr, &paramName);
    if (status != asynSuccess) return status;

    status = setIntegerParam(addr, function, value);

    if (function == NDPoolEmptyFreeList) {
        this->pNDArrayPool->emptyFreeList();
    } else if (function == NDPoolPreAllocBuffers) {
        preAllocateBuffers();
        setIntegerParam(NDPoolPreAllocBuffers, 0);
    } else if (function == NDPoolPollStats) {
        setDoubleParam(NDPoolMaxMemory, this->pNDArrayPool->getMaxMemory() / MEGABYTE_DBL);
        setDoubleParam(NDPoolUsedMemory, this->pNDArrayPool->getMemorySize() / MEGABYTE_DBL);
        setIntegerParam(NDPoolAllocBuffers, this->pNDArrayPool->getNumBuffers());
        setIntegerParam(NDPoolFreeBuffers, this->pNDArrayPool->getNumFree());
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, paramName=%s, value=%d\n",
              driverName, functionName, status, function, paramName, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, paramName=%s, value=%d\n",
              driverName, functionName, function, paramName, value);
    return status;
}

asynStatus asynNDArrayDriver::preAllocateBuffers()
{
    int numBuffers;
    NDArray *pArray;
    static const char *functionName = "preAllocateBuffers";

    /* Make sure there is a valid array */
    if (!this->pArrays[0]) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s: ERROR, must collect an array to get dimensions first\n",
            driverName, functionName);
        return asynError;
    }
    std::vector<NDArray*> buffVector;
    getIntegerParam(NDPoolNumPreAllocBuffers, &numBuffers);
    for (int i=0; i<numBuffers; i++) {
        pArray = this->pNDArrayPool->copy(this->pArrays[0], NULL, true);
        if (!pArray) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s: ERROR, could not allocate array %d\n",
                driverName, functionName, i);
            return asynError;
        }
        buffVector.push_back(pArray);
        setIntegerParam(NDPoolAllocBuffers, this->pNDArrayPool->getNumBuffers());
        setIntegerParam(NDPoolFreeBuffers, this->pNDArrayPool->getNumFree());
        setDoubleParam(NDPoolUsedMemory, this->pNDArrayPool->getMemorySize() / MEGABYTE_DBL);
        callParamCallbacks();
    }
    // Now release these NDArrays to put them on the free list
    for (int i=0; i<numBuffers; i++) {
        buffVector[i]->release();
    }
    return asynSuccess;
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
    if (this->pArrays[0]) {
        fprintf(fp, "\n");
        fprintf(fp, "%s: pArrays[0] report\n", this->portName);
        this->pArrays[0]->report(fp, details);
    }
    if (details > 5) {
        fprintf(fp, "\n");
        fprintf(fp, "%s: private NDArrayPool report\n", this->portName);
        this->pNDArrayPoolPvt_->report(fp, details);
        if (this->pNDArrayPool != this->pNDArrayPoolPvt_) {
            fprintf(fp, "\n");
            fprintf(fp, "%s: shared NDArrayPool report\n", this->portName);
            this->pNDArrayPool->report(fp, details);
        }
        fprintf(fp, "\n");
        fprintf(fp, "%s: pAttributeList report\n", this->portName);
        this->pAttributeList->report(fp, details);
    }
}

static void updateQueuedArrayCountC(void *drvPvt)
{
    asynNDArrayDriver *pPvt = (asynNDArrayDriver *)drvPvt;

    pPvt->updateQueuedArrayCount();
}

void asynNDArrayDriver::updateQueuedArrayCount()
{
    while (queuedArrayUpdateRun_) {
        epicsEventWait(queuedArrayEvent_);
        // Exit early
        if (!queuedArrayUpdateRun_)
            break;

        lock();
        setIntegerParam(NDNumQueuedArrays, getQueuedArrayCount());
        callParamCallbacks();
        unlock();
    }
    epicsEventSignal(queuedArrayUpdateDone_);
}

int asynNDArrayDriver::getQueuedArrayCount()
{
    queuedArrayCountMutex_->lock();
    int count = queuedArrayCount_;
    queuedArrayCountMutex_->unlock();
    return count;
}

asynStatus asynNDArrayDriver::incrementQueuedArrayCount()
{
    queuedArrayCountMutex_->lock();
    queuedArrayCount_++;
    queuedArrayCountMutex_->unlock();
    epicsEventSignal(queuedArrayEvent_);
    return asynSuccess;
}

asynStatus asynNDArrayDriver::decrementQueuedArrayCount()
{
    static const char *functionName = "decrementQueuedArrayCount";

    queuedArrayCountMutex_->lock();
    if (queuedArrayCount_ <= 0) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s error, numQueuedArrays already 0 or less (%d)\n",
            driverName, functionName, queuedArrayCount_);
    }
    queuedArrayCount_--;
    queuedArrayCountMutex_->unlock();
    epicsEventSignal(queuedArrayEvent_);
    return asynSuccess;
}

void asynNDArrayDriver::updateTimeStamps(NDArray *pArray)
{
    updateTimeStamp(&pArray->epicsTS);
    pArray->timeStamp = pArray->epicsTS.secPastEpoch + pArray->epicsTS.nsec/1.e9;
}



/** This is the constructor for the asynNDArrayDriver class.
  * portName, maxAddr, interfaceMask, interruptMask, asynFlags, autoConnect, priority and stackSize
  * are simply passed to asynPortDriver::asynPortDriver.
  * asynNDArrayDriver creates an NDArrayPool object to allocate NDArray
  * objects. maxBuffers and maxMemory are passed to NDArrayPool::NDArrayPool.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxAddr The maximum  number of asyn addr addresses this driver supports. 1 is minimum.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to 0 to allow an unlimited number of buffers.
  *            This value is no longer used and is ignored.  It will be removed in a future major release.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to 0 to allow an unlimited amount of memory.
  * \param[in] interfaceMask Bit mask defining the asyn interfaces that this driver supports.
  * \param[in] interruptMask Bit mask definining the asyn interfaces that can generate interrupts (callbacks)
  * \param[in] asynFlags Flags when creating the asyn port driver; includes ASYN_CANBLOCK and ASYN_MULTIDEVICE.
  * \param[in] autoConnect The autoConnect flag for the asyn port driver.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  *            This value should also be used for any other threads this object creates.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  *            This value should also be used for any other threads this object creates.
  */

asynNDArrayDriver::asynNDArrayDriver(const char *portName, int maxAddr, int maxBuffers,
                                     size_t maxMemory, int interfaceMask, int interruptMask,
                                     int asynFlags, int autoConnect, int priority, int stackSize)
    : asynPortDriver(portName, maxAddr,
                     interfaceMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynInt32ArrayMask | asynGenericPointerMask | asynDrvUserMask,
                     interruptMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynInt32ArrayMask | asynGenericPointerMask,
                     asynFlags, autoConnect, priority, stackSize),
      pNDArrayPool(NULL), queuedArrayCountMutex_(NULL), queuedArrayCount_(0),
      queuedArrayUpdateRun_(true)
{
    char versionString[20];
    static const char *functionName = "asynNDArrayDriver";

    /* Save the stack size and priority for other threads that this object may create */
    if (stackSize <= 0) stackSize = epicsThreadGetStackSize(epicsThreadStackMedium);
    threadStackSize_ = stackSize;
    if (priority <= 0) priority = epicsThreadPriorityMedium;
    threadPriority_ = priority;

    this->pNDArrayPoolPvt_ = new NDArrayPool(this, maxMemory);
    this->pNDArrayPool = this->pNDArrayPoolPvt_;
    this->queuedArrayCountMutex_ = new epicsMutex();

    /* Allocate pArray pointer array */
    this->pArrays = (NDArray **)calloc(maxAddr, sizeof(NDArray *));
    this->pAttributeList = new NDAttributeList();

    createParam(NDPortNameSelfString,         asynParamOctet,           &NDPortNameSelf);
    createParam(NDADCoreVersionString,        asynParamOctet,           &NDADCoreVersion);
    createParam(NDDriverVersionString,        asynParamOctet,           &NDDriverVersion);
    createParam(ADManufacturerString,         asynParamOctet,           &ADManufacturer);
    createParam(ADModelString,                asynParamOctet,           &ADModel);
    createParam(ADSerialNumberString,         asynParamOctet,           &ADSerialNumber);
    createParam(ADSDKVersionString,           asynParamOctet,           &ADSDKVersion);
    createParam(ADFirmwareVersionString,      asynParamOctet,           &ADFirmwareVersion);
    createParam(ADAcquireString,              asynParamInt32,           &ADAcquire);
    createParam(ADAcquireBusyString,          asynParamInt32,           &ADAcquireBusy);
    createParam(ADWaitForPluginsString,       asynParamInt32,           &ADWaitForPlugins);
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
    createParam(NDEpicsTSSecString,           asynParamInt32,           &NDEpicsTSSec);
    createParam(NDEpicsTSNsecString,          asynParamInt32,           &NDEpicsTSNsec);
    createParam(NDBayerPatternString,         asynParamInt32,           &NDBayerPattern);
    createParam(NDCodecString,                asynParamOctet,           &NDCodec);
    createParam(NDCompressedSizeString,       asynParamInt32,           &NDCompressedSize);
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
    createParam(NDFileFreeCaptureString,      asynParamInt32,           &NDFileFreeCapture);
    createParam(NDFileCaptureString,          asynParamInt32,           &NDFileCapture);
    createParam(NDFileDeleteDriverFileString, asynParamInt32,           &NDFileDeleteDriverFile);
    createParam(NDFileLazyOpenString,         asynParamInt32,           &NDFileLazyOpen);
    createParam(NDFileCreateDirString,        asynParamInt32,           &NDFileCreateDir);
    createParam(NDFileTempSuffixString,       asynParamOctet,           &NDFileTempSuffix);
    createParam(NDAttributesFileString,       asynParamOctet,           &NDAttributesFile);
    createParam(NDAttributesStatusString,     asynParamInt32,           &NDAttributesStatus);
    createParam(NDAttributesMacrosString,     asynParamOctet,           &NDAttributesMacros);
    createParam(NDArrayDataString,            asynParamGenericPointer,  &NDArrayData);
    createParam(NDArrayCallbacksString,       asynParamInt32,           &NDArrayCallbacks);
    createParam(NDPoolMaxBuffersString,       asynParamInt32,           &NDPoolMaxBuffers);
    createParam(NDPoolAllocBuffersString,     asynParamInt32,           &NDPoolAllocBuffers);
    createParam(NDPoolPreAllocBuffersString,  asynParamInt32,           &NDPoolPreAllocBuffers);
    createParam(NDPoolNumPreAllocBuffersString, asynParamInt32,         &NDPoolNumPreAllocBuffers);
    createParam(NDPoolFreeBuffersString,      asynParamInt32,           &NDPoolFreeBuffers);
    createParam(NDPoolMaxMemoryString,        asynParamFloat64,         &NDPoolMaxMemory);
    createParam(NDPoolUsedMemoryString,       asynParamFloat64,         &NDPoolUsedMemory);
    createParam(NDPoolEmptyFreeListString,    asynParamInt32,           &NDPoolEmptyFreeList);
    createParam(NDPoolPollStatsString,        asynParamInt32,           &NDPoolPollStats);
    createParam(NDNumQueuedArraysString,      asynParamInt32,           &NDNumQueuedArrays);

    /* Here we set the values of read-only parameters and of read/write parameters that cannot
     * or should not get their values from the database.  Note that values set here will override
     * those in the database for output records because if asyn device support reads a value from
     * the driver with no error during initialization then it sets the output record to that value.
     * If a value is not set here then the read request will return an error (uninitialized).
     * Values set here will be overridden by values from save/restore if they exist. */
    setStringParam (NDPortNameSelf, portName);
    epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d",
                  ADCORE_VERSION, ADCORE_REVISION, ADCORE_MODIFICATION);
    setStringParam(NDADCoreVersion, versionString);
    // We set the driver version to the same thing, which is appropriate for plugins in ADCore
    // Other drivers need to set this after this constructor
    setStringParam(NDDriverVersion, versionString);
    setIntegerParam(NDArraySizeX,   0);
    setIntegerParam(NDArraySizeY,   0);
    setIntegerParam(NDArraySizeZ,   0);
    setIntegerParam(NDArraySize,    0);
    setIntegerParam(NDNDimensions,  0);
    setIntegerParam(NDColorMode,    NDColorModeMono);
    setIntegerParam(NDUniqueId,     0);
    setDoubleParam (NDTimeStamp,    0.);
    setIntegerParam(NDEpicsTSSec,   0);
    setIntegerParam(NDEpicsTSNsec,  0);
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
    setStringParam (NDFilePath, "");
    setStringParam (NDFileName, "");
    setIntegerParam(NDFileNumber, 0);
    setIntegerParam(NDAutoIncrement, 0);
    setStringParam (NDFileTemplate, "%s%s_%3.3d.dat");
    setIntegerParam(NDFileNumCaptured, 0);
    setIntegerParam(NDFileFreeCapture, 0);
    setIntegerParam(NDFileCreateDir, 0);
    setStringParam (NDFileTempSuffix, "");
    setStringParam (NDAttributesFile, "");
    setIntegerParam(NDAttributesStatus, NDAttributesFileNotFound);
    setStringParam (NDAttributesMacros, "");

    setIntegerParam(NDPoolAllocBuffers, this->pNDArrayPool->getNumBuffers());
    setIntegerParam(NDPoolFreeBuffers, this->pNDArrayPool->getNumFree());
    setDoubleParam(NDPoolMaxMemory, 0);
    setDoubleParam(NDPoolUsedMemory, 0);

    setIntegerParam(NDNumQueuedArrays, 0);

    queuedArrayEvent_ = epicsEventCreate(epicsEventEmpty);
    queuedArrayUpdateDone_ = epicsEventCreate(epicsEventEmpty);
    /* Create the thread that updates the queued array count */

    char taskName[100];
    epicsSnprintf(taskName, sizeof(taskName)-1, "%s_updateQueuedArrayCount", portName);
    epicsThreadId queuedArrayThreadId = epicsThreadCreate(taskName,
                                                          this->threadPriority_,
                                                          this->threadStackSize_,
                                                          (EPICSTHREADFUNC)updateQueuedArrayCountC, this);
    if (queuedArrayThreadId == 0) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s error creating updateQueuedArrayCount thread\n",
            driverName, functionName);
    }

}


asynNDArrayDriver::~asynNDArrayDriver()
{
    queuedArrayUpdateRun_ = false;
    epicsEventSignal(queuedArrayEvent_);
    epicsEventWait(queuedArrayUpdateDone_);

    delete this->pNDArrayPoolPvt_;
    free(this->pArrays);
    delete this->pAttributeList;
    delete this->queuedArrayCountMutex_;
}

