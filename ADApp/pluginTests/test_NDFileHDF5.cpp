/* Nothing here yet*/ 
#include <stdio.h>


#include "boost/test/unit_test.hpp"

// AD dependencies
#include <NDPluginDriver.h>
#include <NDArray.h>
#include <asynDriver.h>

#include <string.h>
#include <stdint.h>

#include <deque>
#include <tr1/memory>
using namespace std;

#include "testingutilities.h"
#include "asynPortDriver.h"
#include "HDF5PluginWrapper.h"

struct NDFileHDF5TestFixture
{
  NDArrayPool *arrayPool;
  asynPortDriver* dummy_driver;
  std::tr1::shared_ptr<HDF5PluginWrapper> hdf5;

  static int testCase;

  NDFileHDF5TestFixture()
  {
    arrayPool = new NDArrayPool(100, 0);

    // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
    // change it slightly for each test case.
    std::string dummy_port("simHDF5test"), testport("HDF5");
    uniqueAsynPortName(dummy_port);
    uniqueAsynPortName(testport);

    // We need some upstream driver for our test plugin so that calls to connectToArrayPort don't fail, but we can then ignore it and send
    // arrays by calling processCallbacks directly.
    // Thus we instansiate a basic asynPortDriver object which is never used.
    dummy_driver = new asynPortDriver(dummy_port.c_str(), 0, 1, asynGenericPointerMask, asynGenericPointerMask, 0, 0, 0, 2000000);

    // This is the plugin under test
    hdf5 = std::tr1::shared_ptr<HDF5PluginWrapper>(new HDF5PluginWrapper(testport.c_str(),
                                                                         50,
                                                                         1,
                                                                         dummy_port.c_str(),
                                                                         0,
                                                                         0,
                                                                         2000000));


    // Enable the plugin
    hdf5->write(NDPluginDriverEnableCallbacksString, 1);
    hdf5->write(NDPluginDriverBlockingCallbacksString, 1);

  }
  ~NDFileHDF5TestFixture()
  {
    delete arrayPool;
    hdf5.reset();
    delete dummy_driver;
  }

  void setup_hdf_stream()
  {
    hdf5->write(NDFileWriteModeString, NDFileModeStream);
    hdf5->write(NDFilePathString, "/tmp");
    hdf5->write(NDFileNameString, "testing");
    hdf5->write(NDFileTemplateString, "%s%s_%d.5");
  }
};

BOOST_FIXTURE_TEST_SUITE(NDFileHDF5Tests, NDFileHDF5TestFixture)

BOOST_AUTO_TEST_CASE(test_Capture)
{
  size_t tmpdims[] = {4,6};
  std::vector<size_t>dims(tmpdims, tmpdims + sizeof(tmpdims)/sizeof(tmpdims[0]));

  // Create some test arrays
  std::vector<NDArray*>arrays(10);
  fillNDArrays(dims, NDUInt32, arrays);

  // Configure the HDF5 plugin
  setup_hdf_stream();

  // Initialise the HDF5 plugin with a dummy frame
  hdf5->processCallbacks(arrays[0]);

  // Start capture to disk
  hdf5->write(NDFileNumCaptureString, 10);
  hdf5->write(NDFileCaptureString, 1);

  // Now start capturing frames
  epicsInt32 num_captured=0;
  for (int i = 0; i < 10; i++)
  {
    hdf5->lock();
    BOOST_CHECK_NO_THROW(hdf5->processCallbacks(arrays[i]));
    hdf5->unlock();
    num_captured = hdf5->readInt(NDFileNumCapturedString);
    BOOST_CHECK_EQUAL(num_captured, i+1);
  }



}


BOOST_AUTO_TEST_SUITE_END()
