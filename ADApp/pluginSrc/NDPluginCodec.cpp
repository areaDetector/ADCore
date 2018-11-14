/*
 * NDPluginCodec.cpp
 * 
 * Plugin to compress/decompress NDArrays
 *
 * Author: Bruno Martins
 *
 * Created June 18, 2018
 *
 *
 * Compressed data semantics
 * -------------------------
 *
 * The new NDArray's field `codec` is used to indicate if an NDArray holds
 * compressed or uncompressed data.
 *
 * Uncompressed NDArrays:
 *
 *  - `codec` is empty (`codec.empty()==true`).
 *
 *  - `compressedSize` is equal to `dataSize`.
 *
 * Compressed NDArrays:
 *
 *  - `codec` holds the name of the codec that was used to compress the data.
 *    This plugin currently supports two codecs: "jpeg" and "blosc".
 *
 *  - `compressedSize` holds the length of the compressed data in `pData`, in
 *    bytes.
 *
 *  - `dataSize` holds the length of the allocated `pData` buffer, as usual.
 *
 *  - `pData` holds the compressed data as `unsigned char`.
 *
 *  - `dataType` holds the data type of the *uncompressed* data. This will be
 *    used for decompression.
 */

#include <string>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsTime.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginCodec.h"

#define JPEG_MIN_QUALITY 1
#define JPEG_MAX_QUALITY 100

using std::string;

static const char *driverName="NDPluginCodec";

static string codecName[] = {"", "jpeg", "blosc"};

/* Allocate a new NDArray to hold [un]compressed data.
 * Since there's no way to know the final size of the compressed data, always
 * allocate an array of the same size as the uncompressed one.
 */
static NDArray *alloc(NDArrayPool *pool, NDArray *input, int dataType = -1)
{
    NDDataType_t dt;

    if (dataType == -1)
        dt = input->dataType;
    else
        dt = static_cast<NDDataType_t>(dataType);

    size_t dims[ND_ARRAY_MAX_DIMS];

    for (int i = 0; i < input->ndims; ++i)
        dims[i] = input->dims[i].size;

    return pool->alloc(input->ndims, dims, dt, 0, NULL);
}

static int jpeg_clamp_quality(int quality)
{
    if (quality < JPEG_MIN_QUALITY)
        return JPEG_MIN_QUALITY;

    if (quality > JPEG_MAX_QUALITY)
        return JPEG_MAX_QUALITY;

    return quality;
}

#ifdef HAVE_JPEG

#ifdef __cplusplus
    // Force C interface for jpeg functions
    extern "C" {
#endif
        #include "jpeglib.h"
#ifdef __cplusplus
    }
#endif

NDArray *compressJPEG(NDArrayPool *pool, NDArray *input, int quality)
{
    struct jpeg_compress_struct jpegInfo;
    struct jpeg_error_mgr jpegErr;

    jpeg_create_compress(&jpegInfo);
    jpegInfo.err = jpeg_std_error(&jpegErr);

    int colorMode = NDColorModeMono;
    NDAttribute *pAttribute = input->pAttributeList->find("ColorMode");

    if (pAttribute)
        pAttribute->getValue(NDAttrInt32, &colorMode);

    NDArray *output = alloc(pool, input);

    if (!output) {
        fprintf(stderr, "%s: failed to allocate array\n", __func__);
        return NULL;
    }

    JSAMPROW row_pointer[1];
    int nwrite=0;
    unsigned char *pRed=NULL, *pGreen=NULL, *pBlue=NULL, *pData=NULL, *pOut,
                  *buffer=NULL;
    int sizeX, sizeY;
    int stepSize=0, i;

    switch (input->dataType) {
        case NDInt8:
        case NDUInt8:
            break;
        default:
            fprintf(stderr, "%s: only 8-bit data is supported\n", __func__);
            goto failure;
    }

    // We do some special treatment based on colorMode
    if (input->ndims == 2) {
        jpegInfo.image_width  = (JDIMENSION)input->dims[0].size;
        jpegInfo.image_height = (JDIMENSION)input->dims[1].size;
        jpegInfo.input_components = 1;
        jpegInfo.in_color_space = JCS_GRAYSCALE;
    } else if (input->ndims == 3) {
        jpegInfo.input_components = 3;
        jpegInfo.in_color_space = JCS_RGB;

        if (input->dims[0].size == 3 && colorMode == NDColorModeRGB1) {
            jpegInfo.image_width  = (JDIMENSION)input->dims[1].size;
            jpegInfo.image_height = (JDIMENSION)input->dims[2].size;
        } else if (input->dims[1].size == 3 && colorMode == NDColorModeRGB2) {
            jpegInfo.image_width  = (JDIMENSION)input->dims[0].size;
            jpegInfo.image_height = (JDIMENSION)input->dims[2].size;
        } else if (input->dims[2].size == 3 && colorMode == NDColorModeRGB3) {
            jpegInfo.image_width  = (JDIMENSION)input->dims[0].size;
            jpegInfo.image_height = (JDIMENSION)input->dims[1].size;
        }
    } else {
        fprintf(stderr, "%s: unsupported array structure\n", __func__);
        goto failure;
    }

    jpeg_set_defaults(&jpegInfo);
    jpeg_set_quality(&jpegInfo, jpeg_clamp_quality(quality), TRUE);
    jpeg_mem_dest(&jpegInfo,
                  (unsigned char **)&output->pData,
                  (unsigned long*) &output->compressedSize);
    jpeg_start_compress(&jpegInfo, TRUE);

    sizeX = (int)jpegInfo.image_width;
    sizeY = (int)jpegInfo.image_height;

    switch (colorMode) {
        case NDColorModeMono:
        case NDColorModeRGB1:
            pData = (unsigned char *)input->pData;
            break;
        case NDColorModeRGB2:
            buffer = (unsigned char *)malloc(sizeX * sizeY * 3);
            stepSize = sizeX * 3;
            pRed = (unsigned char *)input->pData;
            pGreen = pRed + sizeX;
            pBlue = pGreen + sizeX;
            break;
        case NDColorModeRGB3:
            buffer = (unsigned char *)malloc(sizeX * sizeY * 3);
            stepSize = sizeX;
            pRed = (unsigned char *)input->pData;
            pGreen = pRed + sizeX * sizeY;
            pBlue = pGreen + sizeX * sizeY;
            break;
        default:
            fprintf(stderr, "%s: unknown color mode %d\n", __func__, colorMode);
            goto failure;
    }

    while ((int)jpegInfo.next_scanline < sizeY) {
        switch (colorMode) {
            case NDColorModeMono:
            case NDColorModeRGB1:
                row_pointer[0] = pData;
                nwrite = jpeg_write_scanlines(&jpegInfo, row_pointer, 1);
                pData += sizeX * jpegInfo.input_components;
                break;
            case NDColorModeRGB2:
            case NDColorModeRGB3:
                row_pointer[0] = buffer;
                pOut = buffer;
                for (i=0; i<sizeX; i++) {
                    *pOut++ = pRed[i];
                    *pOut++ = pGreen[i];
                    *pOut++ = pBlue[i];
                }
                nwrite = jpeg_write_scanlines(&jpegInfo, row_pointer, 1);
                pRed += stepSize;
                pBlue += stepSize;
                pGreen += stepSize;
                break;
            default:
                fprintf(stderr, "%s: unknown color mode %d\n", __func__,
                        colorMode);
                goto failure;
        }

        if (nwrite != 1) {
            fprintf(stderr, "%s: error writing data\n", __func__);
            goto failure;
        }
    }

    if (buffer)
        free(buffer);

    jpeg_finish_compress(&jpegInfo);

    output->codec = codecName[NDCODEC_JPEG];

    return output;

failure:
    if (buffer)
        free(buffer);

    output->release();

    return NULL;
}

/*
 * The result of a decompressed JPEG will be either:
 *   - 8-bit mono
 *   - 8-bit RGB1
 */
NDArray *decompressJPEG(NDArrayPool *pool, NDArray *input)
{
    // Sanity check
    if (input->codec != codecName[NDCODEC_JPEG]) {
        fprintf(stderr, "%s: invalid codec '%s', expected '%s'\n", __func__,
                input->codec.c_str(), codecName[NDCODEC_JPEG].c_str());
        return NULL;
    }

    NDArray *output = alloc(pool, input, NDUInt8);

    if (!output) {
        fprintf(stderr, "%s: failed to allocate output array\n", __func__);
        return NULL;
    }

    struct jpeg_decompress_struct jpegInfo;
    struct jpeg_error_mgr jpegErr;

    jpeg_create_decompress(&jpegInfo);
    jpegInfo.err = jpeg_std_error(&jpegErr);

    jpeg_mem_src(&jpegInfo, static_cast<unsigned char*>(input->pData),
            static_cast<unsigned long>(input->compressedSize));
    jpeg_read_header(&jpegInfo, TRUE);
    jpeg_start_decompress(&jpegInfo);

    unsigned char *dest = static_cast<unsigned char*>(output->pData);

    while (jpegInfo.output_scanline < jpegInfo.output_height) {
        unsigned char *row_pointer[1] = { dest };

        if (jpeg_read_scanlines(&jpegInfo, row_pointer, 1) != 1) {
            fprintf(stderr, "%s: error decoding JPEG\n", __func__);
            break;
        }

        dest += jpegInfo.output_width*jpegInfo.output_components;
    }

    jpeg_finish_decompress(&jpegInfo);

    int oc = jpegInfo.output_components;
    int colorMode = oc == 1 ? NDColorModeMono : NDColorModeRGB1;

    output->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32,
            &colorMode);
    output->codec = codecName[NDCODEC_NONE];

    return output;
}

#else

NDArray *compressJPEG(NDArrayPool*, NDArray*, int)
{
    fprintf(stderr, "No JPEG support");
    return NULL;
}

NDArray *decompressJPEG(NDArrayPool*, NDArray*)
{
    fprintf(stderr, "No JPEG support");
    return NULL;
}

#endif // ifdef HAVE_JPEG

#ifdef HAVE_BLOSC
#include <blosc.h>

static const char* bloscCompName[] = {
    "blosclz",
    "lz4",
    "lz4hc",
    "snappy",
    "zlib",
    "zstd",
};

NDArray *compressBlosc(NDArrayPool *pool, NDArray *input, int clevel,
        bool shuffle, NDCodecBloscComp_t compressor, int numThreads)
{
    if (!input->codec.empty()) {
        fprintf(stderr, "%s: input already compressed\n", __func__);
        return NULL;
    }

    const char *compname = bloscCompName[compressor];

    if (blosc_compname_to_compcode(compname) < 0) {
        fprintf(stderr, "%s: unsupported compressor %s\n", __func__, compname);
        return NULL;
    }

    NDArray *output = alloc(pool, input);

    if (!output) {
        fprintf(stderr, "%s: failed to allocate output array\n", __func__);
        return NULL;
    }

    NDArrayInfo_t info;
    input->getInfo(&info);

    int compSize = blosc_compress_ctx(clevel, shuffle, info.bytesPerElement,
            info.totalBytes, input->pData, output->pData, output->dataSize,
            compname, 0, numThreads);

    if (compSize <= 0) {
        output->release();
        fprintf(stderr, "%s: failed to compress\n", __func__);
        return NULL;
    }

    output->codec = codecName[NDCODEC_BLOSC];
    output->compressedSize = compSize;

    return output;
}

NDArray *decompressBlosc(NDArrayPool *pool, NDArray *input, int numThreads)
{
    // Sanity check
    if (input->codec != codecName[NDCODEC_BLOSC]) {
        fprintf(stderr, "%s: invalid codec '%s', expected '%s'\n", __func__,
                input->codec.c_str(), codecName[NDCODEC_BLOSC].c_str());
        return NULL;
    }

    NDArray *output = alloc(pool, input);

    if (!output) {
        fprintf(stderr, "%s: failed to allocate output array\n", __func__);
        return NULL;
    }

    int ret = blosc_decompress_ctx(input->pData, output->pData,
            output->dataSize, numThreads);

    if (ret <= 0){
        output->release();
        fprintf(stderr, "%s: failed to decompress\n", __func__);
        return NULL;
    }

    output->codec = codecName[NDCODEC_NONE];

    return output;
}

#else

NDArray *compressBlosc(NDArrayPool*, NDArray*, int , bool , NDCodecBloscComp_t,
        int)
{
    fprintf(stderr, "No Blosc support");
    return NULL;
}

NDArray *decompressBlosc(NDArrayPool*, NDArray*, int)
{
    fprintf(stderr, "No Blosc support");
    return NULL;
}

#endif // ifdef HAVE_BLOSC


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Looks for the NDArray attribute called "ColorMode" to determine the color
  * mode of the input array.  Uses the parameter NDPluginCodecColorModeOut
  * to determine the desired color mode of the output array.  The NDArray is converted
  * between these color modes if possible.  If not the input array is passed on without
  * being changed.  Does callbacks to all registered clients on the asynGenericPointer
  * interface with the output array.
  * \param[in] pArray  The NDArray from the callback.
  */ 
void NDPluginCodec::processCallbacks(NDArray *pArray)
{
    /* This function converts the color mode.
     * If no conversion can be performed it simply uses the input as the output
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */

    static const char* functionName = "processCallbacks";
         
    /* Call the base class method */
    NDPluginDriver::beginProcessCallbacks(pArray);

    NDArray *result;
    NDArrayPool *pool = pNDArrayPool;

    int mode, algo;
    getIntegerParam(NDCodecMode, &mode);
    getIntegerParam(NDCodecCompressor, &algo);

    if (algo && mode == NDCODEC_COMPRESS && !pArray->codec.empty()) {
        fprintf(stderr, "%s:%s: array already compressed\n", driverName,
                functionName);
        return;
    }

    double factor = 0.0;

    if (mode == NDCODEC_COMPRESS) {
        switch(algo) {
        case NDCODEC_NONE:
        default:
            result = pArray;
            break;

        case NDCODEC_JPEG: {
            int quality;
            getIntegerParam(NDCodecJPEGQuality, &quality);

            unlock();
            result = compressJPEG(pool, pArray, quality);
            lock();
            break;
        }

        case NDCODEC_BLOSC: {
            int numThreads, clevel, shuffle, compressor;

            getIntegerParam(NDCodecBloscNumThreads, &numThreads);
            getIntegerParam(NDCodecBloscCLevel, &clevel);
            getIntegerParam(NDCodecBloscShuffle, &shuffle);
            getIntegerParam(NDCodecBloscCompressor, &compressor);

            unlock();
            result = compressBlosc(pool, pArray, clevel, shuffle ? true : false,
                    static_cast<NDCodecBloscComp_t>(compressor), numThreads);
            lock();
            break;
        }
        }

        if (result && result != pArray) {
            NDArrayInfo_t info;
            pArray->getInfo(&info);
            factor = 1.0 - (1.0*result->compressedSize) / (1.0*info.totalBytes);
            factor *= 100.0;
        }
    } else {
        if (pArray->codec.empty()) {
            result = pArray;
        } else if (pArray->codec == codecName[NDCODEC_JPEG]) {
            unlock();
            result = decompressJPEG(pool, pArray);
            lock();
        } else if (pArray->codec == codecName[NDCODEC_BLOSC]) {
            int numThreads;
            getIntegerParam(NDCodecBloscNumThreads, &numThreads);

            unlock();
            result = decompressBlosc(pool, pArray, numThreads);
            lock();
        } else {
            fprintf(stderr, "%s: unexpected codec: '%s'", __func__,
                    pArray->codec.c_str());
            result = NULL;
        }
    }

    if (result)
        NDPluginDriver::endProcessCallbacks(result, result == pArray, true);

    setDoubleParam(NDCodecCompFactor, factor);
    callParamCallbacks();
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including NDPluginDriverEnableCallbacks and
  * NDPluginDriverArrayAddr.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus NDPluginCodec::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char* functionName = "writeInt32";

    if (function == NDCodecJPEGQuality) {
        value = jpeg_clamp_quality(value);
    } else if (function == NDCodecBloscCompressor) {
        if (value < NDCODEC_BLOSC_BLOSCLZ)
            value = NDCODEC_BLOSC_BLOSCLZ;
        else if(value > NDCODEC_BLOSC_ZSTD)
            value = NDCODEC_BLOSC_ZSTD;
    } else if (function == NDCodecBloscCLevel) {
        if (value < 0)
            value = 0;
    } else if (function == NDCodecBloscShuffle) {
        value = !!value;
    } else if (function == NDCodecBloscNumThreads) {
        if (value < 1)
            value = 1;
    } else if (function < FIRST_NDCODEC_PARAM) {
        status = NDPluginDriver::writeInt32(pasynUser, value);
    }

    /* Set the parameter in the parameter library. */
    status = (asynStatus) setIntegerParam(function, value);

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    const char* paramName;
    if (status) {
        getParamName( function, &paramName );
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: function=%d %s, value=%d\n",
              driverName, functionName, function, paramName, value);
    }
    else {
        if ( pasynTrace->getTraceMask(pasynUser) & ASYN_TRACEIO_DRIVER ) {
            getParamName( function, &paramName );
            asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                  "%s:%s: function=%d %s, paramvalue=%d\n",
                  driverName, functionName, function, paramName, value);
        }
    }
    return status;
}

/** Constructor for NDPluginCodec; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the 
  * ROI parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to 0 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to 0 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] maxThreads The maximum number of threads this driver is allowed to use. If 0 then 1 will be used.
  */
NDPluginCodec::NDPluginCodec(const char *portName, int queueSize, int blockingCallbacks,
                                           const char *NDArrayPort, int NDArrayAddr, 
                                           int maxBuffers, size_t maxMemory,
                                           int priority, int stackSize, int maxThreads)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks, 
                   NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
                   asynGenericPointerMask, 
                   asynGenericPointerMask,
                   0, 1, priority, stackSize, maxThreads,
                   true)
{
    //static const char *functionName = "NDPluginCodec";

    createParam(NDCodecModeString,            asynParamInt32,   &NDCodecMode);
    createParam(NDCodecCompressorString,      asynParamInt32,   &NDCodecCompressor);
    createParam(NDCodecCompFactorString,      asynParamFloat64, &NDCodecCompFactor);
    createParam(NDCodecJPEGQualityString,     asynParamInt32,   &NDCodecJPEGQuality);
    createParam(NDCodecBloscCompressorString, asynParamInt32,   &NDCodecBloscCompressor);
    createParam(NDCodecBloscCLevelString,     asynParamInt32,   &NDCodecBloscCLevel);
    createParam(NDCodecBloscShuffleString,    asynParamInt32,   &NDCodecBloscShuffle);
    createParam(NDCodecBloscNumThreadsString, asynParamInt32,   &NDCodecBloscNumThreads);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginCodec");
    
    setIntegerParam(NDCodecCompressor,      NDCODEC_NONE);
    setDoubleParam (NDCodecCompFactor,      0.0);
    setIntegerParam(NDCodecJPEGQuality,     85);
    setIntegerParam(NDCodecBloscCompressor, NDCODEC_BLOSC_BLOSCLZ);
    setIntegerParam(NDCodecBloscCLevel,     5);
    setIntegerParam(NDCodecBloscNumThreads, 1);

    // Enable ArrayCallbacks.  
    // This plugin currently ignores this setting and always does callbacks, so make the setting reflect the behavior
    setIntegerParam(NDArrayCallbacks, 1);

    /* Try to connect to the array port */
    connectToArrayPort();
}

extern "C" int NDCodecConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                          const char *NDArrayPort, int NDArrayAddr, 
                                          int maxBuffers, size_t maxMemory,
                                          int priority, int stackSize, int maxThreads)
{
    NDPluginCodec *pPlugin = new NDPluginCodec(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                                                             maxBuffers, maxMemory, priority, stackSize, maxThreads);
    return pPlugin->start();
}

/** EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg initArg9 = { "maxThreads",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9};
static const iocshFuncDef initFuncDef = {"NDCodecConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDCodecConfigure(args[0].sval, args[1].ival, args[2].ival,
                               args[3].sval, args[4].ival, args[5].ival, 
                               args[6].ival, args[7].ival, args[8].ival,
                               args[9].ival);
}

extern "C" void NDCodecRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDCodecRegister);
}
