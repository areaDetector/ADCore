#include <string.h>

#include <hdf5.h>

static herr_t cFlushCallback(hid_t objectID, void *data)
{
  // Do nothing
  return 0;
}

void writeStringAttribute(hid_t element, const char *attr_name, const char *attr_value)
{
  herr_t hdfstatus = -1;
  hid_t hdfdatatype = -1;
  hid_t hdfattr = -1;
  hid_t hdfattrdataspace = -1;
  hdfattrdataspace = H5Screate(H5S_SCALAR);
  hdfdatatype      = H5Tcopy(H5T_C_S1);
  hdfstatus        = H5Tset_size(hdfdatatype, strlen(attr_value));
  hdfstatus        = H5Tset_strpad(hdfdatatype, H5T_STR_NULLTERM);
  hdfattr          = H5Acreate2(element, attr_name, hdfdatatype, hdfattrdataspace, H5P_DEFAULT, H5P_DEFAULT);

  hdfstatus = H5Awrite(hdfattr, hdfdatatype, attr_value);
  H5Aclose (hdfattr);
  H5Sclose(hdfattrdataspace);

  return;
}

void writeInt32Attribute(hid_t element, const char *attr_name, hsize_t dims, int *attr_value)
{
  herr_t hdfstatus = -1;
  hid_t hdfdatatype = -1;
  hid_t hdfattr = -1;
  hid_t hdfattrdataspace = -1;
  
  hdfattrdataspace = H5Screate(H5S_SCALAR);
  hdfdatatype      = H5Tcopy(H5T_NATIVE_INT32);
  if (dims == 1) {
  } else {
    hdfattrdataspace = H5Screate(H5S_SIMPLE);
    H5Sset_extent_simple(hdfattrdataspace, 1, &dims, NULL);
  }
  hdfattr = H5Acreate2(element, attr_name, hdfdatatype, hdfattrdataspace, H5P_DEFAULT, H5P_DEFAULT);
  hdfstatus = H5Awrite(hdfattr, hdfdatatype, attr_value);
  H5Aclose (hdfattr);
  H5Sclose(hdfattrdataspace);

  return;
}

int main(int argc, char *argv[])
{
  hid_t fid           = -1;
  hid_t access_plist  = -1;
  hid_t create_plist  = -1;
  hid_t cparm         = -1;
  hid_t datatype      = -1;
  hid_t dataspace     = -1;
  hid_t dataset       = -1;
  hid_t memspace      = -1;
  hid_t groupDetector = -1;
  int rank = 1;
  hsize_t chunk[2] = {10,10};
  hsize_t dims[2] = {1,1};
  hsize_t elementSize[2] = {1,1};
  hsize_t maxdims[2] = {H5S_UNLIMITED,H5S_UNLIMITED};
  int ivalue[2];
  int fillValue = 0;
  
  /* Open the source file and dataset */
  /* All SWMR files need to use the latest file format */
  access_plist = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_fclose_degree(access_plist, H5F_CLOSE_STRONG);
#if H5_VERSION_GE(1,9,178)
  H5Pset_object_flush_cb(access_plist, cFlushCallback, NULL);
#endif
  H5Pset_libver_bounds(access_plist, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
  create_plist = H5Pcreate(H5P_FILE_CREATE);
  fid = H5Fcreate("test_string_swmr.h5", H5F_ACC_TRUNC, create_plist, access_plist);

  /* Data */
  rank = 2;
  dims[0] = 10;
  dims[1] = 10;
  dataspace = H5Screate_simple(rank, dims, dims);
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_chunk(cparm, rank, chunk);
  datatype = H5Tcopy(H5T_NATIVE_INT8);
  H5Pset_fill_value(cparm, datatype, &fillValue);
  groupDetector = H5Gcreate(fid, "detector", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  access_plist = H5Pcreate(H5P_DATASET_ACCESS);
  H5Pset_chunk_cache(access_plist, 503, 100, 1.0);
  dataset = H5Dcreate2(groupDetector, "data1",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, access_plist);
  ivalue[0] = 1;
  ivalue[1] = 1;
  writeInt32Attribute(dataset, "NDArrayDimBinning", 2, ivalue);
  ivalue[0] = 0;
  ivalue[1] = 0;
  writeInt32Attribute(dataset, "NDArrayDimOffset", 2, ivalue);
  writeInt32Attribute(dataset, "NDArrayDimReverse", 2, ivalue);
  ivalue[0] = 2;
  writeInt32Attribute(dataset, "NDArrayNumDims", 1, ivalue);
  H5Gclose(groupDetector);

  dims[0] = 1;
  dims[1] = 1;
  chunk[0] = 1;
  rank = 1;

  /* Unique ID */
  datatype = H5T_NATIVE_INT32;
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_fill_value(cparm, datatype, &fillValue);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "NDArrayUniqueId",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  memspace = H5Screate_simple(rank, elementSize, NULL);
  writeStringAttribute(dataset, "NDAttrName",        "NDArrayUniqueId");
  writeStringAttribute(dataset, "NDAttrDescription", "The unique ID of the NDArray");
  writeStringAttribute(dataset, "NDAttrSourceType",  "NDAttrSourceDriver");
  writeStringAttribute(dataset, "NDAttrSource",      "Driver");

  /* EPICS Timestemp */
  datatype = H5T_NATIVE_DOUBLE;
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_fill_value(cparm, datatype, &fillValue);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "NDArrayTimeStamp",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  memspace = H5Screate_simple(rank, elementSize, NULL);
  writeStringAttribute(dataset, "NDAttrName",        "NDArrayTimeStamp");
  writeStringAttribute(dataset, "NDAttrDescription", "The timestamp of the NDArray as float64");
  writeStringAttribute(dataset, "NDAttrSourceType",  "NDAttrSourceDriver");
  writeStringAttribute(dataset, "NDAttrSource",      "Driver");

  /* EPICS TS sec */
  datatype = H5T_NATIVE_UINT32;
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_fill_value(cparm, datatype, &fillValue);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "NDArrayEpicsTSSec",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  memspace = H5Screate_simple(rank, elementSize, NULL);
  writeStringAttribute(dataset, "NDAttrName",        "NDArrayEpicsTSSec");
  writeStringAttribute(dataset, "NDAttrDescription", "The NDArray EPICS timestamp seconds past epoch");
  writeStringAttribute(dataset, "NDAttrSourceType",  "NDAttrSourceDriver");
  writeStringAttribute(dataset, "NDAttrSource",      "Driver");

  /* EPICS TS nsec */
  datatype = H5T_NATIVE_UINT32;
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_fill_value(cparm, datatype, &fillValue);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "NDArrayEpicsTSnSec",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  memspace = H5Screate_simple(rank, elementSize, NULL);
  writeStringAttribute(dataset, "NDAttrName",        "NDArrayEpicsTSnSec");
  writeStringAttribute(dataset, "NDAttrDescription", "The NDArray EPICS timestamp nanoseconds");
  writeStringAttribute(dataset, "NDAttrSourceType",  "NDAttrSourceDriver");
  writeStringAttribute(dataset, "NDAttrSource",      "Driver");
 
  /* Color mode */
  datatype = H5T_NATIVE_INT32;
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_fill_value(cparm, datatype, &fillValue);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "ColorMode",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  memspace = H5Screate_simple(rank, elementSize, NULL);
  writeStringAttribute(dataset, "NDAttrName",        "ColorMode");
  writeStringAttribute(dataset, "NDAttrDescription", "Color mode");
  writeStringAttribute(dataset, "NDAttrSourceType",  "NDAttrSourceDriver");
  writeStringAttribute(dataset, "NDAttrSource",      "Driver");

  /* Camera manufacturer */
  datatype = H5Tcopy(H5T_C_S1);
  H5Tset_size(datatype, 256);
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_fill_value(cparm, datatype, &fillValue);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "CameraManufacturer",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  memspace = H5Screate_simple(rank, elementSize, NULL);
  writeStringAttribute(dataset, "NDAttrName",        "CameraManufacturer");
  writeStringAttribute(dataset, "NDAttrDescription", "Camera manufacturer");
  writeStringAttribute(dataset, "NDAttrSourceType",  "NDAttrSourceParam");
  writeStringAttribute(dataset, "NDAttrSource",      "MANUFACTURER");

  /* Performance data */
  rank = 2;
  dims[0] = 1;
  dims[1] = 5;
  chunk[1] = 5;
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "timestamp",
                       H5T_NATIVE_DOUBLE, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  if (!H5Iis_valid(dataset)) {
    printf("Error writing performance dataset");
  }
  H5Sclose(dataspace);

#if H5_VERSION_GE(1,9,178)
  H5Fstart_swmr_write(fid);
#endif
  
  H5Fclose(fid);

  return 0;

} /* end main */

