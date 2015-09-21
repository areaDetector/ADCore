/*
 * HDF5FileReader.h
 *
 *  Created on: 11 Mar 2015
 *      Author: gnx91527
 */

#ifndef PLUGINTESTS_HDF5FileReader_H_
#define PLUGINTESTS_HDF5FileReader_H_

#include <string>
#include <tr1/memory>
#include <map>
#include <vector>
#include <hdf5.h>

typedef enum
{
  NoType,
  Int8,     // Signed 8-bit integer
  UInt8,    // Unsigned 8-bit integer
  Int16,    // Signed 16-bit integer
  UInt16,   // Unsigned 16-bit integer
  Int32,    // Signed 32-bit integer
  UInt32,   // Unsigned 32-bit integer
  Float32,  // 32-bit float
  Float64   // 64-bit float
} TestFileDataType_t;

class HDF5FileReader
{
public:
  HDF5FileReader(const std::string& filename);
  void report();
  void processGroup(hid_t loc_id, const char *name, H5O_type_t type);
  bool checkGroupExists(const std::string& name);
  bool checkDatasetExists(const std::string& name);
  std::vector<hsize_t> getDatasetDimensions(const std::string& name);
  TestFileDataType_t getDatasetType(const std::string& name);
  virtual ~HDF5FileReader();
private:
  hid_t file;

  class HDF5Object
  {
  public:
    HDF5Object(const std::string& name, H5O_type_t type)
    {
      this->name = name;
      this->type = type;
    };

    std::string getTypeString()
    {
      std::string response;
      switch (this->type)
      {
        case H5O_TYPE_GROUP:
          response = "group";
          break;
        case H5O_TYPE_DATASET:
          response = "dataset";
          break;
        case H5O_TYPE_NAMED_DATATYPE:
          response = "datatype";
          break;
        default:
          response = "unknown";
      }
      return response;
    };

    virtual ~HDF5Object(){};

  private:
    std::string name;
    H5O_type_t type;
  };

  std::string cname;
  std::map<std::string, std::tr1::shared_ptr<HDF5Object> > objects;

};

#endif /* PLUGINTESTS_HDF5FileReader_H_ */
