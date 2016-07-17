/* Nothing here yet*/ 
// This must be defined first to hack the relevant classes, turning all private variables and
// methods into public for unit testing.  It's not pretty, but necessary until the classes are
// all re-written to be more OO.
#define _UNITTEST_HDF5_

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
using namespace std;

#include "testingutilities.h"
#include "asynPortDriver.h"
#include "HDF5PluginWrapper.h"
#include "HDF5FileReader.h"

struct NDFileHDF5TestFixture
{
  NDArrayPool *arrayPool;
  asynPortDriver* dummy_driver;
  boost::shared_ptr<HDF5PluginWrapper> hdf5;

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
    hdf5 = boost::shared_ptr<HDF5PluginWrapper>(new HDF5PluginWrapper(testport.c_str(),
                                                                         50,
                                                                         1,
                                                                         dummy_port.c_str(),
                                                                         0,
                                                                         0,
                                                                         2000000));


    // Enable the plugin
    hdf5->start(); // start the plugin thread although not required for this unittesting
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

  void populateAttributeList(NDAttributeList *pAttributeList)
  {
    epicsFloat64 val1 = 1.0;
    NDAttribute *pA1 = new NDAttribute("temperature", "detector temperature", NDAttrSourceEPICSPV, "13SIM1:cam1:Temperature", NDAttrFloat64, &val1);
    pAttributeList->add(pA1);
    char val2[MAX_STRING_SIZE];
    strcpy(val2, "C");
    NDAttribute *pA2 = new NDAttribute("temperature_EGU", "detector temperature units", NDAttrSourceEPICSPV, "13SIM1:cam1:Temperature.EGU", NDAttrString, val2);
    pAttributeList->add(pA2);
    char val3[MAX_STRING_SIZE];
    strcpy(val3, "/tmp/test1_001.h5");
    NDAttribute *pA3 = new NDAttribute("HDF5_filename", "name of this file as written", NDAttrSourceEPICSPV, "13SIM1:HDF1:FullFileName_RBV", NDAttrString, val3);
    pAttributeList->add(pA3);
    epicsInt32 val4 = 0;
    NDAttribute *pA4 = new NDAttribute("ArrayCounter", "Image counter", NDAttrSourceParam, "ARRAY_COUNTER", NDAttrInt32, &val4);
    pAttributeList->add(pA4);
    epicsInt32 val5 = 10;
    NDAttribute *pA5 = new NDAttribute("MaxSizeX", "Detector X size", NDAttrSourceParam, "MAX_SIZE_X", NDAttrInt32, &val5);
    pAttributeList->add(pA5);
    epicsInt32 val6 = 10;
    NDAttribute *pA6 = new NDAttribute("MaxSizeY", "Detector Y size", NDAttrSourceParam, "MAX_SIZE_Y", NDAttrInt32, &val6);
    pAttributeList->add(pA6);
    char val7[MAX_STRING_SIZE];
    strcpy(val7, "Basic Simulator");
    NDAttribute *pA7 = new NDAttribute("Model", "Camera model", NDAttrSourceParam, "MODEL", NDAttrString, val7);
    pAttributeList->add(pA7);
    char val8[MAX_STRING_SIZE];
    strcpy(val8, "Basic Simulator");
    NDAttribute *pA8 = new NDAttribute("Manufacturer", "Camera manufacturer", NDAttrSourceParam, "MANUFACTURER", NDAttrString, val8);
    pAttributeList->add(pA8);
  }
};

BOOST_FIXTURE_TEST_SUITE(NDFileHDF5Tests, NDFileHDF5TestFixture)

BOOST_AUTO_TEST_CASE(test_createDatasetType)
{
  // Verify that dataset is created using hid_t type as return value.
  // From hdf5 version 1-9-222 if the dataset type was not returned correctly
  // then attributes were not attached to the dataset.  This test checks
  // that attributes are attached to the test dataset
  size_t tmpdims[] = {4,6};
  std::vector<size_t>dims(tmpdims, tmpdims + sizeof(tmpdims)/sizeof(tmpdims[0]));

  // Create a test array
  std::vector<NDArray*>arrays(1);
  fillNDArrays(dims, NDUInt32, arrays);

  // Configure the HDF5 plugin
  setup_hdf_stream();

  // Initialise the HDF5 plugin with a dummy frame
  hdf5->processCallbacks(arrays[0]);

  // Start capture to disk (1 frame only)
  hdf5->write(NDFileNumCaptureString, 1);
  hdf5->write(NDFileCaptureString, 1);

  // Now capture the frame
  hdf5->lock();
  BOOST_CHECK_NO_THROW(hdf5->processCallbacks(arrays[0]));
  hdf5->unlock();

  // File has been written, open for reading
  HDF5FileReader fr("/tmp/testing_0.5");
  // Verify there are attributes attached to the dataset
  BOOST_CHECK_GT(fr.getDatasetAttributeCount("/entry/data/data"), 0);
}

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

BOOST_AUTO_TEST_CASE(test_DatasetLayout1)
{
  size_t tmpdims[] = {10,10};
  std::vector<size_t>dims(tmpdims, tmpdims + sizeof(tmpdims)/sizeof(tmpdims[0]));

  // Create some test arrays
  std::vector<NDArray*>arrays(10);
  fillNDArrays(dims, NDUInt32, arrays);

  hdf5->write(NDFileWriteModeString, NDFileModeStream);
  hdf5->write(str_NDFileHDF5_storeAttributes, 1);
  hdf5->write(NDFileNumberString, 1);
  hdf5->write(str_NDFileHDF5_layoutFilename, "<?xml version=\"1.0\" standalone=\"no\" ?>\
<hdf5_layout>\
  <group name=\"example\">\
    <attribute name=\"version\" source=\"constant\" value=\"2015.0225.01\" type=\"string\" />\
\
    <!-- PROBLEM: dataset from EPICS_PV ndattribute -->\
    <dataset name=\"this_file_name\" source=\"ndattribute\" ndattribute=\"HDF5_filename\"/>\
\
    <!-- OK (value): dataset from EPICS_PV ndattribute -->\
    <dataset name=\"temperature\" ndattribute=\"temperature\" source=\"ndattribute\" >\
      <!-- PROBLEM: attributes from EPICS_PV ndattribute -->\
      <attribute name=\"EGU\" ndattribute=\"temperature_EGU\" source=\"ndattribute\" />\
      <attribute name=\"units\" value=\"C\" type=\"string\" source=\"constant\" />\
    </dataset>\
\
    <!-- OK: link to dataset in different group -->\
    <hardlink name=\"data_link_to_other_group\" target=\"/example/detector/data\" />\
\
    <!-- PROBLEM: link to dataset in same group -->\
    <hardlink name=\"data_link_in_same_group\" target=\"/example/temperature\" />\
    <!--\
    Must be a known error since reported by support code:\
    NDFileHDF5::createHardLinks error creating hard link from: /example/this_file_name to /example/data_link_in_same_group\
    -->\
\
    <group name=\"detector\">\
      \
      <!-- OK: store the image -->\
      <dataset name=\"data\" source=\"detector\" det_default=\"true\">\
        <attribute name=\"manufacturer\" ndattribute=\"Manufacturer\" source=\"ndattribute\" />\
        <attribute name=\"model\" ndattribute=\"Model\" source=\"ndattribute\" />\
        <attribute name=\"temperature_EGU\" ndattribute=\"temperature_EGU\" source=\"ndattribute\" />\
      </dataset>\
\
      <!-- PROBLEM: link to dataset in different group -->\
      <hardlink name=\"data_link_to_other_group\" target=\"/example/temperature\" />\
    </group>\
\
    <group name=\"attributes\">\
      <!-- PROBLEM (unrequested attributes): ndattribute -->\
      <dataset name=\"ArrayCounter\" source=\"ndattribute\" ndattribute=\"ArrayCounter\"/>\
    </group>\
\
    <!-- ndattributes MaxSizeX and MaxSizeY not used in the layout, should appear here -->\
    <group name=\"metadata\" ndattr_default=\"true\" />\
\
  </group>\
</hdf5_layout>");

  hdf5->write(NDFilePathString, "/tmp");
  hdf5->write(NDFileNameString, "test1");
  hdf5->write(NDFileTemplateString, "%s%s_%3.3d.h5");
  // Initialise the HDF5 plugin with a dummy frame
  populateAttributeList(arrays[0]->pAttributeList);
  hdf5->processCallbacks(arrays[0]);

  // Start capture to disk
  hdf5->write(NDFileNumCaptureString, 10);
  hdf5->write(NDFileCaptureString, 1);

  // Now start capturing frames
  epicsInt32 num_captured=0;
  for (int i = 0; i < 10; i++)
  {
    // Create the attributes required
    populateAttributeList(arrays[i]->pAttributeList);
    hdf5->lock();
    BOOST_CHECK_NO_THROW(hdf5->processCallbacks(arrays[i]));
    hdf5->unlock();
    num_captured = hdf5->readInt(NDFileNumCapturedString);
    BOOST_CHECK_EQUAL(num_captured, i+1);
  }

  // Once the frames have been captured check the output file to verify it is as expected
  std::vector<hsize_t> odims;
  // Use the file reader utility to open the file
  HDF5FileReader fr("/tmp/test1_001.h5");
  // Verify the main dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/example/detector/data"), true);
  // Verify the main dataset has the dimensions of 10x10x10
  odims = fr.getDatasetDimensions("/example/detector/data");
  BOOST_CHECK_EQUAL(odims.size(), 3);
  BOOST_CHECK_EQUAL(odims[0], 10);
  BOOST_CHECK_EQUAL(odims[1], 10);
  BOOST_CHECK_EQUAL(odims[2], 10);

  // Verify the temperature dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/example/temperature"), true);
  // Verify the temperature dataset has one dimension of 10
  odims = fr.getDatasetDimensions("/example/temperature");
  BOOST_CHECK_EQUAL(odims.size(), 1);
  BOOST_CHECK_EQUAL(odims[0], 10);

  // Verify the data_link_in_same_group dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/example/data_link_in_same_group"), true);
  // Verify the data_link_in_same_group dataset has one dimension of 10
  odims = fr.getDatasetDimensions("/example/data_link_in_same_group");
  BOOST_CHECK_EQUAL(odims.size(), 1);
  BOOST_CHECK_EQUAL(odims[0], 10);


}

BOOST_AUTO_TEST_SUITE_END()
