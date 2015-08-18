/*
 * HDF5FileReader.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: gnx91527
 */

#include "HDF5FileReader.h"

herr_t file_info(hid_t loc_id, const char *name, const H5L_info_t *info, void *opdata)
{
    H5O_info_t infobuf;
    HDF5FileReader *ptr = (HDF5FileReader *)opdata;

    H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
    ptr->processGroup(loc_id, name, infobuf.type);
    return 0;
 }

HDF5FileReader::HDF5FileReader(const std::string& filename)
{
  hsize_t idx = 0;
  file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, NULL);
  H5Literate_by_name(file, "/", H5_INDEX_NAME, H5_ITER_NATIVE, &idx, file_info, this, H5P_DEFAULT);
}

void HDF5FileReader::report()
{
  std::map<std::string, std::tr1::shared_ptr<HDF5Object> >::iterator iter;
  for (iter = objects.begin(); iter != objects.end(); iter++){
    printf("[%s] %s\n", iter->second->getTypeString().c_str(), iter->first.c_str());
  }
}

void HDF5FileReader::processGroup(hid_t loc_id, const char *name, H5O_type_t type)
{
  std::string sname(name);
  std::string oldname = cname;
  cname = cname + "/" + sname;
  objects[cname] = std::tr1::shared_ptr<HDF5Object>(new HDF5Object(name, type));
  if (type == H5O_TYPE_GROUP){
    hsize_t idx = 0;
    H5Literate_by_name(loc_id, name, H5_INDEX_NAME, H5_ITER_NATIVE, &idx, file_info, this, H5P_DEFAULT);
  }
  cname = oldname;
}

bool HDF5FileReader::checkGroupExists(const std::string& name)
{
  if (objects.count(name) == 1){
    return true;
  }
  return false;
}

bool HDF5FileReader::checkDatasetExists(const std::string& name)
{
  if (objects.count(name) == 1){
    if (objects[name]->getTypeString() == "dataset"){
      return true;
    }
  }
  return false;
}

std::vector<hsize_t> HDF5FileReader::getDatasetDimensions(const std::string& name)
{
  hid_t       dataset_id;
  hid_t       dspace_id;
  int         ndims = 0;
  std::vector<hsize_t> vdims;
  // Check the name given is present in the file
  if (objects.count(name) == 1){
    // Check the name given is a dataset
    if (objects[name]->getTypeString() == "dataset"){

      // Open the dataset.
      dataset_id = H5Dopen(file, name.c_str(), H5P_DEFAULT);

      // Get the dataspace
      dspace_id = H5Dget_space(dataset_id);

      ndims = H5Sget_simple_extent_ndims(dspace_id);

      hsize_t dims[ndims];
      hsize_t maxdims[ndims];
      H5Sget_simple_extent_dims(dspace_id, dims, maxdims);

      for (int index = 0; index < ndims; index++){
        vdims.push_back(dims[index]);
      }

      // Close the dataset
      H5Dclose(dataset_id);
    }
  }
  return vdims;
}

TestFileDataType_t HDF5FileReader::getDatasetType(const std::string& name)
{
  hid_t        dataset_id;
  hid_t        dtype_id;
  hid_t        ntype_id;
  TestFileDataType_t type = NoType;
  // Check the name given is present in the file
  if (objects.count(name) == 1){
    // Check the name given is a dataset
    if (objects[name]->getTypeString() == "dataset"){

      // Open the dataset.
      dataset_id = H5Dopen(file, name.c_str(), H5P_DEFAULT);

      // Get the datatype
      dtype_id = H5Dget_type(dataset_id);

      ntype_id = H5Tget_native_type(dtype_id, H5T_DIR_ASCEND);
      if (H5Tequal(ntype_id, H5T_NATIVE_INT8)){
        type = Int8;
      }
      if (H5Tequal(ntype_id, H5T_NATIVE_UINT8)){
        type = UInt8;
      }
      if (H5Tequal(ntype_id, H5T_NATIVE_INT16)){
        type = Int16;
      }
      if (H5Tequal(ntype_id, H5T_NATIVE_UINT16)){
        type = UInt16;
      }
      if (H5Tequal(ntype_id, H5T_NATIVE_INT32)){
        type = Int32;
      }
      if (H5Tequal(ntype_id, H5T_NATIVE_UINT32)){
        type = UInt32;
      }
      if (H5Tequal(ntype_id, H5T_NATIVE_FLOAT)){
        type = Float32;
      }
      if (H5Tequal(ntype_id, H5T_NATIVE_DOUBLE)){
        type = Float64;
      }
      H5Tclose(ntype_id);
      H5Tclose(dtype_id);
      H5Dclose(dataset_id);
    }
  }
  return type;
}

HDF5FileReader::~HDF5FileReader ()
{
  // Close the file
  H5Fclose(file);
}

