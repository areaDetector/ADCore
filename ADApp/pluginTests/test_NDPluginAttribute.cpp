/*
 * test_NDPluginAttribute.cpp
 *
 * Tests NDPluginAttribute, including with compressed arrays.
 *
 *  Created on: 27 Apr 2026
 *      Author: Jakub Wlodek
 */

#include <string.h>
#include <stdlib.h>

#include "boost/test/unit_test.hpp"

#include <NDPluginDriver.h>
#include <NDArray.h>
#include <asynDriver.h>
#include <Codec.h>
#include <NDPluginAttribute.h>
#include <NDPluginCodec.h>

#include "testingutilities.h"
#include "AttributePluginWrapper.h"

#define TEST_XSIZE 8
#define TEST_YSIZE 8
#define TEST_NELEMENTS (TEST_XSIZE * TEST_YSIZE)

struct AttributeTestFixture
{
    NDArrayPool *arrayPool;
    asynNDArrayDriver *dummy_driver;
    AttributePluginWrapper *attr;

    AttributeTestFixture()
    {
        std::string dummy_port("simAttr"), testport("testAttr");
        uniqueAsynPortName(dummy_port);
        uniqueAsynPortName(testport);

        dummy_driver = new asynNDArrayDriver(
            dummy_port.c_str(), 1, 0, 0,
            asynGenericPointerMask, asynGenericPointerMask, 0, 0, 0, 0);
        arrayPool = dummy_driver->pNDArrayPool;

        attr = new AttributePluginWrapper(testport.c_str(), dummy_port.c_str(), 4);
        attr->start();

        attr->write(NDPluginDriverEnableCallbacksString, 1);
        attr->write(NDPluginDriverBlockingCallbacksString, 1);
    }

    ~AttributeTestFixture()
    {
        delete attr;
        delete dummy_driver;
    }

    NDArray *createTestArray(bool compress = false)
    {
        size_t dims[2] = {TEST_XSIZE, TEST_YSIZE};
        NDArray *arr = arrayPool->alloc(2, dims, NDUInt16, 0, NULL);
        epicsUInt16 *pData = (epicsUInt16 *)arr->pData;
        for (int i = 0; i < TEST_NELEMENTS; i++) {
            pData[i] = (epicsUInt16)(i % 256);
        }
#ifdef HAVE_ZLIB
        if (compress) {
            NDCodecStatus_t status;
            char errorMessage[256] = "";
            NDArray *compressed = compressZlib(arr, 6, &status, errorMessage);
            arr->release();
            return compressed;
        }
#else
        (void)compress;
#endif
        return arr;
    }

    void processArray(NDArray *pArray)
    {
        attr->lock();
        attr->processCallbacks(pArray);
        attr->unlock();
    }
};

BOOST_FIXTURE_TEST_SUITE(AttributePluginTests, AttributeTestFixture)

/* Test that NDPluginAttribute reads attributes from an uncompressed array */
BOOST_AUTO_TEST_CASE(test_attribute_uncompressed)
{
    NDArray *input = createTestArray();
    double testVal = 42.5;
    input->pAttributeList->add("TestAttr", "", NDAttrFloat64, &testVal);

    attr->write(NDPluginAttributeAttrNameString, std::string("TestAttr"), 0);

    processArray(input);

    double readVal = attr->readDouble(NDPluginAttributeValString, 0);
    BOOST_CHECK_CLOSE(readVal, 42.5, 0.001);

    input->release();
}

#ifdef HAVE_ZLIB
/* Test that NDPluginAttribute accepts a zlib-compressed array and still reads attributes */
BOOST_AUTO_TEST_CASE(test_attribute_compressed_array)
{
    NDArray *input = createTestArray(true);
    BOOST_REQUIRE(input != NULL);
    BOOST_CHECK_EQUAL(input->codec.name, codecName[NDCODEC_ZLIB]);

    double testVal = 99.0;
    input->pAttributeList->add("CompAttr", "", NDAttrFloat64, &testVal);

    attr->write(NDPluginAttributeAttrNameString, std::string("CompAttr"), 0);

    processArray(input);

    double readVal = attr->readDouble(NDPluginAttributeValString, 0);
    BOOST_CHECK_CLOSE(readVal, 99.0, 0.001);

    input->release();
}

/* Test that uniqueId is readable from a compressed array */
BOOST_AUTO_TEST_CASE(test_attribute_compressed_unique_id)
{
    NDArray *input = createTestArray(true);
    BOOST_REQUIRE(input != NULL);
    input->uniqueId = 12345;

    attr->write(NDPluginAttributeAttrNameString, std::string("NDArrayUniqueId"), 0);

    processArray(input);

    double readVal = attr->readDouble(NDPluginAttributeValString, 0);
    BOOST_CHECK_CLOSE(readVal, 12345.0, 0.001);

    input->release();
}
#endif /* HAVE_ZLIB */

BOOST_AUTO_TEST_SUITE_END()
