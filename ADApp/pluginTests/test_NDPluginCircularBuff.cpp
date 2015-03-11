/* Nothing here yet*/ 
#include <stdio.h>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "NDPluginCircularBuff Tests"
#include "boost/test/unit_test.hpp"

// AD dependencies
#include <NDPluginCircularBuff.h>
#include <simDetector.h>
#include <NDArray.h>
#include <asynDriver.h>
#include <asynPortClient.h>

#include <string.h>
#include <stdint.h>

#include <deque>
using namespace std;



// Mock NDPlugin; simply stores all received NDArrays and provides them to a client on request.
class TestingPlugin : public NDPluginDriver {
public:
    TestingPlugin (const char *portName, int queueSize, int blockingCallbacks,
                                  const char *NDArrayPort, int NDArrayAddr,
                                  int maxBuffers, size_t maxMemory,
                                  int priority, int stackSize);
    ~TestingPlugin();
    void processCallbacks(NDArray *pArray);
    deque<NDArray *> *arrays();
private:
    deque<NDArray *> *arrays_;
};

TestingPlugin::TestingPlugin (const char *portName, int queueSize, int blockingCallbacks,
                              const char *NDArrayPort, int NDArrayAddr,
                              int maxBuffers, size_t maxMemory,
                              int priority, int stackSize)
         /* Invoke the base class constructor */
         : NDPluginDriver(portName, queueSize, blockingCallbacks,
                        NDArrayPort, NDArrayAddr, 1, 0, maxBuffers, maxMemory,
                        asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                        asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                        0, 1, priority, stackSize)
{
    arrays_ = new deque<NDArray *>();

    connectToArrayPort();
}

TestingPlugin::~TestingPlugin()
{
    while(arrays_->front()) {
        arrays_->front()->release();
        arrays_->pop_front();
    }
    delete arrays_;
}

std::deque<NDArray *> *TestingPlugin::arrays()
{
    return arrays_;
}

void TestingPlugin::processCallbacks(NDArray *pArray)
{
    NDPluginDriver::processCallbacks(pArray);
    NDArray *pArrayCpy = this->pNDArrayPool->copy(pArray, NULL, 1);
    if (pArrayCpy) {
        arrays_->push_back(pArrayCpy);
    }
}


struct PluginFixture
{
    NDArrayPool *arrayPool;
    simDetector *driver;
    NDPluginCircularBuff *cb;
    TestingPlugin *ds;
    asynInt32Client *enableCallbacks;
    asynInt32Client *blockingCallbacks;
    asynInt32Client *cbControl;
    asynInt32Client *cbPreTrigger;
    asynInt32Client *cbPostTrigger;
    asynInt32Client *cbSoftTrigger;
    asynOctetClient *cbStatus;
    asynInt32Client *cbCount;
    asynOctetClient *cbTrigA;
    asynOctetClient *cbTrigB;
    asynOctetClient *cbCalc;
    static int testCase;

    PluginFixture()
    {
        arrayPool = new NDArrayPool(100, 0);

        // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
        // change it slightly for each test case.
        char simport[50], testport[50], dsport[50];
        sprintf(simport, "simPort%d", testCase);
        // We need some upstream driver for our test plugin so that calls to connectArrayPort don't fail, but we can then ignore it and send
        // arrays by calling processCallbacks directly.
        driver = new simDetector(simport, 800, 500, NDFloat64, 50, 0, 0, 2000000);

        // This is the plugin under test
        sprintf(testport, "testPort%d", testCase);
        cb = new NDPluginCircularBuff(testport, 50, 0, simport, 0, 1000, -1, 0, 2000000);

        // This is the mock downstream plugin
        sprintf(dsport, "dsPort%d", testCase);
        ds = new TestingPlugin(dsport, 16, 1, testport, 0, 50, -1, 0, 2000000);

        enableCallbacks = new asynInt32Client(dsport, 0, NDPluginDriverEnableCallbacksString);
        blockingCallbacks = new asynInt32Client(dsport, 0, NDPluginDriverBlockingCallbacksString);
        cbControl = new asynInt32Client(testport, 0, NDCircBuffControlString);
        cbPreTrigger = new asynInt32Client(testport, 0, NDCircBuffPreTriggerString);
        cbPostTrigger = new asynInt32Client(testport, 0, NDCircBuffPostTriggerString);
        cbSoftTrigger = new asynInt32Client(testport, 0, NDCircBuffSoftTriggerString);
        cbStatus = new asynOctetClient(testport, 0, NDCircBuffStatusString);
        cbCount = new asynInt32Client(testport, 0, NDCircBuffCurrentImageString);
        cbTrigA = new asynOctetClient(testport, 0, NDCircBuffTriggerAString);
        cbTrigB = new asynOctetClient(testport, 0, NDCircBuffTriggerBString);
        cbCalc = new asynOctetClient(testport, 0, NDCircBuffTriggerCalcString);

        // Set the downstream plugin to receive callbacks from the test plugin and to run in blocking mode, so we don't need to worry about synchronisation
        // with the downstream plugin.
        enableCallbacks->write(1);
        blockingCallbacks->write(1);



        testCase++;

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
        delete blockingCallbacks;
        delete enableCallbacks;
        delete ds;
        delete cb;
        delete driver;
        delete arrayPool;
    }
    void cbProcess(NDArray *pArray)
    {
        cb->lock();
        cb->processCallbacks(pArray);
        cb->unlock();
    }
};

int PluginFixture::testCase = 0;

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
  BOOST_REQUIRE_EQUAL(strcmp(status, "Idle"), 0);

  cbPreTrigger->write(10);
  cbControl->write(1);

  cbStatus->read(status, 50, &gotbytes, &eom);
  BOOST_REQUIRE_EQUAL(strcmp(status, "Buffer filling"), 0);

  size_t dims[2] = {2,5};
  NDArray *testArray = arrayPool->alloc(2,dims,NDFloat64,0,NULL);

  // Pass enough images to start the buffer wrapping
  for (int i = 0; i < 30; i++)
      cbProcess(testArray);

  cbCount->read(&storedImages);
  cbStatus->read(status, 50, &gotbytes, &eom);
  BOOST_REQUIRE_EQUAL(storedImages, 10);
  BOOST_REQUIRE_EQUAL(strcmp(status, "Buffer Wrapping"), 0);
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

    deque<NDArray *> *arrays = ds->arrays();

    BOOST_REQUIRE_EQUAL(11, arrays->size());
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

    deque<NDArray *> *resultArrays = ds->arrays();

    // Check that the arrays have been received in the correct order
    BOOST_REQUIRE_EQUAL(0, ((uint8_t *)(*resultArrays)[0]->pData)[0]);
    BOOST_REQUIRE_EQUAL(1, ((uint8_t *)(*resultArrays)[1]->pData)[0]);
    BOOST_REQUIRE_EQUAL(2, ((uint8_t *)(*resultArrays)[2]->pData)[0]);
    BOOST_REQUIRE_EQUAL(3, ((uint8_t *)(*resultArrays)[3]->pData)[0]);
}

BOOST_AUTO_TEST_SUITE_END()
