/* NDFileTIFFS3.cpp
 * Writes NDArrays to TIFF files on S3 Storage
 *
 * Stuart B. Wilkins 
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

#include <iostream>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/BucketLocationConstraint.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/utils/UUID.h>
#include <aws/core/utils/StringUtils.h>

#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/logging/AWSLogging.h>

#include <tiffio.h>
#include <tiffio.hxx>

#include "NDPluginFile.h"
#include "NDFileTIFF.h"
#include "NDFileTIFFS3.h"

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
asynStatus NDFileTIFFS3::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
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
    if (openMode != NDFileModeWrite) { 
        return(asynError);
    }

    // Now do AMAZON S3 Stuff
    awsStream = Aws::MakeShared<Aws::StringStream>("");
    strncpy(keyName, fileName, 255);

    if ((this->tiff = TIFFStreamOpen("TIFF", (std::ostream*)awsStream.get())) == NULL)
    {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s:%s error opening file %s\n",
                  driverName, functionName, fileName);
        return (asynError);
    }
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
              "%s::%s opened file %s\n",
              driverName, functionName, fileName);

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

void NDFileTIFFS3::PutObjectAsyncFinished(const Aws::S3::S3Client* s3Client, 
    const Aws::S3::Model::PutObjectRequest& request, 
    const Aws::S3::Model::PutObjectOutcome& outcome,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
    std::shared_ptr<const Aws::Client::AsyncCallerContext> __ctx(context);
    std::shared_ptr<const NDFileTIFFS3_AWSContext> _ctx = 
        std::dynamic_pointer_cast<const NDFileTIFFS3_AWSContext>(__ctx);
    std::shared_ptr<NDFileTIFFS3_AWSContext> ctx = std::const_pointer_cast<NDFileTIFFS3_AWSContext>(_ctx);

    ctx->GetTIFFS3()->putObjectFinished(outcome);
}

void NDFileTIFFS3::putObjectFinished(const Aws::S3::Model::PutObjectOutcome& outcome)
{
    const char* functionName = "NDFileTIFFS3::putObjectFinished";
    if (!outcome.IsSuccess()) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s:%s error, AWS S3 Error : %s\n",
                  driverName, functionName, outcome.GetError().GetMessage().c_str());
    }
    this->callParamCallbacks();
}


asynStatus NDFileTIFFS3::closeFile()
{
    NDFileTIFF::closeFile();

    Aws::S3::Model::PutObjectRequest objectRequest;
    objectRequest.SetBucket("");
    objectRequest.SetKey(keyName);
    objectRequest.SetBody(awsStream);

    std::shared_ptr<NDFileTIFFS3_AWSContext> context =
        Aws::MakeShared<NDFileTIFFS3_AWSContext>("PutObjectAllocationTag");
    context->SetUUID(keyName);
    context->SetTIFFS3(this);
    s3Client->PutObjectAsync(objectRequest, NDFileTIFFS3::PutObjectAsyncFinished, context);

    return asynSuccess;
}

/** Constructor for NDFileTIFFS3; all parameters are simply passed to NDPluginFile::NDPluginFile.
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
NDFileTIFFS3::NDFileTIFFS3(const char *portName, int queueSize, int blockingCallbacks,
                       const char *NDArrayPort, int NDArrayAddr,
                       const char *endpoint, int awslog, int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDFileTIFF(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, priority, stackSize)
{
    //static const char *functionName = "NDFileTIFF";

    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDFileTIFFS3");
    this->supportsMultipleArrays = 0;

    if (awslog) 
    {
        Aws::Utils::Logging::LogLevel level;
        switch (awslog)
        {
            case 0:
                level = Aws::Utils::Logging::LogLevel::Off;
                break;
            case 1:
                level = Aws::Utils::Logging::LogLevel::Fatal;
                break;
            case 2:
                level = Aws::Utils::Logging::LogLevel::Error;
                break;
            case 3:
                level = Aws::Utils::Logging::LogLevel::Warn;
                break;
            case 4:
                level = Aws::Utils::Logging::LogLevel::Info;
                break;
            case 5:
                level = Aws::Utils::Logging::LogLevel::Debug;
                break;
            case 6:
                level = Aws::Utils::Logging::LogLevel::Trace;
                break;
            default:
                level = Aws::Utils::Logging::LogLevel::Off;
                break;
        }
        Aws::Utils::Logging::InitializeAWSLogging(
            Aws::MakeShared<Aws::Utils::Logging::DefaultLogSystem>(
            "NDFileTIFFS3", level, "NDFileTIFFS3_"));
        awsLogging = true;
    } else {
        awsLogging = false;
    }

    Aws::InitAPI(options);

    Aws::Client::ClientConfiguration config;
    if (endpoint && endpoint[0]) 
    {
        config.endpointOverride = endpoint;
    }
    config.verifySSL = false;

    s3Client = std::make_shared<Aws::S3::S3Client>(config);
}

NDFileTIFFS3::~NDFileTIFFS3() 
{
    ShutdownAPI(options);
    if (awsLogging)
    {
        Aws::Utils::Logging::ShutdownAWSLogging();
    }
}

/* Configuration routine.  Called directly, or from the iocsh  */

extern "C" int NDFileTIFFS3Configure(const char *portName, int queueSize, int blockingCallbacks,
                                     const char *NDArrayPort, int NDArrayAddr,
                                     const char *endpoint, int awslog, int priority, int stackSize)
{
    // Stack size must be a minimum of 40000 on vxWorks because of automatic variables in NDFileTIFF::openFile()
    #ifdef vxWorks
        if (stackSize < 40000) stackSize = 40000;
    #endif
    NDFileTIFFS3 *pPlugin = new NDFileTIFFS3(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                             endpoint, awslog, priority, stackSize);
    return pPlugin->start();
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "endpoint",iocshArgString};
static const iocshArg initArg6 = { "awslog",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"NDFileTIFFS3Configure",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileTIFFS3Configure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, 
                          args[4].ival, args[5].sval, args[6].ival, args[7].ival,
                          args[8].ival);
}

extern "C" void NDFileTIFFS3Register(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileTIFFS3Register);
}
