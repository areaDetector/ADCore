/*
 * test_NDPosPlugin.cpp
 *
 *  Created on: 24 Jun 2015
 *      Author: gnx91527
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
#include <tr1/memory>
#include <iostream>
#include <fstream>
using namespace std;

#include "testingutilities.h"
#include "PosPluginWrapper.h"
#include "HDF5FileReader.h"
#include "AsynException.h"


static int callbackCount = 0;
static void *cbPtr = 0;

void callback(void *userPvt, asynUser *pasynUser, void *pointer)
{
  cbPtr = pointer;
  callbackCount++;
}

static NDArrayPool *arrayPool;

struct PosPluginTestFixture
{
  std::tr1::shared_ptr<asynNDArrayDriver> driver;
  std::tr1::shared_ptr<PosPluginWrapper> pos;
  std::tr1::shared_ptr<asynGenericPointerClient> client;

  static int testCase;

  PosPluginTestFixture()
  {

    // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
    // change it slightly for each test case.
    std::string simport("simPostest"), testport("POS");
    uniqueAsynPortName(simport);
    uniqueAsynPortName(testport);

    // We need some upstream driver for our test plugin so that calls to connectArrayPort don't fail, but we can then ignore it and send
    // arrays by calling processCallbacks directly.
    driver = std::tr1::shared_ptr<asynNDArrayDriver>(new asynNDArrayDriver(simport.c_str(), 1, 0, 0, asynGenericPointerMask, asynGenericPointerMask, 0, 0, 0, 0));
    arrayPool = driver->pNDArrayPool;

    // This is the plugin under test
    pos = std::tr1::shared_ptr<PosPluginWrapper>(new PosPluginWrapper(testport.c_str(),
                                                                      50,
                                                                      1,
                                                                      simport.c_str(),
                                                                      0,
                                                                      0,
                                                                      0,
                                                                      0,
                                                                      0));

    // Enable the plugin
    pos->write(NDPluginDriverEnableCallbacksString, 1);
    pos->write(NDPluginDriverBlockingCallbacksString, 1);
    pos->write(NDArrayCallbacksString, 1);

    client = std::tr1::shared_ptr<asynGenericPointerClient>(new asynGenericPointerClient(testport.c_str(), 0, NDArrayDataString));
    client->registerInterruptUser(&callback);
  }

  ~PosPluginTestFixture()
  {
    client.reset();
    pos.reset();
    driver.reset();
  }

};

BOOST_FIXTURE_TEST_SUITE(PosPluginTests, PosPluginTestFixture)

BOOST_AUTO_TEST_CASE(test_LoadingDataPoints)
{
  // Start by loading points through the parameter
  // Try to write an invalid load string, verify error
  BOOST_CHECK_THROW(pos->write(str_NDPos_Filename, "<pos_layout><bad xml string</position>"), AsynException);
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_FileValid), 0);
  // Verify load does not load points, qty is zero
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 0);

  // Set a valid load string, verify no error
  pos->write(str_NDPos_Filename, "<pos_layout>\
  <dimensions>\
    <dimension name=\"x\"></dimension>\
    <dimension name=\"y\"></dimension>\
    <dimension name=\"n\"></dimension>\
  </dimensions>\
  <positions>\
    <position x=\"0\" y=\"0\" n=\"0\"></position>\
    <position x=\"1\" y=\"0\" n=\"0\"></position>\
    <position x=\"0\" y=\"1\" n=\"0\"></position>\
    <position x=\"1\" y=\"1\" n=\"0\"></position>\
    <position x=\"0\" y=\"0\" n=\"1\"></position>\
    <position x=\"1\" y=\"0\" n=\"1\"></position>\
    <position x=\"0\" y=\"1\" n=\"1\"></position>\
    <position x=\"1\" y=\"1\" n=\"1\"></position>\
  </positions>\
  </pos_layout>");
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_FileValid), 1);
  // Verify points are loaded, qty is 8
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 8);

  // Verify deleting points will reset to zero qty
  pos->write(str_NDPos_Delete, 1);
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 0);

  // Create bad points file
  {
    std::ofstream out("/tmp/invalid_points.xml");
    out << "<pos_layout><bad xml string</position>";
  }
  // Create good points file
  {
    std::ofstream out("/tmp/valid_points.xml");
    out << "<pos_layout>\
    <dimensions>\
      <dimension name=\"x\"></dimension>\
      <dimension name=\"y\"></dimension>\
      <dimension name=\"n\"></dimension>\
    </dimensions>\
    <positions>\
      <position x=\"0\" y=\"0\" n=\"0\"></position>\
      <position x=\"1\" y=\"0\" n=\"0\"></position>\
      <position x=\"0\" y=\"1\" n=\"0\"></position>\
      <position x=\"1\" y=\"1\" n=\"0\"></position>\
      <position x=\"0\" y=\"0\" n=\"1\"></position>\
      <position x=\"1\" y=\"0\" n=\"1\"></position>\
      <position x=\"0\" y=\"1\" n=\"1\"></position>\
      <position x=\"1\" y=\"1\" n=\"1\"></position>\
    </positions>\
    </pos_layout>";
  }

  // Try to set an invalid load filename, verify error
  BOOST_CHECK_THROW(pos->write(str_NDPos_Filename, "/tmp/incorrect_file.xml"), AsynException);
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_FileValid), 0);
  // Verify load does not load points, qty is 0
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 0);
  // Try to set a valid filename but with bad XML, verify error
  BOOST_CHECK_THROW(pos->write(str_NDPos_Filename, "/tmp/invalid_points.xml"), AsynException);
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_FileValid), 0);
  // Verify load does not load points, qty is 0
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 0);

  // Set a valid load string, verify no error
  pos->write(str_NDPos_Filename, "/tmp/valid_points.xml");
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_FileValid), 1);
  // Verify points are loaded, qty is 8
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 8);

  // Load the file again, verify qty increases to 16
  pos->write(str_NDPos_Filename, "/tmp/valid_points.xml");
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 16);

  // Load the file again, verify qty increases to 24
  pos->write(str_NDPos_Filename, "/tmp/valid_points.xml");
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 24);


  size_t tmpdims[] = {10,10};
  std::vector<size_t>dims(tmpdims, tmpdims + sizeof(tmpdims)/sizeof(tmpdims[0]));
  // Create some test arrays
  std::vector<NDArray*>arrays(24);
  fillNDArraysFromPool(dims, NDUInt32, arrays, arrayPool);

  // Send some arrays to the plugin, verify they pass through with no position data appended
  for (int i = 0; i < 10; i++)
  {
    pos->lock();
    BOOST_CHECK_NO_THROW(pos->processCallbacks(arrays[i]));
    pos->unlock();
    BOOST_CHECK_EQUAL(arrays[i]->pAttributeList->find("n"), (NDAttribute *)0);
    BOOST_CHECK_EQUAL(arrays[i]->pAttributeList->find("x"), (NDAttribute *)0);
    BOOST_CHECK_EQUAL(arrays[i]->pAttributeList->find("y"), (NDAttribute *)0);
  }

  // Verify qty of points remains at 24
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 24);

  // Set mode to discard
  BOOST_CHECK_NO_THROW(pos->write(str_NDPos_Mode, 0));
  // Start position generation
  BOOST_CHECK_NO_THROW(pos->write(str_NDPos_Running, 1));

  BOOST_CHECK_NO_THROW(pos->write(str_NDPos_ExpectedID, 0));
//pos->
  int xvals[24] = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1};
  int yvals[24] = {0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1};
  int nvals[24] = {0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1};
  // Send some arrays to the plugin
  for (int i = 0; i < 10; i++)
  {
    arrays[i]->uniqueId = i;
    pos->lock();
    BOOST_CHECK_NO_THROW(pos->processCallbacks(arrays[i]));
    pos->unlock();
    // Check the callback pointer has been updated
    BOOST_CHECK_EQUAL(callbackCount, i+1);
    // Verify the attributes are attached
    NDArray *arrayPtr = (NDArray *)cbPtr;
    NDAttribute *aPtr;
    int val;
//    aPtr =  arrays[i]->pAttributeList->find("n");
    aPtr =  arrayPtr->pAttributeList->find("n");
    BOOST_CHECK_EQUAL(aPtr->getName(), "n");
    BOOST_CHECK_NO_THROW(aPtr->getValue(NDAttrInt32, &val));
    BOOST_CHECK_EQUAL(val, nvals[i]);
    aPtr =  arrayPtr->pAttributeList->find("x");
    BOOST_CHECK_EQUAL(aPtr->getName(), "x");
    BOOST_CHECK_NO_THROW(aPtr->getValue(NDAttrInt32, &val));
    BOOST_CHECK_EQUAL(val, xvals[i]);
    aPtr =  arrayPtr->pAttributeList->find("y");
    BOOST_CHECK_EQUAL(aPtr->getName(), "y");
    BOOST_CHECK_NO_THROW(aPtr->getValue(NDAttrInt32, &val));
    BOOST_CHECK_EQUAL(val, yvals[i]);

    // Verify qty of points decreases, should be 23 - i
    BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), (23-i));
    // Verify index stays at zero
    BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentIndex), 0);
  }

  // Verify qty of points is 14
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 14);

  // Set mode to keep
  BOOST_CHECK_NO_THROW(pos->write(str_NDPos_Mode, 1));

  // Fire through arrays, verify points attached as expected
  // Send some arrays to the plugin
  for (int i = 10; i < 20; i++)
  {
    arrays[i]->uniqueId = i;
    pos->lock();
    BOOST_CHECK_NO_THROW(pos->processCallbacks(arrays[i]));
    pos->unlock();

    // Check the callback pointer has been updated
    BOOST_CHECK_EQUAL(callbackCount, i+1);
    // Verify the attributes are attached
    NDArray *arrayPtr = (NDArray *)cbPtr;
    NDAttribute *aPtr;
    int val;
    aPtr =  arrayPtr->pAttributeList->find("n");
    BOOST_CHECK_EQUAL(aPtr->getName(), "n");
    BOOST_CHECK_NO_THROW(aPtr->getValue(NDAttrInt32, &val));
    BOOST_CHECK_EQUAL(val, nvals[i]);
    aPtr =  arrayPtr->pAttributeList->find("x");
    BOOST_CHECK_EQUAL(aPtr->getName(), "x");
    BOOST_CHECK_NO_THROW(aPtr->getValue(NDAttrInt32, &val));
    BOOST_CHECK_EQUAL(val, xvals[i]);
    aPtr =  arrayPtr->pAttributeList->find("y");
    BOOST_CHECK_EQUAL(aPtr->getName(), "y");
    BOOST_CHECK_NO_THROW(aPtr->getValue(NDAttrInt32, &val));
    BOOST_CHECK_EQUAL(val, yvals[i]);

    // Verify qty of points does not decrease, should be 14
    BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 14);
    // Verify index increases
    BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentIndex), (i-9));
  }

  // Issue reset, verify index reset to zero
  BOOST_CHECK_NO_THROW(pos->write(str_NDPos_Restart, 1));
  // Verify index is set to 0
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentIndex), 0);
  // Verify quantity of points is still 14
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 14);

  // Issue delete, verify points cleared out
  BOOST_CHECK_NO_THROW(pos->write(str_NDPos_Delete, 1));
  // Verify index is set to 0
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentIndex), 0);
  // Verify quantity of points is set to zero
  BOOST_CHECK_EQUAL(pos->readInt(str_NDPos_CurrentQty), 0);
}

BOOST_AUTO_TEST_SUITE_END()
