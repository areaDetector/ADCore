/*
 * NDFileHDF5AttributeDataset.cpp
 *
 *  Created on: 30 Apr 2015
 *      Author: gnx91527
 */

#include "NDFileHDF5AttributeDataset.h"
#include <epicsString.h>
#include <iostream>

NDFileHDF5AttributeDataset::NDFileHDF5AttributeDataset(hid_t file, const std::string& name, NDAttrDataType_t type) :
  name_(name),
  dsetName_(name),
  file_(file),
  type_(type),
  groupName_(""),
  dataset_(-1),
  dataspace_(-1),
  memspace_(-1),
  datatype_(0),
  cparm_(-1),
  filespace_(-1),
  ptrFillValue_(0),
  dims_(NULL),
  offset_(NULL),
  chunk_(NULL),
  maxdims_(NULL),
  virtualdims_(NULL),
  elementSize_(NULL),
  rank_(0),
  nextRecord_(0),
  extraDimensions_(0),
  whenToSave_(hdf5::OnFrame)
{
  //printf("Constructor called for %s\n", name.c_str());
  // Allocate enough memory for the fill value to accept any data type
  ptrFillValue_ = (void*)calloc(8, sizeof(char));
}

NDFileHDF5AttributeDataset::~NDFileHDF5AttributeDataset()
{
  //printf("Destructor called for %s\n", name_.c_str());
  // Free the memory that was allocated for the fill value
  free(ptrFillValue_);
}

void NDFileHDF5AttributeDataset::setDsetName(const std::string& dsetName)
{
  dsetName_ = dsetName;
}

void NDFileHDF5AttributeDataset::setWhenToSave(hdf5::When_t whenToSave)
{
  whenToSave_ = whenToSave;
}

void NDFileHDF5AttributeDataset::setParentGroupName(const std::string& group)
{
  groupName_ = group;
}

asynStatus NDFileHDF5AttributeDataset::createDataset(int user_chunking)
{
  asynStatus status = asynSuccess;
  // Setup the HDF datatype
  status = this->typeAsHdf();
  // Configure the dimensions for this dataset
  this->configureDims(user_chunking);

  status = createHDF5Dataset();

  return status;
}

asynStatus NDFileHDF5AttributeDataset::createDataset(bool multiframe, int extradimensions, int *extra_dims, int *user_chunking)
{
  asynStatus status = asynSuccess;
  // Setup the HDF datatype
  status = this->typeAsHdf();
  // Configure the dimensions for this dataset
  this->configureDimsFromDataset(multiframe, extradimensions, extra_dims, user_chunking);

  status = createHDF5Dataset();

  return status;
}

asynStatus NDFileHDF5AttributeDataset::createHDF5Dataset()
{
  asynStatus status = asynSuccess;

  cparm_ = H5Pcreate(H5P_DATASET_CREATE);

  H5Pset_fill_value(cparm_, datatype_, ptrFillValue_);

  H5Pset_chunk(cparm_, rank_, chunk_);

  dataspace_ = H5Screate_simple(rank_, dims_, maxdims_);

  // Open the group by its name
  hid_t dsetgroup;

  if (groupName_ != ""){
    dsetgroup = H5Gopen(file_, groupName_.c_str(), H5P_DEFAULT);
  } else {
    dsetgroup = file_;
  }

  // Now create the dataset
  dataset_ = H5Dcreate2(dsetgroup, dsetName_.c_str(),
                        datatype_, dataspace_,
                        H5P_DEFAULT, cparm_, H5P_DEFAULT);

  if (groupName_ != ""){
    H5Gclose(dsetgroup);
  }

  memspace_ = H5Screate_simple(rank_, elementSize_, NULL);

  return status;
}

asynStatus NDFileHDF5AttributeDataset::writeAttributeDataset(hdf5::When_t whenToSave, NDAttribute *ndAttr, int flush)
{
  asynStatus status = asynSuccess;
  char * stackbuf[MAX_ATTRIBUTE_STRING_SIZE];
  void* pDatavalue = stackbuf;
  int ret;
  //check if the attribute is meant to be saved at this time
  if (whenToSave_ == whenToSave) {
    // Extend the dataset as required to store the data
    extendDataSet();

    // find the data based on datatype
    ret = ndAttr->getValue(ndAttr->getDataType(), pDatavalue, MAX_ATTRIBUTE_STRING_SIZE);
    if (ret == ND_ERROR) {
      memset(pDatavalue, 0, MAX_ATTRIBUTE_STRING_SIZE);
    }
    // Work with HDF5 library to select a suitable hyperslab (one element) and write the new data to it
    H5Dset_extent(dataset_, dims_);
    filespace_ = H5Dget_space(dataset_);

    // Select the hyperslab
    H5Sselect_hyperslab(filespace_, H5S_SELECT_SET, offset_, NULL, elementSize_, NULL);

    // Write the data to the hyperslab.
    H5Dwrite(dataset_, datatype_, memspace_, filespace_, H5P_DEFAULT, pDatavalue);


    // Check if we are being asked to flush
    if (flush == 1){
      status = this->flushDataset();
    }

    H5Sclose(filespace_);
    nextRecord_++;
  }

  return status;
}

asynStatus NDFileHDF5AttributeDataset::writeAttributeDataset(hdf5::When_t whenToSave, hsize_t *offsets, NDAttribute *ndAttr, int flush)
{
  asynStatus status = asynSuccess;
  char * stackbuf[MAX_ATTRIBUTE_STRING_SIZE];
  void* pDatavalue = stackbuf;
  int ret;
  //check if the attribute is meant to be saved at this time
  if (whenToSave_ == whenToSave) {
    // Extend the dataset as required to store the data
    extendDataSet(offsets);

    // find the data based on datatype
    ret = ndAttr->getValue(ndAttr->getDataType(), pDatavalue, MAX_ATTRIBUTE_STRING_SIZE);
    if (ret == ND_ERROR) {
      memset(pDatavalue, 0, MAX_ATTRIBUTE_STRING_SIZE);
    }
    // Work with HDF5 library to select a suitable hyperslab (one element) and write the new data to it
    H5Dset_extent(dataset_, dims_);
    filespace_ = H5Dget_space(dataset_);

    // Select the hyperslab
    H5Sselect_hyperslab(filespace_, H5S_SELECT_SET, offset_, NULL, elementSize_, NULL);

    // Write the data to the hyperslab.
    H5Dwrite(dataset_, datatype_, memspace_, filespace_, H5P_DEFAULT, pDatavalue);


    // Check if we are being asked to flush
    if (flush == 1){
      status = this->flushDataset();
    }

    H5Sclose(filespace_);
    nextRecord_++;
  }

  return status;
}

asynStatus NDFileHDF5AttributeDataset::closeAttributeDataset()
{
  //printf("close called for %s\n", name_.c_str());
  H5Dclose(dataset_);
  H5Sclose(memspace_);
  H5Sclose(dataspace_);
  H5Pclose(cparm_);
  return asynSuccess;
}

asynStatus NDFileHDF5AttributeDataset::configureDims(int user_chunking)
{
  asynStatus status = asynSuccess;
  int ndims = 2; // for simple dataset there are only 2 dimensions

  if (this->maxdims_     != NULL) free(this->maxdims_);
  if (this->chunk_       != NULL) free(this->chunk_);
  if (this->dims_        != NULL) free(this->dims_);
  if (this->offset_      != NULL) free(this->offset_);
  if (this->elementSize_ != NULL) free(this->elementSize_);

  this->maxdims_      = (hsize_t*)calloc(ndims, sizeof(hsize_t));
  this->chunk_        = (hsize_t*)calloc(ndims, sizeof(hsize_t));
  this->dims_         = (hsize_t*)calloc(ndims, sizeof(hsize_t));
  this->offset_       = (hsize_t*)calloc(ndims, sizeof(hsize_t));
  this->elementSize_  = (hsize_t*)calloc(ndims, sizeof(hsize_t));

  offset_[0] = 0;
  offset_[1] = 0;

  maxdims_[0] = H5S_UNLIMITED;
  maxdims_[1] = H5S_UNLIMITED;

  // Creating extendible data sets
  dims_[0] = 1;
  if (type_ < NDAttrString){
    dims_[1] = 1;
    chunk_[0] = user_chunking;
    chunk_[1] = 1;
    elementSize_[0] = 1;
    elementSize_[1] = 1;
    rank_ = 1;
  } else {
    // String dataset required, use type N5T_NATIVE_CHAR
    dims_[1] = MAX_ATTRIBUTE_STRING_SIZE;
    chunk_[0] = user_chunking;
    chunk_[1] = MAX_ATTRIBUTE_STRING_SIZE;
    elementSize_[0] = 1;
    elementSize_[1] = MAX_ATTRIBUTE_STRING_SIZE;
    rank_ = 2;
  }

  return status;
}

asynStatus NDFileHDF5AttributeDataset::configureDimsFromDataset(bool multiframe, int extradimensions, int *extra_dims, int *user_chunking)
{
  asynStatus status = asynSuccess;
  int i=0, extradims = 0, ndims=0;

  extradims = extradimensions;
  extraDimensions_ = extradimensions;

  ndims = 2 + extradims;

  // first check whether the dimension arrays have been allocated
  // or the number of dimensions have changed.
  // If necessary free and reallocate new memory.
  if (this->maxdims_ == NULL || this->rank_ != ndims){
    if (this->maxdims_     != NULL) free(this->maxdims_);
    if (this->chunk_       != NULL) free(this->chunk_);
    if (this->dims_        != NULL) free(this->dims_);
    if (this->offset_      != NULL) free(this->offset_);
    if (this->virtualdims_ != NULL) free(this->virtualdims_);
    if (this->elementSize_ != NULL) free(this->elementSize_);

    this->maxdims_       = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->chunk_         = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->dims_          = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->offset_        = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->virtualdims_   = (hsize_t*)calloc(extradims, sizeof(hsize_t));
    this->elementSize_   = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
  }

  if (multiframe){
    // Configure the virtual dimensions -i.e. dimensions in addition to the frame format.
    // Normally set to just 1 by default or -1 unlimited (in HDF5 terms)
    for (i=0; i<extradims; i++){
      this->elementSize_[i] = 1;
      this->chunk_[i]       = user_chunking[i];
      this->maxdims_[i]     = H5S_UNLIMITED;
      this->dims_[i]        = 1;
      this->offset_[i]      = 0; // because we increment offset *before* each write we need to start at -1
      this->virtualdims_[i] = extra_dims[i];
    }
  }

  this->rank_ = ndims;

  if (type_ < NDAttrString){
    elementSize_[extradims]   = 1;
    elementSize_[extradims+1] = 1;
    chunk_[extradims]         = 1;
    chunk_[extradims+1]       = 1;
    maxdims_[extradims]       = 1;
    maxdims_[extradims+1]     = 1;
    dims_[extradims]          = 1;
    dims_[extradims+1]        = 1;
    offset_[extradims]        = 0;
    offset_[extradims+1]      = 0;
  } else {
    // String dataset required, use type N5T_NATIVE_CHAR
    elementSize_[extradims]   = 1;
    elementSize_[extradims+1] = MAX_ATTRIBUTE_STRING_SIZE;
    chunk_[extradims]         = 1;
    chunk_[extradims+1]       = MAX_ATTRIBUTE_STRING_SIZE;
    maxdims_[extradims]       = 1;
    maxdims_[extradims+1]     = MAX_ATTRIBUTE_STRING_SIZE;
    dims_[extradims]          = 1;
    dims_[extradims+1]        = MAX_ATTRIBUTE_STRING_SIZE;
    offset_[extradims]        = 0;
    offset_[extradims+1]      = 0;
  }

  return status;
}

asynStatus NDFileHDF5AttributeDataset::typeAsHdf()
{
  asynStatus status = asynSuccess;
  int fillvalue = 0;

  switch (type_)
  {
    case NDAttrString:
      datatype_ = H5T_NATIVE_CHAR;
      *(epicsUInt8*)this->ptrFillValue_ = (epicsUInt8)fillvalue;
      break;
    case NDInt8:
      datatype_ = H5T_NATIVE_INT8;
      *(epicsInt8*)this->ptrFillValue_ = (epicsInt8)fillvalue;
      break;
    case NDUInt8:
      datatype_ = H5T_NATIVE_UINT8;
      *(epicsUInt8*)this->ptrFillValue_ = (epicsUInt8)fillvalue;
      break;
    case NDInt16:
      datatype_ = H5T_NATIVE_INT16;
      *(epicsInt16*)this->ptrFillValue_ = (epicsInt16)fillvalue;
      break;
    case NDUInt16:
      datatype_ = H5T_NATIVE_UINT16;
      *(epicsUInt16*)this->ptrFillValue_ = (epicsUInt16)fillvalue;
      break;
    case NDInt32:
      datatype_ = H5T_NATIVE_INT32;
      *(epicsInt32*)this->ptrFillValue_ = (epicsInt32)fillvalue;
      break;
    case NDUInt32:
      datatype_ = H5T_NATIVE_UINT32;
      *(epicsUInt32*)this->ptrFillValue_ = (epicsUInt32)fillvalue;
      break;
    case NDFloat32:
      datatype_ = H5T_NATIVE_FLOAT;
      *(epicsFloat32*)this->ptrFillValue_ = (epicsFloat32)fillvalue;
      break;
    case NDFloat64:
      datatype_ = H5T_NATIVE_DOUBLE;
      *(epicsFloat64*)this->ptrFillValue_ = (epicsFloat64)fillvalue;
      break;
    default:
      datatype_ = -1;
  }
  return status;
}

void NDFileHDF5AttributeDataset::extendDataSet()
{
  int i=0;
  bool growdims = true;
  bool growoffset = true;
  int extradims = extraDimensions_;

  // Add the n'th frame dimension (for multiple frames per scan point)
  // first frame already has the offsets and dimensions preconfigured so
  // we dont need to increment anything here
  if (this->nextRecord_ == 0) return;

  // in the simple case where dont use the extra X,Y dimensions we
  // just increment the n'th frame number
  if (extradims == 0 || extradims == 1){
    this->dims_[0]++;
    this->offset_[0]++;
    return;
  }

  // run through the virtual dimensions in reverse order: n,X,Y
  // and increment, reset or ignore the offset of each dimension.
  for (i=extradims-1; i>=0; i--){
    if (this->dims_[i] == this->virtualdims_[i]) growdims = false;

    if (growoffset){
      this->offset_[i]++;
      growoffset = false;
    }

    if (growdims){
      if (this->dims_[i] < this->virtualdims_[i]) {
        this->dims_[i]++;
        growdims = false;
      }
    }

    if (this->offset_[i] == this->virtualdims_[i]) {
      this->offset_[i] = 0;
      growoffset = true;
      growdims = true;
    }
  }
  return;
}

void NDFileHDF5AttributeDataset::extendDataSet(hsize_t *offsets)
{
  // In this case the dimensions and offsets have been supplied to us so simply
  // use these values.
  for (int index = 0; index < this->extraDimensions_; index++){
    if (offsets[index]+1 < this->virtualdims_[index]+1){
      if (this->dims_[index] < offsets[index]+1){
        // Increase the dimension to accomodate the new position
        this->dims_[index] = offsets[index]+1;
      }
      // Always set the offset position even if we don't increase the dims
      this->offset_[index] = offsets[index];
    }
  }
  return;
}

std::string NDFileHDF5AttributeDataset::getName()
{
  return name_;
}

hid_t NDFileHDF5AttributeDataset::getHandle()
{
  return dataset_;
}

asynStatus NDFileHDF5AttributeDataset::flushDataset()
{
  asynStatus status = asynSuccess;

  // We cannot flush for SWMR if the HDF version doesn't support it
  #if H5_VERSION_GE(1,9,178)

  // Flush the dataset
  H5Dflush(dataset_);
  #else
  status = asynError;
  #endif

  return status;
}
