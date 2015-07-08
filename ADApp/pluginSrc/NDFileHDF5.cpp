/* NDFileHDF5.cpp
 * Writes NDArrays to HDF5 files.
 *
 * Ulrik Kofoed Pedersen
 * March 20. 2011
 */

#define H5Gcreate_vers 2
#define H5Dopen_vers 2

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <list>
#include <cmath>
#include <iostream>
#include <sstream>
#include <hdf5.h>
#include <sys/stat.h>
// #include <hdf5_hl.h> // high level HDF5 API not currently used (requires use of library hdf5_hl)

#include <epicsStdio.h>
#include <epicsString.h>
#include <epicsTime.h>
#include <iocsh.h>
#define epicsAssertAuthor "the EPICS areaDetector collaboration (https://github.com/areaDetector/ADCore/issues)"
#include <epicsAssert.h>
#include <osiSock.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDFileHDF5.h"

#define METADATA_NDIMS 1
#define MAX_LAYOUT_LEN 1048576

enum HDF5Compression_t {HDF5CompressNone=0, HDF5CompressNumBits, HDF5CompressSZip, HDF5CompressZlib};

#define DIMSREPORTSIZE 512
#define DIMNAMESIZE 40
#define MAXEXTRADIMS 3
#define ALIGNMENT_BOUNDARY 1048576
#define INFINITE_FRAMES_CAPTURE 10000 /* Used to calculate istorek (the size of the chunk index binar search tree) when capturing infinite number of frames */

#ifdef HDF5_BTREE_IK_MAX_ENTRIES
  #define  MAX_ISTOREK ((HDF5_BTREE_IK_MAX_ENTRIES/2)-1)
#else
  #define MAX_ISTOREK 32767  /* HDF5 Binary Search tree max. */
#endif

static const char *driverName = "NDFileHDF5";


/** Opens a HDF5 file.  
 * In write mode if NDFileModeMultiple is set then the first dataspace dimension is set to H5S_UNLIMITED to allow 
 * multiple arrays to be written to the same file.
 * NOTE: Does not currently support NDFileModeRead or NDFileModeAppend.
 * \param[in] fileName  Absolute path name of the file to open.
 * \param[in] openMode Bit mask with one of the access mode bits NDFileModeRead, NDFileModeWrite, NDFileModeAppend.
 *           May also have the bit NDFileModeMultiple set if the file is to be opened to write or read multiple 
 *           NDArrays into a single file.
 * \param[in] pArray Pointer to an NDArray; this array is used to determine the header information and data 
 *           structure for the file. 
 */
asynStatus NDFileHDF5::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
  int storeAttributes, storePerformance;
  static const char *functionName = "openFile";
  int numCapture;
  asynStatus status = asynSuccess;

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Filename: %s\n", driverName, functionName, fileName);

  /* These operations are accessing parameter library, must take lock */
  this->lock();
  getIntegerParam(NDFileNumCapture, &numCapture);
  getIntegerParam(NDFileHDF5_storeAttributes, &storeAttributes);
  getIntegerParam(NDFileHDF5_storePerformance, &storePerformance);

  // We don't support reading yet
  if (openMode & NDFileModeRead) {
    setIntegerParam(NDFileCapture, 0);
    setIntegerParam(NDWriteFile, 0);
    status = asynError;
  }
  
  // We don't support opening an existing file for appending yet
  if (openMode & NDFileModeAppend) {
    setIntegerParam(NDFileCapture, 0);
    setIntegerParam(NDWriteFile, 0);
    status = asynError;
  }

  // Check if an invalid (<0) number of frames has been configured for capture
  if (numCapture < 0) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
              "%s::%s Invalid number of frames to capture: %d. Please specify a number >= 0\n",
              driverName, functionName, numCapture);
    status = asynError;
  }

  // Verify the XML path and filename. Must be called with lock held.
  if (this->verifyLayoutXMLFile()){
    status = asynError;
  }
  this->unlock();
  if (status != asynSuccess) return status;


  // Check to see if a file is already open and close it
  this->checkForOpenFile();

  if (openMode & NDFileModeMultiple){
    this->multiFrameFile = true;
  } else {
    this->multiFrameFile = false;
    this->lock();
    setIntegerParam(NDFileHDF5_nExtraDims, 0);
    this->unlock();
  }

  epicsTimeGetCurrent(&this->prevts);
  this->opents = this->prevts;
  NDArrayInfo_t info;
  pArray->getInfo(&info);
  this->frameSize = (8.0 * info.totalBytes)/(1024.0 * 1024.0);
  this->bytesPerElement = info.bytesPerElement;

  // Construct an attribute list. We use a separate attribute list from the one in pArray
  // to avoid the need to copy the array.

  // First clear the list
  this->pFileAttributes->clear();

  // Insert default NDAttribute from the NDArray object (timestamps etc)
  this->addDefaultAttributes(pArray);

  // Now get the current values of the attributes for this plugin
  this->getAttributes(this->pFileAttributes);

  // Now append the attributes from the array which are already up to date from the driver and prior plugins
  pArray->pAttributeList->copy(this->pFileAttributes);

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s attribute list copied. num pArray attributes = %i local copy = %d\n",
            driverName, functionName, this->pFileAttributes->count(), pArray->pAttributeList->count());
  
  // Set the next record in the file to 0
  this->nextRecord = 0;

  // Work out the various dimensions used for the incoming data
  if (this->configureDims(pArray)){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s::%s ERROR Failed to configure dimensions\n",
              driverName, functionName);
    return asynError;
  }

  // Create the new file
  if (this->createNewFile(fileName)){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s::%s ERROR Failed to create a new output file\n",
              driverName, functionName);
    return asynError;
  }

  // Now create the layout within the file
  if (this->createFileLayout(pArray)){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s::%s ERROR Failed to create the specified file layout\n",
              driverName, functionName);
    return asynError;
  }

  // Set up the dimensions for each of the available datasets
  if (this->configureDatasetDims(pArray)){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s::%s ERROR Failed to configure dataset dimensions\n",
              driverName, functionName);
    return asynError;
  }

  if (storeAttributes == 1){
    this->createAttributeDataset();
    this->writeAttributeDataset(hdf5::OnFileOpen);


    // Store any attributes that have been marked as onOpen
    if (this->storeOnOpenAttributes()){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s ERROR Failed to store the onOpen attributes\n",
                driverName, functionName);
      return asynError;
    }

  }

  if (storePerformance == 1){
    this->configurePerformanceDataset();
  }

  // Create all of the hardlinks in the file
  hdf5::Root *root = this->layout.get_hdftree();
  this->createHardLinks(root);

  return asynSuccess;
}

/** Create the groups and datasets in the HDF5 file.
 */
asynStatus NDFileHDF5::createXMLFileLayout()
{
  asynStatus retcode = asynSuccess;
  static const char *functionName = "createXMLFileLayout";
 
  // Clear out any previous onOpen and onClose vectors
  onOpenMap.clear();
  onCloseMap.clear();

  hdf5::Root *root = this->layout.get_hdftree();
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Root tree: %s\n",
            driverName, functionName,
            root->_str_().c_str());

  retcode = this->createTree(root, this->file);

  // Only proceed if there was no error in creating the tree
  if (retcode == asynSuccess){
    // Attempt to search for a dataset with a default flag and record the dataset name.
    // If no default is found then set the first as the default.
    // If no datasets are found then this is an error.
    hdf5::Dataset *dset;
    int retval = root->find_detector_default_dset(&dset);
    if (!retval){
      this->defDsetName = dset->get_full_name();
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                "%s::%s Default dataset name: %s\n",
                driverName, functionName, this->defDsetName.c_str());
    } else {
      if (detDataMap.size() > 0){
        // OK, no dataset specified as default, use the first
        std::map<std::string, NDFileHDF5Dataset *>::iterator it_dset = detDataMap.begin();
        this->defDsetName = it_dset->first;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                  "%s::%s No default dataset specified, using: %s\n",
                  driverName, functionName, this->defDsetName.c_str());
      } else {
        // This is bad news, apparently no detector datasets have been defined.
        // Return an error
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                  "%s::%s No detector datasets found, cannot continue\n",
                  driverName, functionName);
        return asynError;
      }
    }

    // Init the dataset destination NDAttribute name
    ndDsetName = "";
    // Check for NDAttribute name of data destination switch
    std::string ddest = this->layout.get_global("detector_data_destination");
    if (ddest != ""){
      ndDsetName = ddest;
    } else {
      // Nothing to do here
    }

  }
  
  return retcode;
}

/**
 * Check through attributes and store any that have been marked as onOpen
 */
asynStatus NDFileHDF5::storeOnOpenAttributes()
{
  asynStatus status = asynSuccess;
  const char *functionName = "storeOnOpenAttributes";

  // Loop over the stored onOpen elements
  for (std::map<std::string, hdf5::Element *>::iterator it_element = onOpenMap.begin() ; it_element != onOpenMap.end(); ++it_element){
    hdf5::Element *element = it_element->second;
    status = storeOnOpenCloseAttribute(element, true);
    if (status != asynSuccess){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s failed to store onOpen attributes\n",
                driverName, functionName);
      return status;
    }
  }
  return status;
}

/** Check through attributes and store any that have been marked as onClose
 */
asynStatus NDFileHDF5::storeOnCloseAttributes()
{
  asynStatus status = asynSuccess;
  const char *functionName = "storeOnCloseAttributes";

  // Loop over the stored onClose elements
  for (std::map<std::string, hdf5::Element *>::iterator it_element = onCloseMap.begin() ; it_element != onCloseMap.end(); ++it_element){
    hdf5::Element *element = it_element->second;
    status = storeOnOpenCloseAttribute(element, false);
    if (status != asynSuccess){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s failed to store onClose attributes\n",
                driverName, functionName);
      return status;
    }
  }
  return status;
}

/** Check attribute and store if marked as onOpen or onClose when opening or closing
 */
asynStatus NDFileHDF5::storeOnOpenCloseAttribute(hdf5::Element *element, bool open)
{
  asynStatus status = asynSuccess;
  NDAttribute *ndAttr = NULL;
  void* datavalue;
  int ret;
  bool saveAttribute = false;
  const char *functionName = "storeOnOpenCloseAttribute";

  hdf5::Element::MapAttributes_t::iterator it_attr;
  // Attempt to Open the Object, we do not know (or care?) if it is a group or dataset
  hid_t hdf_id = H5Oopen(this->file, element->get_full_name().c_str(), H5P_DEFAULT);
  // For each element search for any attributes that match the pArray
  for (it_attr=element->get_attributes().begin(); it_attr != element->get_attributes().end(); ++it_attr){
    saveAttribute = false;
    hdf5::Attribute &attr = it_attr->second; // Take a reference - i.e. *not* a copy!
    if (attr.source.is_src_ndattribute()){
      // Is the attribute marked as we require, if so mark it for saving
      if ((open == true) && (attr.is_onFileOpen() == true)){
        saveAttribute = true;
      }
      if ((open == false) && (attr.is_onFileClose() == true)){
        saveAttribute = true;
      }
      if (saveAttribute == true){
        // find the named attribute in the NDAttributeList
        ndAttr = this->pFileAttributes->find(attr.source.get_src_def().c_str());
        if (ndAttr == NULL){
          asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s could not find attribute: %s\n",
                    driverName, functionName, attr.source.get_src_def().c_str());
        } else {
          // find the data based on datatype
          NDAttrDataType_t dataType;
          size_t dataSize;
          if (ndAttr->getValueInfo(&dataType, &dataSize) != ND_ERROR){
            datavalue = calloc(dataSize, sizeof(char));
            ret = ndAttr->getValue(dataType, datavalue, dataSize);
            if (ret == ND_ERROR) {
              asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s could not get data from attribute: %s\n",
                        driverName, functionName, attr.get_name().c_str());
            } else {
              if (dataType != NDAttrString && dataType != NDAttrUndefined){
                hid_t hdfattrdataspace = H5Screate(H5S_SCALAR);
                hid_t hdfdatatype      = H5Tcopy(this->typeNd2Hdf((NDDataType_t)dataType));
                hid_t hdfattr = H5Acreate2(hdf_id, attr.get_name().c_str(), hdfdatatype, hdfattrdataspace, H5P_DEFAULT, H5P_DEFAULT);
                if (hdfattr < 0) {
                  asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s unable to create attribute: %s\n",
                            driverName, functionName, attr.get_name().c_str());
                  H5Sclose(hdfattrdataspace);
                } else {
                  herr_t hdfstatus = H5Awrite(hdfattr, hdfdatatype, datavalue);
                  if (hdfstatus < 0) {
                    asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s unable to write attribute: %s\n",
                              driverName, functionName, attr.get_name().c_str());
                  }
                  H5Aclose(hdfattr);
                  H5Sclose(hdfattrdataspace);
                }
              } else if(dataType == NDAttrString){
                // This is a string attribute
                std::string str_val((char *)datavalue);
                this->writeH5attrStr(hdf_id, attr.get_name(), str_val);
              }
            }
            if (datavalue){
              free(datavalue);
            }
          } else {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s could not get datatype information for attribute: %s\n",
                      driverName, functionName, attr.get_name().c_str());
          }
        }
      }
    }
  }
  H5Oclose(hdf_id);
  return status;
}

/** Create the root group and recursively create all subgroups and datasets in the HDF5 file.
 *
 */
asynStatus NDFileHDF5::createTree(hdf5::Group* root, hid_t h5handle)
{
  asynStatus retcode = asynSuccess;
  int storeAttributes;
  static const char *functionName = "createTree";

  if (root == NULL) return asynError; // sanity check

  std::string name = root->get_name();
  if (root->get_parent() == NULL){
    // This is a reserved element and should not be created.  Simply pass through
    hdf5::Group::MapGroups_t::const_iterator it_group;
    hdf5::Group::MapGroups_t& groups = root->get_groups();
    for (it_group = groups.begin(); it_group != groups.end(); ++it_group){
      // recursively call this function to create all subgroups
      retcode = this->createTree( it_group->second, h5handle );
    }
  } else {
    //First make the current group inside the given hdf handle.
    hid_t new_group = H5Gcreate(h5handle, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (new_group < 0){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s Failed to create the root group: %s\n",
                driverName, functionName, name.c_str());
      return asynError;
    }

    // Create all the datasets in this group
    hdf5::Group::MapDatasets_t::iterator it_dsets;
    hdf5::Group::MapDatasets_t& datasets = root->get_datasets();
    for (it_dsets = datasets.begin(); it_dsets != datasets.end(); ++it_dsets){
      if (it_dsets->second->data_source().is_src_ndattribute()) {
        // Creation of NDAttribute datasets are deferred to later
        // in createAttributeDataset()
        continue;
      }
      hid_t new_dset = this->createDataset(new_group, it_dsets->second);
      if (new_dset <= 0) {
        hdf5::Dataset *dset = it_dsets->second;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING,
                  "%s::%s Failed to create dataset: %s. Continuing to next.\n",
                  driverName, functionName, dset->get_name().c_str());
        continue; // failure to create the datasets so move on to next. Should we delete the dset entry from the tree here?
      }
      // Write the hdf attributes to the dataset
      this->writeHdfAttributes( new_dset,  it_dsets->second);
      // Datasets are closed after data has been written
    }

    this->lock();
    getIntegerParam(NDFileHDF5_storeAttributes, &storeAttributes);
    this->unlock();
    if (storeAttributes == 1){
      // Set some attributes on the group
      this->writeHdfAttributes(new_group,  root);
    }

    hdf5::Group::MapGroups_t::const_iterator it_group;
    hdf5::Group::MapGroups_t& groups = root->get_groups();
    for (it_group = groups.begin(); it_group != groups.end(); ++it_group){
      // recursively call this function to create all subgroups
      retcode = this->createTree( it_group->second, new_group );
    }
    // close the handle to the group that we've created in this instance
    // of the function. This is to ensure we're not leaving any hanging,
    // unused, and unreferenced handles around.
    H5Gclose(new_group);
  }
  return retcode;
}

/** Create the hardlinks in the HDF5 file.
 *
 */
asynStatus NDFileHDF5::createHardLinks(hdf5::Group* root)
{
  asynStatus retcode = asynSuccess;
  static const char *functionName = "createHardLinks";

  if (root == NULL) return asynError; // sanity check

  std::string name = root->get_name();
  if (root->get_parent() == NULL){
    // This is a reserved element and cannot contain hard links.  Do its groups.
    hdf5::Group::MapGroups_t::const_iterator it_group;
    hdf5::Group::MapGroups_t& groups = root->get_groups();
    for (it_group = groups.begin(); it_group != groups.end(); ++it_group){
      // recursively call this function to create hardlinks in all subgroups
      retcode = this->createHardLinks(it_group->second);
    }
  } else {
    // Create all the hardlinks in this group
    hdf5::Group::MapHardLinks_t::iterator it_hardlinks;
    hdf5::Group::MapHardLinks_t& hardlinks = root->get_hardlinks();
    for (it_hardlinks = hardlinks.begin(); it_hardlinks != hardlinks.end(); ++it_hardlinks){
      std::string targetName = it_hardlinks->second->get_target();
      std::string linkName = it_hardlinks->second->get_full_name();
      herr_t err = H5Lcreate_hard(this->file, targetName.c_str(), this->file, linkName.c_str(), 0, 0);
      if (err < 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s error creating hard link from: %s to %s\n",
                  driverName, functionName, targetName.c_str(), linkName.c_str());
      }
    }

    hdf5::Group::MapGroups_t::const_iterator it_group;
    hdf5::Group::MapGroups_t& groups = root->get_groups();
    for (it_group = groups.begin(); it_group != groups.end(); ++it_group){
      // recursively call this function to create hardlinks in all subgroups
      retcode = this->createHardLinks(it_group->second);
    }
  }
  return retcode;
}

/** Check this element for any attached attributes and write them out.
 *  Supported types are 'string', 'int' and 'float'.
 *
 *  The types 'int' and 'float' can contain 1D arrays, where each element is separated
 *  by a ','
 *
 */
void NDFileHDF5::writeHdfAttributes( hid_t h5_handle, hdf5::Element* element)
{
  hdf5::Element::MapAttributes_t::iterator it_attr;
  hdf5::DataType_t attr_dtype = hdf5::string;

  for (it_attr=element->get_attributes().begin(); it_attr != element->get_attributes().end(); ++it_attr){
    hdf5::Attribute &attr = it_attr->second; // Take a reference - i.e. *not* a copy!
    if (attr.source.is_src_ndattribute()){
      // This is an onOpen/Close ndattribute, store into the appropriate container
      if (attr.is_onFileOpen()){
        onOpenMap[element->get_full_name()] = element;
      } else if (attr.is_onFileClose()){
        onCloseMap[element->get_full_name()] = element;
      }
    } else {
      attr_dtype = attr.source.get_datatype();
      switch ( attr_dtype )
      {
        case hdf5::string:
          this->writeH5attrStr(h5_handle, attr.get_name(), attr.source.get_src_def());
          break;
        case hdf5::float64:
          this->writeH5attrFloat64(h5_handle, attr.get_name(), attr.source.get_src_def());
          break;
        case hdf5::int32:
          this->writeH5attrInt32(h5_handle, attr.get_name(), attr.source.get_src_def());
          break;
        default:
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::writeHdfAttributes unknown type: unable to create attribute: %s\n",
              driverName, attr.get_name().c_str());
          break;
      }
    }
  }
}

hid_t NDFileHDF5::writeHdfConstDataset( hid_t h5_handle, hdf5::Dataset* dset)
{
  hdf5::DataType_t dtype = hdf5::string;

  if(dset != NULL && dset->data_source().is_src_constant())
  {
    dtype = dset->data_source().get_datatype();
    switch ( dtype )
    {
      case hdf5::string:
        return this->writeH5dsetStr(h5_handle, dset->get_name(), dset->data_source().get_src_def());
        break;
      case hdf5::float64:
        return this->writeH5dsetFloat64(h5_handle, dset->get_name(), dset->data_source().get_src_def());
        break;
      case hdf5::int32:
        return this->writeH5dsetInt32(h5_handle, dset->get_name(), dset->data_source().get_src_def());
        break;
      default:
        return -1;
        break;
    }
  }
  return -1;
}

/** 
 * Write a string constant dataset.
 */
hid_t NDFileHDF5::writeH5dsetStr(hid_t element, const std::string &name, const std::string &str_value) const
{
  herr_t hdfstatus = -1;
  hid_t hdfdatatype = -1;
  hid_t hdfdset = -1;
  hid_t hdfdataspace = -1;
  int rank = 1;
  hsize_t dims = 1;
  static const char *functionName = "writeH5dsetStr";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s name=%s value=%s\n",
            driverName, functionName,
            name.c_str(), str_value.c_str());

  hdfdataspace     = H5Screate_simple(rank, &dims, NULL);
  hdfdatatype      = H5Tcopy(H5T_C_S1);
  hdfstatus        = H5Tset_size(hdfdatatype, str_value.size());
  hdfstatus        = H5Tset_strpad(hdfdatatype, H5T_STR_NULLTERM);
  hdfdset          = H5Dcreate2(element, name.c_str(), hdfdatatype, hdfdataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if (hdfdset < 0) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create constant dataset: %s\n",
              driverName, functionName, name.c_str());
    H5Tclose(hdfdatatype);
    H5Sclose(hdfdataspace);
    return -1;
  }

  hdfstatus = H5Dwrite(hdfdset, hdfdatatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, str_value.c_str());
  if (hdfstatus < 0) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write constant dataset: %s\n",
              driverName, functionName, name.c_str());
    H5Aclose (hdfdset);
    H5Tclose(hdfdatatype);
    H5Sclose(hdfdataspace);
    return -1;
  }

  //H5Dclose (hdfdset);
  H5Tclose(hdfdatatype);
  H5Sclose(hdfdataspace);

  return hdfdset;
}

hid_t NDFileHDF5::writeH5dsetInt32(hid_t element, const std::string &name, const std::string &str_value) const
{
  herr_t hdfstatus = -1;
  hid_t hdfdatatype = -1;
  hid_t hdfdset = -1;
  hid_t hdfdataspace = -1;
  static const char *functionName = "writeH5dsetInt32";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s name=%s value=%s\n",
            driverName, functionName,
            name.c_str(), str_value.c_str());

  hdfdataspace = H5Screate(H5S_SCALAR);
  hdfdatatype  = H5Tcopy(H5T_NATIVE_INT32);

  // Check for an array
  std::vector<int> vect;
  std::stringstream ss(str_value);
  int i;
  while (ss >> i){
    vect.push_back(i);
    if (ss.peek() == ','){
      ss.ignore();
    }
  }
  // Here we have just a single value
  if (vect.size() == 1){
    hdfdset = H5Dcreate2(element, name.c_str(), hdfdatatype, hdfdataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (hdfdset < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create dataset: %s\n",
                driverName, functionName, name.c_str());
      H5Sclose(hdfdataspace);
      return -1;
    }
    epicsInt32 ival;
    sscanf(str_value.c_str(), "%d", &ival);
    hdfstatus = H5Dwrite(hdfdset, hdfdatatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &ival);
    if (hdfstatus < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write dataset: %s\n",
                driverName, functionName, name.c_str());
      H5Dclose(hdfdset);
      H5Sclose(hdfdataspace);
      return -1;
    }
  } else {
    // Here we have an array of integer values
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s array found, size: %d\n",
              driverName, functionName, (int)vect.size());
    // Vector array of integers
    hsize_t dims[1];
    dims[0] = vect.size();
    int *ivalues = new int[vect.size()];
    for (int index = 0; index < (int)vect.size(); index++){
      ivalues[index] = vect[index];
    }
    hdfdataspace = H5Screate(H5S_SIMPLE);
    H5Sset_extent_simple(hdfdataspace, 1, dims, NULL);
    hdfdset = H5Dcreate2(element, name.c_str(), hdfdatatype, hdfdataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (hdfdset < 0) {
      delete [] ivalues;
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create dataset: %s\n",
                driverName, functionName, name.c_str());
      H5Sclose(hdfdataspace);
      return -1;
    }
    hdfstatus = H5Dwrite(hdfdset, hdfdatatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, ivalues);
    delete [] ivalues;
    if (hdfstatus < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write dataset: %s\n",
                driverName, functionName, name.c_str());
      H5Dclose (hdfdset);
      H5Sclose(hdfdataspace);
      return -1;
    }
  }
  //H5Dclose (hdfdset);
  H5Sclose(hdfdataspace);
  return hdfdset;

}

hid_t NDFileHDF5::writeH5dsetFloat64(hid_t element, const std::string &name, const std::string &str_value) const
{
  herr_t hdfstatus = -1;
  hid_t hdfdatatype = -1;
  hid_t hdfdset = -1;
  hid_t hdfdataspace = -1;
  static const char *functionName = "writeH5dsetFloat64";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s name=%s value=%s\n",
            driverName, functionName,
            name.c_str(), str_value.c_str());

  hdfdataspace = H5Screate(H5S_SCALAR);
  hdfdatatype  = H5Tcopy(H5T_NATIVE_DOUBLE);

  // Check for an array
  std::vector<double> vect;
  std::stringstream ss(str_value);
  double i;
  while (ss >> i){
    vect.push_back(i);
    if (ss.peek() == ','){
      ss.ignore();
    }
  }
  // Here we have just a single value
  if (vect.size() == 1){
    hdfdset = H5Dcreate2(element, name.c_str(), hdfdatatype, hdfdataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (hdfdset < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create dataset: %s\n",
                driverName, functionName, name.c_str());
      H5Sclose(hdfdataspace);
      return -1;
    }
    double fval;
    sscanf(str_value.c_str(), "%lf", &fval);
    hdfstatus = H5Dwrite(hdfdset, hdfdatatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, &fval);
    if (hdfstatus < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write dataset: %s\n",
                driverName, functionName, name.c_str());
      H5Dclose (hdfdset);
      H5Sclose(hdfdataspace);
      return -1;
    }
  } else {
    // Here we have an array of integer values
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s array found, size: %d\n",
              driverName, functionName, (int)vect.size());
    // Vector array of doubles
    hsize_t dims[1];
    dims[0] = vect.size();
    double *fvalues = new double[vect.size()];
    for (int index = 0; index < (int)vect.size(); index++){
      fvalues[index] = vect[index];
    }
    hdfdataspace = H5Screate(H5S_SIMPLE);
    H5Sset_extent_simple(hdfdataspace, 1, dims, NULL);
    hdfdset = H5Dcreate2(element, name.c_str(), hdfdatatype, hdfdataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (hdfdset < 0) {
      delete [] fvalues;
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create dataset: %s\n",
                driverName, functionName, name.c_str());
      H5Sclose(hdfdataspace);
      return -1;
    }
    hdfstatus = H5Dwrite(hdfdset, hdfdatatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, fvalues);
    delete [] fvalues;
    if (hdfstatus < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write dataset: %s\n",
                driverName, functionName, name.c_str());
      H5Dclose(hdfdset);
      H5Sclose(hdfdataspace);
      return -1;
    }
  }
  //H5Dclose (hdfdset);
  H5Sclose(hdfdataspace);
  return hdfdset;

}

/**
 * Create the dataset and write it out.
 *
 * Only detector and constant datasets are created in the file out at this time.
 *
 * NDAttribute datasets are created elsewhere in createAttributeDatasets()
 */
hid_t NDFileHDF5::createDataset(hid_t group, hdf5::Dataset *dset)
{
  int retcode = -1;
  if (dset == NULL) return -1; // sanity check

  if (dset->data_source().is_src_detector()) {
      retcode = this->createDatasetDetector(group, dset);
  }
  else if(dset->data_source().is_src_constant()) {
      retcode = this->writeHdfConstDataset(group,  dset);
  }
  else {
    retcode = -1;
  }
  return retcode;
}

/**
 * Write a string constant attribute.
 */
void NDFileHDF5::writeH5attrStr(hid_t element, const std::string &attr_name, const std::string &str_attr_value) const
{
  herr_t hdfstatus = -1;
  hid_t hdfdatatype = -1;
  hid_t hdfattr = -1;
  hid_t hdfattrdataspace = -1;
  static const char *functionName = "writeH5attrStr";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s name=%s value=%s\n",
            driverName, functionName,
            attr_name.c_str(), str_attr_value.c_str());

  hdfattrdataspace = H5Screate(H5S_SCALAR);
  hdfdatatype      = H5Tcopy(H5T_C_S1);
  hdfstatus        = H5Tset_size(hdfdatatype, str_attr_value.size());
  hdfstatus        = H5Tset_strpad(hdfdatatype, H5T_STR_NULLTERM);
  hdfattr          = H5Acreate2(element, attr_name.c_str(), hdfdatatype, hdfattrdataspace, H5P_DEFAULT, H5P_DEFAULT);
  if (hdfattr < 0) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create attribute: %s\n",
              driverName, functionName, attr_name.c_str());
    H5Sclose(hdfattrdataspace);
    return;
  }

  hdfstatus = H5Awrite(hdfattr, hdfdatatype, str_attr_value.c_str());
  if (hdfstatus < 0) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write attribute: %s\n",
              driverName, functionName, attr_name.c_str());
    H5Aclose (hdfattr);
    H5Sclose(hdfattrdataspace);
    return;
  }
  H5Aclose (hdfattr);
  H5Sclose(hdfattrdataspace);
  return;
}

/** 
 * Write an int (or array of ints) constant attribute.
 */
void NDFileHDF5::writeH5attrInt32(hid_t element, const std::string &attr_name, const std::string &str_attr_value) const
{
  herr_t hdfstatus = -1;
  hid_t hdfdatatype = -1;
  hid_t hdfattr = -1;
  hid_t hdfattrdataspace = -1;
  static const char *functionName = "writeH5attrInt32";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s name=%s value=%s\n",
            driverName, functionName,
            attr_name.c_str(), str_attr_value.c_str());

  hdfattrdataspace = H5Screate(H5S_SCALAR);
  hdfdatatype      = H5Tcopy(H5T_NATIVE_INT32);

  // Check for an array
  std::vector<int> vect;
  std::stringstream ss(str_attr_value);
  int i;
  while (ss >> i){
    vect.push_back(i);
    if (ss.peek() == ','){
      ss.ignore();
    }
  }
  // Here we have just a single value
  if (vect.size() == 1){
    hdfattr = H5Acreate2(element, attr_name.c_str(), hdfdatatype, hdfattrdataspace, H5P_DEFAULT, H5P_DEFAULT);
    if (hdfattr < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create attribute: %s\n",
                driverName, functionName, attr_name.c_str());
      H5Sclose(hdfattrdataspace);
      return;
    }
    epicsInt32 ival;
    sscanf(str_attr_value.c_str(), "%d", &ival);
    hdfstatus = H5Awrite(hdfattr, hdfdatatype, &ival);
    if (hdfstatus < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write attribute: %s\n",
                driverName, functionName, attr_name.c_str());
      H5Aclose (hdfattr);
      H5Sclose(hdfattrdataspace);
      return;
    }
  } else {
    // Here we have an array of integer values
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s array found, size: %d\n",
              driverName, functionName, (int)vect.size());
    // Vector array of integers
    hsize_t dims[1];
    dims[0] = vect.size();
    int *ivalues = new int[vect.size()];
    for (int index = 0; index < (int)vect.size(); index++){
      ivalues[index] = vect[index];
    }
    hdfattrdataspace = H5Screate(H5S_SIMPLE);
    H5Sset_extent_simple(hdfattrdataspace, 1, dims, NULL);
    hdfattr = H5Acreate2(element, attr_name.c_str(), hdfdatatype, hdfattrdataspace, H5P_DEFAULT, H5P_DEFAULT);
    if (hdfattr < 0) {
      delete [] ivalues;
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create attribute: %s\n",
                driverName, functionName, attr_name.c_str());
      H5Sclose(hdfattrdataspace);
      return;
    }
    hdfstatus = H5Awrite(hdfattr, hdfdatatype, ivalues);
    delete [] ivalues;
    if (hdfstatus < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write attribute: %s\n",
                driverName, functionName, attr_name.c_str());
      H5Aclose (hdfattr);
      H5Sclose(hdfattrdataspace);
      return;
    }
  }
  H5Aclose (hdfattr);
  H5Sclose(hdfattrdataspace);
  return;
}

/** 
 * Write a float (or array of floats) constant attribute.
 */
void NDFileHDF5::writeH5attrFloat64(hid_t element, const std::string &attr_name, const std::string &str_attr_value) const
{
  herr_t hdfstatus = -1;
  hid_t hdfdatatype = -1;
  hid_t hdfattr = -1;
  hid_t hdfattrdataspace = -1;
  static const char *functionName = "writeH5attrFloat64";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s name=%s value=%s\n",
            driverName, functionName,
            attr_name.c_str(), str_attr_value.c_str());

  hdfattrdataspace = H5Screate(H5S_SCALAR);
  hdfdatatype      = H5Tcopy(H5T_NATIVE_DOUBLE);

  // Check for an array
  std::vector<double> vect;
  std::stringstream ss(str_attr_value);
  double i;
  while (ss >> i){
    vect.push_back(i);
    if (ss.peek() == ','){
      ss.ignore();
    }
  }
  // Here we have just a single value
  if (vect.size() == 1){
    hdfattr = H5Acreate2(element, attr_name.c_str(), hdfdatatype, hdfattrdataspace, H5P_DEFAULT, H5P_DEFAULT);
    if (hdfattr < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create attribute: %s\n",
                driverName, functionName, attr_name.c_str());
      H5Sclose(hdfattrdataspace);
      return;
    }
    double fval;
    sscanf(str_attr_value.c_str(), "%lf", &fval);
    hdfstatus = H5Awrite(hdfattr, hdfdatatype, &fval);
    if (hdfstatus < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write attribute: %s\n",
                driverName, functionName, attr_name.c_str());
      H5Aclose (hdfattr);
      H5Sclose(hdfattrdataspace);
      return;
    }
  } else {
    // Here we have an array of integer values
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s array found, size: %d\n",
              driverName, functionName, (int)vect.size());
    // Vector array of doubles
    hsize_t dims[1];
    dims[0] = vect.size();
    double *fvalues = new double[vect.size()];
    for (int index = 0; index < (int)vect.size(); index++){
      fvalues[index] = vect[index];
    }
    hdfattrdataspace = H5Screate(H5S_SIMPLE);
    H5Sset_extent_simple(hdfattrdataspace, 1, dims, NULL);
    hdfattr = H5Acreate2(element, attr_name.c_str(), hdfdatatype, hdfattrdataspace, H5P_DEFAULT, H5P_DEFAULT);
    if (hdfattr < 0) {
      delete [] fvalues;
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to create attribute: %s\n",
                driverName, functionName, attr_name.c_str());
      H5Sclose(hdfattrdataspace);
      return;
    }
    hdfstatus = H5Awrite(hdfattr, hdfdatatype, fvalues);
    delete [] fvalues;
    if (hdfstatus < 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unable to write attribute: %s\n",
                driverName, functionName, attr_name.c_str());
      H5Aclose (hdfattr);
      H5Sclose(hdfattrdataspace);
      return;
    }
  }
  H5Aclose (hdfattr);
  H5Sclose(hdfattrdataspace);
  return;
}

/**
 * Utility method to convert from hdf5 to hid datatype.
 */
hid_t NDFileHDF5::fromHdfToHidDatatype(hdf5::DataType_t in) const
{
  hid_t out;
  switch(in)
  {
  case hdf5::int8:
    out = H5T_NATIVE_INT8; break;
  case hdf5::uint8:
    out = H5T_NATIVE_UINT8; break;
  case hdf5::int16:
    out = H5T_NATIVE_INT16; break;
  case hdf5::uint16:
    out = H5T_NATIVE_UINT16; break;
  case hdf5::int32:
    out = H5T_NATIVE_INT32; break;
  case hdf5::uint32:
    out = H5T_NATIVE_UINT32; break;
  case hdf5::float32:
    out = H5T_NATIVE_FLOAT; break;
  case hdf5::float64:
    out = H5T_NATIVE_DOUBLE; break;
  case hdf5::string:
    out = H5T_C_S1; break;
  default:
    out = H5T_NATIVE_INT8;
    break;
  }
  return out;
}

/** Create a dataset in the HDF5 file with the details defined in the dset argument.
 * Return the hid_t handle to the new dataset on success; -1 on error.
 * Errors: fail to set chunk size or failure to create the dataset in the file.
 */
hid_t NDFileHDF5::createDatasetDetector(hid_t group, hdf5::Dataset *dset)
{
  static const char *functionName = "createDatasetDetector";

  if (dset == NULL) return -1; // sanity check
  hid_t dataset = -1;

  hid_t dset_access_plist = H5Pcreate(H5P_DATASET_ACCESS);
  hsize_t nbytes = this->calcChunkCacheBytes();
  hsize_t nslots = this->calcChunkCacheSlots();
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Setting cache size=%d slots=%d\n",
            driverName, functionName,
            (int)nbytes, (int)nslots);
  H5Pset_chunk_cache( dset_access_plist, (size_t)nslots, (size_t)nbytes, 1.0);

  /*
   * Create a new dataset within the file using cparms
   * creation properties.
   */
  const char * dsetname = dset->get_name().c_str();

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s Creating first empty dataset called \"%s\"\n", 
            driverName, functionName, dsetname);
  dataset = H5Dcreate2(group, dsetname, this->datatype, this->dataspace,
                       H5P_DEFAULT, this->cparms, dset_access_plist);

  // Store the dataset into the detector dataset map
  this->detDataMap[dset->get_full_name()] = new NDFileHDF5Dataset(this->pasynUserSelf, dset->get_name(), dataset);

  return dataset;
}

/** Writes NDArray data to a HDF5 file.
  * \param[in] pArray Pointer to an NDArray to write to the file. This function can be called multiple
  *            times between the call to openFile and closeFile if NDFileModeMultiple was set in 
  *            openMode in the call to NDFileHDF5::openFile.
  */
asynStatus NDFileHDF5::writeFile(NDArray *pArray)
{
  herr_t hdfstatus = 0;
  asynStatus status = asynSuccess;
  int storeAttributes, storePerformance, flush;
  epicsTimeStamp startts, endts;
  epicsInt32 numCaptured;
  double dt=0.0, period=0.0, runtime = 0.0;
  int extradims = 0;
  static const char *functionName = "writeFile";

  if (this->file == 0) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
              "%s::%s file is not open!\n", 
              driverName, functionName);
    return asynError;
  }

  this->lock();
  getIntegerParam(NDFileNumCaptured, &numCaptured);
  getIntegerParam(NDFileHDF5_storeAttributes, &storeAttributes);
  getIntegerParam(NDFileHDF5_storePerformance, &storePerformance);
  getIntegerParam(NDFileHDF5_flushNthFrame, &flush);
  getIntegerParam(NDFileHDF5_nExtraDims, &extradims);
  this->unlock();

  if (numCaptured == 1) epicsTimeGetCurrent(&this->firstFrame);

  if (storeAttributes == 1){
    // Update attribute list. We use a separate attribute list
    // from the one in pArray to avoid the need to copy the array.
    // Get the current values of the attributes for this plugin
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
              "%s::%s getting attribute list\n", 
              driverName, functionName);
    status = (asynStatus)this->getAttributes(this->pFileAttributes);
    if (status != asynSuccess){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s ERROR: could not update the attribute list\n",
                driverName, functionName);
      return asynError;
    }

    // Insert default NDAttribute from the NDArray object (timestamps etc)
    this->addDefaultAttributes(pArray);

    // Now append the attributes from the array which are already up to date from
    // the driver and prior plugins
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
              "%s::%s copying attribute list\n", 
              driverName, functionName);
    status = (asynStatus)pArray->pAttributeList->copy(this->pFileAttributes);
    if (status != asynSuccess){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s ERROR: could not append attributes to NDArray from driver\n",
                driverName, functionName);
      return asynError;
    }
  }

  // If we have a defined dataset destination NDAttribute name then we need to find the
  // destination of this frame
  std::string destination = this->defDsetName;
  if (this->ndDsetName != ""){
    NDAttribute *destAtt = pArray->pAttributeList->find(this->ndDsetName.c_str());
    if (destAtt){
      char pValue[128];
      if (destAtt->getValue(NDAttrString, pValue, 128) != ND_ERROR){
        if (this->detDataMap.count(pValue) == 1){
          destination = std::string(pValue);
          asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                    "%s::%s destination dataset specified as %s\n", 
                    driverName, functionName, destination.c_str());
        } else {
          asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                    "%s::%s destination specified does not exist, using default [%s]\n", 
                    driverName, functionName, destination.c_str());
        }
      } else {
        // Error, unable to get the value from the dataset destination attribute
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s ERROR: could not retrieve destination from specified attribute\n",
                  driverName, functionName);
        return asynError;
      }
    }
  }

  // Get the current time to calculate performance times
  epicsTimeGetCurrent(&startts);

  // For multi frame files we now extend the HDF dataset to fit an additional frame
  if (this->multiFrameFile) this->detDataMap[destination]->extendDataSet(extradims);

  status = this->detDataMap[destination]->writeFile(pArray, this->datatype, this->dataspace, this->framesize);
  if (status != asynSuccess){
    // If dataset creation fails then close file and abort as all following writes will fail as well
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
              "%s::%s ERROR: could not write to dataset. Aborting\n",
              driverName, functionName);
    hdfstatus = H5Sclose(this->dataspace);
    if (hdfstatus){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Dataspace did not close cleanly.\n",
                driverName, functionName);
    }
    hdfstatus = H5Pclose(this->cparms);
    if (hdfstatus){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Cparms did not close cleanly.\n",
                driverName, functionName);
    }
    hdfstatus = H5Tclose(this->datatype);
    if (hdfstatus){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s ERROR: Datatype did not close cleanly.\n",
                driverName, functionName);
    }
    hdfstatus = H5Fclose(this->file);
    if (hdfstatus){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s ERROR: File did not close cleanly.\n",
                driverName, functionName);
    }
    this->file = 0;
    this->lock();
    setIntegerParam(NDFileCapture, 0);
    setIntegerParam(NDWriteFile, 0);
    this->unlock();
    return asynError;
  }

  if (storeAttributes == 1){
    status = this->writeAttributeDataset(hdf5::OnFrame);
    if (status != asynSuccess){
      return status;
    }
  }
  if (storePerformance == 1 && numCaptured <= this->numPerformancePoints){
    epicsTimeGetCurrent(&endts);
    dt = epicsTimeDiffInSeconds(&endts, &startts);
    *this->performancePtr = dt;
    this->performancePtr++;
    period = epicsTimeDiffInSeconds(&endts, &this->prevts);
    *this->performancePtr = period;
    this->prevts = endts;
    this->performancePtr++;
    runtime = epicsTimeDiffInSeconds(&endts, &this->firstFrame);
    *this->performancePtr = runtime;
    this->performancePtr++;
    *this->performancePtr = this->frameSize/period;
    this->performancePtr++;
    *this->performancePtr = (numCaptured * this->frameSize)/runtime;
    this->performancePtr++;
  }

  if (flush > 0){
    if (numCaptured % flush == 0) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
        "%s::%s flushing metadata (%d)\n", 
        driverName, functionName, numCaptured);
      hdfstatus = H5Fflush( this->file, H5F_SCOPE_GLOBAL );
      if (hdfstatus < 0) {
        // If flushing fails then close file and abort as all following writes will fail as well
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s ERROR: could not flush file. Aborting\n",
                  driverName, functionName);
        hdfstatus = H5Sclose(this->dataspace);
        if (hdfstatus){
          asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: Dataspace did not close cleanly.\n",
                    driverName, functionName);
        }
        hdfstatus = H5Pclose(this->cparms);
        if (hdfstatus){
          asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: Cparms did not close cleanly.\n",
                    driverName, functionName);
        }
        hdfstatus = H5Tclose(this->datatype);
        if (hdfstatus){
          asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: Datatype did not close cleanly.\n",
                    driverName, functionName);
        }
        hdfstatus = H5Fclose(this->file);
        if (hdfstatus){
          asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s::%s ERROR: File did not close cleanly.\n",
                    driverName, functionName);
        }
        this->file = 0;
        this->lock();
        setIntegerParam(NDFileCapture, 0);
        setIntegerParam(NDWriteFile, 0);
        this->unlock();
        return asynError;
      }
    }
  }
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s wrote frame. dt=%.5fs (T=%.5fs)\n", 
            driverName, functionName, dt, period);

  this->nextRecord++;
  return asynSuccess;
}

/** Read NDArray data from a HDF5 file; NOTE: not implemented yet.
  * \param[in] pArray Pointer to the address of an NDArray to read the data into.  */ 
asynStatus NDFileHDF5::readFile(NDArray **pArray)
{
  //static const char *functionName = "readFile";
  return asynError;
}

/** Closes the HDF5 file opened with NDFileHDF5::openFile 
 */ 
asynStatus NDFileHDF5::closeFile()
{
  int storeAttributes, storePerformance;
  epicsTimeStamp now;
  double runtime = 0.0, writespeed = 0.0;
  epicsInt32 numCaptured;
  static const char *functionName = "closeFile";

  if (this->file == 0){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
              "%s::%s file was not open! Ignoring close command.\n", 
              driverName, functionName);
    return asynSuccess;
  }

  this->lock();
  getIntegerParam(NDFileHDF5_storeAttributes, &storeAttributes);
  getIntegerParam(NDFileHDF5_storePerformance, &storePerformance);
  this->unlock();
  if (storeAttributes == 1) {
     this->writeAttributeDataset(hdf5::OnFileClose);
     this->storeOnCloseAttributes();
     this->closeAttributeDataset();
  }
  if (storePerformance == 1) this->writePerformanceDataset();

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s closing HDF cparms %d\n", 
            driverName, functionName, this->cparms);

  H5Pclose(this->cparms);

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s closing HDF datatype %d\n", 
            driverName, functionName, this->datatype);

  H5Tclose(this->datatype);

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s closing groups\n", 
            driverName, functionName);

  // Iterate over the stored detector data sets and close them
  std::map<std::string, NDFileHDF5Dataset *>::iterator it_dset;
  for (it_dset = this->detDataMap.begin(); it_dset != this->detDataMap.end(); ++it_dset){
    H5Dclose(it_dset->second->getHandle());
  }
  std::map<std::string, hid_t>::iterator it_hid;
  // Iterate over the stored attribute data sets and close them
  for (it_hid = this->attDataMap.begin(); it_hid != this->attDataMap.end(); ++it_hid){
    H5Dclose(it_hid->second);
  }

  // Just before closing the file lets ensure there are no hanging references
  int obj_count = (int)H5Fget_obj_count(this->file, H5F_OBJ_GROUP);
  if (obj_count > 0){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
              "%s::%s Closing file not totally clean.  Groups remaining=%d\n",
              driverName, functionName, obj_count);
  }
  obj_count = (int)H5Fget_obj_count(this->file, H5F_OBJ_DATASET);
  if (obj_count > 0){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
              "%s::%s Closing file not totally clean.  Datasets remaining=%d\n",
              driverName, functionName, obj_count);
  }
  obj_count = (int)H5Fget_obj_count(this->file, H5F_OBJ_ATTR);
  if (obj_count > 0){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
              "%s::%s Closing file not totally clean.  Attributes remaining=%d\n",
              driverName, functionName, obj_count);
  }

  // Close the HDF file
  H5Fclose(this->file);
  this->file = 0;

  // Unload the XML layout
  this->layout.unload_xml();

  // Reset the default data set and clear out the maps of handles to stale datasets
  detDataMap.clear();
  attDataMap.clear();
  defDsetName = "";

  epicsTimeGetCurrent(&now);
  runtime = epicsTimeDiffInSeconds(&now, &this->opents);
  this->lock();
  getIntegerParam(NDFileNumCaptured, &numCaptured);
  writespeed = (numCaptured * this->frameSize)/runtime;
  setDoubleParam(NDFileHDF5_totalIoSpeed, writespeed);
  setDoubleParam(NDFileHDF5_totalRuntime, runtime);
  this->unlock();

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
            "%s::%s file closed! runtime=%.3f s overall acquisition performance=%.2f Mbit/s\n",
            driverName, functionName, runtime, writespeed);

  return asynSuccess;
}

/** Perform any actions required when an int32 parameter is updated.
 */
asynStatus NDFileHDF5::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int addr=0;
  int oldvalue = 0;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  epicsInt32  numExtraDims, tmp;
  htri_t avail;
  unsigned int filter_info;
  H5Z_filter_t filterId;
  static const char *functionName = "writeInt32";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
  getIntegerParam(function, &oldvalue);

  // By default we set the value in the parameter lib. If problems occur we set the old value back.
  setIntegerParam(function, value);

  asynPrint(pasynUser, ASYN_TRACE_FLOW,
           "%s:%s: function=%d, value=%d old=%d\n",
            driverName, functionName, function, value, oldvalue);

  getIntegerParam(NDFileHDF5_nExtraDims,  &numExtraDims);

  if (function == NDFileHDF5_nExtraDims)
  {
    if (value < 0 || value > MAXEXTRADIMS-1 || this->file != 0)
    {
      status = asynError;
      setIntegerParam(NDFileHDF5_nExtraDims, oldvalue);
    } else
    {
      // If we use the extra dimensions, work out how many frames to capture in total
      if (value > 0) this->calcNumFrames();

    }
  } else if (function == NDFileNumCapture)
  {
    if (value < 0) {
      // It is not allowed to specify a negative number of frames to capture
      setIntegerParam(NDFileNumCapture, oldvalue);
      status = asynError;
    }
    else if (value == 0) {
      // Special case: allow writing infinite number of frames
      //setIntegerParam(NDFileHDF5_storePerformance, 0); // performance dataset does not support infinite length acquisition
      setIntegerParam(NDFileHDF5_nExtraDims, 0); // The extra virtual dimensions do not support infinite length acquisition
    }
    // if we are using the virtual dimensions we cannot allow setting a number of
    // frames to acquire which is larger than the product of all virtual dimension (n,X,Y) sizes
    // as there will not be a suitable location in the file to store the additional frames.
    else if (numExtraDims > 0)
    {
      this->calcNumFrames();
      getIntegerParam(NDFileNumCapture, &tmp);
      if (value < tmp) {
        setIntegerParam(function, value);
      }       
    }

  } else if (function == NDFileHDF5_nRowChunks ||
             function == NDFileHDF5_nColChunks ||
             function == NDFileHDF5_nFramesChunks )
  {
    // It is not allowed to change chunking while a file is open
    if (this->file != 0) {
      status = asynError;
      setIntegerParam(function, oldvalue);
    }
  }

  else if (function == NDFileHDF5_extraDimSizeN ||
             function == NDFileHDF5_extraDimSizeX ||
             function == NDFileHDF5_extraDimSizeY)
    {
    // Not allowed to change dimension sizes once the file is opened
    if (this->file != 0) {
      status = asynError;
      setIntegerParam(function, oldvalue);
    } else if (value <= 0) {
      status = asynError;
      setIntegerParam(function, oldvalue);
    } else {
      // work out how many frames to capture in total
      this->calcNumFrames();
    }
  } else if (function == NDFileHDF5_storeAttributes ||
         function == NDFileHDF5_storePerformance) {
    if (this->file != 0) {
      status = asynError;
      setIntegerParam(function, oldvalue);
    }
  } else if (function == NDFileHDF5_compressionType) {
    if (this->file != 0)
    {
      status = asynError;
      setIntegerParam(function, oldvalue);
    } else
    {
      switch (value)
      {
      case HDF5CompressNone:
        // if no compression desired we do nothing
        filterId = H5Z_FILTER_NONE;
        break;
      case HDF5CompressSZip:
        filterId = H5Z_FILTER_SZIP;
        break;
      case HDF5CompressNumBits:
        filterId = H5Z_FILTER_NBIT;
        break;
      case HDF5CompressZlib:
        filterId = H5Z_FILTER_DEFLATE;
        break;
      default:
        filterId = H5Z_FILTER_NONE;
        status = asynError;
        setIntegerParam(function, oldvalue);
        break;
      }

      // If compression filter required then we do a couple of checks to
      // see if this is possible:
      // 1) Check that the filter (for instance szip library) is available
      if (filterId != H5Z_FILTER_NONE) {
        avail = H5Zfilter_avail(filterId);
        if (!avail) {
          status = asynError;
          setIntegerParam(function, oldvalue);
          asynPrint (pasynUser, ASYN_TRACE_ERROR, 
            "%s::%s ERROR: HDF5 compression filter (%d) not available\n",
            driverName, functionName, (int)filterId);
        } else
        {
          // 2) Check that the filter is configured for encoding
          H5Zget_filter_info (filterId, &filter_info);
          if ( !(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) )
          {
            asynPrint (pasynUser, ASYN_TRACE_ERROR, 
              "%s::%s ERROR: HDF5 compression filter (%d) not available for encoding\n",
              driverName, functionName, (int)filterId);
            status = asynError;
            setIntegerParam(function, oldvalue);
          }
        }
      }
    }
  } else if (function == NDFileHDF5_nbitsPrecision) {
    if (this->file != 0 || value < 0)
    {
      status = asynError;
      setIntegerParam(function, oldvalue);
    } else
    {
      getIntegerParam(NDFileHDF5_nbitsOffset, &tmp);
      // Check if prec+offset is within the size of the datatype

    }
  } else if (function == NDFileHDF5_nbitsOffset) {
    if (this->file != 0 || value < 0)
    {
      status = asynError;
      setIntegerParam(function, oldvalue);
    } else
    {
      getIntegerParam(NDFileHDF5_nbitsPrecision, &tmp);
      // Check if prec+offset is within the size of the datatype

    }

  } else if (function == NDFileHDF5_szipNumPixels) {
    // The szip compression parameter is the number of pixels to group during compression.
    // According to the HDF5 API documentation the value has to be an even number, and
    // no greater than 32.
    if (this->file != 0 || value < 0 || value > 32)
    {
      status = asynError;
      setIntegerParam(function, oldvalue);
    }
  } else if (function == NDFileHDF5_zCompressLevel) {
    if (this->file != 0 || value < 0 || value > 9)
    {
      status = asynError;
      setIntegerParam(function, oldvalue);
    }
  } else
  {
    if (function < FIRST_NDFILE_HDF5_PARAM)
    {
      /* If this parameter belongs to a base class call its method */
      asynPrint(pasynUser, ASYN_TRACE_FLOW,
                "%s:%s: calling base class function=%d, value=%d old=%d\n",
                 driverName, functionName, function, value, oldvalue);
      status = NDPluginFile::writeInt32(pasynUser, value);
      return status;
    }
  }

  /* Do callbacks so higher layers see any changes */
  callParamCallbacks(addr, addr);

  if (status){
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: ERROR status=%d, function=%d, value=%d old=%d\n",
               driverName, functionName, status, function, value, oldvalue);
  }
  else
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s: status=%d function=%d, value=%d old=%d\n",
              driverName, functionName, status, function, value, oldvalue);
  return status;
}

/** Called when asyn clients call pasynOctet->write().
  * This function performs actions for some parameters, including AttributesFile.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] nChars Number of characters to write.
  * \param[out] nActual Number of characters actually written. */
asynStatus NDFileHDF5::writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual)
{
  int addr=0;
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  const char *functionName = "writeOctet";

  status = getAddress(pasynUser, &addr); if (status != asynSuccess) return(status);
  // Set the parameter in the parameter library.
  status = (asynStatus)setStringParam(addr, function, (char *)value);
  if (status != asynSuccess) return(status);

  if (function == NDFileHDF5_layoutFilename){
    status = (asynStatus)this->verifyLayoutXMLFile();
  } 
  
  else if (function < FIRST_NDFILE_HDF5_PARAM) {
      /* If this parameter belongs to a base class call its method */
      status = NDPluginFile::writeOctet(pasynUser, value, nChars, nActual);
  }

  // Do callbacks so higher layers see any changes
  callParamCallbacks(addr, addr);

  if (status){
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, value=%s",
                  driverName, functionName, status, function, value);
  } else {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%s\n",
              driverName, functionName, function, value);
  }
  *nActual = nChars;
  return status;
}

/** Check if the specified filename/path exists
 */
int NDFileHDF5::fileExists(char *filename)
{
  struct stat buffer;   
  return (stat (filename, &buffer) == 0);
}

/** Verify the XML layout file is valid.
 *
 *  This method checks the file exists and can be opened.  If these checks
 *  pass then the file is opened and the XML is also parsed and verified.  Any
 *  error messages are reported and the status set accordingly.
 *  This must be called with the lock already taken.
 */
int NDFileHDF5::verifyLayoutXMLFile()
{
  int status = asynSuccess;
//  Reading in a filename or string of xml. We will need more than 256 bytes
  char *fileName = new char[MAX_LAYOUT_LEN];
  fileName[MAX_LAYOUT_LEN - 1] = '\0';
  const char *functionName = "verifyLayoutXMLFile";

  getStringParam(NDFileHDF5_layoutFilename, MAX_LAYOUT_LEN-1, fileName);
  if (strlen(fileName) == 0){
    setIntegerParam(NDFileHDF5_layoutValid, 1);
    setStringParam(NDFileHDF5_layoutErrorMsg, "Default layout selected");
    delete [] fileName; 
    return(asynSuccess);
  }

  if (strstr(fileName, "<?xml") == NULL) {
    if(!fileExists(fileName)) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                  "%s::%s XML description file could not be opened.\n", 
                  driverName, functionName);
        setIntegerParam(NDFileHDF5_layoutValid, 0);
        setStringParam(NDFileHDF5_layoutErrorMsg, "XML description file cannot be opened");
        delete [] fileName;
        return asynError;
    }
  }

  std::string strFileName = std::string(fileName);
  if (this->layout.verify_xml(strFileName)){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s::%s XML description file parser error.\n", 
              driverName, functionName);
    setIntegerParam(NDFileHDF5_layoutValid, 0);
    setStringParam(NDFileHDF5_layoutErrorMsg, "XML description file parser error");
    delete [] fileName;
    return asynError;
  }

  setIntegerParam(NDFileHDF5_layoutValid, 1);
  setStringParam(NDFileHDF5_layoutErrorMsg, "");
  delete [] fileName;
  return status;
}

/** Constructor for NDFileHDF5; parameters are identical to those for NDPluginFile::NDPluginFile,
    and are passed directly to that base class constructor.
  * After calling the base class constructor this method sets NDPluginFile::supportsMultipleArrays=1.
  */
NDFileHDF5::NDFileHDF5(const char *portName, int queueSize, int blockingCallbacks, 
                       const char *NDArrayPort, int NDArrayAddr,
                       int priority, int stackSize)
  /* Invoke the base class constructor.
   * We allocate 2 NDArrays of unlimited size in the NDArray pool.
   * This driver can block (because writing a file can be slow), and it is not multi-device.  
   * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
  : NDPluginFile(portName, queueSize, blockingCallbacks,
                 NDArrayPort, NDArrayAddr, 1, NUM_NDFILE_HDF5_PARAMS,
                 2, 0, asynGenericPointerMask, asynGenericPointerMask, 
                 ASYN_CANBLOCK, 1, priority, stackSize)
{
  //static const char *functionName = "NDFileHDF5";

  this->createParam(str_NDFileHDF5_nRowChunks,      asynParamInt32,   &NDFileHDF5_nRowChunks);
  this->createParam(str_NDFileHDF5_nColChunks,      asynParamInt32,   &NDFileHDF5_nColChunks);
  this->createParam(str_NDFileHDF5_extraDimSizeN,   asynParamInt32,   &NDFileHDF5_extraDimSizeN);
  this->createParam(str_NDFileHDF5_nFramesChunks,   asynParamInt32,   &NDFileHDF5_nFramesChunks);
  this->createParam(str_NDFileHDF5_chunkBoundaryAlign, asynParamInt32,&NDFileHDF5_chunkBoundaryAlign);
  this->createParam(str_NDFileHDF5_chunkBoundaryThreshold, asynParamInt32,&NDFileHDF5_chunkBoundaryThreshold);
  this->createParam(str_NDFileHDF5_NDAttributeChunk,asynParamInt32,   &NDFileHDF5_NDAttributeChunk);
  this->createParam(str_NDFileHDF5_extraDimNameN,   asynParamOctet,   &NDFileHDF5_extraDimNameN);
  this->createParam(str_NDFileHDF5_nExtraDims,      asynParamInt32,   &NDFileHDF5_nExtraDims);
  this->createParam(str_NDFileHDF5_extraDimSizeX,   asynParamInt32,   &NDFileHDF5_extraDimSizeX);
  this->createParam(str_NDFileHDF5_extraDimNameX,   asynParamOctet,   &NDFileHDF5_extraDimNameX);
  this->createParam(str_NDFileHDF5_extraDimOffsetX, asynParamInt32,   &NDFileHDF5_extraDimOffsetX);
  this->createParam(str_NDFileHDF5_extraDimSizeY,   asynParamInt32,   &NDFileHDF5_extraDimSizeY);
  this->createParam(str_NDFileHDF5_extraDimNameY,   asynParamOctet,   &NDFileHDF5_extraDimNameY);
  this->createParam(str_NDFileHDF5_extraDimOffsetY, asynParamInt32,   &NDFileHDF5_extraDimOffsetY);
  this->createParam(str_NDFileHDF5_storeAttributes, asynParamInt32,   &NDFileHDF5_storeAttributes);
  this->createParam(str_NDFileHDF5_storePerformance,asynParamInt32,   &NDFileHDF5_storePerformance);
  this->createParam(str_NDFileHDF5_totalRuntime,    asynParamFloat64, &NDFileHDF5_totalRuntime);
  this->createParam(str_NDFileHDF5_totalIoSpeed,    asynParamFloat64, &NDFileHDF5_totalIoSpeed);
  this->createParam(str_NDFileHDF5_flushNthFrame,   asynParamInt32,   &NDFileHDF5_flushNthFrame);
  this->createParam(str_NDFileHDF5_compressionType, asynParamInt32,   &NDFileHDF5_compressionType);
  this->createParam(str_NDFileHDF5_nbitsPrecision,  asynParamInt32,   &NDFileHDF5_nbitsPrecision);
  this->createParam(str_NDFileHDF5_nbitsOffset,     asynParamInt32,   &NDFileHDF5_nbitsOffset);
  this->createParam(str_NDFileHDF5_szipNumPixels,   asynParamInt32,   &NDFileHDF5_szipNumPixels);
  this->createParam(str_NDFileHDF5_zCompressLevel,  asynParamInt32,   &NDFileHDF5_zCompressLevel);
  this->createParam(str_NDFileHDF5_layoutErrorMsg,  asynParamOctet,   &NDFileHDF5_layoutErrorMsg);
  this->createParam(str_NDFileHDF5_layoutValid,     asynParamInt32,   &NDFileHDF5_layoutValid);
  this->createParam(str_NDFileHDF5_layoutFilename,  asynParamOctet,   &NDFileHDF5_layoutFilename);

  setIntegerParam(NDFileHDF5_nRowChunks,      0);
  setIntegerParam(NDFileHDF5_nColChunks,      0);
  setIntegerParam(NDFileHDF5_nFramesChunks,   0);
  setIntegerParam(NDFileHDF5_NDAttributeChunk,0);
  setIntegerParam(NDFileHDF5_extraDimSizeN,   1);
  setIntegerParam(NDFileHDF5_chunkBoundaryAlign, 0);
  setIntegerParam(NDFileHDF5_chunkBoundaryThreshold, 65536);
  setIntegerParam(NDFileHDF5_nExtraDims,      0);
  setIntegerParam(NDFileHDF5_extraDimSizeX,   1);
  setIntegerParam(NDFileHDF5_extraDimOffsetX, 0);
  setIntegerParam(NDFileHDF5_extraDimSizeY,   1);
  setIntegerParam(NDFileHDF5_extraDimOffsetY, 0);
  setIntegerParam(NDFileHDF5_storeAttributes, 1);
  setIntegerParam(NDFileHDF5_storePerformance,1);
  setDoubleParam (NDFileHDF5_totalRuntime,    0.0);
  setDoubleParam (NDFileHDF5_totalIoSpeed,    0.0);
  setIntegerParam(NDFileHDF5_flushNthFrame,   0);
  setIntegerParam(NDFileHDF5_compressionType, HDF5CompressNone);
  setIntegerParam(NDFileHDF5_nbitsPrecision,  8);
  setIntegerParam(NDFileHDF5_nbitsOffset,     0);
  setIntegerParam(NDFileHDF5_szipNumPixels,   16);
  setIntegerParam(NDFileHDF5_zCompressLevel,  6);
  setStringParam (NDFileHDF5_layoutErrorMsg,  "");
  setIntegerParam(NDFileHDF5_layoutValid,     1);
  setStringParam (NDFileHDF5_layoutFilename,  "");


  /* Give the virtual dimensions some human readable names */
  this->extraDimNameN = (char*)calloc(DIMNAMESIZE, sizeof(char));
  this->extraDimNameX = (char*)calloc(DIMNAMESIZE, sizeof(char));
  this->extraDimNameY = (char*)calloc(DIMNAMESIZE, sizeof(char));
  epicsSnprintf(this->extraDimNameN, DIMNAMESIZE, "frame number n");
  epicsSnprintf(this->extraDimNameX, DIMNAMESIZE, "scan dimension X");
  epicsSnprintf(this->extraDimNameY, DIMNAMESIZE, "scan dimension Y");
  setStringParam(NDFileHDF5_extraDimNameN, this->extraDimNameN);
  setStringParam(NDFileHDF5_extraDimNameX, this->extraDimNameX);
  setStringParam(NDFileHDF5_extraDimNameY, this->extraDimNameY);
  for (int i=0; i<ND_ARRAY_MAX_DIMS;i++)
    this->ptrDimensionNames[i] = (char*)calloc(DIMNAMESIZE, sizeof(char));
  
  /* Set the plugin type string */  
  unsigned majnum=0, minnum=0, relnum=0;
  H5get_libversion( &majnum, &minnum, &relnum );
  char* plugin_type = (char*)calloc(40, sizeof(char));
  epicsSnprintf(plugin_type, 40, "NDFileHDF5 ver%d.%d.%d", majnum, minnum, relnum);
  //printf("plugin type and version: %s\n", plugin_type );
  setStringParam(NDPluginDriverPluginType, plugin_type);
  this->supportsMultipleArrays = 1;
  this->pAttributeId = NULL;
  this->pFileAttributes = new NDAttributeList;

  // initialise the dimension arrays to a NULL pointer so they
  // will be allocated when opening the first file.
  this->maxdims      = NULL;
  this->chunkdims    = NULL;
  this->framesize    = NULL;
  this->dims         = NULL;
  this->offset       = NULL;
  this->virtualdims  = NULL;
  this->rank         = 0;
  this->file         = 0;
  this->ptrFillValue = (void*)calloc(8, sizeof(char));
  this->dimsreport   = (char*)calloc(DIMSREPORTSIZE, sizeof(char));
  this->performanceBuf       = NULL;
  this->performancePtr       = NULL;
  this->numPerformancePoints = 0;

  this->hostname = (char*)calloc(MAXHOSTNAMELEN, sizeof(char));
  gethostname(this->hostname, MAXHOSTNAMELEN);
}

/** Calculate the total number of frames that the current configured dimensions can contain.
 * Sets the NDFileNumCapture parameter to the total value so file saving will complete at this number.
 * This is called only from writeInt32 so the lock is already taken.
 */
void NDFileHDF5::calcNumFrames()
{
  epicsInt32 virtDimX, virtDimY, numExtraDims, nframes, maxFramesInDims;


  getIntegerParam(NDFileHDF5_extraDimSizeN, &nframes);
  getIntegerParam(NDFileHDF5_extraDimSizeX, &virtDimX);
  getIntegerParam(NDFileHDF5_extraDimSizeY, &virtDimY);
  getIntegerParam(NDFileHDF5_nExtraDims,    &numExtraDims);

  // work out how many frames to capture in total
  maxFramesInDims = 1;
  if (numExtraDims >= 0) maxFramesInDims *= nframes;
  if (numExtraDims >= 1) maxFramesInDims *= virtDimX;
  if (numExtraDims >= 2) maxFramesInDims *= virtDimY;
  setIntegerParam(NDFileNumCapture, maxFramesInDims);
}

unsigned int NDFileHDF5::calcIstorek()
{
  unsigned int retval = 0;
  unsigned int num_chunks = 1; // Number of chunks that fit in the full dataset
  double div_result = 0.0;
  int extradimsizes[MAXEXTRADIMS] = {0,0,0};
  int numExtraDims = 0;
  hsize_t maxdim = 0;
  int extradim = 0;
  int fileNumCapture=0;
  
  this->lock();
  getIntegerParam(NDFileHDF5_nExtraDims,    &numExtraDims);
  getIntegerParam(NDFileHDF5_extraDimSizeN, &extradimsizes[2]);
  getIntegerParam(NDFileHDF5_extraDimSizeX, &extradimsizes[1]);
  getIntegerParam(NDFileHDF5_extraDimSizeY, &extradimsizes[0]);
  getIntegerParam(NDFileNumCapture, &fileNumCapture);
  this->unlock();
  extradim = MAXEXTRADIMS - numExtraDims-1;
  if (numExtraDims == 0) {
    if (fileNumCapture == 0) {
      extradimsizes[2] = INFINITE_FRAMES_CAPTURE;
    } else {
      extradimsizes[2] = fileNumCapture;
    }
  }
  for (int i = 0; i<this->rank; i++){
    maxdim = this->maxdims[i];
    if (maxdim == H5S_UNLIMITED){
      maxdim = extradimsizes[extradim];
      extradim++;
    }
    div_result = (double)maxdim / (double)this->chunkdims[i];
    div_result = ceil(div_result);
    num_chunks *= (unsigned int)div_result;
  }
  retval = num_chunks/2;
  return retval;
}

hsize_t NDFileHDF5::calcChunkCacheBytes()
{
  hsize_t nbytes = 0;
  epicsInt32 n_frames_chunk=0;
  this->lock();
  getIntegerParam(NDFileHDF5_nFramesChunks, &n_frames_chunk);
  this->unlock();
  nbytes = this->maxdims[this->rank - 1] * this->maxdims[this->rank - 2] * this->bytesPerElement * n_frames_chunk;
  return nbytes;
}

/** find out whether or not the input is a prime number.
 * Returns true if number is a prime. False if not.
 */
bool is_prime(unsigned int long number)
{
  //0 and 1 are prime numbers
  if(number == 0 || number == 1) return true;

  //find divisor that divides without a remainder
  int divisor;
  for(divisor = (number/2); (number%divisor) != 0; --divisor){;}

  //if no divisor greater than 1 is found, it is a prime number
  return divisor == 1;
}

hsize_t NDFileHDF5::calcChunkCacheSlots()
{
  unsigned int long nslots = 1;
  unsigned int long num_chunks = 1;
  double div_result = 0.0;
  epicsInt32 n_frames_chunk=0, n_extra_dims=0, n_frames_capture=0;
  
  this->lock();
  getIntegerParam(NDFileHDF5_nFramesChunks, &n_frames_chunk);
  getIntegerParam(NDFileHDF5_nExtraDims, &n_extra_dims);
  getIntegerParam(NDFileNumCapture, &n_frames_capture);
  this->unlock();

  div_result = (double)this->maxdims[this->rank - 1] / (double)this->chunkdims[this->rank -1];
  num_chunks *= (unsigned int long)ceil(div_result);
  div_result = (double)this->maxdims[this->rank - 2] / (double)this->chunkdims[this->rank -2];
  num_chunks *= (unsigned int long)ceil(div_result);
  div_result = (double)n_frames_capture / (double)n_frames_chunk;
  num_chunks *= (unsigned int long)ceil(div_result);
  for(int i=0; i < n_extra_dims; i++){
    num_chunks *= (unsigned int long)this->virtualdims[i];
  }

  // number of slots have to be a prime number which is between 10 and 50 times
  // larger than the numer of chunks that can fit in the file/dataset.
  nslots = num_chunks * 50;
  while( !is_prime( nslots) )
    nslots++;
  return nslots;
}

/** Setup the required allocation for the performance dataset
 */
asynStatus NDFileHDF5::configurePerformanceDataset()
{
  int numCaptureFrames;
  this->lock();
  getIntegerParam(NDFileNumCapture, &numCaptureFrames);
  this->unlock();
  if (numCaptureFrames == 0) {
    // Special case: acquiring an infinite number of frames
    numCaptureFrames = 1000000;
  }

  // only allocate new memory if we need more points than we've used before
  if (numCaptureFrames > this->numPerformancePoints)
  {
    this->numPerformancePoints = numCaptureFrames;
    if (this->performanceBuf != NULL) {free(this->performanceBuf); this->performanceBuf = NULL;}
    if (this->performanceBuf == NULL)
      this->performanceBuf = (epicsFloat64*)  calloc(5 * this->numPerformancePoints, sizeof(double));
  }
  this->performancePtr  = this->performanceBuf;

  return asynSuccess;
}

/** Write out the performance dataset
 */
asynStatus NDFileHDF5::writePerformanceDataset()
{
  hsize_t dims[2];
  hid_t dataspace_id, dataset_id, group_performance;
  epicsInt32 numCaptured;

  hdf5::Root *root = this->layout.get_hdftree();
  if (root){
    hdf5::Group* perf_group = root->find_ndattr_default_group(); // Generally use the default ndattribute dataset for 'timestamp'
    hdf5::Dataset *tsDset = NULL;

    // Look if a "performance/timestamp" dataset can be found.
    // If so, use that dataset to store performance timestamps.
    // This is a slight HACK to retain backwards compatibility
    // with the location of the performance/timestamp dataset. (Ulrik June 2014)
    root->find_dset("timestamp", &tsDset);
    if ( tsDset != NULL) {
      hdf5::Group* grp = dynamic_cast<hdf5::Group*>(tsDset->get_parent());
      // Check that the group name is performance AND that
      // the dataset is not already reserved for an NDAttribute
      if (grp->get_name() == "performance" &&
          !tsDset->data_source().is_src_ndattribute() ) {
          perf_group = grp;
      }
    }

    this->lock();
    getIntegerParam(NDFileNumCaptured, &numCaptured);
    this->unlock();
    dims[1] = 5;
    if (numCaptured < this->numPerformancePoints) dims[0] = numCaptured;
    else dims[0] = this->numPerformancePoints;

    if(perf_group == NULL)
    {
      // Check if the auto_ndattr_default tag is true, if it is then the root group is the file
      if (this->layout.getAutoNDAttrDefault()){
        group_performance = this->file;
      } else {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::writePerformanceDataset No default attribute group defined.\n",
                  driverName);
        return asynError;
      }
    } else {
      group_performance = H5Gopen(this->file, perf_group->get_full_name().c_str(), H5P_DEFAULT);
    }

    /* Create the "timestamp" dataset */
    dataspace_id = H5Screate_simple(2, dims, NULL);
    dataset_id = H5Dcreate2(group_performance, "timestamp", H5T_NATIVE_DOUBLE, dataspace_id,
                            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (!H5Iis_valid(dataset_id)) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING, "NDFileHDF5::writePerformanceDataset: unable to create \'timestamp\' dataset.");
        H5Sclose(dataspace_id);
        if(perf_group != NULL){
          H5Gclose(group_performance);
        }
        return asynError;
    }
    /* Write the second dataset. */
    H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE,
             H5S_ALL, H5S_ALL,
             H5P_DEFAULT, this->performanceBuf);

    /* Close the data space for the second dataset. */
    H5Sclose(dataspace_id);

    /* Close the second dataset */
    H5Dclose(dataset_id);

    /* Close the group. */
    if(perf_group != NULL){
      H5Gclose(group_performance);
    }
  } else {
    return asynError;
  }
  return asynSuccess;
}

/** Create the group of datasets to hold the NDArray attributes
 *
 */
asynStatus NDFileHDF5::createAttributeDataset()
{
  HDFAttributeNode *hdfAttrNode;
  NDAttribute *ndAttr = NULL;
  NDAttrSource_t ndAttrSourceType;
  int extraDims;
  int chunking = 0;
  int fileWriteMode = 0;
  hsize_t maxdims[2] = {H5S_UNLIMITED, H5S_UNLIMITED};
  hid_t groupDefault = -1;
  const char *attrNames[5] = {"NDAttrName", "NDAttrDescription", "NDAttrSourceType", "NDAttrSource", NULL};
  const char *attrStrings[5] = {NULL,NULL,NULL,NULL,NULL};
  int i;
  size_t size;
  static const char *functionName = "createAttributeDataset";

  this->lock();
  getIntegerParam(NDFileHDF5_nExtraDims, &extraDims);
  // Check the chunking value
  getIntegerParam(NDFileHDF5_NDAttributeChunk, &chunking);
  // If the chunking is zero then use the number of frames
  if (chunking == 0){
    // In this case we want to read back the number of frames and use this for chunking
    // First check in case we are in single mode.
    getIntegerParam(NDFileWriteMode, &fileWriteMode);
    // Check that we are not in single mode
    if (fileWriteMode == NDFileModeSingle){
      // We are in single mode so chunk size should be set to 1 no matter the frame count
      chunking = 1;
    } else {
      // We aren't in single mode so read the number of frames
      getIntegerParam(NDFileNumCapture, &chunking);
      if (chunking <= 0) {
        // Special case: writing infinite number of frames, so we guess a good(ish) chunk number
        chunking = 16*1024;
      }
    }
  }
  this->unlock();

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Creating attribute datasets. extradims=%d attribute count=%d\n",
            driverName, functionName, extraDims, this->pFileAttributes->count());

  hdf5::Root *root = this->layout.get_hdftree();
  hdf5::Group* def_group = root->find_ndattr_default_group();
  //check for NULL
  if(def_group != NULL) {
    groupDefault = H5Gopen(this->file, def_group->get_full_name().c_str(), H5P_DEFAULT);
    if (strlen(this->hostname) > 0) {
      this->writeStringAttribute(groupDefault, "hostname", this->hostname);
    }
  }
  else {
    // Check if the auto_ndattr_default tag is true, if it is then the root group is the file
    if (this->layout.getAutoNDAttrDefault()){
      groupDefault = this->file;
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING, "%s::%s No default attribute group defined.\n",
                driverName, functionName);
    }
  }

  ndAttr = this->pFileAttributes->next(ndAttr); // get the first NDAttribute
  while(ndAttr != NULL)
  {
    attrStrings[0] = ndAttr->getName();
    attrStrings[1] = ndAttr->getDescription();
    attrStrings[2] = ndAttr->getSourceInfo(&ndAttrSourceType);
    attrStrings[3] = ndAttr->getSource();
    // allocate another name-nodes
    hdfAttrNode = (HDFAttributeNode*)calloc(1, sizeof(HDFAttributeNode));
    hdfAttrNode->attrName = epicsStrDup(ndAttr->getName()); // copy the attribute name

    hdfAttrNode->offset[0]     = 0;
    hdfAttrNode->offset[1]     = 0;
    hdfAttrNode->hdfcparm   = H5Pcreate(H5P_DATASET_CREATE);
    //set the default save frequence to be every frame
    hdfAttrNode->whenToSave = hdf5::OnFrame;

    // Creating extendible data sets
    hdfAttrNode->hdfdims[0] = 1;
    if (ndAttr->getDataType() < NDAttrString){
      hdfAttrNode->hdfdatatype  = this->typeNd2Hdf((NDDataType_t)ndAttr->getDataType());
      hdfAttrNode->chunk[0]   = chunking;
      hdfAttrNode->hdfrank    = 1;
    } else {
      // String dataset required, use type N5T_NATIVE_CHAR
      hdfAttrNode->hdfdatatype = H5T_NATIVE_CHAR;
      hdfAttrNode->hdfdims[1] = MAX_ATTRIBUTE_STRING_SIZE;
      hdfAttrNode->chunk[0]   = chunking;
      hdfAttrNode->chunk[1]   = MAX_ATTRIBUTE_STRING_SIZE;
      hdfAttrNode->hdfrank    = 2;
    }
    H5Pset_fill_value (hdfAttrNode->hdfcparm, hdfAttrNode->hdfdatatype, this->ptrFillValue );

    H5Pset_chunk(hdfAttrNode->hdfcparm, hdfAttrNode->hdfrank, hdfAttrNode->chunk);

    hdf5::Dataset *dset = NULL;
    // Search for the dataset of the NDAttribute.  If it exists then we use it
    if (root->find_dset_ndattr(hdfAttrNode->attrName, &dset) == 0){
      // In here we need to open the dataset for writing

      hdf5::DataSource dsource = dset->data_source();
      hdfAttrNode->whenToSave = dsource.get_when_to_save();
      if(hdfAttrNode->whenToSave != hdf5::OnFrame) {
          //set dim size to 1 for OnFileOpen and OnFileClose
          hdfAttrNode->hdfdims[0] = 1;
      }

      hdfAttrNode->hdfdataspace = H5Screate_simple(hdfAttrNode->hdfrank, hdfAttrNode->hdfdims, maxdims);
      // Get the group from the dataset
      hid_t dsetgroup = H5Gopen(this->file, dset->get_parent()->get_full_name().c_str(), H5P_DEFAULT);

      // Now create the dataset
      hdfAttrNode->hdfdataset   = H5Dcreate2(dsetgroup, dset->get_name().c_str(),
                                             hdfAttrNode->hdfdatatype, hdfAttrNode->hdfdataspace,
                                             H5P_DEFAULT, hdfAttrNode->hdfcparm, H5P_DEFAULT);

      //save xml tags attributes
      writeHdfAttributes( hdfAttrNode->hdfdataset, dset);
      H5Gclose(dsetgroup);

      // If the dataset exists within the XML layout then the values will be cached and we should not 
      // add the attribute to this list

      // create a memory space of exactly one element dimension to use for writing slabs
      if (ndAttr->getDataType() < NDAttrString){
        hdfAttrNode->elementSize[0]  = 1;
        hdfAttrNode->elementSize[1]  = 1;
      } else {
        hdfAttrNode->elementSize[0]  = 1;
        hdfAttrNode->elementSize[1]  = MAX_ATTRIBUTE_STRING_SIZE;
      }
      hdfAttrNode->hdfmemspace  = H5Screate_simple(hdfAttrNode->hdfrank, hdfAttrNode->elementSize, NULL);

      // Write some description of the NDAttribute as a HDF attribute to the dataset
      for (i=0; attrNames[i] != NULL; i++)
      {
        size = strlen(attrStrings[i]);
        if (size <= 0) continue;
        this->writeStringAttribute(hdfAttrNode->hdfdataset, attrNames[i], attrStrings[i]);
      }

      // Add the attribute to the list
      attrList.push_back(hdfAttrNode);

    } else {
      if(groupDefault > -1) {
        hdfAttrNode->hdfdataspace = H5Screate_simple(hdfAttrNode->hdfrank, hdfAttrNode->hdfdims, maxdims);
        // In here we need to create the dataset
        hdfAttrNode->hdfdataset   = H5Dcreate2(groupDefault, hdfAttrNode->attrName,
                                               hdfAttrNode->hdfdatatype, hdfAttrNode->hdfdataspace,
                                               H5P_DEFAULT, hdfAttrNode->hdfcparm, H5P_DEFAULT);
  
  
        // create a memory space of exactly one element dimension to use for writing slabs
        if (ndAttr->getDataType() < NDAttrString){
          hdfAttrNode->elementSize[0]  = 1;
          hdfAttrNode->elementSize[1]  = 1;
        } else {
          hdfAttrNode->elementSize[0]  = 1;
          hdfAttrNode->elementSize[1]  = MAX_ATTRIBUTE_STRING_SIZE;
        }
        hdfAttrNode->hdfmemspace  = H5Screate_simple(hdfAttrNode->hdfrank, hdfAttrNode->elementSize, NULL);
  
        // Write some description of the NDAttribute as a HDF attribute to the dataset
        for (i=0; attrNames[i] != NULL; i++)
        {
          size = strlen(attrStrings[i]);
          if (size <= 0) continue;
          this->writeStringAttribute(hdfAttrNode->hdfdataset, attrNames[i], attrStrings[i]);
        }
        // Add the attribute to the list
        attrList.push_back(hdfAttrNode);
      }
    }
    ndAttr = this->pFileAttributes->next(ndAttr);
  }
  if(def_group != NULL){
    H5Gclose(groupDefault);
  }

  return asynSuccess;
}

/** Write the NDArray attributes to the file
 *
 */
asynStatus NDFileHDF5::writeAttributeDataset(hdf5::When_t whenToSave)
{
  asynStatus status = asynSuccess;
  HDFAttributeNode *hdfAttrNode = NULL;
  NDAttribute *ndAttr = NULL;
  //hsize_t elementSize = 1;
  char * stackbuf[MAX_ATTRIBUTE_STRING_SIZE];
  void* pDatavalue = stackbuf;
  int ret;
  static const char *functionName = "writeAttributeDataset";

  for (std::list<HDFAttributeNode *>::iterator it_node = attrList.begin(); it_node != attrList.end(); ++it_node){
    hdfAttrNode = *it_node;
    // find the named attribute in the NDAttributeList
    ndAttr = this->pFileAttributes->find(hdfAttrNode->attrName);
    if (ndAttr == NULL)
    {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING,
        "%s::%s WARNING: NDAttribute named \'%s\' not found\n",
        driverName, functionName, hdfAttrNode->attrName);
      continue;
    }
    //check if the attribute is meant to be saved at this time
    if (hdfAttrNode->whenToSave != whenToSave) {
      //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
      //          "%s::%s NDAttribute named \'%s\' when to save miss match\n",
      //          driverName, functionName, hdfAttrNode->attrName);
      continue; // Not saving at this time, and that is OK...
    }

    // find the data based on datatype
    ret = ndAttr->getValue(ndAttr->getDataType(), pDatavalue, MAX_ATTRIBUTE_STRING_SIZE);
    if (ret == ND_ERROR) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s::%s: ERROR did not get data from NDAttribute \'%s\'\n",
        driverName, functionName, ndAttr->getName());
      memset(pDatavalue, 0, MAX_ATTRIBUTE_STRING_SIZE);
    }
    // Work with HDF5 library to select a suitable hyperslab (one element) and write the new data to it
    H5Dset_extent(hdfAttrNode->hdfdataset, hdfAttrNode->hdfdims);
    hdfAttrNode->hdffilespace = H5Dget_space(hdfAttrNode->hdfdataset);
    H5Sselect_hyperslab(hdfAttrNode->hdffilespace, H5S_SELECT_SET,
                                    hdfAttrNode->offset, NULL,
                                    hdfAttrNode->elementSize, NULL);

    // Write the data to the hyperslab.
    H5Dwrite(hdfAttrNode->hdfdataset, hdfAttrNode->hdfdatatype,
                         hdfAttrNode->hdfmemspace, hdfAttrNode->hdffilespace,
                         H5P_DEFAULT, pDatavalue);

    H5Sclose(hdfAttrNode->hdffilespace);
    hdfAttrNode->hdfdims[0]++;
    hdfAttrNode->offset[0]++;
  }
  return status;
}

/** Close all attribute datasets and clear out memory
 */
asynStatus NDFileHDF5::closeAttributeDataset()
{
  asynStatus status = asynSuccess;
  HDFAttributeNode *hdfAttrNode;
  static const char *functionName = "closeAttributeDataset";

  while (attrList.size() > 0){
    hdfAttrNode = attrList.front();
    attrList.pop_front();
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s: closing attribute dataset \'%s\'\n",
              driverName, functionName, hdfAttrNode->attrName);
    H5Dclose(hdfAttrNode->hdfdataset);
    H5Sclose(hdfAttrNode->hdfmemspace);
    H5Sclose(hdfAttrNode->hdfdataspace);
    H5Pclose(hdfAttrNode->hdfcparm);
    free(hdfAttrNode->attrName);
    free(hdfAttrNode);
  }
  return status;
}

/** Convenience function to write a null terminated string as an HDF5 attribute
 *  to an HDF5 element (group or dataset)
 */
asynStatus NDFileHDF5::writeStringAttribute(hid_t element, const char * attrName, const char* attrStrValue)
{
  asynStatus status = asynSuccess;
  hid_t hdfdatatype;
  hid_t hdfattr;
  hid_t hdfattrdataspace;

  if (strlen(attrStrValue) > 0){
    hdfattrdataspace = H5Screate(H5S_SCALAR);
    hdfdatatype      = H5Tcopy(H5T_C_S1);
    H5Tset_size(hdfdatatype, strlen(attrStrValue));
    H5Tset_strpad(hdfdatatype, H5T_STR_NULLTERM);
    hdfattr          = H5Acreate2(element, attrName,
                                  hdfdatatype, hdfattrdataspace,
                                  H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(hdfattr, hdfdatatype, attrStrValue);
    H5Aclose(hdfattr);
    H5Sclose(hdfattrdataspace);
  }
  return status;
}


/** Configure the required dimensions for a dataset.
 *
 *  This method will call the configureDims method for each detector dataset
 *  created.
 */
asynStatus NDFileHDF5::configureDatasetDims(NDArray *pArray)
{
  int i = 0;
  int extradims = 0;
  int *numCapture=NULL;
  asynStatus status = asynSuccess;

  this->lock();
  if (this->multiFrameFile){
    struct extradimdefs_t {
      int sizeParamId;
      char* dimName;
    } extradimdefs[MAXEXTRADIMS] = {
        {NDFileHDF5_extraDimSizeY, this->extraDimNameY},
        {NDFileHDF5_extraDimSizeX, this->extraDimNameX},
        {NDFileHDF5_extraDimSizeN, this->extraDimNameN},
    };
    getIntegerParam(NDFileHDF5_nExtraDims, &extradims);
    extradims += 1;
    numCapture = (int *)calloc(extradims, sizeof(int));
    for (i=0; i<extradims; i++){
      getIntegerParam(extradimdefs[MAXEXTRADIMS - extradims + i].sizeParamId, &numCapture[i]);
    }
  } else {
    numCapture = (int *)calloc(1, sizeof(int));
  }
  int user_chunking[3] = {1,1,1};
  getIntegerParam(NDFileHDF5_nFramesChunks, &user_chunking[2]);
  getIntegerParam(NDFileHDF5_nRowChunks,    &user_chunking[1]);
  getIntegerParam(NDFileHDF5_nColChunks,    &user_chunking[0]);
  this->unlock();

  // Iterate over the stored detector data sets and configure the dimensions
  std::map<std::string, NDFileHDF5Dataset *>::iterator it_dset;
  for (it_dset = this->detDataMap.begin(); it_dset != this->detDataMap.end(); ++it_dset){
    it_dset->second->configureDims(pArray, this->multiFrameFile, extradims, numCapture, user_chunking);
  }
  
  if (numCapture != NULL) free( numCapture );
  return status;
}

/** Configure the HDF5 dimension definitions based on NDArray dimensions.
 * Initially this implementation just copies the dimension sizes from the NDArray
 * in the same order. A last infinite length dimension is added if multiple frames
 * are to be stored in the same file (in Capture or Stream mode).
 */
asynStatus NDFileHDF5::configureDims(NDArray *pArray)
{
  int i=0,j=0, extradims = 0, ndims=0;
  int numCapture;
  asynStatus status = asynSuccess;
  char strdims[DIMSREPORTSIZE];
  static const char *functionName = "configureDims";
  //strdims = (char*)calloc(DIMSREPORTSIZE, sizeof(char));

  this->lock();
  if (this->multiFrameFile)
  {
    getIntegerParam(NDFileHDF5_nExtraDims, &extradims);
    extradims += 1;
  }
  ndims = pArray->ndims + extradims;

  // first check whether the dimension arrays have been allocated
  // or the number of dimensions have changed.
  // If necessary free and reallocate new memory.
  if (this->maxdims == NULL || this->rank != ndims)
  {
    if (this->maxdims     != NULL) free(this->maxdims);
    if (this->chunkdims   != NULL) free(this->chunkdims);
    if (this->framesize   != NULL) free(this->framesize);
    if (this->dims        != NULL) free(this->dims);
    if (this->offset      != NULL) free(this->offset);
    if (this->virtualdims != NULL) free(this->virtualdims);

    //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
    //          "%s::%s allocating dimension arrays\n",
    //          driverName, functionName);
    this->maxdims       = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->chunkdims     = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->framesize     = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->dims          = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->offset        = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    int nvirtual = extradims;
    if (nvirtual <= 0) nvirtual = 1;
    this->virtualdims   = (hsize_t*)calloc(nvirtual, sizeof(hsize_t));
  }

  if (this->multiFrameFile)
  {
    /* Configure the virtual dimensions -i.e. dimensions in addition
     * to the frame format.
     * Normally set to just 1 by default or -1 unlimited (in HDF5 terms)
     */
    //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    //  "%s::%s Creating extradimdefs_t structure. Size=%d\n",
    //  driverName, functionName, MAXEXTRADIMS);
    struct extradimdefs_t {
      int sizeParamId;
      char* dimName;
    } extradimdefs[MAXEXTRADIMS] = {
        {NDFileHDF5_extraDimSizeY, this->extraDimNameY},
        {NDFileHDF5_extraDimSizeX, this->extraDimNameX},
        {NDFileHDF5_extraDimSizeN, this->extraDimNameN},
    };

    //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    //  "%s::%s initialising the extradim sizes. extradims=%d\n",
    //  driverName, functionName, extradims);
    for (i=0; i<extradims; i++)
    {
      this->framesize[i] = (hsize_t)1;
      this->chunkdims[i]   = 1;
      this->maxdims[i]     = H5S_UNLIMITED;
      this->dims[i]        = 1;
      this->offset[i]      = 0; // because we increment offset *before* each write we need to start at -1

      getIntegerParam(extradimdefs[MAXEXTRADIMS - extradims + i].sizeParamId, &numCapture);
      this->virtualdims[i] = numCapture;
      //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
      //  "%s::%s extradim=%d ncapture=%d\n",
      //  driverName, functionName, i, numCapture);
      epicsSnprintf(this->ptrDimensionNames[i], DIMNAMESIZE, "%s", extradimdefs[MAXEXTRADIMS - extradims +i].dimName);
    }
  }

  this->rank = ndims;
  //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
  //  "%s::%s initialising the basic frame dimension sizes. rank=%d\n",
  //  driverName, functionName, this->rank);
  for (j=pArray->ndims-1,i=extradims; i<this->rank; i++,j--)
  {
  this->framesize[i] = (hsize_t)(pArray->dims[j].size);
    this->chunkdims[i]  = pArray->dims[j].size;
    this->maxdims[i]    = pArray->dims[j].size;
    this->dims[i]       = pArray->dims[j].size;
    this->offset[i]     = 0;
    epicsSnprintf(this->ptrDimensionNames[i],DIMNAMESIZE, "NDArray Dim%d", j);
  }

  // Collect the user defined chunking dimensions and check if they're valid
  //
  // A check is made to see if the user has input 0 or negative value (which is invalid)
  // in which case the size of the chunking is set to the maximum size of that dimension (full frame)
  // If the maximum of a particular dimension is set to a negative value -which is the case for
  // infinite lenght dimensions (-1); the chunking value is set to 1.
  int user_chunking[3] = {1,1,1};
  getIntegerParam(NDFileHDF5_nFramesChunks, &user_chunking[2]);
  getIntegerParam(NDFileHDF5_nRowChunks,    &user_chunking[1]);
  getIntegerParam(NDFileHDF5_nColChunks,    &user_chunking[0]);
  int max_items = 0;
  int hdfdim = 0;
  int fileWriteMode = 0;
  // Work out the number of chunking dims we are going to work with (number of array dims)
  int numDimsForChunking = pArray->ndims;
  getIntegerParam(NDFileWriteMode, &fileWriteMode);    
  // Check that we are not in single mode
  if (fileWriteMode != NDFileModeSingle){
    // There is another dimension (frame number)
    numDimsForChunking++;
  }
  // Loop over the number of user_chunking array elements
  for (i = 0; i<numDimsForChunking; i++)
  {
      hdfdim = ndims - i - 1;
      max_items = (int)this->maxdims[hdfdim];
      if (max_items <= 0)
      {
        max_items = 1; // For infinite length dimensions
      } else {
        if (user_chunking[i] > max_items) user_chunking[i] = max_items;
      }
      if (user_chunking[i] < 1) user_chunking[i] = max_items;
      assert(hdfdim >= 0); this->chunkdims[hdfdim] = user_chunking[i];
  }
  setIntegerParam(NDFileHDF5_nFramesChunks, user_chunking[2]);
  setIntegerParam(NDFileHDF5_nRowChunks,    user_chunking[1]);
  setIntegerParam(NDFileHDF5_nColChunks,    user_chunking[0]);
  this->unlock();

  for(i=0; i<pArray->ndims; i++) sprintf(strdims+(i*6), "%5d,", (int)pArray->dims[i].size);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s  NDArray:   { %s }\n", 
    driverName, functionName, strdims);
  //free(strdims);
  char *strdimsrep = this->getDimsReport();
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s dimension report: %s\n", 
    driverName, functionName, strdimsrep);

  return status;
}

/** Configure compression
 */
asynStatus NDFileHDF5::configureCompression()
{
  asynStatus status = asynSuccess;
  int compressionScheme;
  int szipNumPixels = 0;
  int nbitPrecision = 0;
  int nbitOffset = 0;
  int zLevel = 0;
  static const char * functionName = "configureCompression";

  this->lock();
  getIntegerParam(NDFileHDF5_compressionType, &compressionScheme);
  getIntegerParam(NDFileHDF5_nbitsOffset, &nbitOffset);
  getIntegerParam(NDFileHDF5_nbitsPrecision, &nbitPrecision);
  getIntegerParam(NDFileHDF5_szipNumPixels, &szipNumPixels);
  getIntegerParam(NDFileHDF5_zCompressLevel, &zLevel);
  this->unlock();
  switch (compressionScheme)
  {
    case HDF5CompressNone:
      break;
    case HDF5CompressNumBits:
      this->lock();
      this->unlock();
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                "%s::%s Setting N-bit filter precision=%d bit offset=%d bit\n",
                driverName, functionName, nbitPrecision, nbitOffset);
      H5Tset_precision (this->datatype, nbitPrecision);
      H5Tset_offset (this->datatype, nbitOffset);
      H5Pset_nbit (this->cparms);

      // Finally read back the parameters we've just sent to HDF5
      nbitOffset = H5Tget_offset(this->datatype);
      nbitPrecision = (int)H5Tget_precision(this->datatype);
      this->lock();
      setIntegerParam(NDFileHDF5_nbitsOffset, nbitOffset);
      setIntegerParam(NDFileHDF5_nbitsPrecision, nbitPrecision);
      this->unlock();
      break;
    case HDF5CompressSZip:
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                "%s::%s Setting szip compression filter # pixels=%d\n",
                driverName, functionName, szipNumPixels);
      H5Pset_szip (this->cparms, H5_SZIP_NN_OPTION_MASK, szipNumPixels);
      break;
    case HDF5CompressZlib:
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
                "%s::%s Setting zlib compression filter level=%d\n",
                driverName, functionName, zLevel);
      H5Pset_deflate(this->cparms, zLevel);
      break;
  }
  return status;
}

/** Translate the NDArray datatype to HDF5 datatypes 
 */
hid_t NDFileHDF5::typeNd2Hdf(NDDataType_t datatype)
{
  hid_t result;
  int fillvalue = 0;

  switch (datatype) 
  {
    case NDInt8:
      result = H5T_NATIVE_INT8;
      *(epicsInt8*)this->ptrFillValue = (epicsInt8)fillvalue;
      break;
    case NDUInt8:
      result = H5T_NATIVE_UINT8;
      *(epicsUInt8*)this->ptrFillValue = (epicsUInt8)fillvalue;
      break;
    case NDInt16:
      result = H5T_NATIVE_INT16;
      *(epicsInt16*)this->ptrFillValue = (epicsInt16)fillvalue;
      break;
    case NDUInt16:
      result = H5T_NATIVE_UINT16;
      *(epicsUInt16*)this->ptrFillValue = (epicsUInt16)fillvalue;
      break;
    case NDInt32:
      result = H5T_NATIVE_INT32;
      *(epicsInt32*)this->ptrFillValue = (epicsInt32)fillvalue;
      break;
    case NDUInt32:
      result = H5T_NATIVE_UINT32;
      *(epicsUInt32*)this->ptrFillValue = (epicsUInt32)fillvalue;
      break;
    case NDFloat32:
      result = H5T_NATIVE_FLOAT;
      *(epicsFloat32*)this->ptrFillValue = (epicsFloat32)fillvalue;
      break;
    case NDFloat64:
      result = H5T_NATIVE_DOUBLE;
      *(epicsFloat64*)this->ptrFillValue = (epicsFloat64)fillvalue;
      break;
    default:
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s cannot convert NDArrayType: %d to HDF5 datatype\n",
                driverName, "typeNd2Hdf", datatype);
      result = -1;
  }
  return result;
}

/** Iterate through all the local stored dimension information.
 * Returns a string where each line is an ASCII report of the value for each dimension.
 * Intended for development and debugging. This function is not thread safe.
 */
char* NDFileHDF5::getDimsReport()
{
  int i=0,j=0,extradims=0;
  size_t c=0;
  int maxlen = DIMSREPORTSIZE;
  char *strdims = this->dimsreport;

  this->lock();
  getIntegerParam(NDFileHDF5_nExtraDims, &extradims);
  this->unlock();
  extradims += 1;

  struct dimsizes_t {
    const char* name;
    unsigned long long* dimsize;
    int nd;
  } dimsizes[7] = {
      {"framesize", this->framesize, this->rank},
      {"chunkdims", this->chunkdims, this->rank},
      {"maxdims",   this->maxdims,   this->rank},
      {"dims",      this->dims,    this->rank},
      {"offset",    this->offset,    this->rank},
      {"virtual",   this->virtualdims, extradims},
      {NULL,        NULL,            0         },
  };

  strdims[0] = '\0';
  while (dimsizes[i].name!=NULL && c<DIMSREPORTSIZE && dimsizes[i].dimsize != NULL)
  {
    epicsSnprintf(strdims+c, maxlen-c, "\n%10s ", dimsizes[i].name);
    c = strlen(strdims);
    for(j=0; j<dimsizes[i].nd; j++)
    {
      epicsSnprintf(strdims+c, maxlen-c, "%6lli,", dimsizes[i].dimsize[j]);
      c = strlen(strdims);
    }
    //printf("name=%s c=%d rank=%d strdims: %s\n", dimsizes[i].name, c, dimsizes[i].nd, strdims);
    i++;
  }
  return strdims;
}

void NDFileHDF5::report(FILE *fp, int details)
{
  fprintf(fp, "Dimension report: %s\n", this->getDimsReport());
  // Call the base class report
  NDPluginFile::report(fp, details);
}

/** Configuration routine.  Called directly, or from the iocsh function in NDFileEpics */
extern "C" int NDFileHDF5Configure(const char *portName, int queueSize, int blockingCallbacks, 
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
  // Stack Size must be a minimum of 2MB
  if (stackSize < 2097152) stackSize = 2097152;
  new NDFileHDF5(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                 priority, stackSize);
  return(asynSuccess);
}

void NDFileHDF5::checkForOpenFile()
{
  const char * functionName = "checkForOpenFile";
  /* Checking if the plugin already has a file open.
   * It would be nice if NDPluginFile class would accept an asynError returned from
   * this method and not increment the filecounter and name... However, for now we just
   * close the file that is open and open a new one. */
  if (this->file != 0){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s::%s file is already open. Closing it and opening new one.\n", 
              driverName, functionName);
    this->closeFile();
  }
}

/** Add the default attributes from NDArrays into the local NDAttribute list.
 *
 * The relevant attributes are: uniqueId, timeStamp, epicsTS.secPastEpoch and
 * epicsTS.nsec.
 */
void NDFileHDF5::addDefaultAttributes(NDArray *pArray)
{
  this->pFileAttributes->add("NDArrayUniqueId",
                             "The unique ID of the NDArray",
                             NDAttrInt32, (void*)&(pArray->uniqueId));
  this->pFileAttributes->add("NDArrayTimeStamp",
                             "The timestamp of the NDArray as float64",
                             NDAttrFloat64, (void*)&(pArray->timeStamp));
  this->pFileAttributes->add("NDArrayEpicsTSSec",
                             "The NDArray EPICS timestamp seconds past epoch",
                             NDAttrUInt32, (void*)&(pArray->epicsTS.secPastEpoch));
  this->pFileAttributes->add("NDArrayEpicsTSnSec",
                             "The NDArray EPICS timestamp nanoseconds",
                             NDAttrUInt32, (void*)&(pArray->epicsTS.nsec));
}

/** Helper function to create a comma separated list of integers in a string
 *
 */
std::string comma_separated_list(int nelements, size_t *data)
{
  std::ostringstream num_str_convert;

  for (int i = 0; i < nelements; i++)
  {
    num_str_convert << data[i] << ",";
  }

  return num_str_convert.str();
}

/** Add the default attributes from NDArrays as HDF5 attributes on the detector datasets
 *
 */
asynStatus NDFileHDF5::writeDefaultDatasetAttributes(NDArray *pArray)
{
  asynStatus ret = asynSuccess;
  std::ostringstream num_str_convert;
  hdf5::DataSource const_src(hdf5::constant);
  const_src.set_when_to_save(hdf5::OnFileOpen);

  // First create some HDF5 attribute descriptions (constants) for each of the
  // NDArray data elements of interest
  std::vector<hdf5::Attribute> default_ndarray_attributes;

  hdf5::Attribute attr_numdims("NDArrayNumDims", const_src);
  attr_numdims.setOnFileOpen(true);
  num_str_convert << pArray->ndims;
  attr_numdims.source.set_const_datatype_value(hdf5::int32, num_str_convert.str());
  default_ndarray_attributes.push_back(attr_numdims);
  num_str_convert.str("");

  // Create the attributes which has one element for each dimension
  hdf5::Attribute attr_dim_offset("NDArrayDimOffset", const_src);
  for (int i = 0; i<pArray->ndims; i++) num_str_convert << pArray->dims[i].offset << ","; // Create comma separated string
  attr_dim_offset.source.set_const_datatype_value(hdf5::int32, num_str_convert.str());
  num_str_convert.str("");
  default_ndarray_attributes.push_back(attr_dim_offset);

  hdf5::Attribute attr_dim_binning("NDArrayDimBinning", const_src);
  for (int i = 0; i<pArray->ndims; i++) num_str_convert << pArray->dims[i].binning << ","; // Create comma separated string
  attr_dim_binning.source.set_const_datatype_value(hdf5::int32, num_str_convert.str());
  num_str_convert.str("");
  default_ndarray_attributes.push_back(attr_dim_binning);

  hdf5::Attribute attr_dim_reverse("NDArrayDimReverse", const_src);
  for (int i = 0; i<pArray->ndims; i++) num_str_convert << pArray->dims[i].reverse << ","; // Create comma separated string
  attr_dim_reverse.source.set_const_datatype_value(hdf5::int32, num_str_convert.str());
  num_str_convert.str("");
  default_ndarray_attributes.push_back(attr_dim_reverse);

  // Find a map of all detector datasets
  // (string name, Dataset object)
  hdf5::Group::MapDatasets_t det_dsets;
  this->layout.get_hdftree()->find_dsets(hdf5::detector, det_dsets);

  // Iterate over all detector datasets to attach the attributes
  hdf5::Group::MapDatasets_t::iterator it_det_dsets;
  std::vector<hdf5::Attribute>::iterator it_default_ndarray_attributes;
  for (it_det_dsets = det_dsets.begin(); it_det_dsets!=det_dsets.end(); ++it_det_dsets)
  {
    // Attach all the default attributes
    for (it_default_ndarray_attributes = default_ndarray_attributes.begin();
         it_default_ndarray_attributes != default_ndarray_attributes.end();
         ++it_default_ndarray_attributes)
    {
      it_det_dsets->second->add_attribute(*it_default_ndarray_attributes);
    }
  }

  return ret;
}

asynStatus NDFileHDF5::createNewFile(const char *fileName)
{
  herr_t hdfstatus;
  int tempAlign = 0;
  int tempThreshold = 0;
  static const char *functionName = "createNewFile";

  this->lock();
  getIntegerParam(NDFileHDF5_chunkBoundaryAlign, &tempAlign);
  getIntegerParam(NDFileHDF5_chunkBoundaryThreshold, (int*)&tempThreshold);
  this->unlock();

  /* File access property list: set the alignment boundary to a user defined block size
   * which ideally matches disk boundaries.
   * If user sets size to 0 we do not set alignment at all. */
  hid_t access_plist = H5Pcreate(H5P_FILE_ACCESS);
  hsize_t align = 0;
  if (tempAlign > 0){
    align = tempAlign;
  }
  hsize_t threshold = 0;
  if (tempThreshold > 0){
    threshold = tempThreshold;
  }
  if (align > 0){
    hdfstatus = H5Pset_alignment( access_plist, threshold, align );
    if (hdfstatus < 0){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
          "%s%s Warning: failed to set boundary threshod=%llu and alignment=%llu bytes\n",
          driverName, functionName, threshold, align);
      H5Pget_alignment( access_plist, &threshold, &align );
      this->lock();
      setIntegerParam(NDFileHDF5_chunkBoundaryAlign, (int)align);
      setIntegerParam(NDFileHDF5_chunkBoundaryThreshold, (int)threshold);
      this->unlock();
    }
  }

  /* File creation property list: set the i-storek according to HDF group recommendations */
  H5Pset_fclose_degree(access_plist, H5F_CLOSE_STRONG);
  hid_t create_plist = H5Pcreate(H5P_FILE_CREATE);
  unsigned int istorek = this->calcIstorek();
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s::%s Calculated istorek=%u\n",
            driverName, functionName, istorek);
  // Check if the calculated value of istorek is greater than the maximum allowed
  if (istorek > MAX_ISTOREK){
    // Cap the value at the maximum and notify of this
    istorek = MAX_ISTOREK;
    asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING,
              "%s::%s Istorek was greater than %u, using %u\n",
              driverName, functionName, istorek, istorek);
  }
  // Only set the istorek value if it is valid
  if (istorek <= 1){
    // Do not set this value as istorek, simply raise a warning
    asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING,
              "%s::%s Istorek is %u, using default\n",
              driverName, functionName, istorek);
  } else {
    // We should have a valid istorek value, submit it and check the result
    hdfstatus = H5Pset_istore_k(create_plist, istorek);
    if (hdfstatus < 0){
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s%s Warning: failed to set istorek parameter = %u\n",
                driverName, functionName, istorek);
    }
  }

  this->file = H5Fcreate(fileName, H5F_ACC_TRUNC, create_plist, access_plist);
  if (this->file <= 0){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
              "%s::%s Unable to create HDF5 file: %s\n", 
              driverName, functionName, fileName);
    this->file = 0;
    return asynError;
  }
  return asynSuccess;
}

/** Create the output file layout as specified by the XML layout.
 */
asynStatus NDFileHDF5::createFileLayout(NDArray *pArray)
{
  hid_t hdfdatatype;
  static const char *functionName = "createFileLayout";

  /*
   * Create the data space with appropriate dimensions
   */
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s Creating dataspace with given dimensions\n", 
    driverName, functionName);
  this->dataspace = H5Screate_simple(this->rank, this->framesize, this->maxdims);

  /*
   * Modify dataset creation properties, i.e. enable chunking.
   */
  this->cparms = H5Pcreate(H5P_DATASET_CREATE);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s Configuring chunking\n", 
    driverName, functionName);
  H5Pset_chunk(this->cparms, this->rank, this->chunkdims);

  /* Get the datatype */
  hdfdatatype = this->typeNd2Hdf(pArray->dataType);
  this->datatype = H5Tcopy(hdfdatatype);

  /* configure compression if required */
  this->configureCompression();

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s Setting fillvalue\n", 
    driverName, functionName);
  H5Pset_fill_value(this->cparms, this->datatype, this->ptrFillValue );
  

  //We use MAX_LAYOUT_LEN instead of MAX_FILENAME_LEN because we want to be able to load
  // in an xml string or a file containing the xml
  char *layoutFile = new char[MAX_LAYOUT_LEN];
  layoutFile[MAX_LAYOUT_LEN - 1] = '\0';
  this->lock();
  int status = getStringParam(NDFileHDF5_layoutFilename, MAX_LAYOUT_LEN - 1, layoutFile);
  this->unlock();
  if (status){
    delete [] layoutFile;
    return asynError;
  }
  // Test here for invalid filename or empty filename.
  // If empty use default layout
  // If invalid raise an error but still use the default layout
  if (!strcmp(layoutFile, "")){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Empty layout file, use default layout\n", driverName, functionName);
    status = this->layout.load_xml();
    if (status == -1){
      this->layout.unload_xml();
      delete [] layoutFile;
      return asynError;
    }
  } else {
    if (strstr(layoutFile, "<?xml") != NULL || this->fileExists(layoutFile)){
      // File specified and exists, use the file
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Layout file exists, using the file: %s\n",
                driverName, functionName, layoutFile);
    std::string strLayoutFile = std::string(layoutFile);
    status = this->layout.load_xml(strLayoutFile);
      if (status == -1){
        this->layout.unload_xml();
        delete[] layoutFile;
        return asynError;
      }
    } else {
      // The file does not exist, raise an error
      // Note that we should never get here as the file is verified prior to this
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s%s Warning: specified XML file does not exist\n",
                driverName, functionName);
      delete[] layoutFile;
      return asynError;
    }
  }
  delete [] layoutFile;

  // Append the default NDArray attributes to the detector datasets
  if (this->writeDefaultDatasetAttributes(pArray)) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_WARNING,
                "%s::%s WARNING Failed write default NDArray attributes to detector datasets\n",
                driverName, functionName);
      return asynError;
  }

  asynStatus ret = this->createXMLFileLayout();
  return ret;
}


/** EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size (min 2MB)",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"NDFileHDF5Configure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
  NDFileHDF5Configure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, 
                      args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileHDF5Register(void)
{
  iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileHDF5Register);
}

