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

#include "testingutilities.h"

struct NDFileHDF5TestFixture
{
  NDArrayPool *arrayPool;
  simDetector *driver;
  NDFileHDF5 *hdf5;
  asynInt32Client *enableCallbacks;
  asynInt32Client *blockingCallbacks;
  asynOctetClient *file_format;
  asynOctetClient *file_path;
  asynOctetClient *file_name;
  asynInt32Client *file_write_mode;
  asynInt32Client *file_num_capture;
  asynInt32Client *file_capture;
  asynInt32Client *file_num_captured;

  static int testCase;

  NDFileHDF5TestFixture()
  {
    arrayPool = new NDArrayPool(100, 0);

    // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
    // change it slightly for each test case.
    std::string simport("simHDF5test"), testport("HDF5");
    uniqueAsynPortName(simport);
    uniqueAsynPortName(testport);

    // We need some upstream driver for our test plugin so that calls to connectArrayPort don't fail, but we can then ignore it and send
    // arrays by calling processCallbacks directly.
    driver = new simDetector(simport.c_str(), 800, 500, NDFloat64, 50, 0, 0, 2000000);

    // This is the plugin under test
    hdf5 = new NDFileHDF5(testport.c_str(), 50, 1, simport.c_str(), 0, 0, 2000000);


    file_format = new asynOctetClient(testport.c_str(), 0, NDFileTemplateString);
    file_path = new asynOctetClient(testport.c_str(), 0, NDFilePathString);
    file_name = new asynOctetClient(testport.c_str(), 0, NDFileNameString);
    file_write_mode = new asynInt32Client(testport.c_str(), 0, NDFileWriteModeString);
    file_num_capture = new asynInt32Client(testport.c_str(), 0, NDFileNumCaptureString);
    file_capture = new asynInt32Client(testport.c_str(), 0, NDFileCaptureString);
    file_num_captured = new asynInt32Client(testport.c_str(), 0, NDFileNumCapturedString);

    // Enable the plugin
    enableCallbacks = new asynInt32Client(testport.c_str(), 0, NDPluginDriverEnableCallbacksString);
    blockingCallbacks = new asynInt32Client(testport.c_str(), 0, NDPluginDriverBlockingCallbacksString);
    enableCallbacks->write(1);
    blockingCallbacks->write(1);

  }
  ~NDFileHDF5TestFixture()
  {
    delete blockingCallbacks;
    delete enableCallbacks;
    delete file_format;
    delete file_path;
    delete file_name;
    delete file_write_mode;
    delete file_num_capture;
    delete file_capture;
    delete file_num_captured;
    delete driver;
    delete hdf5;
    delete arrayPool;
  }

  void setup_hdf_stream()
  {
    size_t nactual;
    file_write_mode->write(NDFileModeStream);
    std::string str("/tmp");
    file_path->write(str.c_str(), str.length(), &nactual);

    str = "testing";
    file_name->write(str.c_str(), str.length(), &nactual);

    str = "%s%s_%d.5";
    file_format->write(str.c_str(), str.length(), &nactual);

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
  file_num_capture->write(10);
  file_capture->write(1);

  epicsInt32 num_captured=0;
  for (int i = 0; i < 10; i++)
  {
    hdf5->lock();
    BOOST_CHECK_NO_THROW(hdf5->processCallbacks(arrays[i]));
    hdf5->unlock();
    file_num_captured->read(&num_captured);
    BOOST_CHECK_EQUAL(num_captured, i+1);
  }



}


BOOST_AUTO_TEST_SUITE_END()
