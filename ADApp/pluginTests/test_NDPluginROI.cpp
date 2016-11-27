/*
 * test_NDPluginROI.cpp
 *
 *  Created on: 9 Nov 2016
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
#include "ROIPluginWrapper.h"
#include "AsynException.h"


static int callbackCount = 0;
static void *cbPtr = 0;

void ROI_callback(void *userPvt, asynUser *pasynUser, void *pointer)
{
  cbPtr = pointer;
  callbackCount++;
}

// We define 2 structures here.  
// ROITempCaseStr uses fixed length arrays so it can be easily initialized
// and we need to initialize many cases
// ROITestCaseStr used std::vector because that is what the test harness wants
 
typedef struct {
  int inputRank;
  size_t inputDims[ND_ARRAY_MAX_DIMS];
  int roiStart[ND_ARRAY_MAX_DIMS];
  int roiSize[ND_ARRAY_MAX_DIMS];
  int outputRankNormal;
  size_t outputDimsNormal[ND_ARRAY_MAX_DIMS];
  int outputRankCollapse;
  size_t outputDimsCollapse[ND_ARRAY_MAX_DIMS];
} ROITempCaseStr ;

typedef struct {
  int inputRank;
  int outputRankNormal;
  int outputRankCollapse;
  std::vector<size_t> inputDims;
  std::vector<int> roiStart;
  std::vector<int> roiSize;
  std::vector<size_t> outputDimsNormal;
  std::vector<size_t> outputDimsCollapse;
  std::vector<NDArray*> pArrays;
} ROITestCaseStr ;

static void appendTestCase(std::vector<ROITestCaseStr> *pOut, ROITempCaseStr *pIn)
{
  ROITestCaseStr tmp;
  tmp.inputRank          = pIn->inputRank;
  tmp.outputRankNormal   = pIn->outputRankNormal;
  tmp.outputRankCollapse = pIn->outputRankCollapse;
  tmp.inputDims.assign         (pIn->inputDims,          pIn->inputDims          + pIn->inputRank);
  tmp.roiStart.assign          (pIn->roiStart,           pIn->roiStart           + pIn->inputRank);
  tmp.roiSize.assign           (pIn->roiSize,            pIn->roiSize            + pIn->inputRank);
  tmp.outputDimsNormal.assign  (pIn->outputDimsNormal,   pIn->outputDimsNormal   + pIn->outputRankNormal);
  tmp.outputDimsCollapse.assign(pIn->outputDimsCollapse, pIn->outputDimsCollapse + pIn->outputRankCollapse);

  tmp.pArrays.resize(1);
  fillNDArrays(tmp.inputDims, NDFloat32, tmp.pArrays);
  pOut->push_back(tmp);
}

struct ROIPluginTestFixture
{
  NDArrayPool *arrayPool;
  boost::shared_ptr<asynPortDriver> driver;
  boost::shared_ptr<ROIPluginWrapper> roi;
  boost::shared_ptr<asynGenericPointerClient> client;
  TestingPlugin* downstream_plugin; // TODO: we don't put this in a shared_ptr and purposefully leak memory because asyn ports cannot be deleted
  std::vector<ROITestCaseStr> ROITestCaseStrs;
  int expectedArrayCounter;
  

  static int testCase;

  ROIPluginTestFixture()
  {
    arrayPool = new NDArrayPool(100, 0);
    expectedArrayCounter=0;

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
    roi = boost::shared_ptr<ROIPluginWrapper>(new ROIPluginWrapper(testport.c_str(),
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
    roi->start(); // start the plugin thread although not required for this unittesting
    roi->write(NDPluginDriverEnableCallbacksString, 1);
    roi->write(NDPluginDriverBlockingCallbacksString, 1);

    client = boost::shared_ptr<asynGenericPointerClient>(new asynGenericPointerClient(testport.c_str(), 0, NDArrayDataString));
    client->registerInterruptUser(&ROI_callback);

    ROITempCaseStr test1 = {2, {10,10},    {0,0},   {1, 10},   2, {1, 10},   1, {10}};
    appendTestCase(&ROITestCaseStrs, &test1); 
    ROITempCaseStr test2 = {3, {10,10,10}, {0,0,0}, {10,1,10}, 3, {10,1,10}, 2, {10,10}};
    appendTestCase(&ROITestCaseStrs, &test2); 
    ROITempCaseStr test3 = {3, {10,10,10}, {0,0,0}, {1,1,10},  3, {1,1,10},  1, {10}};
    appendTestCase(&ROITestCaseStrs, &test3); 
    ROITempCaseStr test4 = {1, {1},        {0},     {1},       1, {1},       1, {1}};
    appendTestCase(&ROITestCaseStrs, &test4); 
    ROITempCaseStr test5 = {3, {10,20,30}, {0,0,0}, {5,1,1},   3, {5,1,1},   1, {5}};
    appendTestCase(&ROITestCaseStrs, &test5); 

}

  ~ROIPluginTestFixture()
  {
    delete arrayPool;
    client.reset();
    roi.reset();
    driver.reset();
    //delete downstream_plugin; // TODO: We can't delete a TestingPlugin because it tries to delete an asyn port which doesnt work
  }

};

BOOST_FIXTURE_TEST_SUITE(ROIPluginTests, ROIPluginTestFixture)


BOOST_AUTO_TEST_CASE(validate_input_NDArrays)
{
  BOOST_MESSAGE("Validating the input NDArrays " <<
                "- if any of these fail the test-cases are also likely to fail");

  ROITestCaseStr *pStr = &ROITestCaseStrs[0];
  for (size_t i=0; i<ROITestCaseStrs.size(); i++, pStr++)  {
    for (size_t dim=0; dim<pStr->inputDims.size(); dim++) {
       BOOST_REQUIRE_EQUAL(pStr->inputDims[dim], pStr->pArrays[0]->dims[dim].size);
    }
    BOOST_REQUIRE_EQUAL(pStr->pArrays[0]->dataType, NDFloat32);
    BOOST_REQUIRE_EQUAL(pStr->inputDims.size(), pStr->pArrays[0]->ndims);
  }
}


BOOST_AUTO_TEST_CASE(basic_roi_operation)
{

  ROITestCaseStr *pStr = &ROITestCaseStrs[0];
  for (size_t i=0; i<ROITestCaseStrs.size(); i++, pStr++)  {
  
    BOOST_MESSAGE("Test " << (i+1) << " input rank: " << pStr->inputRank);

    if (pStr->inputRank > 0) {
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim0MinString,      pStr->roiStart[0]));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim0SizeString,     pStr->roiSize[0]));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim0EnableString,   1));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim0BinString,      1));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim0ReverseString,  0));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim0AutoSizeString, 0));
   }
    if (pStr->inputRank > 1) {
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim1MinString,      pStr->roiStart[1]));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim1SizeString,     pStr->roiSize[1]));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim1EnableString,   1));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim1BinString,      1));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim1ReverseString,  0));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim1AutoSizeString, 0));
    }
    if (pStr->inputRank > 2) {
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim2MinString,      pStr->roiStart[2]));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim2SizeString,     pStr->roiSize[2]));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim2EnableString,   1));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim2BinString,      1));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim2ReverseString,  0));
      BOOST_CHECK_NO_THROW(roi->write(NDPluginROIDim2AutoSizeString, 0));
    }
    BOOST_CHECK_NO_THROW(roi->write(NDArrayCallbacksString, 1));
    BOOST_CHECK_EQUAL(roi->readInt(NDArrayCallbacksString), 1);

    // Process array through the ROI plugin with CollapseDims disabled.
    BOOST_CHECK_NO_THROW(roi->write(NDPluginROICollapseDimsString, 0));
    roi->lock();
    BOOST_CHECK_NO_THROW(roi->processCallbacks(pStr->pArrays[0]));
    roi->unlock();
    expectedArrayCounter++;  
    BOOST_CHECK_EQUAL(roi->readInt("ARRAY_COUNTER"), expectedArrayCounter);

    // Check the downstream receiver of the ROI array
    BOOST_MESSAGE("  expected normal output rank: " << pStr->outputRankNormal
                  << " actual: " << downstream_plugin->arrays[0]->ndims);

    BOOST_REQUIRE_EQUAL(downstream_plugin->arrays.size(), expectedArrayCounter);
//    BOOST_REQUIRE_EQUAL(downstream_plugin->arrays[0]->ndims, pStr->outputRankNormal);
    for (int j=0; j<pStr->outputRankNormal; j++) {
      BOOST_MESSAGE("    expected normal dimension " << j 
                    << " size:"   <<  pStr->outputDimsNormal[j]
                    << " actual:" << downstream_plugin->arrays[0]->dims[j].size);
//      BOOST_REQUIRE_EQUAL(downstream_plugin->arrays[0]->dims[j].size, pStr->outputDimsNormal[j]);
    }

    // Process array through the ROI plugin with CollapseDims enabled.
    BOOST_CHECK_NO_THROW(roi->write(NDPluginROICollapseDimsString, 1));
    roi->lock();
    BOOST_CHECK_NO_THROW(roi->processCallbacks(pStr->pArrays[0]));
    roi->unlock();
    expectedArrayCounter++;  
    BOOST_CHECK_EQUAL(roi->readInt("ARRAY_COUNTER"), expectedArrayCounter);

    // Check the downstream receiver of the ROI array
    BOOST_MESSAGE("  expected collapsed output rank: " << pStr->outputRankCollapse
                  << " actual: " << downstream_plugin->arrays[0]->ndims);
    BOOST_REQUIRE_EQUAL(downstream_plugin->arrays.size(), expectedArrayCounter);
    BOOST_REQUIRE_EQUAL(downstream_plugin->arrays[0]->ndims, pStr->outputRankCollapse);
    for (int j=0; j<pStr->outputRankCollapse; j++) {
      BOOST_MESSAGE("    expected collapsed dimension " << j 
                    << " size:"   <<  pStr->outputDimsCollapse[j]
                    << " actual:" << downstream_plugin->arrays[0]->dims[j].size);
      BOOST_REQUIRE_EQUAL(downstream_plugin->arrays[0]->dims[j].size, pStr->outputDimsCollapse[j]);
    }
  }
}


BOOST_AUTO_TEST_SUITE_END() // Done!
