#include <boost/test/tools/old/interface.hpp>
#include <string.h>
#include <stdlib.h>

#include "boost/test/unit_test.hpp"

#include <NDPluginDriver.h>
#include <NDArray.h>
#include <asynDriver.h>
#include <NDPluginColorConvert.h>

#include "testingutilities.h"
#include "ColorConvertPluginWrapper.h"

/* Dimensions of the test array: 8x8 of UInt16 */
#define TEST_XSIZE 8
#define TEST_YSIZE 8
#define TEST_NELEMENTS (TEST_XSIZE * TEST_YSIZE)

struct ColorConvertTestFixture
{
    NDArrayPool *arrayPool;
    std::vector<NDArray*>pArrays;
    std::vector<size_t> dims = {TEST_XSIZE, TEST_YSIZE};
    asynNDArrayDriver *dummy_driver;
    ColorConvertPluginWrapper *cc;
    TestingPlugin* downstream_plugin;

    ColorConvertTestFixture()
    {
        std::string dummy_port("simCC"), testport("testCC");
        uniqueAsynPortName(dummy_port);
        uniqueAsynPortName(testport);

        dummy_driver = new asynNDArrayDriver(
            dummy_port.c_str(), 1, 0, 0,
            asynGenericPointerMask, asynGenericPointerMask, 0, 0, 0, 0);
        arrayPool = dummy_driver->pNDArrayPool;

        pArrays.resize(1);
        fillNDArraysFromPool(dims, NDUInt16, pArrays, arrayPool);

        cc = new ColorConvertPluginWrapper(testport.c_str(), dummy_port.c_str());

        // This is the mock downstream plugin
        downstream_plugin = new TestingPlugin(testport.c_str(), 0);

        cc->start();

        cc->write(NDPluginDriverEnableCallbacksString, 1);
        cc->write(NDPluginDriverBlockingCallbacksString, 1);
    }

    ~ColorConvertTestFixture()
    {
        delete cc;
        delete dummy_driver;
    }

    void processArray(NDArray *pArray)
    {
        cc->lock();
        cc->processCallbacks(pArray);
        cc->unlock();
    }

    /* set array metadata and convert it to desired color mode*/
    void prepareArray(NDArray *input, NDColorMode_t colorModeOut)
    {
        input->timeStamp = 1777406490.0;
        cc->write(NDPluginColorConvertColorModeOutString, colorModeOut);
        processArray(input);
    }
};

BOOST_FIXTURE_TEST_SUITE(ColorConvertTests, ColorConvertTestFixture)

/* Check if metadata remains the same after color conversion
 * and if the dimension matches the expected
 * - RGB1 = [3, X, Y]
 * - RGB2 = [X, 3, Y]
 * - RGB3 = [X, Y, 3]
 */

/* From Mono to RGB1 */
BOOST_AUTO_TEST_CASE(test_metadata_grayscale_to_rgb1)
{
    NDArray *input = pArrays[0];
    BOOST_REQUIRE(input != NULL);

    prepareArray(input, NDColorModeRGB1);

    NDArray *output = downstream_plugin->arrays.back();
    BOOST_REQUIRE(output != NULL);

    BOOST_CHECK_EQUAL(output->uniqueId, input->uniqueId);
    BOOST_CHECK_CLOSE(output->timeStamp, input->timeStamp, 0.001);
    BOOST_CHECK_EQUAL(output->ndims, 3);
    BOOST_CHECK_EQUAL(output->dims[0].size, 3);
    BOOST_CHECK_EQUAL(output->dims[1].size, TEST_XSIZE);
    BOOST_CHECK_EQUAL(output->dims[2].size, TEST_YSIZE);
    BOOST_CHECK_EQUAL(output->compressedSize, output->dataSize);
}

 /* From Mono to RGB2 */
BOOST_AUTO_TEST_CASE(test_metadata_grayscale_to_rgb2)
{
    NDArray *input = pArrays[0];
    BOOST_REQUIRE(input != NULL);

    prepareArray(input, NDColorModeRGB2);

    NDArray *output = downstream_plugin->arrays.back();
    BOOST_REQUIRE(output != NULL);

    BOOST_CHECK_EQUAL(output->uniqueId, input->uniqueId);
    BOOST_CHECK_CLOSE(output->timeStamp, input->timeStamp, 0.001);
    BOOST_CHECK_EQUAL(output->ndims, 3);
    BOOST_CHECK_EQUAL(output->dims[0].size, TEST_XSIZE);
    BOOST_CHECK_EQUAL(output->dims[1].size, 3);
    BOOST_CHECK_EQUAL(output->dims[2].size, TEST_YSIZE);
    BOOST_CHECK_EQUAL(output->compressedSize, output->dataSize);
}

/* From Mono to RGB3 */
BOOST_AUTO_TEST_CASE(test_metadata_grayscale_to_rgb3)
{
    NDArray *input = pArrays[0];
    BOOST_REQUIRE(input != NULL);

    prepareArray(input, NDColorModeRGB3);

    NDArray *output = downstream_plugin->arrays.back();
    BOOST_REQUIRE(output != NULL);

    BOOST_CHECK_EQUAL(output->uniqueId, input->uniqueId);
    BOOST_CHECK_CLOSE(output->timeStamp, input->timeStamp, 0.001);
    BOOST_CHECK_EQUAL(output->ndims, 3);
    BOOST_CHECK_EQUAL(output->dims[0].size, TEST_XSIZE);
    BOOST_CHECK_EQUAL(output->dims[1].size, TEST_YSIZE);
    BOOST_CHECK_EQUAL(output->dims[2].size, 3);
    BOOST_CHECK_EQUAL(output->compressedSize, output->dataSize);
}

BOOST_AUTO_TEST_SUITE_END()
