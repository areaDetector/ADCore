/* NDFileMagick.cpp
 * Writes NDArrays to any file format supported by ImageMagick.
 *
 * Mark Rivers
 * September 30, 2010
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <epicsStdio.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginFile.h"
#include "NDFileMagick.h"

static const char *driverName = "NDFileMagick";

static CompressionType compressionTypes[] = {NoCompression, BZipCompression, FaxCompression, Group4Compression, 
                                             JPEGCompression, LZWCompression, RLECompression, ZipCompression};

/** Opens a Magick file.
  * \param[in] fileName The name of the file to open.
  * \param[in] openMode Mask defining how the file should be opened; bits are 
  *            NDFileModeRead, NDFileModeWrite, NDFileModeAppend, NDFileModeMultiple
  * \param[in] pArray A pointer to an NDArray; this is used to determine the array and attribute properties.
  */
asynStatus NDFileMagick::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
    static const char *functionName = "openFile";
    NDAttribute *pAttribute;

    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);
    
    strncpy(this->fileName, fileName, sizeof(this->fileName));
    this->colorMode = NDColorModeMono;

    /* We do some special treatment based on colorMode */
    pAttribute = pArray->pAttributeList->find("ColorMode");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &this->colorMode);

    switch (pArray->dataType) {
        case NDInt8:
        case NDUInt8:
            this->storageType = CharPixel;
            break;
        case NDInt16:
        case NDUInt16:
            this->storageType = ShortPixel;
            break;
        case NDInt32:
        case NDUInt32:
            this->storageType = IntegerPixel;
            break;
        case NDFloat32:
            this->storageType = FloatPixel;
            break;
        case NDFloat64:
            this->storageType = DoublePixel;
            break;
    }
    if (pArray->ndims == 2) {
        sizeX = pArray->dims[0].size;
        sizeY = pArray->dims[1].size;
        this->colorMap = "R";
        this->imageType = GrayscaleType;
    } else if ((pArray->ndims == 3) && (pArray->dims[0].size == 3) && (this->colorMode == NDColorModeRGB1)) {
        sizeX = pArray->dims[1].size;
        sizeY = pArray->dims[2].size;
        this->colorMap = "RGB";
        this->imageType = TrueColorType;
    } else if ((pArray->ndims == 3) && (pArray->dims[1].size == 3) && (this->colorMode == NDColorModeRGB2)) {
        sizeX = pArray->dims[0].size;
        sizeY = pArray->dims[2].size;
        this->colorMap = "RGB";
        this->imageType = TrueColorType;
    } else if ((pArray->ndims == 3) && (pArray->dims[2].size == 3) && (this->colorMode == NDColorModeRGB3)) {
        sizeX = pArray->dims[0].size;
        sizeY = pArray->dims[1].size;
        this->colorMap = "RGB";
        this->imageType = TrueColorType;
    } else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: unsupported array structure\n",
            driverName, functionName);
        return(asynError);
    }

    return(asynSuccess);
}

/** Writes single NDArray to the Magick file.
  * \param[in] pArray Pointer to the NDArray to be written
  */
asynStatus NDFileMagick::writeFile(NDArray *pArray)
{
    static const char *functionName = "writeFile";
    Image image;
    int quality;
    int depth;
    int compressIndex;
    CompressionType compressType;

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
              "%s:%s: size=[%lu, %lu]\n", 
              driverName, functionName, (unsigned long)this->sizeX, (unsigned long)this->sizeY);
              
    /* Must lock when accessing parameter library */
    this->lock();
    getIntegerParam(NDFileMagickQuality, &quality);
    getIntegerParam(NDFileMagickBitDepth, &depth);
    getIntegerParam(NDFileMagickCompressType, &compressIndex);
    this->unlock();
    compressType = compressionTypes[compressIndex];

    switch (this->colorMode) {
        case NDColorModeMono:
        case NDColorModeRGB1:
        case NDColorModeBayer:
            image.type(this->imageType);
            image.read((unsigned int)this->sizeX, (unsigned int)this->sizeY, this->colorMap, this->storageType, pArray->pData);
            if (this->colorMode == NDColorModeMono) image.channel(RedChannel);
            image.quality(quality);
            image.depth(depth);
            image.compressType(compressType);
            break;
        case NDColorModeRGB2:
            break;
        case NDColorModeRGB3:
            break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: unknown color mode %d\n",
                driverName, functionName, this->colorMode);
            return(asynError);
            break;
    }
    try {
        image.write(this->fileName);
    }
    catch (exception ex) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
            "%s:%s: error writing data to file\n",
            driverName, functionName);
        return(asynError);
    }

    return(asynSuccess);
}

/** Reads single NDArray from a file; NOT CURRENTLY IMPLEMENTED.
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus NDFileMagick::readFile(NDArray **pArray)
{
    //static const char *functionName = "readFile";

    return asynError;
}


/** Closes the file. */
asynStatus NDFileMagick::closeFile()
{
    //static const char *functionName = "closeFile";

    return asynSuccess;
}


/** Constructor for NDFileMagick; all parameters are simply passed to NDPluginFile::NDPluginFile.
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
NDFileMagick::NDFileMagick(const char *portName, int queueSize, int blockingCallbacks,
                           const char *NDArrayPort, int NDArrayAddr,
                           int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDFILE_MAGICK_PARAMS,
                   2, 0, asynGenericPointerMask, asynGenericPointerMask, 
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    //static const char *functionName = "NDFileMagick";

    /* Set the plugin type string */    
    setStringParam(NDPluginDriverPluginType, "NDFileMagick");
    this->supportsMultipleArrays = 0;
    
    createParam(NDFileMagickQualityString,       asynParamInt32, &NDFileMagickQuality);
    createParam(NDFileMagickCompressTypeString,  asynParamInt32, &NDFileMagickCompressType);
    createParam(NDFileMagickBitDepthString,      asynParamInt32, &NDFileMagickBitDepth);
    
    InitializeMagick(NULL);
}

/* Configuration routine.  Called directly, or from the iocsh  */

extern "C" int NDFileMagickConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
    new NDFileMagick(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
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
static const iocshFuncDef initFuncDef = {"NDFileMagickConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileMagickConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileMagickRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileMagickRegister);
}
