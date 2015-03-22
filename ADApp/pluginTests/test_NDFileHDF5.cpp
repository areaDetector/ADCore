/* Nothing here yet*/ 
#include <stdio.h>


#include "boost/test/unit_test.hpp"

// AD dependencies
#include <NDFileHDF5.h>
#include <simDetector.h>
#include <NDArray.h>
#include <asynDriver.h>
#include <asynPortClient.h>

#include <string.h>
#include <stdint.h>

#include <deque>
using namespace std;



struct NDFileHDF5TestFixture
{
    NDArrayPool *arrayPool;
    simDetector *driver;
    NDFileHDF5 *hdf5;
    asynInt32Client *enableCallbacks;
    asynInt32Client *blockingCallbacks;
    static int testCase;

    NDFileHDF5TestFixture()
    {
        arrayPool = new NDArrayPool(100, 0);

        // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
        // change it slightly for each test case.
        char simport[50], testport[50];
        sprintf(simport, "simPortHDF5test-%d", testCase);
        // We need some upstream driver for our test plugin so that calls to connectArrayPort don't fail, but we can then ignore it and send
        // arrays by calling processCallbacks directly.
        driver = new simDetector(simport, 800, 500, NDFloat64, 50, 0, 0, 2000000);

        // This is the plugin under test
        sprintf(testport, "testPortHDF5-%d", testCase);
        hdf5 = new NDFileHDF5(testport, 50, 1, simport, 0, 0, 2000000);

        enableCallbacks = new asynInt32Client(testport, 0, NDPluginDriverEnableCallbacksString);
        blockingCallbacks = new asynInt32Client(testport, 0, NDPluginDriverBlockingCallbacksString);


        // Set the downstream plugin to receive callbacks from the test plugin and to run in blocking mode, so we don't need to worry about synchronisation
        // with the downstream plugin.
        enableCallbacks->write(1);
        blockingCallbacks->write(1);

        testCase++;

    }
    ~NDFileHDF5TestFixture()
    {
        delete blockingCallbacks;
        delete enableCallbacks;
        delete driver;
        delete hdf5;
        delete arrayPool;
    }
};

int NDFileHDF5TestFixture::testCase = 0;

BOOST_FIXTURE_TEST_SUITE(NDFileHDF5Tests, NDFileHDF5TestFixture)

BOOST_AUTO_TEST_CASE(test_Capture)
{

  size_t dims[2] = {2,5};
  NDArray *testArray = arrayPool->alloc(2,dims,NDFloat64,0,NULL);

}


BOOST_AUTO_TEST_SUITE_END()
