/*
 * test_NDFileHDF5AttributeDataset.cpp
 *
 *  Created on: 8 May 2015
 *      Author: gnx91527
 */

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

#include "hdf5.h"
#include "testingutilities.h"
#include "HDF5FileReader.h"
#include "NDFileHDF5AttributeDataset.h"

BOOST_AUTO_TEST_CASE(test_AttributeOriginalDataset)
{
  // Open an HDF5 file for testing
  std::string filename = "/tmp/test_att.h5";
  hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, 0, 0);
  BOOST_REQUIRE_GT(file, -1);

  // Add a test group.
  std::string gname = "group";
  hid_t group = H5Gcreate(file, gname.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  BOOST_REQUIRE_GT(group, -1);

  std::tr1::shared_ptr<NDFileHDF5AttributeDataset> adPtr;

  // Create an attribute dataset of type NDAttrInt8
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att1", NDAttrInt8));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att1");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset1");
  adPtr->setParentGroupName(gname);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsInt8 val1 = 0;
  for (epicsInt8 index = 0; index < 32; index++){
    val1 = index;
    NDAttribute ndAttr("att1", "Test attribute 1", NDAttrSourceFunct, "test", NDAttrInt8, &val1);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrUInt8
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att2", NDAttrUInt8));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att2");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset2");
  adPtr->setParentGroupName(gname);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsUInt8 val2 = 0;
  for (epicsUInt8 index = 0; index < 34; index++){
    val2 = index;
    NDAttribute ndAttr("att2", "Test attribute 2", NDAttrSourceFunct, "test", NDAttrUInt8, &val2);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrInt16
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att3", NDAttrInt16));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att3");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset3");
  adPtr->setParentGroupName(gname);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsInt16 val3 = 0;
  for (epicsInt16 index = 0; index < 36; index++){
    val3 = index;
    NDAttribute ndAttr("att3", "Test attribute 3", NDAttrSourceFunct, "test", NDAttrInt16, &val3);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrUInt16
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att4", NDAttrUInt16));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att4");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset4");
  adPtr->setParentGroupName(gname);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsUInt16 val4 = 0;
  for (epicsUInt16 index = 0; index < 38; index++){
    val4 = index;
    NDAttribute ndAttr("att4", "Test attribute 4", NDAttrSourceFunct, "test", NDAttrUInt16, &val4);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrInt32
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att5", NDAttrInt32));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att5");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset5");
  adPtr->setParentGroupName(gname);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsInt32 val5 = 0;
  for (epicsInt32 index = 0; index < 40; index++){
    val5 = index;
    NDAttribute ndAttr("att5", "Test attribute 5", NDAttrSourceFunct, "test", NDAttrInt32, &val5);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrUInt32
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att6", NDAttrUInt32));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att6");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset6");
  adPtr->setParentGroupName(gname);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsUInt32 val6 = 0;
  for (epicsUInt32 index = 0; index < 42; index++){
    val6 = index;
    NDAttribute ndAttr("att6", "Test attribute 6", NDAttrSourceFunct, "test", NDAttrUInt32, &val6);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrFloat32
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att7", NDAttrFloat32));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att7");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset7");
  adPtr->setParentGroupName(gname);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsFloat32 val7 = 0;
  for (epicsFloat32 index = 0; index < 44; index++){
    val7 = index;
    NDAttribute ndAttr("att7", "Test attribute 7", NDAttrSourceFunct, "test", NDAttrFloat32, &val7);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrFloat64
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att8", NDAttrFloat64));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att8");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset8");
  adPtr->setParentGroupName(gname);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsFloat64 val8 = 0;
  for (epicsFloat64 index = 0; index < 46; index++){
    val8 = index;
    NDAttribute ndAttr("att8", "Test attribute 8", NDAttrSourceFunct, "test", NDAttrFloat64, &val8);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrString
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att9", NDAttrString));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att9");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset9");
  adPtr->setParentGroupName(gname);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  for (int index = 0; index < 48; index++){
    std::stringstream val9;
    val9 << "This is a test string: " << index;
    char sval9[128];
    strcpy(sval9, val9.str().c_str());
    NDAttribute ndAttr("att9", "Test attribute 9", NDAttrSourceFunct, "test", NDAttrString, &sval9);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute, set it for only writing OnOpen, then send it multiple OnFrame values
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att10", NDAttrInt8));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att10");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset10");
  adPtr->setParentGroupName(gname);
  // Set the when value to OnOpen
  adPtr->setWhenToSave(hdf5::OnFileOpen);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsInt8 val10 = 64;
  {
    NDAttribute ndAttr("att10", "Test attribute 10", NDAttrSourceFunct, "test", NDAttrInt8, &val10);
    adPtr->writeAttributeDataset(hdf5::OnFileOpen, &ndAttr, 0);
  }
  for (epicsInt8 index = 0; index < 32; index++){
    val10 = index;
    NDAttribute ndAttr("att10", "Test attribute 10", NDAttrSourceFunct, "test", NDAttrInt8, &val10);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  val10 = 65;
  {
    NDAttribute ndAttr("att10", "Test attribute 10", NDAttrSourceFunct, "test", NDAttrInt8, &val10);
    adPtr->writeAttributeDataset(hdf5::OnFileClose, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute, set it for only writing OnClose, then send it multiple OnFrame values
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att11", NDAttrInt8));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att11");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset11");
  adPtr->setParentGroupName(gname);
  // Set the when value to OnOpen
  adPtr->setWhenToSave(hdf5::OnFileClose);
  // Create the dataset with single item chunking
  adPtr->createDataset(1);
  // Now add some data values to the dataset
  epicsInt8 val11 = 64;
  {
    NDAttribute ndAttr("att11", "Test attribute 11", NDAttrSourceFunct, "test", NDAttrInt8, &val11);
    adPtr->writeAttributeDataset(hdf5::OnFileOpen, &ndAttr, 0);
  }
  for (epicsInt8 index = 0; index < 32; index++){
    val11 = index;
    NDAttribute ndAttr("att11", "Test attribute 11", NDAttrSourceFunct, "test", NDAttrInt8, &val11);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  val11 = 65;
  {
    NDAttribute ndAttr("att11", "Test attribute 11", NDAttrSourceFunct, "test", NDAttrInt8, &val11);
    adPtr->writeAttributeDataset(hdf5::OnFileClose, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Close the group
  H5Gclose(group);
  // Close the file
  H5Fclose(file);

  // Use the file reader utility to open the file
  HDF5FileReader fr(filename);
  // Verify the att1 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset1"), true);
  // Verify the att1 dataset has a single dimension of size 1
  std::vector<hsize_t> dims = fr.getDatasetDimensions("/group/dset1");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 32);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset1"), Int8);

  // Verify the att1 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset2"), true);
  // Verify the att1 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset2");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 34);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset2"), UInt8);

  // Verify the att1 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset3"), true);
  // Verify the att1 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset3");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 36);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset3"), Int16);

  // Verify the att1 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset4"), true);
  // Verify the att1 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset4");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 38);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset4"), UInt16);

  // Verify the att1 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset5"), true);
  // Verify the att1 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset5");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 40);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset5"), Int32);

  // Verify the att1 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset6"), true);
  // Verify the att1 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset6");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 42);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset6"), UInt32);

  // Verify the att1 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset7"), true);
  // Verify the att1 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset7");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 44);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset7"), Float32);

  // Verify the att1 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset8"), true);
  // Verify the att1 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset8");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 46);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset8"), Float64);

  // Verify the att2 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset9"), true);
  // Verify the att2 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset9");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 48);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset9"), String);

  // Verify the att10 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset10"), true);
  // Verify the att3 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset10");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset10"), Int8);

  // Verify the att11 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset11"), true);
  // Verify the att11 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset11");
  BOOST_CHECK_EQUAL(dims.size(), 1);
  BOOST_CHECK_EQUAL(dims[0], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset11"), Int8);

}

BOOST_AUTO_TEST_CASE(test_AttributeDimensionalDataset)
{
  // Open an HDF5 file for testing
  std::string filename = "/tmp/test_att.h5";
  hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, 0, 0);
  BOOST_REQUIRE_GT(file, -1);

  // Add a test group.
  std::string gname = "group";
  hid_t group = H5Gcreate(file, gname.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  BOOST_REQUIRE_GT(group, -1);

  std::tr1::shared_ptr<NDFileHDF5AttributeDataset> adPtr;
  // Create a 3 dimensional description of a dataset (3x4x5)
  int dimsize[3] = {3, 4, 5};
  int chunking[3] = {1, 1, 1};

  // Create an attribute dataset of type NDAttrInt8
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att1", NDAttrInt8));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att1");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset1");
  adPtr->setParentGroupName(gname);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsInt8 val1 = 0;
  for (epicsInt8 index = 0; index < 60; index++){
    val1 = index;
    NDAttribute ndAttr("att1", "Test attribute 1", NDAttrSourceFunct, "test", NDAttrInt8, &val1);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrUInt8
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att2", NDAttrUInt8));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att2");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset2");
  adPtr->setParentGroupName(gname);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsUInt8 val2 = 0;
  for (epicsUInt8 index = 0; index < 60; index++){
    val2 = index;
    NDAttribute ndAttr("att2", "Test attribute 2", NDAttrSourceFunct, "test", NDAttrUInt8, &val2);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrInt16
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att3", NDAttrInt16));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att3");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset3");
  adPtr->setParentGroupName(gname);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsInt16 val3 = 0;
  for (epicsInt16 index = 0; index < 60; index++){
    val3 = index;
    NDAttribute ndAttr("att3", "Test attribute 3", NDAttrSourceFunct, "test", NDAttrInt16, &val3);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrUInt16
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att4", NDAttrUInt16));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att4");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset4");
  adPtr->setParentGroupName(gname);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsUInt16 val4 = 0;
  for (epicsUInt16 index = 0; index < 60; index++){
    val4 = index;
    NDAttribute ndAttr("att4", "Test attribute 4", NDAttrSourceFunct, "test", NDAttrUInt16, &val4);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrInt32
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att5", NDAttrInt32));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att5");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset5");
  adPtr->setParentGroupName(gname);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsInt32 val5 = 0;
  for (epicsInt32 index = 0; index < 60; index++){
    val5 = index;
    NDAttribute ndAttr("att5", "Test attribute 5", NDAttrSourceFunct, "test", NDAttrInt32, &val5);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrUInt32
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att6", NDAttrUInt32));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att6");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset6");
  adPtr->setParentGroupName(gname);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsUInt32 val6 = 0;
  for (epicsUInt32 index = 0; index < 60; index++){
    val6 = index;
    NDAttribute ndAttr("att6", "Test attribute 6", NDAttrSourceFunct, "test", NDAttrUInt32, &val6);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrFloat32
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att7", NDAttrFloat32));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att7");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset7");
  adPtr->setParentGroupName(gname);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsFloat32 val7 = 0;
  for (epicsFloat32 index = 0; index < 60; index++){
    val7 = index;
    NDAttribute ndAttr("att7", "Test attribute 7", NDAttrSourceFunct, "test", NDAttrFloat32, &val7);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrFloat64
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att8", NDAttrFloat64));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att8");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset8");
  adPtr->setParentGroupName(gname);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsFloat64 val8 = 0;
  for (epicsFloat64 index = 0; index < 60; index++){
    val8 = index;
    NDAttribute ndAttr("att8", "Test attribute 8", NDAttrSourceFunct, "test", NDAttrFloat64, &val8);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute dataset of type NDAttrString
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att9", NDAttrString));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att9");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset9");
  adPtr->setParentGroupName(gname);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  for (int index = 0; index < 60; index++){
    std::stringstream val9;
    val9 << "This is a test string: " << index;
    char sval9[128];
    strcpy(sval9, val9.str().c_str());
    NDAttribute ndAttr("att9", "Test attribute 9", NDAttrSourceFunct, "test", NDAttrString, &sval9);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute, set it for only writing OnOpen, then send it multiple OnFrame values
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att10", NDAttrInt8));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att10");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset10");
  adPtr->setParentGroupName(gname);
  // Set the when value to OnOpen
  adPtr->setWhenToSave(hdf5::OnFileOpen);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsInt8 val10 = 64;
  {
    NDAttribute ndAttr("att10", "Test attribute 10", NDAttrSourceFunct, "test", NDAttrInt8, &val10);
    adPtr->writeAttributeDataset(hdf5::OnFileOpen, &ndAttr, 0);
  }
  for (epicsInt8 index = 0; index < 32; index++){
    val10 = index;
    NDAttribute ndAttr("att10", "Test attribute 10", NDAttrSourceFunct, "test", NDAttrInt8, &val10);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  val10 = 65;
  {
    NDAttribute ndAttr("att10", "Test attribute 10", NDAttrSourceFunct, "test", NDAttrInt8, &val10);
    adPtr->writeAttributeDataset(hdf5::OnFileClose, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Create an attribute, set it for only writing OnClose, then send it multiple OnFrame values
  adPtr = std::tr1::shared_ptr<NDFileHDF5AttributeDataset>(new NDFileHDF5AttributeDataset(file, "att11", NDAttrInt8));
  BOOST_CHECK_EQUAL(adPtr->getName(), "att11");
  // Set up the parent group name and the dataset name
  adPtr->setDsetName("dset11");
  adPtr->setParentGroupName(gname);
  // Set the when value to OnOpen
  adPtr->setWhenToSave(hdf5::OnFileClose);
  // Create the dataset with 3 dimensions
  adPtr->createDataset(true, 3, dimsize, chunking);
  // Now add some data values to the dataset
  epicsInt8 val11 = 64;
  {
    NDAttribute ndAttr("att11", "Test attribute 11", NDAttrSourceFunct, "test", NDAttrInt8, &val11);
    adPtr->writeAttributeDataset(hdf5::OnFileOpen, &ndAttr, 0);
  }
  for (epicsInt8 index = 0; index < 32; index++){
    val11 = index;
    NDAttribute ndAttr("att11", "Test attribute 11", NDAttrSourceFunct, "test", NDAttrInt8, &val11);
    adPtr->writeAttributeDataset(hdf5::OnFrame, &ndAttr, 0);
  }
  val11 = 65;
  {
    NDAttribute ndAttr("att11", "Test attribute 11", NDAttrSourceFunct, "test", NDAttrInt8, &val11);
    adPtr->writeAttributeDataset(hdf5::OnFileClose, &ndAttr, 0);
  }
  // Close the dataset
  adPtr->closeAttributeDataset();

  // Close the group
  H5Gclose(group);
  // Close the file
  H5Fclose(file);

  // Use the file reader utility to open the file
  HDF5FileReader fr(filename);
  // Verify the att1 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset1"), true);
  // Verify the att1 dataset has a single dimension of size 1
  std::vector<hsize_t> dims = fr.getDatasetDimensions("/group/dset1");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 3);
  BOOST_CHECK_EQUAL(dims[1], 4);
  BOOST_CHECK_EQUAL(dims[2], 5);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset1"), Int8);

  // Verify the att2 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset2"), true);
  // Verify the att2 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset2");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 3);
  BOOST_CHECK_EQUAL(dims[1], 4);
  BOOST_CHECK_EQUAL(dims[2], 5);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset2"), UInt8);

  // Verify the att3 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset3"), true);
  // Verify the att3 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset3");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 3);
  BOOST_CHECK_EQUAL(dims[1], 4);
  BOOST_CHECK_EQUAL(dims[2], 5);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset3"), Int16);

  // Verify the att4 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset4"), true);
  // Verify the att4 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset4");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 3);
  BOOST_CHECK_EQUAL(dims[1], 4);
  BOOST_CHECK_EQUAL(dims[2], 5);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset4"), UInt16);

  // Verify the att5 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset5"), true);
  // Verify the att5 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset5");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 3);
  BOOST_CHECK_EQUAL(dims[1], 4);
  BOOST_CHECK_EQUAL(dims[2], 5);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset5"), Int32);

  // Verify the att6 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset6"), true);
  // Verify the att6 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset6");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 3);
  BOOST_CHECK_EQUAL(dims[1], 4);
  BOOST_CHECK_EQUAL(dims[2], 5);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset6"), UInt32);

  // Verify the att7 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset7"), true);
  // Verify the att7 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset7");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 3);
  BOOST_CHECK_EQUAL(dims[1], 4);
  BOOST_CHECK_EQUAL(dims[2], 5);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset7"), Float32);

  // Verify the att8 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset8"), true);
  // Verify the att8 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset8");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 3);
  BOOST_CHECK_EQUAL(dims[1], 4);
  BOOST_CHECK_EQUAL(dims[2], 5);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset8"), Float64);

  // Verify the att9 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset9"), true);
  // Verify the att9 dataset has a a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset9");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 3);
  BOOST_CHECK_EQUAL(dims[1], 4);
  BOOST_CHECK_EQUAL(dims[2], 5);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset9"), String);

  // Verify the att10 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset10"), true);
  // Verify the att3 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset10");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 1);
  BOOST_CHECK_EQUAL(dims[1], 1);
  BOOST_CHECK_EQUAL(dims[2], 1);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset10"), Int8);

  // Verify the att11 dataset exists
  BOOST_CHECK_EQUAL(fr.checkDatasetExists("/group/dset11"), true);
  // Verify the att11 dataset has a single dimension of size 1
  dims = fr.getDatasetDimensions("/group/dset11");
  BOOST_CHECK_EQUAL(dims.size(), 5);
  BOOST_CHECK_EQUAL(dims[0], 1);
  BOOST_CHECK_EQUAL(dims[1], 1);
  BOOST_CHECK_EQUAL(dims[2], 1);
  BOOST_CHECK_EQUAL(dims[3], 1);
  BOOST_CHECK_EQUAL(dims[4], 1);
  BOOST_CHECK_EQUAL(fr.getDatasetType("/group/dset11"), Int8);

}

