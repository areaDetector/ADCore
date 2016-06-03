/*
 * test_NDFileHDF5ExtraDimensions.cpp
 *
 *  Created on: 10 Nov 2015
 *      Author: gnx91527
 */

// This must be defined first to hack the relevant classes, turning all private variables and
// methods into public for unit testing.  It's not pretty, but necessary until the classes are
// all re-written to be more OO.
#define _UNITTEST_HDF5_


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
#include "HDF5PluginWrapper.h"
#include "HDF5FileReader.h"
#include "NDFileHDF5Dataset.h"

const int TEST_RANK = 5;
const int MAX_DIM_SIZE = 10;
hid_t dataspace;
NDArray* parr;

NDFileHDF5Dataset *createTestDataset(int rank, int *max_dim_size, asynUser *pasynUser, hid_t groupID, const std::string& dsetname)
{
  // Add the test dataset.
  hid_t datasetID = -1;

  hid_t dset_access_plist = H5Pcreate(H5P_DATASET_ACCESS);
  hsize_t nbytes = 1024;
  hsize_t nslots = 50001;
  hid_t datatype = H5T_NATIVE_INT8;
  hsize_t dims[rank];
  for (int i=0; i < rank-2; i++) dims[i] = 1;
  for (int i=rank-2; i < rank; i++) dims[i] = max_dim_size[i];
  hsize_t maxdims[rank];
  for (int i=0; i < rank; i++) maxdims[i] = max_dim_size[i];
  hsize_t chunkdims[rank];
  for (int i=0; i < rank-2; i++) chunkdims[i] = 1;
  for (int i=rank-2; i < rank; i++) chunkdims[i] = max_dim_size[i];
  //hid_t dataspace = H5Screate_simple(rank, dims, maxdims);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  hid_t cparms = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_chunk(cparms, rank, chunkdims);
  void *ptrFillValue = (void*)calloc(8, sizeof(char));
  *(char *)ptrFillValue = (char)0;
  H5Pset_fill_value(cparms, datatype, ptrFillValue);

  H5Pset_chunk_cache(dset_access_plist, (size_t)nslots, (size_t)nbytes, 1.0);
  datasetID = H5Dcreate2(groupID, dsetname.c_str(), datatype, dataspace, H5P_DEFAULT, cparms, dset_access_plist);

  // Now create a dataset
  NDFileHDF5Dataset *dataset = new NDFileHDF5Dataset(pasynUser, dsetname, datasetID);
  int extraDims = rank-2;
  int extra_dims[extraDims];
  for (int i=0; i < extraDims; i++) extra_dims[i] = max_dim_size[i];
  int user_chunking[extraDims];
  for (int i=0; i < extraDims; i++) user_chunking[i] = 1;

  // Create a test array
  NDArrayInfo_t arrinfo;
  parr = new NDArray();
  parr->dataType = NDInt8;
  parr->ndims = 2;
  parr->pNDArrayPool = NULL;
  parr->getInfo(&arrinfo);
  parr->dataSize = arrinfo.bytesPerElement;
  for (unsigned int i = 0; i < 2; i++){
    unsigned int dim_index = rank-(i+1);
    parr->dataSize *= max_dim_size[dim_index];
    parr->dims[i].size = max_dim_size[dim_index];
  }
  parr->pData = calloc(parr->dataSize, sizeof(char));
  memset(parr->pData, 0, parr->dataSize);
  parr->uniqueId = 0;

  dataset->configureDims(parr, true, extraDims, extra_dims, user_chunking);

  return dataset;
}

void testDimensions(NDFileHDF5Dataset *dataset, int ndims, int extradims, int *values)
{
  hsize_t val;
  int counter = 0;

  // Test the maximum dimensions of the dataset
  for (int i=0; i < extradims; i++){
    //BOOST_TEST_MESSAGE("Verify maxdim[" << i << "] == " << values[counter]);
    val = dataset->maxdims_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }
  for (int i=extradims; i < extradims+ndims; i++){
    //BOOST_TEST_MESSAGE("Verify maxdim[" << i << "] == " << values[counter]);
    val = dataset->maxdims_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }

  // Test the current dimension sizes of the dataset
  for (int i=0; i < extradims; i++){
    //BOOST_TEST_MESSAGE("Verify current dim[" << i << "] == " << values[counter]);
    val = dataset->dims_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }
  for (int i=extradims; i < extradims+ndims; i++){
    //BOOST_TEST_MESSAGE("Verify current dim[" << i << "] == " << values[counter]);
    val = dataset->dims_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }

  // Test the offsets of the dataset
  for (int i=0; i < extradims+ndims; i++){
    //BOOST_TEST_MESSAGE("Verify offset[" << i << "] == " << values[counter]);
    val = dataset->offset_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }

  // Test the virtual dimension sizes of the dataset
  for (int i=0; i < extradims; i++){
    //BOOST_TEST_MESSAGE("Verify current virtualdim[" << i << "] == " << values[counter]);
    val = dataset->virtualdims_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }
}

void testOffsets(NDFileHDF5Dataset *dataset, int ndims, int extradims, int *values)
{
  hsize_t val;

  // Test the offsets of the dataset
  for (int i=0; i < extradims+ndims; i++){
    //BOOST_TEST_MESSAGE("Verify offset[" << i << "] == " << values[i]);
    val = dataset->offset_[i];
    BOOST_REQUIRE_EQUAL(val, values[i]);
  }
}

void testDims(NDFileHDF5Dataset *dataset, int ndims, int extradims, int *values)
{
  hsize_t val;
  int counter = 0;

  // Test the current dimension sizes of the dataset
  for (int i=0; i < extradims; i++){
    //BOOST_TEST_MESSAGE("Verify current dim[" << i << "] == " << values[counter]);
    val = dataset->dims_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }
  for (int i=extradims; i < extradims+ndims; i++){
    //BOOST_TEST_MESSAGE("Verify current dim[" << i << "] == " << values[counter]);
    val = dataset->dims_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }
}

void testMaxDims(NDFileHDF5Dataset *dataset, int ndims, int extradims, int *values)
{
  hsize_t val;
  int counter = 0;

  // Test the maximum dimensions of the dataset
  for (int i=0; i < extradims; i++){
    //BOOST_TEST_MESSAGE("Verify maxdim[" << i << "] == " << values[counter]);
    val = dataset->maxdims_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }
  for (int i=extradims; i < extradims+ndims; i++){
    //BOOST_TEST_MESSAGE("Verify maxdim[" << i << "] == " << values[counter]);
    val = dataset->maxdims_[i];
    BOOST_REQUIRE_EQUAL(val, values[counter]);
    counter++;
  }
}

void testVirtualDims(NDFileHDF5Dataset *dataset, int ndims, int extradims, int *values)
{
  hsize_t val;

  // Test the virtual dimension sizes of the dataset
  for (int i=0; i < extradims; i++){
    //BOOST_TEST_MESSAGE("Verify current virtualdim[" << i << "] == " << values[i]);
    val = dataset->virtualdims_[i];
    BOOST_REQUIRE_EQUAL(val, values[i]);
  }
}

void updateOffsets(int dim, int *offsets, int *vdimsizes)
{
  if (dim >= 0){
    offsets[dim]++;
    if (offsets[dim] == vdimsizes[dim]){
      offsets[dim] = 0;
      updateOffsets(dim-1, offsets, vdimsizes);
    }
  }
}

void updateDimensions(int frame, int dim, int *dims, int *vdimsizes)
{
  dims[dim]++;
  if (dims[dim] > vdimsizes[dim]){
    dims[dim] = vdimsizes[dim];
  }
  if (dim > 0){
    int nextindex = frame / vdimsizes[dim];
    if (nextindex >= dims[dim-1]){
      updateDimensions(nextindex, dim-1, dims, vdimsizes);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_ExtraDatasetDimensions)
{
  // Create ourselves an asyn user
  asynUser *pasynUser = pasynManager->createAsynUser(0, 0);

  // Open an HDF5 file for testing
  std::string filename = "/tmp/test_dim1.h5";
  hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, 0, 0);
  BOOST_REQUIRE_GT(file, -1);

  // Add a test group.
  std::string gname = "group";
  hid_t group = H5Gcreate(file, gname.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  BOOST_REQUIRE_GT(group, -1);

  // Now create a dataset (2x3x4x10x8)
  int rank = 5;
  int dims[5] = {2, 3, 4, 10, 8};
  NDFileHDF5Dataset *dataset = createTestDataset(rank, dims, pasynUser, group, "test_data");

  // Setup the expected dimension sizes
  int maxdims[5]     = {-1, -1, -1, 10,  8}; // Current dimension sizes
  int testdims[5]    = { 1,  1,  1, 10,  8}; // Current dimension sizes
  int testoffsets[5] = { 0,  0,  0,  0,  0}; // Current offsets
  int vdimsizes[3]   = { 2,  3,  4};         // Current offsets

  // Test the dataset internal dimensions against our expected values
  testVirtualDims(dataset, 2, rank-2, vdimsizes);
  testMaxDims(dataset, 2, rank-2, maxdims);
  testOffsets(dataset, 2, rank-2, testoffsets);
  testDims(dataset, 2, rank-2, testdims);

  // Now extend the dataset
  dataset->extendDataSet(rank-3);

  // Re-test the dataset internal dimensions, they should be unchanged
  testVirtualDims(dataset, 2, rank-2, vdimsizes);
  testMaxDims(dataset, 2, rank-2, maxdims);
  testOffsets(dataset, 2, rank-2, testoffsets);
  testDims(dataset, 2, rank-2, testdims);

  // Setup the frame size
  hsize_t framesize[5] = {1, 1, 1, 10, 8};

  // Now perform test writes and extensions
  for (int writes = 1; writes <= 24; writes++){
    //BOOST_TEST_MESSAGE("Write frame " << writes);
    // Write a frame
    dataset->writeFile(parr, H5T_NATIVE_INT8, dataspace, framesize);

    // Extend the dataset
    dataset->extendDataSet(rank-3);

    // Update the expected offset values
    updateOffsets(2, testoffsets, vdimsizes);
    // Update the expected dimension values
    updateDimensions(writes, 2, testdims, vdimsizes);

    // Re-test the dataset internal dimensions, they should be unchanged
    testVirtualDims(dataset, 2, rank-2, vdimsizes);
    testMaxDims(dataset, 2, rank-2, maxdims);
    testOffsets(dataset, 2, rank-2, testoffsets);
    testDims(dataset, 2, rank-2, testdims);
  }
}

BOOST_AUTO_TEST_CASE(test_TenExtraDimensions)
{
  // Create ourselves an asyn user
  asynUser *pasynUser = pasynManager->createAsynUser(0, 0);

  // Open an HDF5 file for testing
  std::string filename = "/tmp/test_dim2.h5";
  hid_t file = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, 0, 0);
  BOOST_REQUIRE_GT(file, -1);

  // Add a test group.
  std::string gname = "group";
  hid_t group = H5Gcreate(file, gname.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  BOOST_REQUIRE_GT(group, -1);

  // Now create a dataset (2x3x4x2x3x4x2x3x4x2x10x8)
  int rank = 12;
  int dims[12] = {2, 3, 4, 2, 3, 4, 2, 3, 4, 2, 10, 8};
  NDFileHDF5Dataset *dataset = createTestDataset(rank, dims, pasynUser, group, "test_data");

  // Setup the expected dimension sizes
  int maxdims[12]     = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 8}; // Current dimension sizes
  int testdims[12]    = { 1,  1,  1,  1,  1,  1,  1 , 1,  1,  1, 10, 8}; // Current dimension sizes
  int testoffsets[12] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0}; // Current offsets
  int vdimsizes[10]   = { 2,  3,  4,  2,  3,  4,  2,  3,  4,  2};        // Current offsets

  // Test the dataset internal dimensions against our expected values
  testVirtualDims(dataset, 2, rank-2, vdimsizes);
  testMaxDims(dataset, 2, rank-2, maxdims);
  testOffsets(dataset, 2, rank-2, testoffsets);
  testDims(dataset, 2, rank-2, testdims);

  // Now extend the dataset
  dataset->extendDataSet(rank-3);

  // Re-test the dataset internal dimensions, they should be unchanged
  testVirtualDims(dataset, 2, rank-2, vdimsizes);
  testMaxDims(dataset, 2, rank-2, maxdims);
  testOffsets(dataset, 2, rank-2, testoffsets);
  testDims(dataset, 2, rank-2, testdims);


  // Setup the framesize
  hsize_t framesize[12] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 10, 8};

  // Now perform test writes and extensions
  for (int writes = 1; writes <= 27647; writes++){
    //BOOST_TEST_MESSAGE("Write frame " << writes);
    // Write a frame
    dataset->writeFile(parr, H5T_NATIVE_INT8, dataspace, framesize);

    // Extend the dataset
    dataset->extendDataSet(rank-3);

    // Update the expected offset values
    updateOffsets(9, testoffsets, vdimsizes);
    // Update the expected dimension values
    updateDimensions(writes, 9, testdims, vdimsizes);

    // Re-test the dataset internal dimensions
    testVirtualDims(dataset, 2, rank-2, vdimsizes);
    testMaxDims(dataset, 2, rank-2, maxdims);
    testOffsets(dataset, 2, rank-2, testoffsets);
    testDims(dataset, 2, rank-2, testdims);
  }
}


BOOST_AUTO_TEST_CASE(test_PluginExtraDimensions)
{
  std::tr1::shared_ptr<asynPortDriver> driver;
  std::tr1::shared_ptr<HDF5PluginWrapper> hdf5;

  new NDArrayPool(100, 0);

  // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
  // change it slightly for each test case.
  std::string simport("simHDF5test"), testport("HDF5");
  uniqueAsynPortName(simport);
  uniqueAsynPortName(testport);

  // We need some upstream driver for our test plugin so that calls to connectArrayPort don't fail, but we can then ignore it and send
  // arrays by calling processCallbacks directly.
  driver = std::tr1::shared_ptr<asynPortDriver>(new asynPortDriver(simport.c_str(), 0, 1, asynGenericPointerMask, asynGenericPointerMask, 0, 0, 0, 2000000));

  // This is the plugin under test
  hdf5 = std::tr1::shared_ptr<HDF5PluginWrapper>(new HDF5PluginWrapper(testport.c_str(),
                                                                       50,
                                                                       1,
                                                                       simport.c_str(),
                                                                       0,
                                                                       0,
                                                                       2000000));


  // Enable the plugin
  hdf5->write(NDPluginDriverEnableCallbacksString, 1);
  hdf5->write(NDPluginDriverBlockingCallbacksString, 1);


  size_t tmpdims[] = {1024,512};
  std::vector<size_t>dims(tmpdims, tmpdims + sizeof(tmpdims)/sizeof(tmpdims[0]));

  // Create some test arrays
  std::vector<NDArray*>arrays(10);
  fillNDArrays(dims, NDUInt32, arrays);


  // Test method: NDFileHDF5::calcNumFrames()

  int numCapture = 0;
  // First try 1 extra dim, (n)4x6
  hdf5->write(str_NDFileHDF5_nExtraDims, 1);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 6);
  hdf5->calcNumFrames();
  numCapture = hdf5->readInt(NDFileNumCaptureString);
  BOOST_REQUIRE_EQUAL(numCapture, 24);

  // Try 2 extra dims, 5x7x9
  numCapture = 0;
  hdf5->write(str_NDFileHDF5_nExtraDims, 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 5);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 7);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 9);
  hdf5->calcNumFrames();
  numCapture = hdf5->readInt(NDFileNumCaptureString);
  BOOST_REQUIRE_EQUAL(numCapture, 315);

  // Try 3 extra dims, 2x3x4x5
  numCapture = 0;
  hdf5->write(str_NDFileHDF5_nExtraDims, 3);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 3);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[3], 5);
  hdf5->calcNumFrames();
  numCapture = hdf5->readInt(NDFileNumCaptureString);
  BOOST_REQUIRE_EQUAL(numCapture, 120);

  // Try 4 extra dims, 2x4x6x8x10
  numCapture = 0;
  hdf5->write(str_NDFileHDF5_nExtraDims, 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 6);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[3], 8);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[4], 10);
  hdf5->calcNumFrames();
  numCapture = hdf5->readInt(NDFileNumCaptureString);
  BOOST_REQUIRE_EQUAL(numCapture, 3840);

  // Try 5 extra dims, 2x3x4x5x6x7
  numCapture = 0;
  hdf5->write(str_NDFileHDF5_nExtraDims, 5);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 3);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[3], 5);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[4], 6);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[5], 7);
  hdf5->calcNumFrames();
  numCapture = hdf5->readInt(NDFileNumCaptureString);
  BOOST_REQUIRE_EQUAL(numCapture, 5040);

  // Try 6 extra dims, 2x3x4x5x6x7x8
  numCapture = 0;
  hdf5->write(str_NDFileHDF5_nExtraDims, 6);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 3);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[3], 5);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[4], 6);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[5], 7);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[6], 8);
  hdf5->calcNumFrames();
  numCapture = hdf5->readInt(NDFileNumCaptureString);
  BOOST_REQUIRE_EQUAL(numCapture, 40320);

  // Try 7 extra dims, 2x3x4x5x6x7x8x9
  numCapture = 0;
  hdf5->write(str_NDFileHDF5_nExtraDims, 7);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 3);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[3], 5);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[4], 6);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[5], 7);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[6], 8);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[7], 9);
  hdf5->calcNumFrames();
  numCapture = hdf5->readInt(NDFileNumCaptureString);
  BOOST_REQUIRE_EQUAL(numCapture, 362880);

  // Try 8 extra dims, 2x3x4x5x6x7x8x9x10
  numCapture = 0;
  hdf5->write(str_NDFileHDF5_nExtraDims, 8);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 3);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[3], 5);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[4], 6);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[5], 7);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[6], 8);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[7], 9);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[8], 10);
  hdf5->calcNumFrames();
  numCapture = hdf5->readInt(NDFileNumCaptureString);
  BOOST_REQUIRE_EQUAL(numCapture, 3628800);

  // Try 9 extra dims, 2x3x4x5x6x7x8x9x10x11
  numCapture = 0;
  hdf5->write(str_NDFileHDF5_nExtraDims, 9);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 3);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[3], 5);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[4], 6);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[5], 7);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[6], 8);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[7], 9);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[8], 10);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[9], 11);
  hdf5->calcNumFrames();
  numCapture = hdf5->readInt(NDFileNumCaptureString);
  BOOST_REQUIRE_EQUAL(numCapture, 39916800);

  // Test method: NDFileHDF5::configureDims()

  // Set 2 extra dims
  hdf5->write(str_NDFileHDF5_nExtraDims, 2);
  // Set multiframe true
  hdf5->multiFrameFile = true;
  // Set extra dim sizes n=2 x=3 y=4
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 3);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 4);
  // Set nFrameChunks = 1
  hdf5->write(str_NDFileHDF5_nFramesChunks, 1);
  // Set nRowChunks = 512
  hdf5->write(str_NDFileHDF5_nRowChunks, 512);
  // Set nColChunks = 1024
  hdf5->write(str_NDFileHDF5_nColChunks, 1024);
  // Set the file write mode to stream
  hdf5->write(NDFileWriteModeString, NDFileModeStream);
  // Call the configure dims method
  hdf5->configureDims(arrays[0]);
  // Verify the dimensions
  BOOST_REQUIRE_EQUAL(hdf5->dims[0], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[1], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[2], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[3], 512);
  BOOST_REQUIRE_EQUAL(hdf5->dims[4], 1024);
  // Verify the maximum dimensions
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[0], 4);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[1], 3);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[2], 2);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[3], 512);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[4], 1024);
  // Verify the chunk dimensions
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[0], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[1], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[2], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[3], 512);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[4], 1024);
  // Verify the offsets
  BOOST_REQUIRE_EQUAL(hdf5->offset[0], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[1], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[2], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[3], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[4], 0);
  // Verify the virtual dims
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[0], 4);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[1], 3);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[2], 2);


  // Set 9 extra dims
  hdf5->write(str_NDFileHDF5_nExtraDims, 9);
  // Set multiframe true
  hdf5->multiFrameFile = true;
  // Set extra dim sizes 1st=2 2nd=3 3rd=4 4th=5 5th=6 6th=7 7th=8 8th=9 9th=10 10th=11
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[0], 2);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[1], 3);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[2], 4);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[3], 5);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[4], 6);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[5], 7);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[6], 8);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[7], 9);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[8], 10);
  hdf5->write(NDFileHDF5::str_NDFileHDF5_extraDimSize[9], 11);
  // Set nFrameChunks = 1
  hdf5->write(str_NDFileHDF5_nFramesChunks, 1);
  // Set nRowChunks = 512
  hdf5->write(str_NDFileHDF5_nRowChunks, 512);
  // Set nColChunks = 1024
  hdf5->write(str_NDFileHDF5_nColChunks, 1024);
  // Set the file write mode to stream
  hdf5->write(NDFileWriteModeString, NDFileModeStream);
  // Call the configure dims method
  hdf5->configureDims(arrays[0]);
  // Verify the dimensions
  BOOST_REQUIRE_EQUAL(hdf5->dims[0], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[1], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[2], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[3], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[4], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[5], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[6], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[7], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[8], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[9], 1);
  BOOST_REQUIRE_EQUAL(hdf5->dims[10], 512);
  BOOST_REQUIRE_EQUAL(hdf5->dims[11], 1024);
  // Verify the maximum dimensions
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[0],  11);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[1],  10);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[2],  9);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[3],  8);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[4],  7);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[5],  6);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[6],  5);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[7],  4);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[8],  3);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[9],  2);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[10], 512);
  BOOST_REQUIRE_EQUAL(hdf5->maxdims[11], 1024);
  // Verify the chunk dimensions
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[0], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[1], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[2], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[3], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[4], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[5], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[6], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[7], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[8], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[9], 1);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[10], 512);
  BOOST_REQUIRE_EQUAL(hdf5->chunkdims[11], 1024);
  // Verify the offsets
  BOOST_REQUIRE_EQUAL(hdf5->offset[0], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[1], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[2], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[3], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[4], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[5], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[6], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[7], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[8], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[9], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[10], 0);
  BOOST_REQUIRE_EQUAL(hdf5->offset[11], 0);
  // Verify the virtual dims
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[0], 11);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[1], 10);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[2], 9);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[3], 8);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[4], 7);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[5], 6);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[6], 5);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[7], 4);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[8], 3);
  BOOST_REQUIRE_EQUAL(hdf5->virtualdims[9], 2);

}

