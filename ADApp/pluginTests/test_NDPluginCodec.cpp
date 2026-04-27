/*
 * test_NDPluginCodec.cpp
 *
 * Tests NDPluginCodec with compress/decompress for each codec.
 *
 *  Created on: 20 Apr 2026
 *      Author: Jakub Wlodek
 */

#include <string.h>
#include <stdlib.h>

#include "boost/test/unit_test.hpp"

#include <NDPluginDriver.h>
#include <NDArray.h>
#include <asynDriver.h>
#include <NDCodec.h>
#include <NDPluginCodec.h>

#include "testingutilities.h"
#include "CodecPluginWrapper.h"

/* Dimensions of the test array: 8x8 of UInt16 */
#define TEST_XSIZE 8
#define TEST_YSIZE 8
#define TEST_NELEMENTS (TEST_XSIZE * TEST_YSIZE)

struct CodecTestFixture
{
    NDArrayPool *arrayPool;
    asynNDArrayDriver *dummy_driver;
    CodecPluginWrapper *codec;
    TestingPlugin *ds;

    CodecTestFixture()
    {
        std::string dummy_port("simCodec"), testport("testCodec");
        uniqueAsynPortName(dummy_port);
        uniqueAsynPortName(testport);

        dummy_driver = new asynNDArrayDriver(
            dummy_port.c_str(), 1, 0, 0,
            asynGenericPointerMask, asynGenericPointerMask, 0, 0, 0, 0);
        arrayPool = dummy_driver->pNDArrayPool;

        codec = new CodecPluginWrapper(testport.c_str(), dummy_port.c_str());
        codec->start();

        ds = new TestingPlugin(testport.c_str(), 0);

        codec->write(NDPluginDriverEnableCallbacksString, 1);
        codec->write(NDPluginDriverBlockingCallbacksString, 1);
    }

    ~CodecTestFixture()
    {
        delete codec;
        delete dummy_driver;
    }

    /* Create a test array filled with known data */
    NDArray *createTestArray()
    {
        size_t dims[2] = {TEST_XSIZE, TEST_YSIZE};
        NDArray *arr = arrayPool->alloc(2, dims, NDUInt16, 0, NULL);
        epicsUInt16 *pData = (epicsUInt16 *)arr->pData;
        for (int i = 0; i < TEST_NELEMENTS; i++) {
            pData[i] = (epicsUInt16)(i % 256);
        }
        return arr;
    }

    /* Process an array through the plugin */
    void processArray(NDArray *pArray)
    {
        codec->lock();
        codec->processCallbacks(pArray);
        codec->unlock();
    }

    /* Verify that decompressed data matches the original */
    void verifyDataMatch(NDArray *original, NDArray *result)
    {
        BOOST_REQUIRE(original->pData != NULL);
        BOOST_REQUIRE(result->pData != NULL);
        NDArrayInfo_t origInfo, resultInfo;
        original->getInfo(&origInfo);
        result->getInfo(&resultInfo);
        BOOST_REQUIRE_EQUAL(origInfo.totalBytes, resultInfo.totalBytes);
        BOOST_REQUIRE_EQUAL(result->dataType, original->dataType);
        BOOST_CHECK_EQUAL(memcmp(original->pData, result->pData, origInfo.totalBytes), 0);
    }
};

BOOST_FIXTURE_TEST_SUITE(CodecPluginTests, CodecTestFixture)

/* Test that passthrough with no codec works */
BOOST_AUTO_TEST_CASE(test_no_compression)
{
    NDArray *input = createTestArray();
    codec->write(NDCodecModeString, NDCODEC_COMPRESS);
    codec->write(NDCodecCompressorString, NDCODEC_NONE);

    ds->arrays.clear();
    processArray(input);

    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);
    NDArray *output = ds->arrays[0];
    BOOST_CHECK(output->codec.empty());
    verifyDataMatch(input, output);
    input->release();
}

#ifdef HAVE_JPEG
BOOST_AUTO_TEST_CASE(test_jpeg_compress_decompress)
{
    /* JPEG is lossy so we only check that the compress/decompress produces a valid
       array of the same dimensions, not bit-identical data.
       JPEG only supports 8-bit data. */
    size_t dims[2] = {TEST_XSIZE, TEST_YSIZE};
    NDArray *input = arrayPool->alloc(2, dims, NDUInt8, 0, NULL);
    epicsUInt8 *pData = (epicsUInt8 *)input->pData;
    for (int i = 0; i < TEST_NELEMENTS; i++) {
        pData[i] = (epicsUInt8)(i % 256);
    }

    /* Compress */
    codec->write(NDCodecModeString, NDCODEC_COMPRESS);
    codec->write(NDCodecCompressorString, NDCODEC_JPEG);
    codec->write(NDCodecJPEGQualityString, 95);

    ds->arrays.clear();
    processArray(input);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *compressed = ds->arrays[0];
    BOOST_CHECK_EQUAL(compressed->codec.name, NDCodecName[NDCODEC_JPEG]);
    BOOST_CHECK(compressed->compressedSize > 0);
    BOOST_CHECK(compressed->compressedSize <= compressed->dataSize);

    /* Decompress */
    codec->write(NDCodecModeString, NDCODEC_DECOMPRESS);
    ds->arrays.clear();
    processArray(compressed);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *decompressed = ds->arrays[0];
    BOOST_CHECK(decompressed->codec.empty());
    BOOST_CHECK_EQUAL(decompressed->dataType, NDUInt8);

    NDArrayInfo_t info;
    decompressed->getInfo(&info);
    BOOST_CHECK(info.totalBytes > 0);

    input->release();
}
#endif /* HAVE_JPEG */

#ifdef HAVE_ZLIB
BOOST_AUTO_TEST_CASE(test_zlib_compress_decompress)
{
    NDArray *input = createTestArray();

    /* Compress */
    codec->write(NDCodecModeString, NDCODEC_COMPRESS);
    codec->write(NDCodecCompressorString, NDCODEC_ZLIB);
    codec->write(NDCodecZlibCLevelString, 6);

    ds->arrays.clear();
    processArray(input);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *compressed = ds->arrays[0];
    BOOST_CHECK_EQUAL(compressed->codec.name, NDCodecName[NDCODEC_ZLIB]);
    BOOST_CHECK(compressed->compressedSize > 0);
    BOOST_CHECK(compressed->compressedSize <= compressed->dataSize);

    /* Decompress */
    codec->write(NDCodecModeString, NDCODEC_DECOMPRESS);
    ds->arrays.clear();
    processArray(compressed);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *decompressed = ds->arrays[0];
    BOOST_CHECK(decompressed->codec.empty());
    verifyDataMatch(input, decompressed);

    input->release();
}

BOOST_AUTO_TEST_CASE(test_zlib_compression_levels)
{
    /* Test that different compression levels all produce valid compress/decompress */
    for (int level = 0; level <= 9; level += 3) {
        NDArray *input = createTestArray();

        codec->write(NDCodecModeString, NDCODEC_COMPRESS);
        codec->write(NDCodecCompressorString, NDCODEC_ZLIB);
        codec->write(NDCodecZlibCLevelString, level);

        ds->arrays.clear();
        processArray(input);
        BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

        NDArray *compressed = ds->arrays[0];
        BOOST_CHECK_EQUAL(compressed->codec.name, NDCodecName[NDCODEC_ZLIB]);

        codec->write(NDCodecModeString, NDCODEC_DECOMPRESS);
        ds->arrays.clear();
        processArray(compressed);
        BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

        NDArray *decompressed = ds->arrays[0];
        verifyDataMatch(input, decompressed);

        input->release();
    }
}
#endif /* HAVE_ZLIB */

#ifdef HAVE_BLOSC
BOOST_AUTO_TEST_CASE(test_blosc_compress_decompress)
{
    NDArray *input = createTestArray();

    /* Compress */
    codec->write(NDCodecModeString, NDCODEC_COMPRESS);
    codec->write(NDCodecCompressorString, NDCODEC_BLOSC);
    codec->write(NDCodecBloscCLevelString, 5);
    codec->write(NDCodecBloscShuffleString, 1);
    codec->write(NDCodecBloscCompressorString, NDCODEC_BLOSC_BLOSCLZ);
    codec->write(NDCodecBloscNumThreadsString, 1);

    ds->arrays.clear();
    processArray(input);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *compressed = ds->arrays[0];
    BOOST_CHECK_EQUAL(compressed->codec.name, NDCodecName[NDCODEC_BLOSC]);
    BOOST_CHECK(compressed->compressedSize > 0);

    /* Decompress */
    codec->write(NDCodecModeString, NDCODEC_DECOMPRESS);
    ds->arrays.clear();
    processArray(compressed);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *decompressed = ds->arrays[0];
    BOOST_CHECK(decompressed->codec.empty());
    verifyDataMatch(input, decompressed);

    input->release();
}

BOOST_AUTO_TEST_CASE(test_blosc_compressors)
{
    /* Test several blosc sub-compressors compress/decompress. */
    int compressors[] = {
        NDCODEC_BLOSC_BLOSCLZ,
        NDCODEC_BLOSC_LZ4,
        NDCODEC_BLOSC_LZ4HC,
        NDCODEC_BLOSC_ZLIB,
        NDCODEC_BLOSC_ZSTD,
    };
    int nCompressors = sizeof(compressors) / sizeof(compressors[0]);

    for (int c = 0; c < nCompressors; c++) {
        NDArray *input = createTestArray();

        codec->write(NDCodecModeString, NDCODEC_COMPRESS);
        codec->write(NDCodecCompressorString, NDCODEC_BLOSC);
        codec->write(NDCodecBloscCLevelString, 5);
        codec->write(NDCodecBloscShuffleString, 1);
        codec->write(NDCodecBloscCompressorString, compressors[c]);
        codec->write(NDCodecBloscNumThreadsString, 1);

        ds->arrays.clear();
        processArray(input);
        BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

        NDArray *compressed = ds->arrays[0];
        BOOST_CHECK_EQUAL(compressed->codec.name, NDCodecName[NDCODEC_BLOSC]);
        BOOST_CHECK_EQUAL(compressed->codec.compressor, compressors[c]);

        codec->write(NDCodecModeString, NDCODEC_DECOMPRESS);
        ds->arrays.clear();
        processArray(compressed);
        BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

        NDArray *decompressed = ds->arrays[0];
        verifyDataMatch(input, decompressed);

        input->release();
    }
}
#endif /* HAVE_BLOSC */

#ifdef HAVE_BITSHUFFLE
BOOST_AUTO_TEST_CASE(test_lz4_compress_decompress)
{
    NDArray *input = createTestArray();

    /* Compress */
    codec->write(NDCodecModeString, NDCODEC_COMPRESS);
    codec->write(NDCodecCompressorString, NDCODEC_LZ4);

    ds->arrays.clear();
    processArray(input);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *compressed = ds->arrays[0];
    BOOST_CHECK_EQUAL(compressed->codec.name, NDCodecName[NDCODEC_LZ4]);
    BOOST_CHECK(compressed->compressedSize > 0);

    /* Decompress */
    codec->write(NDCodecModeString, NDCODEC_DECOMPRESS);
    ds->arrays.clear();
    processArray(compressed);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *decompressed = ds->arrays[0];
    BOOST_CHECK(decompressed->codec.empty());
    verifyDataMatch(input, decompressed);

    input->release();
}

BOOST_AUTO_TEST_CASE(test_lz4hdf5_compress_decompress)
{
    NDArray *input = createTestArray();

    /* Compress */
    codec->write(NDCodecModeString, NDCODEC_COMPRESS);
    codec->write(NDCodecCompressorString, NDCODEC_LZ4HDF5);
    codec->write(NDCodecLZ4HDF5BlockSizeString, 0);

    ds->arrays.clear();
    processArray(input);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *compressed = ds->arrays[0];
    BOOST_CHECK_EQUAL(compressed->codec.name, NDCodecName[NDCODEC_LZ4HDF5]);
    BOOST_CHECK(compressed->compressedSize > 0);

    /* Decompress */
    codec->write(NDCodecModeString, NDCODEC_DECOMPRESS);
    ds->arrays.clear();
    processArray(compressed);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *decompressed = ds->arrays[0];
    BOOST_CHECK(decompressed->codec.empty());
    verifyDataMatch(input, decompressed);

    input->release();
}

BOOST_AUTO_TEST_CASE(test_bslz4_compress_decompress)
{
    NDArray *input = createTestArray();

    /* Compress */
    codec->write(NDCodecModeString, NDCODEC_COMPRESS);
    codec->write(NDCodecCompressorString, NDCODEC_BSLZ4);

    ds->arrays.clear();
    processArray(input);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *compressed = ds->arrays[0];
    BOOST_CHECK_EQUAL(compressed->codec.name, NDCodecName[NDCODEC_BSLZ4]);
    BOOST_CHECK(compressed->compressedSize > 0);

    /* Decompress */
    codec->write(NDCodecModeString, NDCODEC_DECOMPRESS);
    ds->arrays.clear();
    processArray(compressed);
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);

    NDArray *decompressed = ds->arrays[0];
    BOOST_CHECK(decompressed->codec.empty());
    verifyDataMatch(input, decompressed);

    input->release();
}
#endif /* HAVE_BITSHUFFLE */

/* Test that compressing an already-compressed array is handled gracefully */
BOOST_AUTO_TEST_CASE(test_double_compress_warning)
{
    NDArray *input = createTestArray();
    input->codec.name = NDCodecName[NDCODEC_ZLIB];
    input->compressedSize = 10;

    codec->write(NDCodecModeString, NDCODEC_COMPRESS);
    codec->write(NDCodecCompressorString, NDCODEC_ZLIB);

    ds->arrays.clear();
    processArray(input);
    /* Plugin should still produce output (the original array passed through) */
    BOOST_REQUIRE_EQUAL(ds->arrays.size(), (size_t)1);
    /* Status should indicate warning */
    BOOST_CHECK_EQUAL(codec->readInt(NDCodecCodecStatusString), (int)NDCODEC_WARNING);

    input->release();
}

BOOST_AUTO_TEST_SUITE_END()
