/*
 * NDFileHDF5AttributeDataset.h
 *
 *  Created on: 30 Apr 2015
 *      Author: gnx91527
 */

#ifndef ADAPP_PLUGINSRC_NDFILEHDF5ATTRIBUTEDATASET_H_
#define ADAPP_PLUGINSRC_NDFILEHDF5ATTRIBUTEDATASET_H_

#include <string>
#include <hdf5.h>
#include <asynDriver.h>
#include <NDPluginFile.h>
#include <NDArray.h>
#include "NDFileHDF5Layout.h"
#include "NDFileHDF5VersionCheck.h"

class NDFileHDF5AttributeDataset
{
public:
  NDFileHDF5AttributeDataset(hid_t file, const std::string& name, NDAttrDataType_t type, hdf5::StringAttributeDataType_t stringType);
  virtual ~NDFileHDF5AttributeDataset();

  void setDsetName(const std::string& dsetName);
  void setWhenToSave(hdf5::When_t whenToSave);
  void setParentGroupName(const std::string& group);
  asynStatus createDataset(int user_chunking);
  asynStatus createDataset(bool multiframe, int extradimensions, int *extra_dims, int *user_chunking);
  asynStatus writeAttributeDataset(hdf5::When_t whenToSave, NDAttribute *ndAttr, int flush);
  asynStatus writeAttributeDataset(hdf5::When_t whenToSave, hsize_t *offsets, NDAttribute *ndAttr, int flush, int indexed);
  asynStatus closeAttributeDataset();
  asynStatus flushDataset();
  std::string getName();
  hid_t getHandle();

private:
  asynStatus createHDF5Dataset();
  asynStatus configureDims(int user_chunking);
  asynStatus configureDimsFromDataset(bool multiframe, int extradimensions, int *extra_dims, int *user_chunking);
  asynStatus typeAsHdf();
  void extendDataSet();
  void extendDataSet(hsize_t *offsets);
  void extendIndexDataSet(hsize_t offset);

  std::string      name_;            // Name of the attribute
  std::string      dsetName_;        // Name of the dataset to store
  hid_t            file_;            // File handle
  NDAttrDataType_t type_;            // NDAttribute type
  hdf5::StringAttributeDataType_t stringType_; //  String attribute data type
  std::string      groupName_;       // Name of the parent group
  hid_t            dataset_;         // Dataset handle
  hid_t            dataspace_;
  hid_t            memspace_;
  hid_t            datatype_;
  hid_t            cparm_;
  hid_t            filespace_;
  void             *ptrFillValue_;
  hsize_t          *dims_;
  hsize_t          *offset_;
  hsize_t          *chunk_;
  hsize_t          *maxdims_;
  hsize_t          *virtualdims_;
  hsize_t          *elementSize_;
  int              rank_;            // number of dimensions
  int              nextRecord_;
  int              extraDimensions_;
  hdf5::When_t     whenToSave_;

};

#endif /* ADAPP_PLUGINSRC_NDFILEHDF5ATTRIBUTEDATASET_H_ */
