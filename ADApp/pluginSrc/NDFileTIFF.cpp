/* NDFileTIFF.cpp
 * Writes NDArrays to TIFF files.
 *
 * John Hammonds and Mark Rivers
 * May 11, 2009
 */

/*
 * Modifed to write all NDAttributes as TIFF Ascii file tags.
 * This uses private tag numbers between 65010 and 65500.
 * The NDAttribute name is encoded along with the value as
 * a string (because adding the name to the TIFFFieldInfo 
 * struct doesn't seem to make it to the tag in the file).
 *
 * The NDAttributes from the driver are read from the NDArray.
 * NDAttributes from the driver are appended to the 
 * NDAttributeList for this plugin.
 * 
 * Matt Pearson, April 4, 2014.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>

#include <epicsStdio.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginFile.h"
#include "tiffio.h"
#include "NDFileTIFF.h"

static const char *driverName = "NDFileTIFF";

const int NDFileTIFF::TIFFTAG_START_ = 65010;
const int NDFileTIFF::TIFFTAG_END_ = 65500;

#define MAX_ATTRIBUTE_STRING_SIZE 256

/** Opens a TIFF file.
  * \param[in] fileName The name of the file to open.
  * \param[in] openMode Mask defining how the file should be opened; bits are 
  *            NDFileModeRead, NDFileModeWrite, NDFileModeAppend, NDFileModeMultiple
  * \param[in] pArray A pointer to an NDArray; this is used to determine the array and attribute properties.
  */
asynStatus NDFileTIFF::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
    /* When we create TIFF variables and dimensions, we get back an
     * ID for each one. */
    static const char *functionName = "openFile";
    size_t sizeX, sizeY, rowsPerStrip;
    int bitsPerSample=8, sampleFormat=SAMPLEFORMAT_INT, samplesPerPixel, photoMetric, planarConfig;
    int colorMode=NDColorModeMono;
    NDAttribute *pAttribute = NULL;
    char tagString[MAX_ATTRIBUTE_STRING_SIZE] = {0};
    char attrString[MAX_ATTRIBUTE_STRING_SIZE] = {0};

    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);

   /* Create the file. */
    if ((this->output = TIFFOpen(fileName, "w")) == NULL ) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s:%s error opening file %s\n",
        driverName, functionName, fileName);
        return(asynError);
    }
    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colorMode);

    switch (pArray->dataType) {
        case NDInt8:
            sampleFormat = SAMPLEFORMAT_INT;
            bitsPerSample = 8;
            break;
        case NDUInt8:
            sampleFormat = SAMPLEFORMAT_UINT;
            bitsPerSample = 8;
            break;
        case NDInt16:
            sampleFormat = SAMPLEFORMAT_INT;
            bitsPerSample = 16;
            break;
        case NDUInt16:
            sampleFormat = SAMPLEFORMAT_UINT;
            bitsPerSample = 16;
            break;
        case NDInt32:
            sampleFormat = SAMPLEFORMAT_INT;
            bitsPerSample = 32;
            break;
        case NDUInt32:
            sampleFormat = SAMPLEFORMAT_UINT;
            bitsPerSample = 32;
            break;
        case NDFloat32:
            sampleFormat = SAMPLEFORMAT_IEEEFP;
            bitsPerSample = 32;
            break;
        case NDFloat64:
            sampleFormat = SAMPLEFORMAT_IEEEFP;
            bitsPerSample = 64;
            break;
    }
    if (pArray->ndims == 2) {
        sizeX = pArray->dims[0].size;
        sizeY = pArray->dims[1].size;
        rowsPerStrip = sizeY;
        samplesPerPixel = 1;
        photoMetric = PHOTOMETRIC_MINISBLACK;
        planarConfig = PLANARCONFIG_CONTIG;
        this->colorMode = NDColorModeMono;
    } else if ((pArray->ndims == 3) && (pArray->dims[0].size == 3) && (colorMode == NDColorModeRGB1)) {
        sizeX = pArray->dims[1].size;
        sizeY = pArray->dims[2].size;
        rowsPerStrip = sizeY;
        samplesPerPixel = 3;
        photoMetric = PHOTOMETRIC_RGB;
        planarConfig = PLANARCONFIG_CONTIG;
        this->colorMode = NDColorModeRGB1;
    } else if ((pArray->ndims == 3) && (pArray->dims[1].size == 3) && (colorMode == NDColorModeRGB2)) {
        sizeX = pArray->dims[0].size;
        sizeY = pArray->dims[2].size;
        rowsPerStrip = 1;
        samplesPerPixel = 3;
        photoMetric = PHOTOMETRIC_RGB;
        planarConfig = PLANARCONFIG_SEPARATE;
        this->colorMode = NDColorModeRGB2;
    } else if ((pArray->ndims == 3) && (pArray->dims[2].size == 3) && (colorMode == NDColorModeRGB3)) {
        sizeX = pArray->dims[0].size;
        sizeY = pArray->dims[1].size;
        rowsPerStrip = sizeY;
        samplesPerPixel = 3;
        photoMetric = PHOTOMETRIC_RGB;
        planarConfig = PLANARCONFIG_SEPARATE;
        this->colorMode = NDColorModeRGB3;
    } else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: unsupported array structure\n",
            driverName, functionName);
        return(asynError);
    }

    /* this is in the unallocated 'reusable' range */
    static const int TIFFTAG_NDTIMESTAMP    = 65000;
    static const int TIFFTAG_UNIQUEID       = 65001;
    static const int TIFFTAG_EPICSTSSEC     = 65002;
    static const int TIFFTAG_EPICSTSNSEC    = 65003;
    static const TIFFFieldInfo NDTimeStampFI = {
        TIFFTAG_NDTIMESTAMP,1,1,TIFF_DOUBLE,FIELD_CUSTOM,1,0,(char *)"NDTimeStamp"
    };
    static const TIFFFieldInfo NDUniqueIdFI = {
        TIFFTAG_UNIQUEID,1,1,TIFF_LONG,FIELD_CUSTOM,1,0,(char *)"NDUniqueId"
    };
    static const TIFFFieldInfo EPICSTSSecFI = {
        TIFFTAG_EPICSTSSEC,1,1,TIFF_LONG,FIELD_CUSTOM,1,0,(char *)"EPICSTSSec"
    };
    static const TIFFFieldInfo EPICSTSNsecFI = {
        TIFFTAG_EPICSTSNSEC,1,1,TIFF_LONG,FIELD_CUSTOM,1,0,(char *)"EPICSTSNsec"
    };
    TIFFMergeFieldInfo(output, &NDTimeStampFI, 1);
    TIFFMergeFieldInfo(output, &NDUniqueIdFI, 1);
    TIFFMergeFieldInfo(output, &EPICSTSSecFI, 1);
    TIFFMergeFieldInfo(output, &EPICSTSNsecFI, 1);
    TIFFSetField(this->output, TIFFTAG_NDTIMESTAMP, pArray->timeStamp);
    TIFFSetField(this->output, TIFFTAG_UNIQUEID, pArray->uniqueId);
    TIFFSetField(this->output, TIFFTAG_EPICSTSSEC, pArray->epicsTS.secPastEpoch);
    TIFFSetField(this->output, TIFFTAG_EPICSTSNSEC, pArray->epicsTS.nsec);
    TIFFSetField(this->output, TIFFTAG_BITSPERSAMPLE, bitsPerSample);
    TIFFSetField(this->output, TIFFTAG_SAMPLEFORMAT, sampleFormat);
    TIFFSetField(this->output, TIFFTAG_SAMPLESPERPIXEL, samplesPerPixel);
    TIFFSetField(this->output, TIFFTAG_PHOTOMETRIC, photoMetric);
    TIFFSetField(this->output, TIFFTAG_PLANARCONFIG, planarConfig);
    TIFFSetField(this->output, TIFFTAG_IMAGEWIDTH, (epicsUInt32)sizeX);
    TIFFSetField(this->output, TIFFTAG_IMAGELENGTH, (epicsUInt32)sizeY);
    TIFFSetField(this->output, TIFFTAG_ROWSPERSTRIP, (epicsUInt32)rowsPerStrip);
    
    this->pFileAttributes->clear();
    this->getAttributes(this->pFileAttributes);
    pArray->pAttributeList->copy(this->pFileAttributes);
 
    pAttribute = this->pFileAttributes->find("Model");
    if (pAttribute) {
        pAttribute->getValue(NDAttrString, tagString);
        TIFFSetField(this->output, TIFFTAG_MODEL, tagString);
    } else {
        TIFFSetField(this->output, TIFFTAG_MODEL, "Unknown");
    }
    
    pAttribute = this->pFileAttributes->find("Manufacturer");
    if (pAttribute) {
        pAttribute->getValue(NDAttrString, tagString);
        TIFFSetField(this->output, TIFFTAG_MAKE, tagString);
    } else {
        TIFFSetField(this->output, TIFFTAG_MAKE, "Unknown");
    }

    TIFFSetField(this->output, TIFFTAG_SOFTWARE, "EPICS areaDetector");

    int count = 0;
    int tagId = TIFFTAG_START_;
   
    numAttributes_ = this->pFileAttributes->count();
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s this->pFileAttributes->count(): %d\n",
        driverName, functionName, numAttributes_);

    fieldInfo_ = (TIFFFieldInfo**) malloc(numAttributes_ * sizeof(TIFFFieldInfo *));
    if (fieldInfo_ == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s error, fieldInfo_ malloc failed. file: %s\n",
            driverName, functionName, fileName);
        return asynError;
    }
    for (int i=0; i<numAttributes_; ++i) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s Initializing %d fieldInfo_ entry.\n",
            driverName, functionName, i);
        fieldInfo_[i] = NULL;
    }
    
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s Looping over attributes...\n",
        driverName, functionName);

    pAttribute = this->pFileAttributes->next(NULL);
    while (pAttribute) {
        const char *attributeName = pAttribute->getName();
        //const char *attributeDescription = pAttribute->getDescription();
        const char *attributeSource = pAttribute->getSource();

        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
          "%s:%s : attribute: %s, source: %s\n",
          driverName, functionName, attributeName, attributeSource);

        NDAttrDataType_t attrDataType;
        size_t attrSize;
        NDAttrValue value;
        pAttribute->getValueInfo(&attrDataType, &attrSize);
        memset(tagString, 0, MAX_ATTRIBUTE_STRING_SIZE);

        switch (attrDataType) {
            case NDAttrInt8:
            case NDAttrUInt8:
            case NDAttrInt16:
            case NDAttrUInt16:
            case NDAttrInt32:
            case NDAttrUInt32: {
                pAttribute->getValue(attrDataType, &value.i32);
                epicsSnprintf(tagString, MAX_ATTRIBUTE_STRING_SIZE, "%s:%d", attributeName, value.i32);
                break;
            }
            case NDAttrFloat32: {
                pAttribute->getValue(attrDataType, &value.f32);
                epicsSnprintf(tagString, MAX_ATTRIBUTE_STRING_SIZE, "%s:%f", attributeName, value.f32);
                break;
            }
            case NDAttrFloat64: {
                pAttribute->getValue(attrDataType, &value.f64);
                epicsSnprintf(tagString, MAX_ATTRIBUTE_STRING_SIZE, "%s:%f", attributeName, value.f64);
                break;
            }
            case NDAttrString: {
                memset(attrString, 0, MAX_ATTRIBUTE_STRING_SIZE);
                pAttribute->getValue(attrDataType, attrString, MAX_ATTRIBUTE_STRING_SIZE);
                epicsSnprintf(tagString, MAX_ATTRIBUTE_STRING_SIZE, "%s:%s", attributeName, attrString);
                break;
            }
            case NDAttrUndefined:
                break;
            default:
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                          "%s:%s error, unknown attrDataType=%d\n",
                          driverName, functionName, attrDataType);
                return asynError;
                break;
        }

        if (attrDataType != NDAttrUndefined) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s : tagId: %d, tagString: %s\n",
                  driverName, functionName, tagId, tagString);
            fieldInfo_[count] = (TIFFFieldInfo*) malloc(sizeof(TIFFFieldInfo));
            populateAsciiFieldInfo(fieldInfo_[count], tagId, attributeName);
            TIFFMergeFieldInfo(output, fieldInfo_[count], 1);
            TIFFSetField(this->output, tagId, tagString);
            ++count;
            ++tagId;
            if ((tagId == TIFFTAG_END_) || (count > numAttributes_)) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s error, Too many tags/attributes for file. tagId: %d, count: %d\n",
                    driverName, functionName, tagId, count);
                break;
            }
        }
        pAttribute = this->pFileAttributes->next(pAttribute);
    }
    
    return(asynSuccess);
}

/**
 * Populate a TIFFFieldInfo structure with fields suitable for 
 * writing a ASCII TIFF custom tag.
 * \param[in] fieldInfo Pointer to a TIFFFieldInfo structure.
 * \param[in] fieldTag TIFF tag number to use.
 * \param[in] tagName Pointer to a char array for the tag name.
 */
asynStatus NDFileTIFF::populateAsciiFieldInfo(TIFFFieldInfo *fieldInfo, int fieldTag, const char *tagName)
{
    asynStatus status = asynSuccess;

    if (fieldInfo) {
        fieldInfo->field_tag = fieldTag;
        fieldInfo->field_readcount = 1;
        fieldInfo->field_writecount = 1;
        fieldInfo->field_type = TIFF_ASCII;
        fieldInfo->field_bit = FIELD_CUSTOM;
        fieldInfo->field_oktochange = 1;
        fieldInfo->field_passcount = 0;
        if (tagName) {
            fieldInfo->field_name = (char *)tagName;
        } else {
            status = asynError;
        }
    } else {
        status = asynError;
    }
  
    return status;

}


/** Writes single NDArray to the TIFF file.
  * \param[in] pArray Pointer to the NDArray to be written
  */
asynStatus NDFileTIFF::writeFile(NDArray *pArray)
{
    unsigned long stripSize;
    tsize_t nwrite=0;
    int strip, sizeY;
    unsigned char *pRed, *pGreen, *pBlue;
    static const char *functionName = "writeFile";

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
              "%s:%s: %lu, %lu\n", 
              driverName, functionName, (unsigned long)pArray->dims[0].size, (unsigned long)pArray->dims[1].size);

    if (this->output == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s:%s NULL TIFF file\n",
        driverName, functionName);
        return(asynError);
    }

    stripSize = (unsigned long)TIFFStripSize(this->output);
    TIFFGetField(this->output, TIFFTAG_IMAGELENGTH, &sizeY);

    switch (this->colorMode) {
        case NDColorModeMono:
        case NDColorModeRGB1:
            nwrite = TIFFWriteEncodedStrip(this->output, 0, pArray->pData, stripSize); 
            break;
        case NDColorModeRGB2:
            /* TIFF readers don't support row interleave, put all the red strips first, then all the blue, then green. */
            for (strip=0; strip<sizeY; strip++) {
                pRed   = (unsigned char *)pArray->pData + 3*strip*stripSize;
                pGreen = pRed + stripSize;
                pBlue  = pRed + 2*stripSize;
                nwrite = TIFFWriteEncodedStrip(this->output, strip, pRed, stripSize);
                nwrite = TIFFWriteEncodedStrip(this->output, sizeY+strip, pBlue, stripSize);
                nwrite = TIFFWriteEncodedStrip(this->output, 2*sizeY+strip, pGreen, stripSize);
            }
            break;
        case NDColorModeRGB3:
            for (strip=0; strip<3; strip++) {
                nwrite = TIFFWriteEncodedStrip(this->output, strip, (unsigned char *)pArray->pData+stripSize*strip, stripSize);
            }
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: unknown color mode %d\n",
                driverName, functionName, this->colorMode);
            return(asynError);
            break;
    }
    if (nwrite <= 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: error writing data to file\n",
            driverName, functionName);
        return(asynError);
    }

    return(asynSuccess);
}

/** Reads single NDArray from a TIFF file; NOT CURRENTLY IMPLEMENTED.
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus NDFileTIFF::readFile(NDArray **pArray)
{
    //static const char *functionName = "readFile";

    return asynError;
}


/** Closes the TIFF file. */
asynStatus NDFileTIFF::closeFile()
{
    static const char *functionName = "closeFile";

    if (this->output == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s:%s NULL TIFF file\n",
        driverName, functionName);
        return(asynError);
    }

    TIFFClose(this->output);

    for (int i=0; i<numAttributes_; ++i) {
      free(fieldInfo_[i]);
    }
    free(fieldInfo_);

    return asynSuccess;
}


/** Constructor for NDFileTIFF; all parameters are simply passed to NDPluginFile::NDPluginFile.
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
NDFileTIFF::NDFileTIFF(const char *portName, int queueSize, int blockingCallbacks,
                       const char *NDArrayPort, int NDArrayAddr,
                       int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDFILE_TIFF_PARAMS,
                   2, 0, asynGenericPointerMask, asynGenericPointerMask, 
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    //static const char *functionName = "NDFileTIFF";

    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDFileTIFF");
    this->supportsMultipleArrays = 0;

    this->pAttributeId = NULL;
    this->pFileAttributes = new NDAttributeList;
}

/* Configuration routine.  Called directly, or from the iocsh  */

extern "C" int NDFileTIFFConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
    new NDFileTIFF(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                   priority, stackSize);
    return(asynSuccess);
}


/* EPICS iocsh shell commands */

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
static const iocshFuncDef initFuncDef = {"NDFileTIFFConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileTIFFConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileTIFFRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileTIFFRegister);
}
