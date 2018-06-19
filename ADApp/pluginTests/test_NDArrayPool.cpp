/* Nothing here yet*/ 
#include <stdio.h>


#include "boost/test/unit_test.hpp"

// AD and asyn dependencies
#include <NDArray.h>
#include <asynNDArrayDriver.h>

#include <string.h>
#include <stdint.h>

#include "testingutilities.h"

using namespace std;


struct NDArrayPoolFixture
{
    NDArrayPool *pPool;
    asynNDArrayDriver *dummy_driver;
    #define MAX_MEMORY 60000

    NDArrayPoolFixture()
    {
        std::string dummy_port("simPort");

        // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
        // change it slightly for each test case.
        uniqueAsynPortName(dummy_port);

        // Thus we instantiate a basic asynPortDriver object which is used for the NDArrayPool
        dummy_driver = new asynNDArrayDriver(dummy_port.c_str(), 1, 0, MAX_MEMORY, asynGenericPointerMask, asynGenericPointerMask, 0, 0, 0, 0);
        pPool = dummy_driver->pNDArrayPool;

    }
    ~NDArrayPoolFixture()
    {
        delete dummy_driver;
    }
};

BOOST_FIXTURE_TEST_SUITE(NDArrayPoolTests, NDArrayPoolFixture)

BOOST_AUTO_TEST_CASE(test_Pool)
{
  #define MAX_ARRAYS 5
  size_t bufferSizes[MAX_ARRAYS] = {100, 150, 250, 1000, 50000};
  NDArray *pArrays[MAX_ARRAYS];
  NDArray *pArrayTest;
  size_t dims;
  size_t totalMemory = 0;
  int i;

  for (i=0; i<MAX_ARRAYS; i++) {
    dims = bufferSizes[i];
    pArrays[i] = pPool->alloc(1, &dims, NDUInt8, 0, NULL);
    totalMemory += bufferSizes[i];
  }
  pPool->report(stdout, 6);

  // Check the statistics on the pool
  BOOST_CHECK_EQUAL(pPool->getNumBuffers(), MAX_ARRAYS);
  BOOST_CHECK_EQUAL(pPool->getMaxMemory(), MAX_MEMORY);
  BOOST_CHECK_EQUAL(pPool->getMemorySize(), totalMemory);
  BOOST_CHECK_EQUAL(pPool->getNumFree(), 0);

  // Release all of the arrays, check NumFree again
  for (i=0; i<MAX_ARRAYS; i++) {
    pArrays[i]->release();
  }
  pPool->report(stdout, 6);
  BOOST_CHECK_EQUAL(pPool->getNumFree(), MAX_ARRAYS);

  // Allocate an array of size of second array; should get pArrays[1] as address
  dims = bufferSizes[1];
  pArrayTest = pPool->alloc(1, &dims, NDUInt8, 0, NULL);
  BOOST_CHECK_EQUAL(pArrayTest, pArrays[1]);
  BOOST_CHECK_EQUAL(pArrayTest->dataSize, bufferSizes[1]);
  BOOST_CHECK_EQUAL(pPool->getNumFree(), MAX_ARRAYS-1);
  pArrayTest->release();
  pPool->report(stdout, 6);

  // Allocate an array of size of 200; should get pArrays[2] as address and size will be 250, not 200 because difference is less than 1.5 threshold
  dims = 200;
  pArrayTest = pPool->alloc(1, &dims, NDUInt8, 0, NULL);
  BOOST_CHECK_EQUAL(pArrayTest, pArrays[2]);
  BOOST_CHECK_EQUAL(pArrayTest->dataSize, bufferSizes[2]);
  BOOST_CHECK_EQUAL(pPool->getNumFree(), MAX_ARRAYS-1);
  pArrayTest->release();
  pPool->report(stdout, 6);

  // Allocate an array of size of 500; should get pArrays[3] as address and size will be 500, not 1000 because difference is more than 1.5 threshold
  dims = 500;
  pArrayTest = pPool->alloc(1, &dims, NDUInt8, 0, NULL);
  BOOST_CHECK_EQUAL(pArrayTest, pArrays[3]);
  BOOST_CHECK_EQUAL(pArrayTest->dataSize, 500);
  BOOST_CHECK_EQUAL(pPool->getNumFree(), MAX_ARRAYS-1);
  pArrayTest->release();
  pPool->report(stdout, 6);

  // Allocate an array of size of MAX_MEMORY; this should cause all existing arrays to be deleted
  pPool->report(stdout, 6);
  dims = MAX_MEMORY;
  pArrayTest = pPool->alloc(1, &dims, NDUInt8, 0, NULL);
  BOOST_CHECK(pArrayTest != 0);
  BOOST_CHECK_EQUAL(pArrayTest->dataSize, MAX_MEMORY);
  BOOST_CHECK_EQUAL(pPool->getNumFree(), 0);
  pArrayTest->release();
  pPool->report(stdout, 6);

  // Allocate an array of size of MAX_MEMORY/2; this should cause the one existing array to be deleted
  dims = MAX_MEMORY/2;
  pArrayTest = pPool->alloc(1, &dims, NDUInt8, 0, NULL);
  BOOST_CHECK(pArrayTest != 0);
  BOOST_CHECK_EQUAL(pArrayTest->dataSize, MAX_MEMORY/2);
  BOOST_CHECK_EQUAL(pPool->getNumFree(), 0);
  pArrayTest->release();
  pPool->report(stdout, 6);

  // Allocate an array of size of MAX_MEMORY*2; this should cause the one existing array to be deleted
  // and then return a NULL pointer because MAX_MEMORY is exceeded.
  dims = MAX_MEMORY*2;
  pArrayTest = pPool->alloc(1, &dims, NDUInt8, 0, NULL);
  BOOST_CHECK(pArrayTest == 0);
  BOOST_CHECK_EQUAL(pPool->getNumFree(), 0);
  pPool->report(stdout, 6);
    
}

BOOST_AUTO_TEST_SUITE_END()
