/*
 * NDFileHDF5AttributeDataset.cpp
 *
 *  Created on: 30 Apr 2015
 *      Author: gnx91527
 */

#include "NDFileHDF5AttributeDataset.h"
#include <epicsString.h>
#include <iostream>

NDFileHDF5AttributeDataset::NDFileHDF5AttributeDataset(asynUser *pAsynUser, hid_t file, const std::string& name, NDAttrDataType_t type) :
  pAsynUser_(pAsynUser),
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
  rank_(0),
  whenToSave_(hdf5::OnFrame)
{
  // Allocate enough memory for the fill value to accept any data type
  ptrFillValue_ = (void*)calloc(8, sizeof(char));
}

NDFileHDF5AttributeDataset::~NDFileHDF5AttributeDataset()
{
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

  cparm_ = H5Pcreate(H5P_DATASET_CREATE);

  H5Pset_fill_value(cparm_, datatype_, ptrFillValue_);

  H5Pset_chunk(cparm_, rank_, chunk_);

  dataspace_ = H5Screate_simple(rank_, dims_, maxdims_);

  // Open the group by its name
  hid_t dsetgroup = H5Gopen(file_, groupName_.c_str(), H5P_DEFAULT);

  // Now create the dataset
  dataset_ = H5Dcreate2(dsetgroup, dsetName_.c_str(),
                        datatype_, dataspace_,
                        H5P_DEFAULT, cparm_, H5P_DEFAULT);

  H5Gclose(dsetgroup);

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
    dims_[0]++;
    offset_[0]++;
  }

  return status;
}

asynStatus NDFileHDF5AttributeDataset::closeAttributeDataset()
{
  H5Dclose(dataset_);
  H5Sclose(memspace_);
  H5Sclose(dataspace_);
  H5Pclose(cparm_);
  return asynSuccess;
}

asynStatus NDFileHDF5AttributeDataset::configureDims(int user_chunking)
{
  asynStatus status = asynSuccess;
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



/*asynStatus NDFileHDF5AttributeDataset::configureDims(int dsize, bool multiframe, int extradimensions, int *extra_dims, int user_chunking)
{
  int i=0,j=0, extradims = 0, ndims=0;
  int datadims = 0;
  asynStatus status = asynSuccess;

  extradims = extradimensions;

  if (dsize > 1){
    // The attribute data is not a single value but an array (chars for example)
    datadims = 1;
  }
  ndims = datadims + extradims;

  // first check whether the dimension arrays have been allocated
  // or the number of dimensions have changed.
  // If necessary free and reallocate new memory.
  if (this->maxdims_ == NULL || this->rank_ != ndims){
    if (this->maxdims_     != NULL) free(this->maxdims_);
    if (this->chunkdims_   != NULL) free(this->chunkdims_);
    if (this->dims_        != NULL) free(this->dims_);
    if (this->offset_      != NULL) free(this->offset_);
    if (this->virtualdims_ != NULL) free(this->virtualdims_);

    this->maxdims_       = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->chunkdims_     = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->dims_          = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->offset_        = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->virtualdims_   = (hsize_t*)calloc(extradims, sizeof(hsize_t));
  }

  if (multiframe){
    // Configure the virtual dimensions -i.e. dimensions in addition to the frame format.
    // Normally set to just 1 by default or -1 unlimited (in HDF5 terms)
    for (i=0; i<extradims; i++){
      this->chunkdims_[i]   = 1;
      this->maxdims_[i]     = H5S_UNLIMITED;
      this->dims_[i]        = 1;
      this->offset_[i]      = 0; // because we increment offset *before* each write we need to start at -1
      this->virtualdims_[i] = extra_dims[i];
    }
  }

  this->rank_ = ndims;

  if (datadims == 1){
    // This is an Attribute dataset which is not a single value (eg array of chars)
    this->chunkdims_[extradims]  = dsize;
    this->maxdims_[extradims]    = dsize;
    this->dims_[extradims]       = dsize;
    this->offset_[extradims]     = 0;
  }

  // Collect the user defined chunking dimensions and check if they're valid
  //
  // A check is made to see if the user has input 0 or negative value (which is invalid)
  // in which case the size of the chunking is set to the maximum size of that dimension (full frame)
  // If the maximum of a particular dimension is set to a negative value -which is the case for
  // infinite lenght dimensions (-1); the chunking value is set to 1.
  int max_items = 0;
  int hdfdim = 0;
  for (i = 0; i<ndims; i++){
    hdfdim = ndims - i - 1;
    max_items = (int)this->maxdims_[hdfdim];
    if (max_items <= 0){
      max_items = 1; // For infinite length dimensions
    } else {
      if (user_chunking[i] > max_items) user_chunking[i] = max_items;
    }
    if (user_chunking[i] < 1) user_chunking[i] = max_items;
    this->chunkdims_[hdfdim] = user_chunking[i];
  }
  return status;
}
*/

void NDFileHDF5AttributeDataset::extendDataSet(int extradims)
{

}

//asynStatus NDFileHDF5AttributeDataset::writeFile(NDArray *pArray, hid_t datatype, hid_t dataspace, hsize_t *framesize)
//{
//
//}

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

  // There is no SWMR mode if the HDF version doesn't support it
  #if H5_VERSION_GE(1,9,178)

  // Flush the dataset
  H5Dflush(dataset_);

  #endif

  return status;
}
