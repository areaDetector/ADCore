#ifndef NDPluginCodec_H
#define NDPluginCodec_H

#include <epicsTypes.h>

#include "NDPluginDriver.h"

#define NDCodecModeString             "MODE"             /* (NDCodecMode_t r/w) Mode: Compress/Decompress */
#define NDCodecCompressorString       "COMPRESSOR"       /* (NDCodecCompressor_t r/w) Which codec to use */
#define NDCodecCompFactorString       "COMP_FACTOR"      /* (double r/o) Compression percentage (0 = no compression) */
#define NDCodecCodecStatusString      "CODEC_STATUS"     /* (int r/o) Compression status: success or failure */
#define NDCodecCodecErrorString       "CODEC_ERROR"      /* (string r/o) Error message if compression fails */
#define NDCodecJPEGQualityString      "JPEG_QUALITY"     /* (int r/w) JPEG Compression quality */
#define NDCodecBloscCompressorString  "BLOSC_COMPRESSOR" /* (NDCodecBloscComp_t r/w) Which Blosc compressor to use */
#define NDCodecBloscCLevelString      "BLOSC_CLEVEL"     /* (int r/w) Blosc compression level */
#define NDCodecBloscShuffleString     "BLOSC_SHUFFLE"    /* (bool r/w) Should Blosc apply shuffling? */
#define NDCodecBloscNumThreadsString  "BLOSC_NUMTHREADS" /* (int r/w) Number of threads to be used by Blosc */

/** Compress/decompress NDArrays according to available codecs.
  * This plugin is a source of NDArray callbacks, passing the (possibly
  * compressed/decompressed) NDArray data to clients that register for callbacks.
  * The plugin currently supports the following codecs (if available at compile
  * time):
  * <ul>
  *  <li> JPEG</li>
  *  <li> Blosc</li>
  * </ul>
  */

typedef enum {
    NDCODEC_COMPRESS,
    NDCODEC_DECOMPRESS,
}NDCodecMode_t;

typedef enum {
    NDCODEC_NONE,
    NDCODEC_JPEG,
    NDCODEC_BLOSC
}NDCodecCompressor_t;

typedef enum {
    NDCODEC_BLOSC_BLOSCLZ,
    NDCODEC_BLOSC_LZ4,
    NDCODEC_BLOSC_LZ4HC,
    NDCODEC_BLOSC_SNAPPY,
    NDCODEC_BLOSC_ZLIB,
    NDCODEC_BLOSC_ZSTD,
}NDCodecBloscComp_t;

/*
 * The [de]compress* functions below take an input array and return a
 * pool-allocated output array on success or NULL on error. They are
 * thread-safe.
 */

NDArray *compressJPEG(NDArrayPool *pool, NDArray *input, int quality);
NDArray *decompressJPEG(NDArrayPool *pool, NDArray *input);

NDArray *compressBlosc(NDArrayPool *pool, NDArray *input, int clevel,
        bool shuffle, NDCodecBloscComp_t compressor, int numThreads);
NDArray *decompressBlosc(NDArrayPool *pool, NDArray *input, int numThreads);


class epicsShareClass NDPluginCodec : public NDPluginDriver {
public:
    NDPluginCodec(const char *portName, int queueSize, int blockingCallbacks,
                  const char *NDArrayPort, int NDArrayAddr,
                  int maxBuffers, size_t maxMemory,
                  int priority, int stackSize, int maxThreads);

    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
    int NDCodecMode;
    #define FIRST_NDCODEC_PARAM NDCodecMode
    int NDCodecCompressor;
    int NDCodecCompFactor;
    int NDCodecCodecStatus;
    int NDCodecCodecError;
    int NDCodecJPEGQuality;
    int NDCodecBloscCompressor;
    int NDCodecBloscCLevel;
    int NDCodecBloscShuffle;
    int NDCodecBloscNumThreads;

    NDArray *compressJPEG(NDArrayPool *pool, NDArray *input, int quality);
    NDArray *decompressJPEG(NDArrayPool *pool, NDArray *input);
    NDArray *compressBlosc(NDArrayPool *pool, NDArray *input, int clevel,
             int shuffle, NDCodecBloscComp_t compressor, int numThreads);
    NDArray *decompressBlosc(NDArrayPool *pool, NDArray *input, int numThreads);

private:
    char errorMessage[256];
};
 
#endif
