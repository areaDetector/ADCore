#include <string.h>

#include <hdf5.h>

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

int main(int argc, char *argv[])
{
  hid_t fid           = -1;
  hid_t access_plist  = -1;
  hid_t create_plist  = -1;
  hid_t cparm         = -1;
  hid_t datatype      = -1;
  hid_t dataspace     = -1;
  hid_t dataset       = -1;
  hid_t groupDetector = -1;
  int rank = 1;
  hsize_t chunk[2] = {10,10};
  hsize_t dims[2] = {1,1};
  hsize_t elementSize[2] = {1,1};
  hsize_t maxdims[2] = {H5S_UNLIMITED,H5S_UNLIMITED};
  int fillValue = 0;
  
  /* Open the source file and dataset */
  /* All SWMR files need to use the latest file format */
  access_plist = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_fclose_degree(access_plist, H5F_CLOSE_STRONG);
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
  dataset = H5Dcreate2(groupDetector, "data1",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, access_plist);
  H5Gclose(groupDetector);
  
  rank = 1;
  dims[0] = 1;
  dims[1] = 1;
  chunk[0] = 1;

  /* EPICS Timestemp */
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  datatype = H5T_NATIVE_DOUBLE;
  H5Pset_fill_value(cparm, datatype, &fillValue);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "NDArrayTimeStamp",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  H5Screate_simple(rank, elementSize, NULL);

  /* Color mode */
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  datatype = H5T_NATIVE_INT32;
  H5Pset_fill_value(cparm, datatype, &fillValue);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "ColorMode",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  H5Screate_simple(rank, elementSize, NULL);

  /* Camera manufacturer */
  cparm = H5Pcreate(H5P_DATASET_CREATE);
  datatype = H5Tcopy(H5T_C_S1);
  H5Tset_size(datatype, 256);
  H5Pset_fill_value(cparm, datatype, &fillValue);
  H5Pset_chunk(cparm, rank, chunk);
  dataspace = H5Screate_simple(rank, dims, maxdims);
  dataset = H5Dcreate2(fid, "CameraManufacturer",
                       datatype, dataspace,
                       H5P_DEFAULT, cparm, H5P_DEFAULT);
  H5Screate_simple(rank, elementSize, NULL);
  writeStringAttribute(dataset, "NDAttrSourceType",  "NDAttrSourceParam");

#if H5_VERSION_GE(1,9,178)
  H5Fstart_swmr_write(fid);
#endif
  
  H5Fclose(fid);

  return 0;

} /* end main */
