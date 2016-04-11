/*
 * test_NDPluginTimeSeries.cpp
 *
 *  Created on: 21 Mar 2016
 *      Author: Ulrik Pedersen
 */

#include <stdio.h>


#include "boost/test/unit_test.hpp"

// AD dependencies
#include <NDPluginDriver.h>
#include <NDArray.h>
#include <NDAttribute.h>
#include <asynDriver.h>

#include <string.h>
#include <stdint.h>

#include <deque>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <fstream>
using namespace std;

#include "testingutilities.h"
#include "FFTPluginWrapper.h"
#include "AsynException.h"


static int callbackCount = 0;
static void *cbPtr = 0;

void FFT_callback(void *userPvt, asynUser *pasynUser, void *pointer)
{
  cbPtr = pointer;
  callbackCount++;
}

struct FFTPluginTestFixture
{
  NDArrayPool *arrayPool;
  boost::shared_ptr<asynPortDriver> driver;
  boost::shared_ptr<FFTPluginWrapper> fft;
  boost::shared_ptr<asynGenericPointerClient> client;
  TestingPlugin* downstream_plugin; // TODO: we don't put this in a shared_ptr and purposefully leak memory because asyn ports cannot be deleted
  std::vector<NDArray*>arrays_1d;
  std::vector<size_t>dims_1d;
  std::vector<NDArray*>arrays_2d;
  std::vector<size_t>dims_2d;
  std::vector<NDArray*>arrays_3d;
  std::vector<size_t>dims_3d;

  static int testCase;

  FFTPluginTestFixture()
  {
    arrayPool = new NDArrayPool(100, 0);

    // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers
    // (even if only one is ever instantiated at once), so we change it slightly for each test case.
    std::string simport("simTS"), testport("TS");
    uniqueAsynPortName(simport);
    uniqueAsynPortName(testport);

    // We need some upstream driver for our test plugin so that calls to connectArrayPort
    // don't fail, but we can then ignore it and send arrays by calling processCallbacks directly.
    driver = boost::shared_ptr<asynPortDriver>(new asynPortDriver(simport.c_str(),
                                                                     1, 1,
                                                                     asynGenericPointerMask,
                                                                     asynGenericPointerMask,
                                                                     0, 0, 0, 2000000));

    // This is the plugin under test
    fft = boost::shared_ptr<FFTPluginWrapper>(new FFTPluginWrapper(testport.c_str(),
                                                                      50,
                                                                      1,
                                                                      simport.c_str(),
                                                                      0,
                                                                      0,
                                                                      0,
                                                                      2000000));
    // This is the mock downstream plugin
    downstream_plugin = new TestingPlugin(testport.c_str(), 0);

    // Enable the plugin
    fft->start(); // start the plugin thread although not required for this unittesting
    fft->write(NDPluginDriverEnableCallbacksString, 1);
    fft->write(NDPluginDriverBlockingCallbacksString, 1);

    client = boost::shared_ptr<asynGenericPointerClient>(new asynGenericPointerClient(testport.c_str(), 0, NDArrayDataString));
    client->registerInterruptUser(&FFT_callback);

    // 1D: 20 scalar samples
    size_t tmpdims_1d[] = {20};
    dims_1d.assign(tmpdims_1d, tmpdims_1d + sizeof(tmpdims_1d)/sizeof(tmpdims_1d[0]));
    arrays_1d.resize(200); // We create 200 1D arrays
    fillNDArrays(dims_1d, NDFloat32, arrays_1d); // Fill some NDArrays with unimportant data

    // 2D: image of 20x40 pixels
    size_t tmpdims_2d[] = {20,40};
    dims_2d.assign(tmpdims_2d, tmpdims_2d + sizeof(tmpdims_2d)/sizeof(tmpdims_2d[0]));
    arrays_2d.resize(24);
    fillNDArrays(dims_2d, NDFloat32, arrays_2d);

    // 3D: four channels with 2D images of 5x6 pixel (like an RGB image)
    size_t tmpdims_3d[] = {4,5,6};
    dims_3d.assign(tmpdims_3d, tmpdims_3d + sizeof(tmpdims_3d)/sizeof(tmpdims_3d[0]));
    arrays_3d.resize(24);
    fillNDArrays(dims_3d, NDFloat32, arrays_3d);
}

  ~FFTPluginTestFixture()
  {
    delete arrayPool;
    client.reset();
    fft.reset();
    driver.reset();
    //delete downstream_plugin; // TODO: We can't delete a TestingPlugin because it tries to delete an asyn port which doesnt work
  }

};

BOOST_FIXTURE_TEST_SUITE(FFTPluginTests, FFTPluginTestFixture)


BOOST_AUTO_TEST_CASE(validate_input_NDArrays)
{
  BOOST_MESSAGE("Validating the input NDArrays " <<
                "- if any of these fail the test-cases are also likely to fail");

  // Double check one of the 1D NDArrays dimensions and datatype
  BOOST_REQUIRE_EQUAL(arrays_1d[0]->ndims, 1);
  BOOST_CHECK_EQUAL(arrays_1d[0]->dims[0].size, (size_t)20);
  BOOST_CHECK_EQUAL(arrays_1d[0]->dataType, NDFloat32);

  // Double check one of the 2D NDArrays dimensions and datatype
  BOOST_REQUIRE_EQUAL(arrays_2d[0]->ndims, 2);
  BOOST_CHECK_EQUAL(arrays_2d[0]->dims[0].size, (size_t)20);
  BOOST_CHECK_EQUAL(arrays_2d[0]->dims[1].size, (size_t)40);
  BOOST_CHECK_EQUAL(arrays_2d[0]->dataType, NDFloat32);

  // Double check one of the 3D NDArrays dimensions
  BOOST_REQUIRE_EQUAL(arrays_3d[0]->ndims, 3);
}


BOOST_AUTO_TEST_CASE(invalid_number_dimensions)
{
  // Actually the processCallbacks() function returns void and do not generally
  // throw exceptions. The ideal testing is to check for an exception but alas
  // we cannot do that here. Thus we comment out the ideal test:
  // BOOST_CHECK_THROW(ts->processCallbacks(arrays[0]), AsynException);

  // At least we can test that it doesn't crash...
  BOOST_MESSAGE("Expecting stdout message \"error, number of array dimensions...\"");
  fft->lock();
  BOOST_CHECK_NO_THROW(fft->processCallbacks(arrays_3d[0]));   // non-ideal test
  fft->unlock();
}


BOOST_AUTO_TEST_CASE(basic_1D_operation)
{
  BOOST_MESSAGE("Testing 1D input arrays: " << arrays_1d[0]->dims[0].size
                << " Channels with a single scalar value. Averaging=" << 1
                << " samples.");

  BOOST_CHECK_NO_THROW(fft->write(FFTDirectionString, 0));
  BOOST_CHECK_EQUAL(fft->readInt(FFTDirectionString), 0);
  BOOST_CHECK_NO_THROW(fft->write(FFTNumAverageString, 1));
  BOOST_CHECK_EQUAL(fft->readInt(FFTNumAverageString), 1);
  BOOST_CHECK_NO_THROW(fft->write(FFTSuppressDCString, 0));
  BOOST_CHECK_EQUAL(fft->readInt(FFTSuppressDCString), 0);
  BOOST_CHECK_NO_THROW(fft->write(NDArrayCallbacksString, 1));
  BOOST_CHECK_EQUAL(fft->readInt(NDArrayCallbacksString), 1);

  // Process 200 arrays through the TS plugin.
  for (int i = 0; i < 200; i++)
  {
    fft->lock();
    BOOST_CHECK_NO_THROW(fft->processCallbacks(arrays_1d[i]));
    fft->unlock();
  }
  BOOST_CHECK_EQUAL(fft->readInt("ARRAY_COUNTER"), 200);
  BOOST_CHECK_EQUAL(fft->readInt(FFTNumAveragedString), 1);

  // Check the downstream receiver of the FFT array
  BOOST_CHECK_EQUAL(downstream_plugin->arrays.size(), (size_t)200);
  BOOST_REQUIRE_GT(downstream_plugin->arrays.size(), (size_t)0);
  BOOST_REQUIRE_EQUAL(downstream_plugin->arrays[0]->ndims, 1);
  for (int i=0; i<200; i++) {
    BOOST_REQUIRE_EQUAL(downstream_plugin->arrays[i]->dims[0].size, (size_t)16);
  }
}


BOOST_AUTO_TEST_SUITE_END() // Done!
