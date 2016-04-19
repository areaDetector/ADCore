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
#include "TimeSeriesPluginWrapper.h"
#include "AsynException.h"


static int callbackCount = 0;
static void *cbPtr = 0;

void TS_callback(void *userPvt, asynUser *pasynUser, void *pointer)
{
  cbPtr = pointer;
  callbackCount++;
}

struct TimeSeriesPluginTestFixture
{
  NDArrayPool *arrayPool;
  boost::shared_ptr<asynPortDriver> driver;
  boost::shared_ptr<TimeSeriesPluginWrapper> ts;
  boost::shared_ptr<asynGenericPointerClient> client;
  TestingPlugin* downstream_plugin; // TODO: we don't put this in a shared_ptr and purposefully leak memory because asyn ports cannot be deleted
  std::vector<NDArray*>arrays_1d;
  std::vector<size_t>dims_1d;
  std::vector<NDArray*>arrays_2d;
  std::vector<size_t>dims_2d;
  std::vector<NDArray*>arrays_3d;
  std::vector<size_t>dims_3d;

  static int testCase;

  TimeSeriesPluginTestFixture()
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
    ts = boost::shared_ptr<TimeSeriesPluginWrapper>(new TimeSeriesPluginWrapper(testport.c_str(),
                                                                      50,
                                                                      1,
                                                                      simport.c_str(),
                                                                      0,
                                                                      1,
                                                                      0,
                                                                      0,
                                                                      2000000));
    // This is the mock downstream plugin
    downstream_plugin = new TestingPlugin(testport.c_str(), 0);

    // Enable the plugin
    ts->start(); // start the plugin thread although not required for this unittesting
    ts->write(NDPluginDriverEnableCallbacksString, 1);
    ts->write(NDPluginDriverBlockingCallbacksString, 1);

    client = boost::shared_ptr<asynGenericPointerClient>(new asynGenericPointerClient(testport.c_str(), 0, NDArrayDataString));
    client->registerInterruptUser(&TS_callback);

    // 1D: 8 channels with a single scalar sample element in each one
    size_t tmpdims_1d[] = {8};
    dims_1d.assign(tmpdims_1d, tmpdims_1d + sizeof(tmpdims_1d)/sizeof(tmpdims_1d[0]));
    arrays_1d.resize(200); // We create 200 samples
    fillNDArrays(dims_1d, NDFloat32, arrays_1d); // Fill some NDArrays with unimportant data

    // 2D: three time series channels, each with 20 elements
    size_t tmpdims_2d[] = {3,20};
    dims_2d.assign(tmpdims_2d, tmpdims_2d + sizeof(tmpdims_2d)/sizeof(tmpdims_2d[0]));
    arrays_2d.resize(24);
    fillNDArrays(dims_2d, NDFloat32, arrays_2d);

    // 3D: four channels with 2D images of 5x6 pixel (like an RGB image)
    // Not valid input for the Time Series plugin
    size_t tmpdims_3d[] = {4,5,6};
    dims_3d.assign(tmpdims_3d, tmpdims_3d + sizeof(tmpdims_3d)/sizeof(tmpdims_3d[0]));
    arrays_3d.resize(24);
    fillNDArrays(dims_3d, NDFloat32, arrays_3d);

    // Plugin setup: TimePerPoint=0.001 and AveragingTime=0.01 thus NumAverage=10
    BOOST_REQUIRE_NO_THROW(ts->write(TSTimePerPointString, 0.001));
    BOOST_REQUIRE_NO_THROW(ts->write(TSAveragingTimeString, 0.01));
    BOOST_REQUIRE_NO_THROW(ts->write(TSNumPointsString, 20));
    BOOST_REQUIRE_NO_THROW(ts->write(TSAcquireModeString, 0)); // TSAcquireModeFixed=0

    // Double check plugin setup
    BOOST_REQUIRE_EQUAL(ts->readInt(TSNumAverageString), 10);
    BOOST_REQUIRE_EQUAL(ts->readInt(TSNumPointsString), 20);
}

  ~TimeSeriesPluginTestFixture()
  {
    delete arrayPool;
    client.reset();
    ts.reset();
    driver.reset();
    //delete downstream_plugin; // TODO: We can't delete a TestingPlugin because it tries to delete an asyn port which doesnt work
  }

};

BOOST_FIXTURE_TEST_SUITE(TimeSeriesPluginTests, TimeSeriesPluginTestFixture)


BOOST_AUTO_TEST_CASE(validate_input_NDArrays)
{
  BOOST_MESSAGE("Validating the input NDArrays " <<
                "- if any of these fail the test-cases are also likely to fail");

  // Double check one of the 1D NDArrays dimensions and datatype
  BOOST_REQUIRE_EQUAL(arrays_1d[0]->ndims, 1);
  BOOST_CHECK_EQUAL(arrays_1d[0]->dims[0].size, 8);
  BOOST_CHECK_EQUAL(arrays_1d[0]->dataType, NDFloat32);

  // Double check one of the 2D NDArrays dimensions and datatype
  BOOST_REQUIRE_EQUAL(arrays_2d[0]->ndims, 2);
  BOOST_CHECK_EQUAL(arrays_2d[0]->dims[0].size, 3);
  BOOST_CHECK_EQUAL(arrays_2d[0]->dims[1].size, 20);
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
  ts->lock();
  BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_3d[0]));   // non-ideal test
  ts->unlock();
  BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), 0);    // non-ideal test
}


BOOST_AUTO_TEST_CASE(basic_1D_operation)
{
  BOOST_MESSAGE("Testing 1D input arrays: " << arrays_1d[0]->dims[0].size
                << " Channels with a single scalar value. Averaging=" << 10
                << " samples. Time series length=" << 20);

  BOOST_CHECK_NO_THROW(ts->write(TSAcquireString, 1));
  BOOST_CHECK_EQUAL(ts->readInt(TSAcquireString), 1);

  // Process 200 arrays through the TS plugin. As we have averaged by 10 TimePoints
  // (see TSNumAverage) we should then have a new 10 point Time Series output.
  for (int i = 0; i < 200; i++)
  {
    ts->lock();
    BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_1d[i]));
    ts->unlock();
  }
  BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), 20);
  // As we are using Fixed Lenght mode, acqusition should now have stopped
  BOOST_REQUIRE_EQUAL(ts->readInt(TSAcquireString), 0);
}


BOOST_AUTO_TEST_CASE(basic_2D_operation)
{
  BOOST_MESSAGE("Testing 2D input arrays: " << arrays_2d[0]->dims[0].size
                << " channels with " << arrays_2d[0]->dims[1].size
                << " elements. Averaging=" << 10 << " Time series length=" << 20);

  BOOST_CHECK_NO_THROW(ts->write(TSAcquireString, 1));
  BOOST_CHECK_EQUAL(ts->readInt(TSAcquireString), 1);

  // Process 10 arrays through the TS plugin. As we have averaged by 10 TimePoints
  // (see TSNumAverage) we should then have a new 20 point Time Series output.
  for (int i = 0; i < 10; i++)
  {
    ts->lock();
    BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_2d[i]));
    ts->unlock();
    BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), (i+1)*2); // num points in NDArray timeseries / NumAverage
  }

  // As we are using Fixed Length mode, acquisition should now have stopped
  BOOST_CHECK_EQUAL(ts->readInt(TSAcquireString), 0);

  // Processing an extra array through should not have an effect as acquisition
  // has stopped
  ts->lock();
  BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_2d[10]));
  ts->unlock();
  BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), 20);
}


BOOST_AUTO_TEST_CASE(circular_2D_operation)
{
  // Fill some NDArrays with unimportant data

  BOOST_MESSAGE("Testing 2D input arrays: " << arrays_2d[0]->dims[0].size
                << " channels with " << arrays_2d[0]->dims[1].size
                << " elements. Averaging=" << 10 << " Time series length=" << 20);

  BOOST_CHECK_NO_THROW(ts->write(TSAcquireModeString, 1)); // TSAcquireModeCircular=1
  BOOST_CHECK_NO_THROW(ts->write(TSAcquireString, 1));
  BOOST_CHECK_EQUAL(ts->readInt(TSAcquireString), 1);

  // Process 9 arrays through the TS plugin.
  for (int i = 0; i < 9; i++)
  {
    ts->lock();
    BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_2d[i]));
    ts->unlock();
    BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), (i+1)*2); // num points in NDArray timeseries / NumAverage
  }

  // Process 10th array through the TS plugin
  ts->lock();
  BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_2d[9]));
  ts->unlock();
  // The current point should now have been reset to 0 as the buffer has filled
  // up and wrapped around
  BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), 0);

  BOOST_CHECK_EQUAL(ts->readInt(TSAcquireString), 1);

  // Process 11th array through the TS plugin
  ts->lock();
  BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_2d[10]));
  ts->unlock();
  BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), 2);
  BOOST_CHECK_EQUAL(ts->readInt(TSAcquireString), 1);
}



BOOST_AUTO_TEST_CASE(output_NDArray_fixed_mode)
{
  BOOST_MESSAGE("Checking output NDArray in Fixed Mode, 2D input: " << arrays_2d[0]->dims[0].size
                << " channels with " << arrays_2d[0]->dims[1].size
                << " elements. Averaging=" << 10 << " Time series length=" << 20);

  BOOST_CHECK_NO_THROW(ts->write(NDArrayCallbacksString, 1));
  BOOST_CHECK_NO_THROW(ts->write(TSAcquireString, 1));
  BOOST_CHECK_EQUAL(ts->readInt(TSAcquireString), 1);

  // Process 9 arrays through the TS plugin.
  for (int i = 0; i < 9; i++)
  {
    ts->lock();
    BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_2d[i]));
    ts->unlock();
    BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), (i+1)*2); // num points in NDArray timeseries / NumAverage
  }

  // The downstream plugin must not yet have received any arrays
  BOOST_REQUIRE_EQUAL(downstream_plugin->arrays.size(), 0);

  // Process 10th array through the TS plugin
  ts->lock();
  BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_2d[9]));
  ts->unlock();
  BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), 20);
  BOOST_CHECK_EQUAL(ts->readInt(TSAcquireString), 0);

  // The downstream plugin must have received one array now
  BOOST_REQUIRE_EQUAL(downstream_plugin->arrays.size(), 1);
  BOOST_REQUIRE_EQUAL(downstream_plugin->arrays[0]->ndims, 1);
  BOOST_CHECK_EQUAL(downstream_plugin->arrays[0]->dims[0].size, 20);

}


BOOST_AUTO_TEST_CASE(trigger_output_NDArray_circular_mode)
{
  BOOST_MESSAGE("Checking that NDArrays are output when triggered in Circular Mode, 2D input: " << arrays_2d[0]->dims[0].size
                << " channels with " << arrays_2d[0]->dims[1].size
                << " elements. Averaging=" << 10 << " Time series length=" << 20);

  BOOST_CHECK_NO_THROW(ts->write(NDArrayCallbacksString, 1));
  BOOST_CHECK_NO_THROW(ts->write(TSAcquireModeString, 1)); // TSAcquireModeCircular=1
  BOOST_CHECK_NO_THROW(ts->write(TSAcquireString, 1));
  BOOST_CHECK_EQUAL(ts->readInt(TSAcquireString), 1);

  // Process 24 arrays through the TS plugin.
  for (int i = 0; i < 24; i++)
  {
    ts->lock();
    BOOST_CHECK_NO_THROW(ts->processCallbacks(arrays_2d[i]));
    ts->unlock();
    BOOST_CHECK_EQUAL(ts->readInt(TSCurrentPointString), ((i+1)*2)%20); // num points in NDArray timeseries / NumAverage
  }

  // The downstream plugin must not yet have any arrays
  BOOST_REQUIRE_EQUAL(downstream_plugin->arrays.size(), 0);

  // Trigger an output with the TS_READ parameter
  BOOST_CHECK_NO_THROW(ts->write(TSReadString, 1));
  // The downstream plugin must have received one array now
  BOOST_REQUIRE_EQUAL(downstream_plugin->arrays.size(), 1);
  BOOST_REQUIRE_EQUAL(downstream_plugin->arrays[0]->ndims, 1);
  BOOST_CHECK_EQUAL(downstream_plugin->arrays[0]->dims[0].size, 20);
}


BOOST_AUTO_TEST_SUITE_END() // Done!
