/*
 * test_NDPluginBadPixel.cpp
 *
 *  Created on: 06 Jul 2026
 *      Author: Jakub Wlodek
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
#include <functional>
using namespace std;

#include "testingutilities.h"
#include "BadPixelPluginWrapper.h"
#include "AsynException.h"


static int callbackCount = 0;
static void *cbPtr = 0;

void BadPixel_callback(void *userPvt, asynUser *pasynUser, void *pointer)
{
  cbPtr = pointer;
  callbackCount++;
}

struct BadPixelPluginTestFixture
{
  boost::shared_ptr<asynNDArrayDriver> driver;
  boost::shared_ptr<BadPixelPluginWrapper> badPixel;
  boost::shared_ptr<asynGenericPointerClient> client;
  TestingPlugin* downstream_plugin;
  NDArrayPool *arrayPool;
  int expectedArrayCounter;
  std::vector<std::string> tmpFiles;
  int fileCounter;

  static int testCase;

  BadPixelPluginTestFixture()
  {
    expectedArrayCounter = 0;
    callbackCount = 0;
    cbPtr = 0;
    fileCounter = 0;

    std::string simport("simBP"), testport("BP");
    uniqueAsynPortName(simport);
    uniqueAsynPortName(testport);

    driver = boost::shared_ptr<asynNDArrayDriver>(new asynNDArrayDriver(simport.c_str(),
                                                                     1, 0, 0,
                                                                     asynGenericPointerMask,
                                                                     asynGenericPointerMask,
                                                                     0, 0, 0, 0));
    arrayPool = driver->pNDArrayPool;

    badPixel = boost::shared_ptr<BadPixelPluginWrapper>(new BadPixelPluginWrapper(testport,
                                                                      50, 1, simport,
                                                                      0, 0, 0, 0, 1));

    downstream_plugin = new TestingPlugin(testport.c_str(), 0);

    badPixel->start();
    badPixel->write(NDPluginDriverEnableCallbacksString, 1);
    badPixel->write(NDPluginDriverBlockingCallbacksString, 1);

    client = boost::shared_ptr<asynGenericPointerClient>(new asynGenericPointerClient(testport.c_str(), 0, NDArrayDataString));
    client->registerInterruptUser(&BadPixel_callback);
  }

  ~BadPixelPluginTestFixture()
  {
    client.reset();
    badPixel.reset();
    driver.reset();
    for (auto it = tmpFiles.rbegin(); it != tmpFiles.rend(); ++it) {
      remove(it->c_str());
    }
  }

  std::string writeJsonToFile(const std::string& jsonStr)
  {
    std::string filename = "test_badpixel_" + std::to_string(fileCounter++) + ".json";
    std::ofstream file(filename.c_str());
    file << jsonStr;
    file.close();
    tmpFiles.push_back(filename);
    return filename;
  }

  void loadFromString(const std::string& jsonStr) {
    badPixel->write(NDPluginBadPixelFileNameString, jsonStr);
  }

  void loadFromFile(const std::string& jsonStr) {
    std::string filePath = writeJsonToFile(jsonStr);
    badPixel->write(NDPluginBadPixelFileNameString, filePath);
  }

  typedef void (BadPixelPluginTestFixture::*LoadMethod)(const std::string&);

  // Parametrize test to run with both loadFromString and loadFromFile methods
  void forEachLoadMethod(std::function<void(LoadMethod)> body)
  {
    LoadMethod methods[] = {
      &BadPixelPluginTestFixture::loadFromString,
      &BadPixelPluginTestFixture::loadFromFile
    };
    for (int i = 0; i < 2; i++) {
      body(methods[i]);
    }
  }

  epicsFloat64* processAndGetOutput(NDArray *pArray)
  {
    badPixel->lock();
    badPixel->processCallbacks(pArray);
    badPixel->unlock();
    return (epicsFloat64 *)downstream_plugin->arrays.back()->pData;
  }

  NDArray* createSequentialArray(size_t xSize, size_t ySize)
  {
    size_t dims[2] = {xSize, ySize};
    NDArray *pArray = arrayPool->alloc(2, dims, NDFloat64, 0, 0);
    epicsFloat64 *pData = (epicsFloat64 *)pArray->pData;
    for (size_t i = 0; i < xSize * ySize; i++) pData[i] = (epicsFloat64)i;
    return pArray;
  }

  NDArray* createUniformArray(size_t xSize, size_t ySize, double value)
  {
    size_t dims[2] = {xSize, ySize};
    NDArray *pArray = arrayPool->alloc(2, dims, NDFloat64, 0, 0);
    epicsFloat64 *pData = (epicsFloat64 *)pArray->pData;
    for (size_t i = 0; i < xSize * ySize; i++) pData[i] = (epicsFloat64)value;
    return pArray;
  }

  // Template helpers for data type parametrized tests
  template <typename T>
  void verifySetMode(NDDataType_t dtype, double setValue)
  {
    loadFromString("{\"Bad pixels\": [{\"Pixel\": [3, 2], \"Set\": " + std::to_string(setValue) + "}]}");
    size_t dims[2] = {10, 10};
    NDArray *p = arrayPool->alloc(2, dims, dtype, 0, 0);
    T *d = (T *)p->pData;
    for (int i = 0; i < 100; i++) d[i] = (T)i;
    badPixel->lock(); badPixel->processCallbacks(p); badPixel->unlock();
    BOOST_CHECK_EQUAL(((T *)downstream_plugin->arrays.back()->pData)[23], (T)setValue);
    p->release();
  }

  template <typename T>
  void verifyReplaceMode(NDDataType_t dtype)
  {
    loadFromString("{\"Bad pixels\": [{\"Pixel\": [5, 5], \"Replace\": [1, 0]}]}");
    size_t dims[2] = {10, 10};
    NDArray *p = arrayPool->alloc(2, dims, dtype, 0, 0);
    T *d = (T *)p->pData;
    for (int i = 0; i < 100; i++) d[i] = (T)(i * 10);
    badPixel->lock(); badPixel->processCallbacks(p); badPixel->unlock();
    // (5,5)->offset=55, replaced by (6,5)->offset=56, value=560
    BOOST_CHECK_EQUAL(((T *)downstream_plugin->arrays.back()->pData)[55], (T)560);
    p->release();
  }

  template <typename T>
  void verifyMedianMode(NDDataType_t dtype)
  {
    loadFromString("{\"Bad pixels\": [{\"Pixel\": [5, 5], \"Median\": [1, 1]}]}");
    size_t dims[2] = {10, 10};
    NDArray *p = arrayPool->alloc(2, dims, dtype, 0, 0);
    T *d = (T *)p->pData;
    for (int i = 0; i < 100; i++) d[i] = (T)i;
    d[55] = (T)9999;
    badPixel->lock(); badPixel->processCallbacks(p); badPixel->unlock();
    // Neighbors: 44,45,46,54,56,64,65,66 -> median=(54+56)/2=55
    BOOST_CHECK_EQUAL(((T *)downstream_plugin->arrays.back()->pData)[55], (T)55);
    p->release();
  }
};


// Test helper function to get value from json object given multiple
// possible keys, returning the first match found.
BOOST_AUTO_TEST_SUITE(FindJsonKeyTests)

BOOST_AUTO_TEST_CASE(finds_first_matching_key)
{
  json obj = {{"Alpha", 1}, {"beta", 2}};
  auto it = findJsonKey(obj, {"Alpha"});
  BOOST_REQUIRE(it != obj.end());
  BOOST_CHECK_EQUAL(*it, 1);
}

BOOST_AUTO_TEST_CASE(finds_second_option_when_first_missing)
{
  json obj = {{"beta", 42}};
  auto it = findJsonKey(obj, {"Alpha", "beta"});
  BOOST_REQUIRE(it != obj.end());
  BOOST_CHECK_EQUAL(*it, 42);
}

BOOST_AUTO_TEST_CASE(finds_third_option)
{
  json obj = {{"gamma", 99}};
  auto it = findJsonKey(obj, {"Alpha", "beta", "gamma"});
  BOOST_REQUIRE(it != obj.end());
  BOOST_CHECK_EQUAL(*it, 99);
}

BOOST_AUTO_TEST_CASE(returns_end_when_no_match)
{
  json obj = {{"foo", 1}, {"bar", 2}};
  BOOST_CHECK(findJsonKey(obj, {"baz", "qux"}) == obj.end());
}

BOOST_AUTO_TEST_CASE(returns_end_for_empty_keys_list)
{
  json obj = {{"foo", 1}};
  BOOST_CHECK(findJsonKey(obj, {}) == obj.end());
}

BOOST_AUTO_TEST_CASE(priority_returns_first_match)
{
  json obj = {{"A", 10}, {"a", 20}};
  auto it = findJsonKey(obj, {"A", "a"});
  BOOST_REQUIRE(it != obj.end());
  BOOST_CHECK_EQUAL(*it, 10);
}

BOOST_AUTO_TEST_SUITE_END()


// Tests parsing bad pixel json into badPixelList_t objects
BOOST_FIXTURE_TEST_SUITE(BadPixelParsingTests, BadPixelPluginTestFixture)

BOOST_AUTO_TEST_CASE(parse_set_mode)
{
  badPixelList_t result = badPixel->testParseBadPixelList("{\"Bad pixels\": [{\"Pixel\": [3, 2], \"Set\": 42.0}]}");
  BOOST_REQUIRE_EQUAL(result.size(), (size_t)1);
  auto it = result.begin();
  BOOST_CHECK_EQUAL(it->coordinate.x, 3);
  BOOST_CHECK_EQUAL(it->coordinate.y, 2);
  BOOST_CHECK_EQUAL(it->mode, badPixelModeSet);
  BOOST_CHECK_EQUAL(it->setValue, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_replace_mode)
{
  badPixelList_t result = badPixel->testParseBadPixelList("{\"Bad pixels\": [{\"Pixel\": [5, 7], \"Replace\": [1, -1]}]}");
  BOOST_REQUIRE_EQUAL(result.size(), (size_t)1);
  auto it = result.begin();
  BOOST_CHECK_EQUAL(it->mode, badPixelModeReplace);
  BOOST_CHECK_EQUAL(it->replaceCoordinate.x, 1);
  BOOST_CHECK_EQUAL(it->replaceCoordinate.y, -1);
}

BOOST_AUTO_TEST_CASE(parse_median_mode)
{
  badPixelList_t result = badPixel->testParseBadPixelList("{\"Bad pixels\": [{\"Pixel\": [4, 4], \"Median\": [2, 3]}]}");
  BOOST_REQUIRE_EQUAL(result.size(), (size_t)1);
  BOOST_CHECK_EQUAL(result.begin()->mode, badPixelModeMedian);
  BOOST_CHECK_EQUAL(result.begin()->medianCoordinate.x, 2);
  BOOST_CHECK_EQUAL(result.begin()->medianCoordinate.y, 3);
}

BOOST_AUTO_TEST_CASE(parse_multiple_mixed_modes)
{
  badPixelList_t result = badPixel->testParseBadPixelList(
    "{\"Bad pixels\": ["
    "{\"Pixel\": [1, 1], \"Set\": 0.0},"
    "{\"Pixel\": [2, 3], \"Replace\": [-1, 0]},"
    "{\"Pixel\": [5, 5], \"Median\": [1, 1]}]}");
  BOOST_REQUIRE_EQUAL(result.size(), (size_t)3);
  auto it = result.begin();
  BOOST_CHECK_EQUAL(it->mode, badPixelModeSet); ++it;
  BOOST_CHECK_EQUAL(it->mode, badPixelModeReplace); ++it;
  BOOST_CHECK_EQUAL(it->mode, badPixelModeMedian);
}

BOOST_AUTO_TEST_CASE(parse_empty_list)
{
  BOOST_CHECK_EQUAL(badPixel->testParseBadPixelList("{\"Bad pixels\": []}").size(), (size_t)0);
}

BOOST_AUTO_TEST_CASE(parse_duplicate_coordinates_deduplicates)
{
  badPixelList_t result = badPixel->testParseBadPixelList(
    "{\"Bad pixels\": [{\"Pixel\": [3, 3], \"Set\": 1.0}, {\"Pixel\": [3, 3], \"Set\": 2.0}]}");
  BOOST_CHECK_EQUAL(result.size(), (size_t)1);
}

BOOST_AUTO_TEST_CASE(parse_ordering_by_y_then_x)
{
  badPixelList_t result = badPixel->testParseBadPixelList(
    "{\"Bad pixels\": ["
    "{\"Pixel\": [9, 0], \"Set\": 0},"
    "{\"Pixel\": [0, 9], \"Set\": 0},"
    "{\"Pixel\": [5, 5], \"Set\": 0},"
    "{\"Pixel\": [0, 0], \"Set\": 0}]}");
  BOOST_REQUIRE_EQUAL(result.size(), (size_t)4);
  auto it = result.begin();
  BOOST_CHECK_EQUAL(it->coordinate.y, 0); BOOST_CHECK_EQUAL(it->coordinate.x, 0); ++it;
  BOOST_CHECK_EQUAL(it->coordinate.y, 0); BOOST_CHECK_EQUAL(it->coordinate.x, 9); ++it;
  BOOST_CHECK_EQUAL(it->coordinate.y, 5); ++it;
  BOOST_CHECK_EQUAL(it->coordinate.y, 9);
}

BOOST_AUTO_TEST_CASE(parse_mode_priority_last_wins)
{
  badPixelList_t result = badPixel->testParseBadPixelList(
    "{\"Bad pixels\": [{\"Pixel\": [1, 1], \"Median\": [1, 1], \"Set\": 5.0, \"Replace\": [0, 1]}]}");
  BOOST_CHECK_EQUAL(result.begin()->mode, badPixelModeReplace);
}

BOOST_AUTO_TEST_CASE(parse_lowercase_top_level_key)
{
  badPixelList_t result = badPixel->testParseBadPixelList("{\"bad_pixels\": [{\"Pixel\": [2, 3], \"Set\": 10.0}]}");
  BOOST_REQUIRE_EQUAL(result.size(), (size_t)1);
  BOOST_CHECK_EQUAL(result.begin()->setValue, 10.0);
}

BOOST_AUTO_TEST_CASE(parse_all_lowercase_keys)
{
  badPixelList_t result = badPixel->testParseBadPixelList("{\"bad_pixels\": [{\"pixel\": [4, 5], \"replace\": [-1, 1]}]}");
  BOOST_REQUIRE_EQUAL(result.size(), (size_t)1);
  BOOST_CHECK_EQUAL(result.begin()->mode, badPixelModeReplace);
  BOOST_CHECK_EQUAL(result.begin()->coordinate.x, 4);
}

BOOST_AUTO_TEST_CASE(parse_lowercase_individual_keys)
{
  BOOST_CHECK_EQUAL(
    badPixel->testParseBadPixelList("{\"Bad pixels\": [{\"pixel\": [1, 1], \"set\": 5.0}]}").begin()->mode,
    badPixelModeSet);
  BOOST_CHECK_EQUAL(
    badPixel->testParseBadPixelList("{\"Bad pixels\": [{\"pixel\": [1, 1], \"median\": [1, 1]}]}").begin()->mode,
    badPixelModeMedian);
  BOOST_CHECK_EQUAL(
    badPixel->testParseBadPixelList("{\"Bad pixels\": [{\"pixel\": [1, 1], \"replace\": [0, 1]}]}").begin()->mode,
    badPixelModeReplace);
}

BOOST_AUTO_TEST_CASE(parse_missing_top_level_key_returns_empty)
{
  BOOST_CHECK_EQUAL(
    badPixel->testParseBadPixelList("{\"wrong_key\": [{\"Pixel\": [1, 1], \"Set\": 0.0}]}").size(), (size_t)0);
}

BOOST_AUTO_TEST_CASE(handle_file_update_from_file)
{
  std::string filePath = writeJsonToFile("{\"Bad pixels\": [{\"Pixel\": [3, 4], \"Set\": 7.0}]}");
  BOOST_CHECK_EQUAL(badPixel->testHandleBadPixelFileUpdate(filePath.c_str()), asynSuccess);
  BOOST_CHECK_EQUAL(badPixel->getBadPixelList().begin()->setValue, 7.0);
}

BOOST_AUTO_TEST_CASE(handle_file_update_from_json_string)
{
  BOOST_CHECK_EQUAL(
    badPixel->testHandleBadPixelFileUpdate("{\"Bad pixels\": [{\"Pixel\": [6, 2], \"Median\": [2, 2]}]}"),
    asynSuccess);
  BOOST_CHECK_EQUAL(badPixel->getBadPixelList().begin()->mode, badPixelModeMedian);
}

BOOST_AUTO_TEST_CASE(handle_file_update_invalid_json)
{
  BOOST_CHECK_EQUAL(badPixel->testHandleBadPixelFileUpdate("not valid json"), asynError);
}

BOOST_AUTO_TEST_CASE(handle_file_update_nonexistent_file)
{
  BOOST_CHECK_EQUAL(badPixel->testHandleBadPixelFileUpdate("/nonexistent/path/file.json"), asynError);
}

BOOST_AUTO_TEST_CASE(handle_file_update_replaces_previous_list)
{
  badPixel->testHandleBadPixelFileUpdate("{\"Bad pixels\": [{\"Pixel\": [1, 1], \"Set\": 1.0}, {\"Pixel\": [2, 2], \"Set\": 2.0}]}");
  BOOST_CHECK_EQUAL(badPixel->getBadPixelList().size(), (size_t)2);
  badPixel->testHandleBadPixelFileUpdate("{\"Bad pixels\": [{\"Pixel\": [5, 5], \"Set\": 5.0}]}");
  BOOST_CHECK_EQUAL(badPixel->getBadPixelList().size(), (size_t)1);
}

BOOST_AUTO_TEST_SUITE_END()


// Tests for end-to-end processing of arrays with bad pixels, using both loadFromString and loadFromFile methods
BOOST_FIXTURE_TEST_SUITE(BadPixelPluginEndToEndTests, BadPixelPluginTestFixture)

BOOST_AUTO_TEST_CASE(passthrough_no_bad_pixels)
{
  NDArray *pArray = createSequentialArray(10, 10);
  epicsFloat64 *out = processAndGetOutput(pArray);
  for (size_t i = 0; i < 100; i++) BOOST_CHECK_EQUAL(out[i], (epicsFloat64)i);
  pArray->release();
}

BOOST_AUTO_TEST_CASE(set_mode)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [3, 2], \"Set\": 0.0}]}");
    NDArray *p = createSequentialArray(10, 10);
    epicsFloat64 *out = processAndGetOutput(p);
    BOOST_CHECK_EQUAL(out[23], 0.0);
    BOOST_CHECK_EQUAL(out[22], 22.0);
    BOOST_CHECK_EQUAL(out[24], 24.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(set_mode_specific_value)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [5, 5], \"Set\": 999.0}]}");
    NDArray *p = createSequentialArray(10, 10);
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[55], 999.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(replace_mode)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [3, 2], \"Replace\": [1, 0]}]}");
    NDArray *p = createSequentialArray(10, 10);
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[23], 24.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(replace_mode_negative_offset)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [5, 5], \"Replace\": [-1, -1]}]}");
    NDArray *p = createSequentialArray(10, 10);
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[55], 44.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(median_mode_uniform)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [5, 5], \"Median\": [1, 1]}]}");
    NDArray *p = createUniformArray(10, 10, 100.0);
    ((epicsFloat64 *)p->pData)[55] = 9999.0;
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[55], 100.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(median_mode_sequential)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [5, 5], \"Median\": [1, 1]}]}");
    NDArray *p = createSequentialArray(10, 10);
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[55], 55.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(median_5x5_kernel)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [5, 5], \"Median\": [2, 2]}]}");
    NDArray *p = createUniformArray(10, 10, 50.0);
    ((epicsFloat64 *)p->pData)[55] = 9999.0;
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[55], 50.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(multiple_bad_pixels)
{
  std::string j = "{\"Bad pixels\": ["
    "{\"Pixel\": [0, 0], \"Set\": -1.0},"
    "{\"Pixel\": [9, 9], \"Set\": -2.0},"
    "{\"Pixel\": [5, 3], \"Set\": -3.0}]}";
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)(j);
    NDArray *p = createSequentialArray(10, 10);
    epicsFloat64 *out = processAndGetOutput(p);
    BOOST_CHECK_EQUAL(out[0], -1.0);
    BOOST_CHECK_EQUAL(out[99], -2.0);
    BOOST_CHECK_EQUAL(out[35], -3.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(out_of_bounds_pixel_ignored)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [20, 20], \"Set\": 0.0}]}");
    NDArray *p = createSequentialArray(10, 10);
    epicsFloat64 *out = processAndGetOutput(p);
    for (size_t i = 0; i < 100; i++) BOOST_CHECK_EQUAL(out[i], (epicsFloat64)i);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(replace_with_bad_pixel_skipped)
{
  std::string j = "{\"Bad pixels\": ["
    "{\"Pixel\": [3, 2], \"Replace\": [1, 0]},"
    "{\"Pixel\": [4, 2], \"Set\": 0.0}]}";
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)(j);
    NDArray *p = createSequentialArray(10, 10);
    epicsFloat64 *out = processAndGetOutput(p);
    BOOST_CHECK_EQUAL(out[23], 23.0);
    BOOST_CHECK_EQUAL(out[24], 0.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(update_list_replaces_old)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [0, 0], \"Set\": -1.0}]}");
    NDArray *p1 = createSequentialArray(10, 10);
    BOOST_CHECK_EQUAL(processAndGetOutput(p1)[0], -1.0);

    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [1, 1], \"Set\": -5.0}]}");
    NDArray *p2 = createSequentialArray(10, 10);
    epicsFloat64 *out2 = processAndGetOutput(p2);
    BOOST_CHECK_EQUAL(out2[0], 0.0);
    BOOST_CHECK_EQUAL(out2[11], -5.0);
    p1->release();
    p2->release();
  });
}

BOOST_AUTO_TEST_CASE(invalid_json_throws)
{
  BOOST_CHECK_THROW(loadFromString("not valid json"), AsynException);
  std::string filePath = writeJsonToFile("not valid json");
  BOOST_CHECK_THROW(badPixel->write(NDPluginBadPixelFileNameString, filePath), AsynException);
}

BOOST_AUTO_TEST_CASE(binning)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [4, 4], \"Set\": 0.0}]}");
    size_t dims[2] = {5, 5};
    NDArray *p = arrayPool->alloc(2, dims, NDFloat64, 0, 0);
    epicsFloat64 *d = (epicsFloat64 *)p->pData;
    for (size_t i = 0; i < 25; i++) d[i] = (epicsFloat64)(i + 1);
    p->dims[0].binning = 2;
    p->dims[1].binning = 2;
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[12], 0.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(offset)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [5, 5], \"Set\": 0.0}]}");
    size_t dims[2] = {5, 5};
    NDArray *p = arrayPool->alloc(2, dims, NDFloat64, 0, 0);
    epicsFloat64 *d = (epicsFloat64 *)p->pData;
    for (size_t i = 0; i < 25; i++) d[i] = (epicsFloat64)(i + 100);
    p->dims[0].offset = 3;
    p->dims[1].offset = 3;
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[12], 0.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(one_dimensional_array)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [5, 0], \"Set\": -1.0}]}");
    size_t dims[1] = {10};
    NDArray *p = arrayPool->alloc(1, dims, NDFloat64, 0, 0);
    epicsFloat64 *d = (epicsFloat64 *)p->pData;
    for (size_t i = 0; i < 10; i++) d[i] = (epicsFloat64)i;
    epicsFloat64 *out = processAndGetOutput(p);
    BOOST_CHECK_EQUAL(out[5], -1.0);
    BOOST_CHECK_EQUAL(out[4], 4.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(mixed_modes)
{
  std::string j = "{\"Bad pixels\": ["
    "{\"Pixel\": [1, 1], \"Set\": 0.0},"
    "{\"Pixel\": [3, 3], \"Replace\": [1, 0]},"
    "{\"Pixel\": [7, 7], \"Median\": [1, 1]}]}";
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)(j);
    NDArray *p = createSequentialArray(10, 10);
    epicsFloat64 *out = processAndGetOutput(p);
    BOOST_CHECK_EQUAL(out[11], 0.0);
    BOOST_CHECK_EQUAL(out[33], 34.0);
    BOOST_CHECK_EQUAL(out[77], 77.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(corner_pixel_median)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [0, 0], \"Median\": [1, 1]}]}");
    NDArray *p = createSequentialArray(10, 10);
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[0], 10.0);
    p->release();
  });
}

BOOST_AUTO_TEST_CASE(replace_out_of_bounds_target_skipped)
{
  forEachLoadMethod([&](LoadMethod load) {
    (this->*load)("{\"Bad pixels\": [{\"Pixel\": [0, 0], \"Replace\": [-1, 0]}]}");
    NDArray *p = createSequentialArray(10, 10);
    BOOST_CHECK_EQUAL(processAndGetOutput(p)[0], 0.0);
    p->release();
  });
}


// Check for report output
BOOST_AUTO_TEST_CASE(report_output)
{
  loadFromString("{\"Bad pixels\": ["
    "{\"Pixel\": [1, 0], \"Set\": 42.0},"
    "{\"Pixel\": [2, 1], \"Replace\": [1, 0]},"
    "{\"Pixel\": [3, 2], \"Median\": [2, 3]}]}");

  char buffer[2048];
  FILE *fp = fmemopen(buffer, sizeof(buffer), "w");
  badPixel->report(fp, 1);
  fclose(fp);
  std::string output(buffer);
  BOOST_CHECK(output.find("Set, value=42") != std::string::npos);
  BOOST_CHECK(output.find("Replace, relative coordinates=[1,0]") != std::string::npos);
  BOOST_CHECK(output.find("Median, size=[2,3]") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(report_no_details_omits_pixels)
{
  loadFromString("{\"Bad pixels\": [{\"Pixel\": [1, 1], \"Set\": 0.0}]}");
  char buffer[2048];
  memset(buffer, 0, sizeof(buffer));
  FILE *fp = fmemopen(buffer, sizeof(buffer), "w");
  badPixel->report(fp, 0);
  fclose(fp);
  BOOST_CHECK(std::string(buffer).find("Bad pixel") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(median_skips_bad_neighbor)
{
  loadFromString("{\"Bad pixels\": ["
    "{\"Pixel\": [5, 5], \"Median\": [1, 1]},"
    "{\"Pixel\": [4, 4], \"Set\": 0.0}]}");
  NDArray *p = createSequentialArray(10, 10);
  epicsFloat64 *out = processAndGetOutput(p);
  BOOST_CHECK_EQUAL(out[44], 0.0);
  // Excluding (4,4): 45,46,54,56,64,65,66 -> median=56
  BOOST_CHECK_EQUAL(out[55], 56.0);
  p->release();
}

// Data type parametrized tests - all types exercised in one test each
BOOST_AUTO_TEST_CASE(set_mode_all_data_types)
{
  verifySetMode<epicsInt8>   (NDInt8,    0.0);
  verifySetMode<epicsUInt8>  (NDUInt8,   100.0);
  verifySetMode<epicsInt16>  (NDInt16,  -100.0);
  verifySetMode<epicsUInt16> (NDUInt16,  1000.0);
  verifySetMode<epicsInt32>  (NDInt32,  -50000.0);
  verifySetMode<epicsUInt32> (NDUInt32,  99999.0);
  verifySetMode<epicsInt64>  (NDInt64,  -123456789.0);
  verifySetMode<epicsUInt64> (NDUInt64,  987654321.0);
  verifySetMode<epicsFloat32>(NDFloat32, 3.0);
  verifySetMode<epicsFloat64>(NDFloat64, 999.0);
}

BOOST_AUTO_TEST_CASE(replace_mode_all_data_types)
{
  verifyReplaceMode<epicsInt8>   (NDInt8);
  verifyReplaceMode<epicsUInt8>  (NDUInt8);
  verifyReplaceMode<epicsInt16>  (NDInt16);
  verifyReplaceMode<epicsUInt16> (NDUInt16);
  verifyReplaceMode<epicsInt32>  (NDInt32);
  verifyReplaceMode<epicsUInt32> (NDUInt32);
  verifyReplaceMode<epicsInt64>  (NDInt64);
  verifyReplaceMode<epicsUInt64> (NDUInt64);
  verifyReplaceMode<epicsFloat32>(NDFloat32);
  verifyReplaceMode<epicsFloat64>(NDFloat64);
}

BOOST_AUTO_TEST_CASE(median_mode_all_data_types)
{
  verifyMedianMode<epicsInt8>   (NDInt8);
  verifyMedianMode<epicsUInt8>  (NDUInt8);
  verifyMedianMode<epicsInt16>  (NDInt16);
  verifyMedianMode<epicsUInt16> (NDUInt16);
  verifyMedianMode<epicsInt32>  (NDInt32);
  verifyMedianMode<epicsUInt32> (NDUInt32);
  verifyMedianMode<epicsInt64>  (NDInt64);
  verifyMedianMode<epicsUInt64> (NDUInt64);
  verifyMedianMode<epicsFloat32>(NDFloat32);
  verifyMedianMode<epicsFloat64>(NDFloat64);
}

BOOST_AUTO_TEST_SUITE_END()
