/* Nothing here yet*/ 
#include <stdio.h>


#include "boost/test/unit_test.hpp"

// AD and asyn dependencies
#include <NDPluginCircularBuff.h>
#include <asynPortDriver.h>
#include <NDArray.h>
#include <asynDriver.h>
#include <asynPortClient.h>

#include <string.h>
#include <stdint.h>

#include "testingutilities.h"

using namespace std;


struct PluginFixture
{
    NDArrayPool *arrayPool;
    asynPortDriver *dummy_driver;
    NDPluginCircularBuff *cb;
    TestingPlugin *ds;
    asynInt32Client *cbControl;
    asynInt32Client *cbPreTrigger;
    asynInt32Client *cbPostTrigger;
    asynInt32Client *cbSoftTrigger;
    asynOctetClient *cbStatus;
    asynInt32Client *cbCount;
    asynOctetClient *cbTrigA;
    asynOctetClient *cbTrigB;
    asynOctetClient *cbCalc;

    PluginFixture()
    {
        arrayPool = new NDArrayPool(100, 0);

        std::string dummy_port("simPort"), testport("testPort");

        // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
        // change it slightly for each test case.
        uniqueAsynPortName(dummy_port);
        uniqueAsynPortName(testport);

        // We need some upstream driver for our test plugin so that calls to connectToArrayPort don't fail, but we can then ignore it and send
        // arrays by calling processCallbacks directly.
        // Thus we instansiate a basic asynPortDriver object which is never used.
        dummy_driver = new asynPortDriver(dummy_port.c_str(), 0, 1, asynGenericPointerMask, asynGenericPointerMask, 0, 0, 0, 2000000);

        // This is the plugin under test
        cb = new NDPluginCircularBuff(testport.c_str(), 50, 0, dummy_port.c_str(), 0, 1000, -1, 0, 2000000);

        // This is the mock downstream plugin
        ds = new TestingPlugin(testport.c_str(), 0);

        cbControl = new asynInt32Client(testport.c_str(), 0, NDCircBuffControlString);
        cbPreTrigger = new asynInt32Client(testport.c_str(), 0, NDCircBuffPreTriggerString);
        cbPostTrigger = new asynInt32Client(testport.c_str(), 0, NDCircBuffPostTriggerString);
        cbSoftTrigger = new asynInt32Client(testport.c_str(), 0, NDCircBuffSoftTriggerString);
        cbStatus = new asynOctetClient(testport.c_str(), 0, NDCircBuffStatusString);
        cbCount = new asynInt32Client(testport.c_str(), 0, NDCircBuffCurrentImageString);
        cbTrigA = new asynOctetClient(testport.c_str(), 0, NDCircBuffTriggerAString);
        cbTrigB = new asynOctetClient(testport.c_str(), 0, NDCircBuffTriggerBString);
        cbCalc = new asynOctetClient(testport.c_str(), 0, NDCircBuffTriggerCalcString);

    }
    ~PluginFixture()
    {
        delete cbCalc;
        delete cbTrigB;
        delete cbTrigA;
        delete cbCount;
        delete cbStatus;
        delete cbSoftTrigger;
        delete cbPostTrigger;
        delete cbPreTrigger;
        delete cbControl;
        //delete ds; // TODO: something is wrong here - if we delete ds we get a memory corruption error (in a loop!?!?)
        delete cb;
        delete dummy_driver;
        delete arrayPool;
    }
    void cbProcess(NDArray *pArray)
    {
        cb->lock();
        cb->processCallbacks(pArray);
        cb->unlock();
    }
};

BOOST_FIXTURE_TEST_SUITE(CircularBuffTests, PluginFixture)

BOOST_AUTO_TEST_CASE(test_BufferWrappingAndStatusMessages)
{
  // readOctet segfaults if we don't supply something for its eom and gotbytes arguments
  size_t gotbytes;
  int eom;

  // Disable the attribute based triggering.
  cbCalc->write("0", 2, &gotbytes);

  int storedImages;
  char status[50] = {0};

  cbStatus->read(status, 50, &gotbytes, &eom);
  BOOST_REQUIRE_EQUAL(status, "Idle");

  cbPreTrigger->write(10);
  cbControl->write(1);

  cbStatus->read(status, 50, &gotbytes, &eom);
  BOOST_CHECK_EQUAL(status, "Buffer filling");

  size_t dims[2] = {2,5};
  NDArray *testArray = arrayPool->alloc(2,dims,NDFloat64,0,NULL);

  // Pass enough images to start the buffer wrapping
  for (int i = 0; i < 30; i++)
      cbProcess(testArray);

  cbCount->read(&storedImages);
  cbStatus->read(status, 50, &gotbytes, &eom);
  BOOST_CHECK_EQUAL(storedImages, 10);
  BOOST_CHECK_EQUAL(status, "Buffer Wrapping");
}

BOOST_AUTO_TEST_CASE(test_OutputCount)
{
    size_t gotbytes;
    cbCalc->write("0", 2, &gotbytes);

    cbPreTrigger->write(10);
    cbControl->write(1);

    size_t dims[2] = {2,5};
    NDArray *testArray = arrayPool->alloc(2,dims,NDFloat64,0,NULL);

    for (int i = 0; i < 10; i++)
        cbProcess(testArray);

    // Trigger the buffer flush
    cbSoftTrigger->write(1);

    cbProcess(testArray);

    BOOST_REQUIRE_EQUAL((size_t)11, ds->arrays.size());
}

BOOST_AUTO_TEST_CASE(test_PreBufferOrder)
{
    size_t gotbytes;
    cbCalc->write("0", 2, &gotbytes);

    cbPreTrigger->write(3);
    cbControl->write(1);

    size_t dims = 3;
    NDArray *testArrays[10];
    for (int i = 0; i < 10; i++) {
        testArrays[i] = arrayPool->alloc(1,&dims,NDUInt8,0,NULL);
        memset(testArrays[i]->pData, i, 3);
    }

    // Fill the pre-buffer
    for (int i = 0; i < 3; i++) {
        cbProcess(testArrays[i]);
    }

    // Trigger the buffer flush
    cbSoftTrigger->write(1);

    // Really we should unlock around every call to processCallbacks since it expects the lock to be taken, but in practice this is only necessary if the
    // call will trigger output to the downstream plugin (i.e. if cb will actually call unlock() at some point).
    cbProcess(testArrays[3]);

    // Check that the arrays have been received in the correct order
    BOOST_CHECK_EQUAL(0, ((uint8_t *)ds->arrays[0]->pData)[0]);
    BOOST_CHECK_EQUAL(1, ((uint8_t *)ds->arrays[1]->pData)[0]);
    BOOST_CHECK_EQUAL(2, ((uint8_t *)ds->arrays[2]->pData)[0]);
    BOOST_CHECK_EQUAL(3, ((uint8_t *)ds->arrays[3]->pData)[0]);
}

BOOST_AUTO_TEST_SUITE_END()
