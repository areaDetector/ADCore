/*
 * test_NDPluginOverlay.cpp
 *
 *  Created on: 26 Nov 2016
 *      Author: Mark Rivers
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
#include "OverlayPluginWrapper.h"
#include "AsynException.h"


static int callbackCount = 0;
static void *cbPtr = 0;

void Overlay_callback(void *userPvt, asynUser *pasynUser, void *pointer)
{
  cbPtr = pointer;
  callbackCount++;
}

// We define 2 structures here.  
// overlayTempCaseStr uses fixed length arrays so it can be easily initialized
// and we need to initialize many cases
// overlayTestCaseStr used std::vector because that is what the test harness wants
 
typedef struct {
  int overlayNum;
  int positionX;
  int positionY;
  int sizeX;
  int sizeY;
  int widthX;
  int widthY;
  int red;
  int green;
  int blue;
  NDOverlayShape_t shape;
  NDOverlayDrawMode_t drawMode;
  int rank;
  size_t arrayDims[ND_ARRAY_MAX_DIMS];
  NDColorMode_t colorMode;
} overlayTempCaseStr ;

typedef struct {
  int overlayNum;
  int positionX;
  int positionY;
  int sizeX;
  int sizeY;
  int widthX;
  int widthY;
  int red;
  int green;
  int blue;
  NDOverlayShape_t shape;
  NDOverlayDrawMode_t drawMode;
  int rank;
  std::vector<size_t> arrayDims;
  std::vector<NDArray*> pArrays;
  NDColorMode_t colorMode;
} overlayTestCaseStr ;


static void appendTestCase(std::vector<overlayTestCaseStr> *pOut, overlayTempCaseStr *pIn)
{
  overlayTestCaseStr tmp;

  tmp.overlayNum = pIn->overlayNum;
  tmp.positionX  = pIn->positionX;
  tmp.positionY  = pIn->positionY;
  tmp.sizeX      = pIn->sizeX;
  tmp.sizeY      = pIn->sizeY;
  tmp.widthX     = pIn->widthX;
  tmp.widthY     = pIn->widthY;
  tmp.red        = pIn->red;
  tmp.green      = pIn->green;
  tmp.blue       = pIn->blue;
  tmp.shape      = pIn->shape;
  tmp.drawMode   = pIn->drawMode;
  tmp.rank       = pIn->rank;
  tmp.arrayDims.assign (pIn->arrayDims, pIn->arrayDims + pIn->rank);
  tmp.colorMode  = pIn->colorMode;

  tmp.pArrays.resize(1);
  fillNDArrays(tmp.arrayDims, NDFloat32, tmp.pArrays);
  pOut->push_back(tmp);
}

struct OverlayPluginTestFixture
{
  NDArrayPool *arrayPool;
  boost::shared_ptr<asynPortDriver> driver;
  boost::shared_ptr<OverlayPluginWrapper> Overlay;
  boost::shared_ptr<asynGenericPointerClient> client;
  TestingPlugin* downstream_plugin; // TODO: we don't put this in a shared_ptr and purposefully leak memory because asyn ports cannot be deleted
  std::vector<overlayTestCaseStr> overlayTestCaseStrs;
  int expectedArrayCounter;
  

  static int testCase;

  OverlayPluginTestFixture()
  {
    arrayPool = new NDArrayPool(100, 0);
    expectedArrayCounter=0;

    // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers
    // (even if only one is ever instantiated at once), so we change it slightly for each test case.
    std::string simport("simOVER1"), testport("OVER1");
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
    Overlay = boost::shared_ptr<OverlayPluginWrapper>(new OverlayPluginWrapper(testport.c_str(),
                                                                      50,
                                                                      1,
                                                                      simport.c_str(),
                                                                      0,
                                                                      8,
                                                                      0,
                                                                      0,
                                                                      2000000));
    // This is the mock downstream plugin
    downstream_plugin = new TestingPlugin(testport.c_str(), 0);

    // Enable the plugin
    Overlay->start(); // start the plugin thread although not required for this unittesting
    Overlay->write(NDPluginDriverEnableCallbacksString, 1);
    Overlay->write(NDPluginDriverBlockingCallbacksString, 1);

    client = boost::shared_ptr<asynGenericPointerClient>(new asynGenericPointerClient(testport.c_str(), 0, NDArrayDataString));
    client->registerInterruptUser(&Overlay_callback);

    // Test a "normal" case
    overlayTempCaseStr test1 = {0, 500, 500, 50, 50, 1, 1, 0, 255, 0, NDOverlayCross, NDOverlaySet, 2, {1024, 1024}, NDColorModeMono};
    appendTestCase(&overlayTestCaseStrs, &test1); 
    // Test a case with size larger than array
    overlayTempCaseStr test2 = {0, 500, 500, 5000, 5000, 1, 1, 0, 255, 0, NDOverlayEllipse, NDOverlaySet, 2, {1024, 1024}, NDColorModeMono};
    appendTestCase(&overlayTestCaseStrs, &test2); 
    // Test a case with zero size
    overlayTempCaseStr test3 = {0, 500, 500, 0, 0, 1, 1, 0, 255, 0, NDOverlayRectangle, NDOverlaySet, 2, {1024, 1024}, NDColorModeMono};
    appendTestCase(&overlayTestCaseStrs, &test3); 
    // Test a case with negative position
    overlayTempCaseStr test4 = {0, -500, -500, 50, 50, 1, 1, 0, 255, 0, NDOverlayCross, NDOverlaySet, 2, {1024, 1024}, NDColorModeMono};
    appendTestCase(&overlayTestCaseStrs, &test4); 
    // Test a case with position larger than array size
    overlayTempCaseStr test5 = {0, 1500, 1500, 50, 50, 1, 1, 0, 255, 0, NDOverlayEllipse, NDOverlaySet, 2, {1024, 1024}, NDColorModeMono};
    appendTestCase(&overlayTestCaseStrs, &test5); 
    // Test an RGB1 case 
    overlayTempCaseStr test6 = {0, 500, 500, 50, 50, 1, 1, 0, 255, 0, NDOverlayRectangle, NDOverlaySet, 3, {3, 1024, 1024}, NDColorModeRGB1};
    appendTestCase(&overlayTestCaseStrs, &test6); 
    // Test an case with very large width
    overlayTempCaseStr test7 = {0, 500, 500, 50, 50, 5000, 5000, 0, 255, 0, NDOverlayCross, NDOverlaySet, 2, {1024, 1024}, NDColorModeMono};
    appendTestCase(&overlayTestCaseStrs, &test7); 
    // Test an case with zero width
    overlayTempCaseStr test8 = {0, 500, 500, 50, 50, 0, 0, 0, 255, 0, NDOverlayCross, NDOverlaySet, 2, {1024, 1024}, NDColorModeMono};
    appendTestCase(&overlayTestCaseStrs, &test8);
    // Test a "normal" case using address 1
    overlayTempCaseStr test9 = {1, 500, 500, 50, 50, 1, 1, 0, 255, 0, NDOverlayCross, NDOverlaySet, 2, {1024, 1024}, NDColorModeMono};
    appendTestCase(&overlayTestCaseStrs, &test9);
  }

  ~OverlayPluginTestFixture()
  {
    delete arrayPool;
    client.reset();
    Overlay.reset();
    driver.reset();
    //delete downstream_plugin; // TODO: We can't delete a TestingPlugin because it tries to delete an asyn port which doesnt work
  }

};

BOOST_FIXTURE_TEST_SUITE(OverlayPluginTests, OverlayPluginTestFixture)


BOOST_AUTO_TEST_CASE(validate_input_NDArrays)
{
  BOOST_MESSAGE("Validating the input NDArrays " <<
                "- if any of these fail the test-cases are also likely to fail");

  overlayTestCaseStr *pStr = &overlayTestCaseStrs[0];
  for (size_t i=0; i<overlayTestCaseStrs.size(); i++, pStr++)  {
    for (size_t dim=0; dim<pStr->arrayDims.size(); dim++) {
       BOOST_REQUIRE_EQUAL(pStr->arrayDims[dim], pStr->pArrays[0]->dims[dim].size);
    }
    BOOST_REQUIRE_EQUAL(pStr->pArrays[0]->dataType, NDFloat32);
    BOOST_REQUIRE_EQUAL(pStr->arrayDims.size(), pStr->pArrays[0]->ndims);
  }
}


BOOST_AUTO_TEST_CASE(basic_overlay_operation)
{

  overlayTestCaseStr *pStr = &overlayTestCaseStrs[0];
  for (size_t i=0; i<overlayTestCaseStrs.size(); i++, pStr++)  {
  
    BOOST_MESSAGE("Test " << (i+1) << " rank: " << pStr->rank);

    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayUseString,       1,               pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayPositionXString, pStr->positionX, pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayPositionYString, pStr->positionY, pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlaySizeXString,     pStr->sizeX,     pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlaySizeYString,     pStr->sizeY,     pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayWidthXString,    pStr->widthX,    pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayWidthYString,    pStr->widthY,    pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayShapeString,     pStr->shape,     pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayDrawModeString,  pStr->drawMode,  pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayRedString,       pStr->red,       pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayGreenString,     pStr->green,     pStr->overlayNum));
    BOOST_CHECK_NO_THROW(Overlay->write(NDPluginOverlayBlueString,      pStr->blue,      pStr->overlayNum));

    BOOST_CHECK_NO_THROW(Overlay->write(NDArrayCallbacksString, 1));
    BOOST_CHECK_EQUAL(Overlay->readInt(NDArrayCallbacksString), 1);

    Overlay->lock();
    BOOST_CHECK_NO_THROW(Overlay->processCallbacks(pStr->pArrays[0]));
    Overlay->unlock();
    expectedArrayCounter++;  
    BOOST_CHECK_EQUAL(Overlay->readInt("ARRAY_COUNTER"), expectedArrayCounter);

    // Check the downstream receiver of the Overlay array
    BOOST_REQUIRE_EQUAL(downstream_plugin->arrays.size(), expectedArrayCounter);
  }
}


BOOST_AUTO_TEST_SUITE_END() // Done!
