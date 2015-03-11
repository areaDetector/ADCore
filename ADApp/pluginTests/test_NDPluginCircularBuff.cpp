/* Nothing here yet*/ 
#include <stdio.h>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "NDPluginCircularBuff Tests"
#include "boost/test/unit_test.hpp"

// AD dependencies
#include "NDPluginCircularBuff.h"
#include "simDetector.h"
#include "NDPluginROI.h"
#include "NDArray.h"
#include "asynDriver.h"

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
    // simDetectorConfig(portName, maxSizeX, maxSizeY, dataType, maxBuffers, maxMemory)
    simDetector *driver;
    NDPluginCircularBuff *cb;
    TestingPlugin *ds;
    asynUser *user;
    static int count;

    PluginFixture()
    {
        arrayPool = new NDArrayPool(100, 0);

        // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
        // change it slightly for each test case.
        char simport[50], testport[50], dsport[50];
        sprintf(simport, "simPort%d", count);
        // We need some upstream driver for our test plugin so that calls to connectArrayPort don't fail, but we can then ignore it and send
        // arrays by calling processCallbacks directly.
        driver = new simDetector(simport, 800, 500, NDFloat64, 50, 0, 0, 2000000);

        // This is the plugin under test
        sprintf(testport, "testPort%d", count);
        cb = new NDPluginCircularBuff(testport, 50, 0, simport, 0, 1000, -1, 0, 2000000);

        // This is the mock downstream plugin
        sprintf(dsport, "dsPort%d", count);
        ds = new TestingPlugin(dsport, 16, 1, testport, 0, 50, -1, 0, 2000000);

        // We need a valid asynUser to pass to writeInt32, as otherwise it segfaults in calls to epicsSnprintf, so get one from the global asynManager.
        user = pasynManager->createAsynUser(0,0);

        // Set the downstream plugin to receive callbacks from the test plugin and to run in blocking mode, so we don't need to worry about synchronisation
        // with the downstream plugin.
        // Would be nice if we could use the actual paramater names rather than having to look up the numbers, but they are private members.
        user->reason = 53 - 3; // NDPluginDriverEnableCallbacks
        ds->lock();
        ds->writeInt32(user, 1);
        ds->unlock();
        user->reason = 53 - 2; //NDPluginDriverBlockingCallbacks
        ds->lock();
        ds->writeInt32(user, 1);
        ds->unlock();

        count++;

    }
    ~PluginFixture()
    {
        pasynManager->freeAsynUser(user);
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

int PluginFixture::count = 0;

BOOST_FIXTURE_TEST_SUITE(CircularBuffTests, PluginFixture)

BOOST_AUTO_TEST_CASE(test_BufferWrappingAndStatusMessages)
{
  // readOctet segfaults if we don't supply something for its eom and gotbytes arguments
  size_t gotbytes;
  int eom;

  // To store values read back from driver
  int storedImages;
  char status[50] = {0};

  user->reason = 53 + 1; // NDPluginCircularBuffStatus
  cb->readOctet(user, status, 50, &gotbytes, &eom);
  BOOST_REQUIRE_EQUAL(strcmp(status, "Idle"), 0);

  user->reason = 53 + 2; // NDPluginCircularBuffPreTrigger
  cb->writeInt32(user, 10);

  user->reason = 53 + 0; // NDPluginCircularBuffControl
  cb->writeInt32(user, 1);

  user->reason = 53 + 1; // NDPluginCircularBuffStatus
  cb->readOctet(user, status, 50, &gotbytes, &eom);
  BOOST_REQUIRE_EQUAL(strcmp(status, "Buffer filling"), 0);

  size_t dims[2] = {2,5};
  NDArray *testArray = arrayPool->alloc(2,dims,NDFloat64,0,NULL);

  // Pass enough images to start the buffer wrapping
  for (int i = 0; i < 30; i++)
      cbProcess(testArray);

  user->reason = 53 + 4; // NDPluginCircularBuffCurrentImage

  cb->readInt32(user, &storedImages);
  user->reason = 53 + 1; // NDPluginCircularBuffStatus

  cb->readOctet(user, status, 50, &gotbytes, &eom);

  BOOST_REQUIRE_EQUAL(storedImages, 10);
  BOOST_REQUIRE_EQUAL(strcmp(status, "Buffer Wrapping"), 0);

}

BOOST_AUTO_TEST_CASE(test_OutputCount)
{
    user->reason = 53 + 2; // NDPluginCircularBuffPreTrigger
    cb->writeInt32(user, 10);

    user->reason = 53 + 0; // NDPluginCircularBuffControl
    cb->writeInt32(user, 1);

    size_t dims[2] = {2,5};
    NDArray *testArray = arrayPool->alloc(2,dims,NDFloat64,0,NULL);

    for (int i = 0; i < 10; i++)
        cbProcess(testArray);

    // Trigger the buffer flush
    user->reason = 53 + 6; // NDPluginCircularBuffSoftTrigger
    cb->writeInt32(user, 1);

    cbProcess(testArray);

    deque<NDArray *> *arrays = ds->arrays();

    BOOST_REQUIRE_EQUAL(11, arrays->size());
}

BOOST_AUTO_TEST_CASE(test_PreBufferOrder)
{
    user->reason = 53 + 2; // NDPluginCircularBuffPreTrigger
    cb->writeInt32(user, 3);

    user->reason = 53 + 0; // NDPluginCircularBuffControl
    cb->writeInt32(user, 1);

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
    user->reason = 53 + 6; // NDPluginCircularBuffSoftTrigger
    cb->writeInt32(user, 1);

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
