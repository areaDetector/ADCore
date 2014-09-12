/* NDFileNetCDF.cpp
 * Writes NDArrays to netCDF files.
 *
 * Mark Rivers
 * April 17, 2008
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>

#include <epicsStdio.h>
#include <epicsThread.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginFile.h"
#include "NDFileNetCDF.h"

static const char *driverName = "NDFileNetCDF";

/* Handle errors by printing an error message and exiting with a
 * non-zero status. */
#define ERR(e) {asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, \
                "%s:%s error=%s\n", \
                driverName, functionName, nc_strerror(e)); \
                return(asynError);}

/** Opens a netCDF file.  
  * In write mode if NDFileModeMultiple is set then the first dimension is set to NC_UNLIMITED to allow 
  * multiple arrays to be written to the same file.
  * NOTE: Does not currently support NDFileModeRead or NDFileModeAppend.
  * \param[in] fileName  Absolute path name of the file to open.
  * \param[in] openMode Bit mask with one of the access mode bits NDFileModeRead, NDFileModeWrite, NDFileModeAppend.
  *           May also have the bit NDFileModeMultiple set if the file is to be opened to write or read multiple 
  *           NDArrays into a single file.
  * \param[in] pArray Pointer to an NDArray; this array is used to determine the header information and data 
  *           structure for the file. */
asynStatus NDFileNetCDF::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
    /* When we create netCDF variables and dimensions, we get back an
     * ID for each one. */
    int dimIds[ND_ARRAY_MAX_DIMS+1];
    int stringDimIds[2];
    int size[ND_ARRAY_MAX_DIMS], offset[ND_ARRAY_MAX_DIMS];
    int binning[ND_ARRAY_MAX_DIMS], reverse[ND_ARRAY_MAX_DIMS];
    char dimName[25];
    int dim0;
    int retval;
    nc_type ncType=NC_NAT;
    int i, j;
    NDAttribute *pAttribute;
    char tempString[MAX_ATTRIBUTE_STRING_SIZE];
    const char *dataTypeString=NULL;
    NDAttrDataType_t attrDataType;
    size_t attrSize;
    int numAttributes, attrCount;
    double fileVersion;
    static const char *functionName = "openFile";

    /* We don't support reading yet */    
    if (openMode & NDFileModeRead) return(asynError);
    
    /* We don't support opening an existing file for appending yet */    
    if (openMode & NDFileModeAppend) return(asynError);

    /* Construct an attribute list. We use a separate attribute list
     * from the one in pArray to avoid the need to copy the array. */
    /* First clear the list*/
    this->pFileAttributes->clear();
    /* Now get the current values of the attributes for this plugin */
    this->getAttributes(this->pFileAttributes);
    /* Now append the attributes from the array which are already up to date from
     * the driver and prior plugins */
    pArray->pAttributeList->copy(this->pFileAttributes);
    
    /* Set the next record in the file to 0 */
    this->nextRecord = 0;

    /* Create the file. The NC_CLOBBER parameter tells netCDF to
     * overwrite this file, if it already exists.*/
    if ((retval = nc_create(fileName, NC_CLOBBER, &this->ncId)))
        ERR(retval);

    /* Create global attribute for the data type because netCDF does not
     * distinguish signed and unsigned.  Readers can use this to know how to treat
     * integer data. */
    if ((retval = nc_put_att_int(this->ncId, NC_GLOBAL, "dataType", 
                                 NC_INT, 1, (const int*)&pArray->dataType)))
        ERR(retval);

    /* Create global attribute with NDNetCDFFileVersion so readers can handle changes
     * in file contents */
    fileVersion = NDNetCDFFileVersion;
    if ((retval = nc_put_att_double(this->ncId, NC_GLOBAL, "NDNetCDFFileVersion",
                                 NC_DOUBLE, 1, &fileVersion)))
        ERR(retval);

    /* Create global attribute for number of dimensions and dimensions
     * in each NDArray.
     * This is redundant with information netCDF puts in, but the netCDF
     * info includes the number of arrays in the file.  This can make it
     * easier to write readers */
    if ((retval = nc_put_att_int(this->ncId, NC_GLOBAL, "numArrayDims", 
                                 NC_INT, 1, &pArray->ndims)))
        ERR(retval);

    /* Define the dimensions. NetCDF will hand back an ID for each.
     * netCDF has the first dimension changing slowest, opposite of NDArrayBuff
     * convention. 
     * We make the first dimension the number of arrays in the file.  This is either
     * 1 or NC_UNLIMITED */
    dim0 = 1;
    if (openMode & NDFileModeMultiple) dim0 = NC_UNLIMITED;
    if ((retval = nc_def_dim(this->ncId, "numArrays", dim0, &dimIds[0])))
        ERR(retval);

    /* The next dimensions are the dimensions of the data in reversed order */
    for (i=0; i<pArray->ndims; i++) {
        j = pArray->ndims - i - 1;
        sprintf(dimName, "dim%d", i);
        if ((retval = nc_def_dim(this->ncId, dimName, pArray->dims[j].size, &dimIds[i+1])))
            ERR(retval);
        size[i]    = (int)pArray->dims[i].size;
        offset[i]  = (int)pArray->dims[i].offset;
        binning[i] = pArray->dims[i].binning;
        reverse[i]  = pArray->dims[i].reverse;
    }

    /* String attributes are special.  The first dimension is the number of arrays, the second is the string size */
    stringDimIds[0] = dimIds[0];
    if ((retval = nc_def_dim(this->ncId, "attrStringSize", MAX_ATTRIBUTE_STRING_SIZE, &stringDimIds[1])))
        ERR(retval);

    /* Create global attribute for information about the dimensions */
    if ((retval = nc_put_att_int(this->ncId, NC_GLOBAL, "dimSize", 
                                 NC_INT, pArray->ndims, size)))
        ERR(retval);
    if ((retval = nc_put_att_int(this->ncId, NC_GLOBAL, "dimOffset", 
                                 NC_INT, pArray->ndims, offset)))
        ERR(retval);
    if ((retval = nc_put_att_int(this->ncId, NC_GLOBAL, "dimBinning", 
                                 NC_INT, pArray->ndims, binning)))
        ERR(retval);
    if ((retval = nc_put_att_int(this->ncId, NC_GLOBAL, "dimReverse", 
                                 NC_INT, pArray->ndims, reverse)))
        ERR(retval);

    /* Convert from NDArray data types to netCDF data types */
    switch (pArray->dataType) {
        case NDInt8:
        case NDUInt8:
            ncType = NC_BYTE;
            break;
        case NDInt16:
        case NDUInt16:
            ncType = NC_SHORT;
            break;
        case NDInt32:
        case NDUInt32:
            ncType = NC_INT;
            break;
        case NDFloat32:
            ncType = NC_FLOAT;
            break;
        case NDFloat64:
            ncType = NC_DOUBLE;
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s error, unknown array datatype=%d\n",
                driverName, functionName, pArray->dataType);
            return asynError;
    }

    /* Define the uniqueId data variable. */
    if ((retval = nc_def_var(this->ncId, "uniqueId", NC_INT, 1, 
                 &dimIds[0], &this->uniqueIdId)))
        ERR(retval);

    /* Define the timestamp data variable. */
    if ((retval = nc_def_var(this->ncId, "timeStamp", NC_DOUBLE, 1, 
                 &dimIds[0], &this->timeStampId)))
        ERR(retval);

    /* Define the EPICS timestamp data variables. */
    if ((retval = nc_def_var(this->ncId, "epicsTSSec", NC_INT, 1, 
                 &dimIds[0], &this->epicsTSSecId)))
        ERR(retval);

    if ((retval = nc_def_var(this->ncId, "epicsTSNsec", NC_INT, 1, 
                 &dimIds[0], &this->epicsTSNsecId)))
        ERR(retval);

    /* Define the array data variable. */
    if ((retval = nc_def_var(this->ncId, "array_data", ncType, pArray->ndims+1,
                 dimIds, &this->arrayDataId)))
        ERR(retval);

    /* Create a variable for each attribute in the array */
    free(this->pAttributeId);
    numAttributes = this->pFileAttributes->count();
    attrCount = 0;
    this->pAttributeId = (int *)calloc(numAttributes, sizeof(int));
    pAttribute = this->pFileAttributes->next(NULL);
    while (pAttribute) {
        const char *attributeName = pAttribute->getName();
        const char *attributeDescription = pAttribute->getDescription();
        const char *attributeSource = pAttribute->getSource();
        NDAttrSource_t attributeSourceType;
        const char *attributeSourceTypeString = pAttribute->getSourceInfo(&attributeSourceType);
        pAttribute->getValueInfo(&attrDataType, &attrSize);
        switch (attrDataType) {
            case NDAttrInt8:
                dataTypeString = "Int8";
                break;
            case NDAttrUInt8:
                dataTypeString = "UInt8";
                break;
            case NDAttrInt16:
                dataTypeString = "Int16";
                break;
            case NDAttrUInt16:
                dataTypeString = "UInt16";
                break;
            case NDAttrInt32:
                dataTypeString = "Int32";
                break;
            case NDAttrUInt32:
                dataTypeString = "UInt32";
                break;
            case NDAttrFloat32:
                dataTypeString = "Float32";
                break;
            case NDAttrFloat64:
                dataTypeString = "Float64";
                break;
            case NDAttrString:
                dataTypeString = "String";
                break;
            case NDAttrUndefined:
                dataTypeString = "Undefined";
                break;
            default:
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s error, unknown attrDataType=%d\n",
                    driverName, functionName, attrDataType);
                return asynError;
                break;
        }
        epicsSnprintf(tempString, sizeof(tempString), "Attr_%s_DataType", attributeName);
        if ((retval = nc_put_att_text(this->ncId, NC_GLOBAL, tempString, 
                                     strlen(dataTypeString), dataTypeString)))
            ERR(retval);
        epicsSnprintf(tempString, sizeof(tempString), "Attr_%s_Description", attributeName);
        if ((retval = nc_put_att_text(this->ncId, NC_GLOBAL, tempString, 
                                     strlen(attributeDescription), attributeDescription)))
            ERR(retval);
        epicsSnprintf(tempString, sizeof(tempString), "Attr_%s_Source", attributeName);
        if ((retval = nc_put_att_text(this->ncId, NC_GLOBAL, tempString, 
                                     strlen(attributeSource), attributeSource)))
            ERR(retval);
        epicsSnprintf(tempString, sizeof(tempString), "Attr_%s_SourceType", attributeName);

        if ((retval = nc_put_att_text(this->ncId, NC_GLOBAL, tempString, 
                                     strlen(attributeSourceTypeString), 
                                     attributeSourceTypeString)))
            ERR(retval);
        switch (attrDataType) {
            case NDAttrInt8:
            case NDAttrUInt8:
                ncType = NC_BYTE;
                break;
            case NDAttrInt16:
            case NDAttrUInt16:
                ncType = NC_SHORT;
                break;
            case NDAttrInt32:
            case NDAttrUInt32:
                ncType = NC_INT;
                break;
            case NDAttrFloat32:
                ncType = NC_FLOAT;
                break;
            case NDAttrFloat64:
                ncType = NC_DOUBLE;
                break;
            case NDAttrString:
                ncType = NC_CHAR;
                break;
            case NDAttrUndefined:
                ncType = NC_BYTE;
                break;
            default:
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s error, unknown attrDataType=%d\n",
                    driverName, functionName, attrDataType);
                return asynError;
                break;
        }
        epicsSnprintf(tempString, sizeof(tempString), "Attr_%s", pAttribute->getName());
        if (attrDataType == NDAttrString) {
            if ((retval = nc_def_var(this->ncId, tempString, ncType, 2,
                    stringDimIds, &this->pAttributeId[attrCount++])))
                    ERR(retval);
        } else {
            if ((retval = nc_def_var(this->ncId, tempString, ncType, 1,
                    &dimIds[0], &this->pAttributeId[attrCount++])))
                    ERR(retval);
        }
        pAttribute = this->pFileAttributes->next(pAttribute);
    }

    /* End define mode. This tells netCDF we are done defining
     * metadata. */
    if ((retval = nc_enddef(this->ncId)))
        ERR(retval);
    return(asynSuccess);
}


/** Writes NDArray data to a netCDF file.
  * \param[in] pArray Pointer to an NDArray to write to the file. This function can be called multiple
  *           times between the call to openFile and closeFile if
  *           NDFileModeMultiple was set in openMode in the call to NDFileNetCDF::openFile. */ 
asynStatus NDFileNetCDF::writeFile(NDArray *pArray)
{       
    int retval;
    size_t stringCount[2];
    size_t start[ND_ARRAY_MAX_DIMS+1], count[ND_ARRAY_MAX_DIMS+1];
    int attrId;
    NDAttrValue attrVal;
    int i, j;
    NDAttribute *pAttribute;
    NDAttrDataType_t attrDataType;
    size_t attrSize;
    int attrCount;
    char attrString[MAX_ATTRIBUTE_STRING_SIZE];
    static const char *functionName = "writeFile";

    
    /* Update attribute list. We use a separate attribute list
     * from the one in pArray to avoid the need to copy the array. */
    /* Get the current values of the attributes for this plugin */
    this->getAttributes(this->pFileAttributes);
    /* Now append the attributes from the array which are already up to date from
     * the driver and prior plugins */
    pArray->pAttributeList->copy(this->pFileAttributes);

    count[0] = 1;
    start[0] = this->nextRecord;
    for (i=0; i<pArray->ndims; i++) {
        j = pArray->ndims - i - 1;
        count[i+1] = pArray->dims[j].size;
        start[i+1] = 0;
    }
 
    /* Write the data to the file. */
    if ((retval = nc_put_vara_int(this->ncId, this->uniqueIdId, start, count, &pArray->uniqueId)))
                ERR(retval);
    if ((retval = nc_put_vara_double(this->ncId, this->timeStampId, start, count, &pArray->timeStamp)))
                ERR(retval);
    if ((retval = nc_put_vara_int(this->ncId, this->epicsTSSecId, start, count, (int *)&pArray->epicsTS.secPastEpoch)))
                ERR(retval);
    if ((retval = nc_put_vara_int(this->ncId, this->epicsTSNsecId, start, count, (int *)&pArray->epicsTS.nsec)))
                ERR(retval);

    switch (pArray->dataType) {
        case NDInt8:
            if ((retval = nc_put_vara_schar(this->ncId, this->arrayDataId, start, count, (signed char*)pArray->pData)))
                ERR(retval);
            break;
        case NDUInt8:
            if ((retval = nc_put_vara_uchar(this->ncId, this->arrayDataId, start, count, (unsigned char*)pArray->pData)))
                ERR(retval);
            break;
        case NDInt16:
        case NDUInt16:
            if ((retval = nc_put_vara_short(this->ncId, this->arrayDataId, start, count, (short *)pArray->pData)))
                ERR(retval);
            break;
        case NDInt32:
        case NDUInt32:
            if ((retval = nc_put_vara_int(this->ncId, this->arrayDataId, start, count, (int *)pArray->pData)))
                ERR(retval);
            break;
        case NDFloat32:
            if ((retval = nc_put_vara_float(this->ncId, this->arrayDataId, start, count, (float *)pArray->pData)))
                ERR(retval);
            break;
        case NDFloat64:
            if ((retval = nc_put_vara_double(this->ncId, this->arrayDataId, start, count, (double *)pArray->pData)))
                ERR(retval);
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s error, unknown array data type =%d\n",
                driverName, functionName, pArray->dataType);
            return asynError;
            break;
    }
    /* Write the attributes.  Loop through the list of attributes.  These must not have changed since define time! */
    pAttribute = this->pFileAttributes->next(NULL);
    attrCount = 0;
    while (pAttribute) {
        pAttribute->getValueInfo(&attrDataType, &attrSize);
        attrId = this->pAttributeId[attrCount++];
        switch (attrDataType) {
            case NDAttrInt8:
            case NDAttrUInt8:
                pAttribute->getValue(attrDataType, &attrVal.i8);
                if ((retval = nc_put_vara_schar(this->ncId, attrId, start, count, (signed char*)&attrVal.i8)))
                    ERR(retval);
                break;
            case NDAttrInt16:
            case NDAttrUInt16:
                pAttribute->getValue(attrDataType, &attrVal.i16);
                if ((retval = nc_put_vara_short(this->ncId, attrId, start, count, (short *)&attrVal.i16)))
                    ERR(retval);
                break;
            case NDAttrInt32:
            case NDAttrUInt32:
                pAttribute->getValue(attrDataType, &attrVal.i32);
                if ((retval = nc_put_vara_int(this->ncId, attrId, start, count, (int *)&attrVal.i32)))
                    ERR(retval);
                break;
            case NDAttrFloat32:
                pAttribute->getValue(attrDataType, &attrVal.f32);
                if ((retval = nc_put_vara_float(this->ncId, attrId, start, count, (float *)&attrVal.f32)))
                    ERR(retval);
                break;
            case NDAttrFloat64:
                pAttribute->getValue(attrDataType, &attrVal.f64);
                if ((retval = nc_put_vara_double(this->ncId, attrId, start, count, (double *)&attrVal.f64)))
                    ERR(retval);
                break;
            case NDAttrString:
                pAttribute->getValue(attrDataType, attrString, sizeof(attrString));
                stringCount[0] = 1;
                stringCount[1] = strlen(attrString);
                if ((retval = nc_put_vara_text(this->ncId, attrId, start, stringCount, attrString)))
                    ERR(retval);
                break;
            case NDAttrUndefined:
                /* netCDF does not have a way of storing NaN, etc. We just use 0 byte */
                attrVal.i8 = 0;
                if ((retval = nc_put_vara_schar(this->ncId, attrId, start, count, (signed char*)&attrVal.i8)))
                    ERR(retval);
                break;
            default:
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s error, unknown attrDataType=%d\n",
                    driverName, functionName, attrDataType);
                return asynError;
                break;
        }
        pAttribute = this->pFileAttributes->next(pAttribute);
    }
    this->nextRecord++;
    return(asynSuccess);
}

/** Read NDArray data from a netCDF file; NOTE: not implemented yet.
  * \param[in] pArray Pointer to the address of an NDArray to read the data into.  */ 
asynStatus NDFileNetCDF::readFile(NDArray **pArray)
{
    //static const char *functionName = "readFile";

    return asynError;
}


/** Closes the netCDF file opened with NDFileNetCDF::openFile */ 
asynStatus NDFileNetCDF::closeFile()
{
    int retval;
    static const char *functionName = "closeFile";

    if (this->ncId == 0) return asynSuccess;
    if ((retval = nc_close(this->ncId)))
        ERR(retval);
    this->ncId = 0;
    return asynSuccess;
}


/** Constructor for NDFileNetCDF; parameters are identical to those for NDPluginFile::NDPluginFile,
    and are passed directly to that base class constructor.
  * After calling the base class constructor this method sets NDPluginFile::supportsMultipleArrays=1.
  */
NDFileNetCDF::NDFileNetCDF(const char *portName, int queueSize, int blockingCallbacks, 
                           const char *NDArrayPort, int NDArrayAddr,
                           int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDFILE_NETCDF_PARAMS,
                   2, 0, asynGenericPointerMask, asynGenericPointerMask, 
                   ASYN_CANBLOCK, 1, priority, 
                   /* netCDF needs a relatively large stack, make the default be large */
                   (stackSize==0) ? epicsThreadGetStackSize(epicsThreadStackBig) : stackSize)
{
    //static const char *functionName = "NDFileNetCDF";
    
    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDFileNetCDF");
    this->supportsMultipleArrays = 1;
    this->pAttributeId = NULL;
    this->ncId = 0;
    this->pFileAttributes = new NDAttributeList;
}

/** Configuration routine.  Called directly, or from the iocsh function in NDFileEpics */
extern "C" int NDFileNetCDFConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                                     const char *NDArrayPort, int NDArrayAddr,
                                     int priority, int stackSize)
{
    new NDFileNetCDF(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                     priority, stackSize);
    return(asynSuccess);
}


/** EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"NDFileNetCDFConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileNetCDFConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, 
                             args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileNetCDFRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileNetCDFRegister);
}
