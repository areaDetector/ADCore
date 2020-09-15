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

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsTime.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>

#include "tiffio.h"
#include "NDFileTIFF.h"

#define STRING_BUFFER_SIZE 2048

static const char *driverName = "NDFileTIFF";


static const int TIFFTAG_NDTIMESTAMP     = 65000;
static const int TIFFTAG_UNIQUEID        = 65001;
static const int TIFFTAG_EPICSTSSEC      = 65002;
static const int TIFFTAG_EPICSTSNSEC     = 65003;
static const int TIFFTAG_FIRST_ATTRIBUTE = 65010;
static const int TIFFTAG_LAST_ATTRIBUTE  = 65535;

#define NUM_CUSTOM_TIFF_TAGS (4 + TIFFTAG_LAST_ATTRIBUTE - TIFFTAG_FIRST_ATTRIBUTE + 1)

static TIFFFieldInfo tiffFieldInfo[NUM_CUSTOM_TIFF_TAGS] = {
    {TIFFTAG_NDTIMESTAMP, 1, 1, TIFF_DOUBLE,FIELD_CUSTOM, 1, 0, (char *)"NDTimeStamp"},
    {TIFFTAG_UNIQUEID,    1, 1, TIFF_LONG,FIELD_CUSTOM,   1, 0, (char *)"NDUniqueId"},
    {TIFFTAG_EPICSTSSEC,  1, 1, TIFF_LONG,FIELD_CUSTOM,   1, 0, (char *)"EPICSTSSec"},
    {TIFFTAG_EPICSTSNSEC, 1, 1, TIFF_LONG,FIELD_CUSTOM,   1, 0, (char *)"EPICSTSNsec"}
};

static void registerCustomTIFFTags(TIFF *tif)
{
    /* Install the extended Tag field info */
    TIFFMergeFieldInfo(tif, tiffFieldInfo, sizeof(tiffFieldInfo)/sizeof(tiffFieldInfo[0]));
}

static void augmentLibTiffWithCustomTags() {
    static bool first_time = true;
    if (!first_time) return;
    first_time = false;
    TIFFSetTagExtender(registerCustomTIFFTags);
}


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
    char tagString[STRING_BUFFER_SIZE] = {0};
    char attrString[STRING_BUFFER_SIZE] = {0};
    char tagName[STRING_BUFFER_SIZE] = {0};
    int i;
    TIFFFieldInfo fieldInfo = {0, 1, 1, TIFF_ASCII, FIELD_CUSTOM, 1, 0, tagName};

    for (i=TIFFTAG_FIRST_ATTRIBUTE; i<=TIFFTAG_LAST_ATTRIBUTE; i++) {
        sprintf(tagName, "Attribute_%d", i-TIFFTAG_FIRST_ATTRIBUTE+1);
        fieldInfo.field_tag = i;
        tiffFieldInfo[4+i-TIFFTAG_FIRST_ATTRIBUTE] = fieldInfo;
    }

    augmentLibTiffWithCustomTags();

    /* Suppress error and warning messages from the TIFF library */
    TIFFSetErrorHandler(NULL);
    TIFFSetWarningHandler(NULL);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);

    /* Open for reading */
    else if (openMode & NDFileModeRead) {
        /* Open the file. */
        if ((this->tiff = TIFFOpen(fileName, "rc")) == NULL ) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s error opening file %s\n",
            driverName, functionName, fileName);
            return(asynError);
        }
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s::%s opened file %s\n",
            driverName, functionName, fileName);
    }

    /* Open file for writing */
    else if (openMode & NDFileModeWrite) {
        if ((this->tiff = TIFFOpen(fileName, "w")) == NULL ) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s error opening file %s\n",
            driverName, functionName, fileName);
            return(asynError);
        }
        asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
            "%s::%s opened file %s\n",
            driverName, functionName, fileName);
    }

    // If the file is open for reading we are done
    if (openMode & NDFileModeRead) return asynSuccess;

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
        case NDInt64:
            sampleFormat = SAMPLEFORMAT_INT;
            bitsPerSample = 64;
            break;
        case NDUInt64:
            sampleFormat = SAMPLEFORMAT_UINT;
            bitsPerSample = 64;
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
    if (pArray->ndims == 1) {
        sizeX = pArray->dims[0].size;
        sizeY = 1;
        rowsPerStrip = sizeY;
        samplesPerPixel = 1;
        photoMetric = PHOTOMETRIC_MINISBLACK;
        planarConfig = PLANARCONFIG_CONTIG;
        this->colorMode = NDColorModeMono;
    } else if (pArray->ndims == 2) {
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

    TIFFSetField(this->tiff, TIFFTAG_NDTIMESTAMP, pArray->timeStamp);
    TIFFSetField(this->tiff, TIFFTAG_UNIQUEID, pArray->uniqueId);
    TIFFSetField(this->tiff, TIFFTAG_EPICSTSSEC, pArray->epicsTS.secPastEpoch);
    TIFFSetField(this->tiff, TIFFTAG_EPICSTSNSEC, pArray->epicsTS.nsec);
    TIFFSetField(this->tiff, TIFFTAG_BITSPERSAMPLE, bitsPerSample);
    TIFFSetField(this->tiff, TIFFTAG_SAMPLEFORMAT, sampleFormat);
    TIFFSetField(this->tiff, TIFFTAG_SAMPLESPERPIXEL, samplesPerPixel);
    TIFFSetField(this->tiff, TIFFTAG_PHOTOMETRIC, photoMetric);
    TIFFSetField(this->tiff, TIFFTAG_PLANARCONFIG, planarConfig);
    TIFFSetField(this->tiff, TIFFTAG_IMAGEWIDTH, (epicsUInt32)sizeX);
    TIFFSetField(this->tiff, TIFFTAG_IMAGELENGTH, (epicsUInt32)sizeY);
    TIFFSetField(this->tiff, TIFFTAG_ROWSPERSTRIP, (epicsUInt32)rowsPerStrip);

    this->pFileAttributes->clear();
    this->getAttributes(this->pFileAttributes);
    pArray->pAttributeList->copy(this->pFileAttributes);

    pAttribute = this->pFileAttributes->find("Model");
    if (pAttribute) {
        pAttribute->getValue(NDAttrString, tagString, sizeof(tagString)-1);
        TIFFSetField(this->tiff, TIFFTAG_MODEL, tagString);
    } else {
        TIFFSetField(this->tiff, TIFFTAG_MODEL, "Unknown");
    }

    pAttribute = this->pFileAttributes->find("Manufacturer");
    if (pAttribute) {
        pAttribute->getValue(NDAttrString, tagString);
        TIFFSetField(this->tiff, TIFFTAG_MAKE, tagString, sizeof(tagString)-1);
    } else {
        TIFFSetField(this->tiff, TIFFTAG_MAKE, "Unknown");
    }

    TIFFSetField(this->tiff, TIFFTAG_SOFTWARE, "EPICS areaDetector");

    // If the attribute TIFFImageDescription exists use it to set the TIFFTAG_IMAGEDESCRIPTION
    pAttribute = this->pFileAttributes->find("TIFFImageDescription");
    if (pAttribute) {
        pAttribute->getValue(NDAttrString, tagString, sizeof(tagString)-1);
        TIFFSetField(this->tiff, TIFFTAG_IMAGEDESCRIPTION, tagString);
    }

    int count = 0;
    int tagId = TIFFTAG_FIRST_ATTRIBUTE;

    numAttributes_ = this->pFileAttributes->count();
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
        "%s:%s this->pFileAttributes->count(): %d\n",
        driverName, functionName, numAttributes_);

    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
        "%s:%s Looping over attributes...\n",
        driverName, functionName);

    pAttribute = this->pFileAttributes->next(NULL);
    while (pAttribute) {
        const char *attributeName = pAttribute->getName();
        //const char *attributeDescription = pAttribute->getDescription();
        const char *attributeSource = pAttribute->getSource();

        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
          "%s:%s : attribute: %s, source: %s\n",
          driverName, functionName, attributeName, attributeSource);

        NDAttrDataType_t attrDataType;
        size_t attrSize;
        NDAttrValue value;
        pAttribute->getValueInfo(&attrDataType, &attrSize);
        memset(tagString, 0, sizeof(tagString));

        switch (attrDataType) {
            case NDAttrInt8:
            case NDAttrUInt8:
            case NDAttrInt16:
            case NDAttrUInt16:
            case NDAttrInt32:
            case NDAttrUInt32:
            case NDAttrInt64:
            case NDAttrUInt64: {
                pAttribute->getValue(attrDataType, &value.i64);
                epicsSnprintf(tagString, sizeof(tagString)-1, "%s:%lld", attributeName, value.i64);
                break;
            }
            case NDAttrFloat32: {
                pAttribute->getValue(attrDataType, &value.f32);
                epicsSnprintf(tagString, sizeof(tagString)-1, "%s:%f", attributeName, value.f32);
                break;
            }
            case NDAttrFloat64: {
                pAttribute->getValue(attrDataType, &value.f64);
                epicsSnprintf(tagString, sizeof(tagString)-1, "%s:%f", attributeName, value.f64);
                break;
            }
            case NDAttrString: {
                memset(attrString, 0, sizeof(tagString)-1);
                pAttribute->getValue(attrDataType, attrString, sizeof(attrString)-1);
                epicsSnprintf(tagString, sizeof(tagString)-1, "%s:%s", attributeName, attrString);
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
            asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
                "%s:%s : tagId: %d, tagString: %s\n",
                  driverName, functionName, tagId, tagString);
            TIFFSetField(this->tiff, tagId, tagString);
            ++count;
            ++tagId;
            if ((tagId > TIFFTAG_LAST_ATTRIBUTE) || (count > numAttributes_)) {
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
              "%s:%s: writing file dimensions=[%lu, %lu]\n",
              driverName, functionName, (unsigned long)pArray->dims[0].size, (unsigned long)pArray->dims[1].size);

    if (this->tiff == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s NULL TIFF file\n",
        driverName, functionName);
        return(asynError);
    }

    stripSize = (unsigned long)TIFFStripSize(this->tiff);
    TIFFGetField(this->tiff, TIFFTAG_IMAGELENGTH, &sizeY);

    switch (this->colorMode) {
        case NDColorModeMono:
        case NDColorModeRGB1:
            nwrite = TIFFWriteEncodedStrip(this->tiff, 0, pArray->pData, stripSize);
            break;
        case NDColorModeRGB2:
            /* TIFF readers don't support row interleave, put all the red strips first, then all the blue, then green. */
            for (strip=0; strip<sizeY; strip++) {
                pRed   = (unsigned char *)pArray->pData + 3*strip*stripSize;
                pGreen = pRed + stripSize;
                pBlue  = pRed + 2*stripSize;
                nwrite = TIFFWriteEncodedStrip(this->tiff, strip, pRed, stripSize);
                nwrite = TIFFWriteEncodedStrip(this->tiff, sizeY+strip, pGreen, stripSize);
                nwrite = TIFFWriteEncodedStrip(this->tiff, 2*sizeY+strip, pBlue, stripSize);
            }
            break;
        case NDColorModeRGB3:
            for (strip=0; strip<3; strip++) {
                nwrite = TIFFWriteEncodedStrip(this->tiff, strip, (unsigned char *)pArray->pData+stripSize*strip, stripSize);
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

/** Reads single NDArray from a TIFF file;
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus NDFileTIFF::readFile(NDArray **pArray)
{
    epicsInt16 bitsPerSample, sampleFormat, samplesPerPixel, photoMetric, planarConfig;
    epicsInt32 sizeX, sizeY, rowsPerStrip;
    size_t totalSize=0;
    int strip, numStrips;
    NDDataType_t dataType;
    int ndims;
    int size;
    size_t dims[3];
    NDArray *pImage;
    epicsFloat64 tempDouble;
    epicsInt32 tempLong;
    char *tempString;
    char *buffer;
    int fieldStat;
    epicsInt32 clrMode;
    asynStatus status = asynSuccess;
    static const char *functionName = "readFile";


    TIFFGetField(this->tiff, TIFFTAG_BITSPERSAMPLE,    &bitsPerSample);
    TIFFGetField(this->tiff, TIFFTAG_SAMPLEFORMAT,     &sampleFormat);
    TIFFGetField(this->tiff, TIFFTAG_SAMPLESPERPIXEL,  &samplesPerPixel);
    TIFFGetField(this->tiff, TIFFTAG_PHOTOMETRIC,      &photoMetric);
    TIFFGetField(this->tiff, TIFFTAG_PLANARCONFIG,     &planarConfig);
    TIFFGetField(this->tiff, TIFFTAG_IMAGEWIDTH,       &sizeX);
    TIFFGetField(this->tiff, TIFFTAG_IMAGELENGTH,      &sizeY);
    TIFFGetField(this->tiff, TIFFTAG_ROWSPERSTRIP,     &rowsPerStrip);
    numStrips= TIFFNumberOfStrips(this->tiff);

    if (0 == sampleFormat)
    {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s Sample format is not defined! Default UINT is used.\n",
            driverName, __FUNCTION__);
        sampleFormat = SAMPLEFORMAT_UINT;
    }

    if      ((bitsPerSample == 8)  && (sampleFormat == SAMPLEFORMAT_INT))     dataType = NDInt8;
    else if ((bitsPerSample == 8)  && (sampleFormat == SAMPLEFORMAT_UINT))    dataType = NDUInt8;
    else if ((bitsPerSample == 16) && (sampleFormat == SAMPLEFORMAT_INT))     dataType = NDInt16;
    else if ((bitsPerSample == 16) && (sampleFormat == SAMPLEFORMAT_UINT))    dataType = NDUInt16;
    else if ((bitsPerSample == 32) && (sampleFormat == SAMPLEFORMAT_INT))     dataType = NDInt32;
    else if ((bitsPerSample == 32) && (sampleFormat == SAMPLEFORMAT_UINT))    dataType = NDUInt32;
    else if ((bitsPerSample == 64) && (sampleFormat == SAMPLEFORMAT_INT))     dataType = NDInt64;
    else if ((bitsPerSample == 64) && (sampleFormat == SAMPLEFORMAT_UINT))    dataType = NDUInt64;
    else if ((bitsPerSample == 32) && (sampleFormat == SAMPLEFORMAT_IEEEFP))  dataType = NDFloat32;
    else if ((bitsPerSample == 64) && (sampleFormat == SAMPLEFORMAT_IEEEFP))  dataType = NDFloat64;
    else {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s unsupport bitsPerSample=%d and sampleFormat=%d\n",
            driverName, functionName, bitsPerSample, sampleFormat);
        return asynError;
    }

    if ((photoMetric == PHOTOMETRIC_MINISBLACK)  &&
        (planarConfig == PLANARCONFIG_CONTIG) &&
        (samplesPerPixel == 1)) {
        ndims = 2;
        dims[0] = sizeX;
        dims[1] = sizeY;
        clrMode = NDColorModeMono;
    }
    else if ((photoMetric == PHOTOMETRIC_RGB) &&
        (planarConfig == PLANARCONFIG_CONTIG)   &&
        (samplesPerPixel == 3)) {
        ndims = 3;
        dims[0] = 3;
        dims[1] = sizeX;
        dims[2] = sizeY;
        clrMode = NDColorModeRGB1;
    }
    else if ((photoMetric == PHOTOMETRIC_RGB) &&
        (planarConfig == PLANARCONFIG_SEPARATE)   &&
        (samplesPerPixel == 3)) {
        ndims = 3;
        dims[0] = sizeX;
        dims[1] = sizeY;
        dims[2] = 3;
        clrMode = NDColorModeRGB3;
    }
    else {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s unsupport photoMetric=%d, planarConfig=%d, and samplesPerPixel=%d\n",
            driverName, functionName, photoMetric, planarConfig, samplesPerPixel);
        return asynError;
    }

    pImage = this->pNDArrayPool->alloc(ndims, dims, dataType, 0, 0);
    *pArray = pImage;
    buffer = (char *)pImage->pData;
    for (strip=0; strip < numStrips; strip++) {
        size = (int)TIFFReadEncodedStrip(this->tiff, strip, buffer, pImage->dataSize-totalSize);
        if (size == -1) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s::%s, error reading TIFF file\n",
                driverName, functionName);
            status = asynError;
            break;
        }
        buffer += size;
        totalSize += size;
        if (totalSize > pImage->dataSize) {
            status = asynError;
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s, file size too large =%lu, must be <= %lu\n",
                driverName, functionName, (unsigned long)totalSize, (unsigned long)pImage->dataSize);
            status = asynError;
            break;
        }
    }

    // Get the attribute list
    this->getAttributes(pImage->pAttributeList);

    // Set the ColorMode attribute
    pImage->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32, &clrMode);

    // If the TIFF file contains the standard NDArray attributes then read them
    fieldStat = TIFFGetField(this->tiff, TIFFTAG_NDTIMESTAMP, &tempDouble);
    if (fieldStat == 1) pImage->timeStamp = tempDouble;
    fieldStat = TIFFGetField(this->tiff, TIFFTAG_UNIQUEID, &tempLong);
    if (fieldStat == 1) pImage->uniqueId = tempLong;
    fieldStat = TIFFGetField(this->tiff, TIFFTAG_EPICSTSSEC, &tempLong);
    if (fieldStat == 1) pImage->epicsTS.secPastEpoch = tempLong;
    fieldStat = TIFFGetField(this->tiff, TIFFTAG_EPICSTSNSEC, &tempLong);
    if (fieldStat == 1) pImage->epicsTS.nsec = tempLong;

    for (int i=TIFFTAG_FIRST_ATTRIBUTE; i<=TIFFTAG_LAST_ATTRIBUTE; i++) {
        fieldStat = TIFFGetField(this->tiff, i, &tempString);
        if (fieldStat == 1) {
            std::string ts = tempString;
            int pc = (int)ts.find(':');
            std::string attrName = ts.substr(0, pc);
            std::string attrValue = ts.substr(pc+1);
            // Don't process ColorMode attribute from the attributes in the TIFF file, already done above.
            if (attrName == "ColorMode") continue;
            pImage->pAttributeList->add(attrName.c_str(), attrName.c_str(), NDAttrString, (void *)attrValue.c_str());
        }
    }
    return status;
}


/** Closes the TIFF file. */
asynStatus NDFileTIFF::closeFile()
{
    static const char *functionName = "closeFile";

    if (this->tiff == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s NULL TIFF file\n",
        driverName, functionName);
        return(asynError);
    }

    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s::%s closing file\n",
        driverName, functionName);
    TIFFClose(this->tiff);

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
                   NDArrayPort, NDArrayAddr, 1,
                   2, 0, asynGenericPointerMask, asynGenericPointerMask,
                   ASYN_CANBLOCK, 1, priority, stackSize, 1),
    numAttributes_(0)
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
    // Stack size must be a minimum of 40000 on vxWorks because of automatic variables in NDFileTIFF::openFile()
    #ifdef vxWorks
        if (stackSize < 40000) stackSize = 40000;
    #endif
    NDFileTIFF *pPlugin = new NDFileTIFF(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                         priority, stackSize);
    return pPlugin->start();
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
