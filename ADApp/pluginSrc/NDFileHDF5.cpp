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
#include <cmath>
#include <hdf5.h>
// #include <hdf5_hl.h> // high level HDF5 API not currently used (requires use of library hdf5_hl)

#include <epicsStdio.h>
#include <epicsString.h>
#include <epicsTime.h>
#include <iocsh.h>
#include <epicsExport.h>
#include "NDPluginFile.h"
//#include "winsock2.h"
#include "osiSock.h"

typedef struct HDFAttributeNode {
  ELLNODE node;
  char *attrName;
  hid_t hdfdataset;
  hid_t hdfdataspace;
  hid_t hdfmemspace;
  hid_t hdfdatatype;
  hid_t hdfcparm;
  hid_t hdffilespace;
  hsize_t hdfdims;
  hsize_t offset;
  hsize_t elementSize;
  int hdfrank;
} HDFAttributeNode;

enum HDF5Compression_t {HDF5CompressNone=0, HDF5CompressNumBits, HDF5CompressSZip, HDF5CompressZlib};

class NDFileHDF5 : public NDPluginFile {
public:
  NDFileHDF5(const char *portName, int queueSize, int blockingCallbacks, 
             const char *NDArrayPort, int NDArrayAddr,
             int priority, int stackSize);
       
  /* The methods that this class implements */
  virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
  virtual asynStatus readFile(NDArray **pArray);
  virtual asynStatus writeFile(NDArray *pArray);
  virtual asynStatus closeFile();
  virtual void report(FILE *fp, int details);
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
  /* plugin parameters */
  int NDFileHDF5_nRowChunks;
  #define FIRST_NDFILE_HDF5_PARAM NDFileHDF5_nRowChunks
  int NDFileHDF5_nColChunks;
  int NDFileHDF5_extraDimSizeN;
  int NDFileHDF5_nFramesChunks;
  int NDFileHDF5_chunkBoundaryAlign;
  int NDFileHDF5_chunkBoundaryThreshold;
  int NDFileHDF5_nExtraDims;
  int NDFileHDF5_extraDimNameN;
  int NDFileHDF5_extraDimSizeX;
  int NDFileHDF5_extraDimNameX;
  int NDFileHDF5_extraDimOffsetX;
  int NDFileHDF5_extraDimSizeY;
  int NDFileHDF5_extraDimNameY;
  int NDFileHDF5_extraDimOffsetY;
  int NDFileHDF5_storeAttributes;
  int NDFileHDF5_storePerformance;
  int NDFileHDF5_totalRuntime;
  int NDFileHDF5_totalIoSpeed;
  int NDFileHDF5_flushNthFrame;
  int NDFileHDF5_compressionType;
  int NDFileHDF5_nbitsPrecision;
  int NDFileHDF5_nbitsOffset;
  int NDFileHDF5_szipNumPixels;
  int NDFileHDF5_zCompressLevel;
  #define LAST_NDFILE_HDF5_PARAM NDFileHDF5_zCompressLevel

private:
  /* private helper functions */
  hid_t type_nd2hdf(NDDataType_t datatype);
  asynStatus configureDims(NDArray *pArray);
  asynStatus configureCompression();
  void extendDataSet();
  char* getDimsReport();
  asynStatus writeRawdataAttribute();
  asynStatus writeStringAttribute(hid_t element, const char* attrName, const char* attrStrValue);
  asynStatus createAttributeDataset();
  asynStatus writeAttributeDataset();
  asynStatus closeAttributeDataset();
  asynStatus configurePerformanceDataset();
  asynStatus writePerformanceDataset();
  void calcNumFrames();
  unsigned int calc_istorek();
  hsize_t calc_chunk_cache_bytes();
  hsize_t calc_chunk_cache_slots();

  int arrayDataId;
  int uniqueIdId;
  int timeStampId;
  int nextRecord;
  hid_t h5type;
  int *pAttributeId;
  NDAttributeList *pFileAttributes;
  epicsInt32 arrayDims[ND_ARRAY_MAX_DIMS];
  bool multiFrameFile;
  char *extraDimNameN;
  char *extraDimNameX;
  char *extraDimNameY;
  double *performanceBuf;
  double *performancePtr;
  epicsInt32 numPerformancePoints;
  epicsTimeStamp prevts;
  epicsTimeStamp opents;
  epicsTimeStamp firstFrame;
  double frameSize;  /** < frame size in megabits. For performance measurement. */
  int bytesPerElement;
  char *hostname;

  ELLLIST attrList;

  /* HDF5 handles and references */
  hid_t file;
  hid_t groupEntry;
  hid_t groupInstrument;
  hid_t groupDetector;
  hid_t groupNDAttributes;
  hid_t groupPerfomance;
  hid_t groupEpicsPvData;
  hid_t dataspace;
  hid_t dataset;
  hid_t datatype;
  hid_t cparms;
  hid_t memtypeid;
  void *ptrFillValue;

  /* dimension descriptors */
  int rank;               /** < number of dimensions */
  hsize_t *dims;          /** < Array of current dimension sizes. This updates as various dimensions grow. */
  hsize_t *maxdims;       /** < Array of maximum dimension sizes. The value -1 is HDF5 term for infinite. */
  hsize_t *chunkdims;     /** < Array of chunk size in each dimension. Only the dimensions that indicate the frame size (width, height) can really be tweaked. All other dimensions should be set to 1. */
  hsize_t *offset;        /** < Array of current offset in each dimension. The frame dimensions always have 0 offset but additional dimensions may grow as new frames are added. */
  hsize_t *framesize;     /** < The frame size in each dimension. Frame width, height and possibly depth (RGB) have real values -all other dimensions set to 1. */
  hsize_t *virtualdims;   /** < The desired sizes of the extra (virtual) dimensions: {Y, X, n} */
  char *ptrDimensionNames[ND_ARRAY_MAX_DIMS]; /** Array of strings with human readable names for each dimension */

  char *dimsreport;       /** < A string which contain a verbose report of all dimension sizes. The method getDimsReport fill in this */
};

#define NUM_NDFILE_HDF5_PARAMS ((int)(&LAST_NDFILE_HDF5_PARAM - &FIRST_NDFILE_HDF5_PARAM + 1))
#define DIMSREPORTSIZE 512
#define DIMNAMESIZE 40
#define MAXEXTRADIMS 3
#define ALIGNMENT_BOUNDARY 1048576

static const char *driverName = "NDFileHDF5";

#define str_NDFileHDF5_nRowChunks        "HDF5_nRowChunks"
#define str_NDFileHDF5_nColChunks        "HDF5_nColChunks"
#define str_NDFileHDF5_extraDimSizeN     "HDF5_extraDimSizeN"
#define str_NDFileHDF5_nFramesChunks     "HDF5_nFramesChunks"
#define str_NDFileHDF5_chunkBoundaryAlign "HDF5_chunkBoundaryAlign"
#define str_NDFileHDF5_chunkBoundaryThreshold "HDF5_chunkBoundaryThreshold"
#define str_NDFileHDF5_extraDimNameN     "HDF5_extraDimNameN"
#define str_NDFileHDF5_nExtraDims        "HDF5_nExtraDims"
#define str_NDFileHDF5_extraDimSizeX     "HDF5_extraDimSizeX"
#define str_NDFileHDF5_extraDimNameX     "HDF5_extraDimNameX"
#define str_NDFileHDF5_extraDimOffsetX   "HDF5_extraDimOffsetX"
#define str_NDFileHDF5_extraDimSizeY     "HDF5_extraDimSizeY"
#define str_NDFileHDF5_extraDimNameY     "HDF5_extraDimNameY"
#define str_NDFileHDF5_extraDimOffsetY   "HDF5_extraDimOffsetY"
#define str_NDFileHDF5_storeAttributes   "HDF5_storeAttributes"
#define str_NDFileHDF5_storePerformance  "HDF5_storePerformance"
#define str_NDFileHDF5_totalRuntime      "HDF5_totalRuntime"
#define str_NDFileHDF5_totalIoSpeed      "HDF5_totalIoSpeed"
#define str_NDFileHDF5_flushNthFrame     "HDF5_flushNthFrame"
#define str_NDFileHDF5_compressionType   "HDF5_compressionType"
#define str_NDFileHDF5_nbitsPrecision    "HDF5_nbitsPrecision"
#define str_NDFileHDF5_nbitsOffset       "HDF5_nbitsOffset"
#define str_NDFileHDF5_szipNumPixels     "HDF5_szipNumPixels"
#define str_NDFileHDF5_zCompressLevel    "HDF5_zCompressLevel"


/** Opens a HDF5 file.  
  * In write mode if NDFileModeMultiple is set then the first dataspace dimension is set to H5S_UNLIMITED to allow 
  * multiple arrays to be written to the same file.
  * NOTE: Does not currently support NDFileModeRead or NDFileModeAppend.
  * \param[in] fileName  Absolute path name of the file to open.
  * \param[in] openMode Bit mask with one of the access mode bits NDFileModeRead, NDFileModeWrite, NDFileModeAppend.
  *           May also have the bit NDFileModeMultiple set if the file is to be opened to write or read multiple 
  *           NDArrays into a single file.
  * \param[in] pArray Pointer to an NDArray; this array is used to determine the header information and data 
  *           structure for the file. */
asynStatus NDFileHDF5::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray)
{
  herr_t hdfstatus;
  hid_t hdfdatatype;
  int storeAttributes, storePerformance;
  static const char *functionName = "openFile";

  //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
  //  "%s::%s filename: %s\n", 
  //  driverName, functionName, fileName);

  /* We don't support reading yet */  
  if (openMode & NDFileModeRead) {
    setIntegerParam(NDFileCapture, 0);
    setIntegerParam(NDWriteFile, 0);
    return(asynError);
  }
  
  /* We don't support opening an existing file for appending yet */  
  if (openMode & NDFileModeAppend) {
    setIntegerParam(NDFileCapture, 0);
    setIntegerParam(NDWriteFile, 0);
    return(asynError);
  }

  /* Checking if the plugin already has a file open.
   * It would be nice if NDPluginFile class would accept an asynError returned from
   * this method and not increment the filecounter and name... However, for now we just
   * close the file that is open and open a new one. */
  if (this->file != 0)
  {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
      "%s::%s file is already open. Closing it and opening new one.\n", 
      driverName, functionName);
    this->closeFile();
    //return(asynError);
  }

  if (openMode & NDFileModeMultiple) this->multiFrameFile = true;
  else {
    this->multiFrameFile = false;
    setIntegerParam(NDFileHDF5_nExtraDims, 0);
  }

  epicsTimeGetCurrent(&this->prevts);
  this->opents = this->prevts;
  NDArrayInfo_t info;
  pArray->getInfo(&info);
  this->frameSize = (8.0 * info.totalBytes)/(1024.0 * 1024.0);
  this->bytesPerElement = info.bytesPerElement;

  /* Construct an attribute list. We use a separate attribute list
   * from the one in pArray to avoid the need to copy the array. */
  /* First clear the list*/
  this->pFileAttributes->clear();
  /* Now get the current values of the attributes for this plugin */
  this->getAttributes(this->pFileAttributes);
  /* Now append the attributes from the array which are already up to date from
   * the driver and prior plugins */
  pArray->pAttributeList->copy(this->pFileAttributes);
  //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
  //  "%s::%s attribute list copied. num pArray attributes = %i local copy = %d\n",
  //  driverName, functionName, this->pFileAttributes->count(), pArray->pAttributeList->count());
  
  /* Set the next record in the file to 0 */
  this->nextRecord = 0;

  /* Work out the various dimensions used in the dataset */
  this->configureDims(pArray);

  /* File access property list: set the alignment boundary to a user defined block size
   * which ideally matches disk boundaries.
   * If user sets size to 0 we do not set alignment at all. */
  hid_t access_plist = H5Pcreate(H5P_FILE_ACCESS);
  hsize_t align = 0;
  hsize_t threshold = 0;
  getIntegerParam(NDFileHDF5_chunkBoundaryAlign, (int*)&align);
  getIntegerParam(NDFileHDF5_chunkBoundaryThreshold, (int*)&threshold);
  if (align > 0)
  {
    hdfstatus = H5Pset_alignment( access_plist, threshold, align );
    if (hdfstatus < 0)
    {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
          "%s%s Warning: failed to set boundary threshod=%llu and alignment=%llu bytes\n",
          driverName, functionName, threshold, align);
      H5Pget_alignment( access_plist, &threshold, &align );
      setIntegerParam(NDFileHDF5_chunkBoundaryAlign, (int)align);
      setIntegerParam(NDFileHDF5_chunkBoundaryThreshold, (int)threshold);
    }
  }

  /* File creation property list: set the i-storek according to HDF group recommendations */
  hid_t create_plist = H5Pcreate(H5P_FILE_CREATE);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s::%s Setting istorek=%d\n",
            driverName, functionName, this->calc_istorek());
  hdfstatus = H5Pset_istore_k(create_plist, this->calc_istorek());

  /* Create the file. Overwrite this file, if it already exists.*/
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s Creating or overwriting HDF5 file: %s framesize=%.3f megabit\n", 
    driverName, functionName, fileName, this->frameSize);
  this->file = H5Fcreate(fileName, H5F_ACC_TRUNC, create_plist, access_plist);
  if (this->file <= 0){
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
              "%s::%s Unable to create HDF5 file: %s\n", 
              driverName, functionName, fileName);
    this->file = 0;
    return asynError;
  }

  /* Create the file structure */
  this->groupEntry    =   H5Gcreate(this->file,      "/entry",   H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  this->writeStringAttribute(this->groupEntry,      "NX_class", "NXentry");
  this->groupInstrument = H5Gcreate(this->groupEntry,    "instrument", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  this->writeStringAttribute(this->groupInstrument, "NX_class", "NXinstrument");
  this->groupDetector   = H5Gcreate(this->groupInstrument, "detector",   H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  this->writeStringAttribute(this->groupDetector,   "NX_class", "NXdetector");


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
  hdfstatus = H5Pset_chunk( this->cparms, this->rank, this->chunkdims);

  /* Get the datatype */
  hdfdatatype = this->type_nd2hdf(pArray->dataType);
  this->datatype = H5Tcopy(hdfdatatype);

  /* configure compression if required */
  this->configureCompression();

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s Setting fillvalue\n", 
    driverName, functionName);
  hdfstatus = H5Pset_fill_value (this->cparms, this->datatype, this->ptrFillValue );

  hid_t dset_access_plist = H5Pcreate(H5P_DATASET_ACCESS);
  hsize_t nbytes = this->calc_chunk_cache_bytes();
  hsize_t nslots = this->calc_chunk_cache_slots();
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Setting cache size=%d slots=%d\n",
            driverName, functionName,
            (int)nbytes, (int)nslots);
  H5Pset_chunk_cache( dset_access_plist, (size_t)nslots, (size_t)nbytes, 1.0);

  /*
   * Create a new dataset within the file using cparms
   * creation properties.
   */
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s Creating first empty dataset called \"data\"\n", 
    driverName, functionName);
  this->dataset = H5Dcreate2(this->groupDetector, "data", this->datatype, this->dataspace,
                             H5P_DEFAULT, this->cparms, dset_access_plist);
  if (this->dataset == 0) {
    // If dataset creation fails then close file and abort as all following writes will fail as well
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
              "%s::%s ERROR: could not create dataset in file. Aborting.\n",
              driverName, functionName);
    H5Sclose(this->dataspace);
    H5Pclose(this->cparms);
    H5Tclose(this->datatype);
    H5Gclose(this->groupDetector);
    H5Gclose(this->groupInstrument);
    H5Gclose(this->groupEntry);
    H5Fclose(this->file);
    this->file = 0;
    setIntegerParam(NDFileCapture, 0);
    setIntegerParam(NDWriteFile, 0);
    return asynError;
  }
  
  this->writeStringAttribute(this->dataset, "NX_class", "SDS");
  this->writeRawdataAttribute();
  //H5Dclose(this->dataset);

  getIntegerParam(NDFileHDF5_storeAttributes, &storeAttributes);
  getIntegerParam(NDFileHDF5_storePerformance, &storePerformance);
  if (storeAttributes == 1)  this->createAttributeDataset();
  if (storePerformance == 1) this->configurePerformanceDataset();

  /* End define mode. */
  return(asynSuccess);
}


/** Writes NDArray data to a HDF5 file.
  * \param[in] pArray Pointer to an NDArray to write to the file. This function can be called multiple
  *           times between the call to openFile and closeFile if
  *           NDFileModeMultiple was set in openMode in the call to NDFileHDF5::openFile. */ 
asynStatus NDFileHDF5::writeFile(NDArray *pArray)
{       
  herr_t hdfstatus;
  int storeAttributes, storePerformance, flush;
  epicsTimeStamp startts, endts;
  epicsInt32 numCaptured;
  double dt=0.0, period=0.0, runtime = 0.0;
  static const char *functionName = "writeFile";

  if (this->file == 0) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
      "%s::%s file is not open!\n", 
      driverName, functionName);
    return asynError;
  }

  getIntegerParam(NDFileNumCaptured, &numCaptured);
  if (numCaptured == 1) epicsTimeGetCurrent(&this->firstFrame);

  getIntegerParam(NDFileHDF5_storeAttributes, &storeAttributes);
  getIntegerParam(NDFileHDF5_storePerformance, &storePerformance);
  getIntegerParam(NDFileHDF5_flushNthFrame, &flush);

  if (storeAttributes == 1)
  {
    /* Update attribute list. We use a separate attribute list
     * from the one in pArray to avoid the need to copy the array. */
    /* Get the current values of the attributes for this plugin */
    //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    //  "%s::%s getting attribute list\n", 
    //  driverName, functionName);
    this->getAttributes(this->pFileAttributes);
    /* Now append the attributes from the array which are already up to date from
     * the driver and prior plugins */
    //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    //  "%s::%s copying attribute list\n", 
    //  driverName, functionName);
    pArray->pAttributeList->copy(this->pFileAttributes);
  }

  // Get the current time to calculate performance times
  epicsTimeGetCurrent(&startts);

  // For multi frame files we now extend the HDF dataset to fit an additional frame
  if (this->multiFrameFile) this->extendDataSet();
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s: set_extent dims={%d,%d,%d}\n",
            driverName, functionName,
            (int)this->dims[0], (int)this->dims[1], (int)this->dims[2]);
  hdfstatus = H5Dset_extent(this->dataset, this->dims);

  // Select a hyperslab.
  hid_t fspace = H5Dget_space(this->dataset);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s: select_hyperslab offset={%d,%d,%d} fsize={%d,%d,%d}\n",
            driverName, functionName,
            (int)this->offset[0], (int)this->offset[1], (int)this->offset[2],
            (int)this->framesize[0], (int)this->framesize[1], (int)this->framesize[2]);
  hdfstatus = H5Sselect_hyperslab(fspace, H5S_SELECT_SET,
                                  this->offset, NULL,
                                  this->framesize, NULL);

  // Write the data to the hyperslab.
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s: Writing dataset: %p\n",
            driverName, functionName, pArray->pData);
  hdfstatus = H5Dwrite(this->dataset, this->datatype, this->dataspace, fspace,
                       H5P_DEFAULT, pArray->pData);
  H5Sclose(fspace);
  if (hdfstatus < 0) {
    // If dataset creation fails then close file and abort as all following writes will fail as well
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
              "%s::%s ERROR: could not write to dataset. Aborting\n",
              driverName, functionName);
    H5Dclose(this->dataset);
    H5Sclose(this->dataspace);
    H5Pclose(this->cparms);
    H5Tclose(this->datatype);
    H5Gclose(this->groupDetector);
    H5Gclose(this->groupInstrument);
    H5Gclose(this->groupEntry);
    H5Fclose(this->file);
    this->file = 0;
    setIntegerParam(NDFileCapture, 0);
    setIntegerParam(NDWriteFile, 0);
    return asynError;
  }

  if (storeAttributes == 1)  this->writeAttributeDataset();
  if (storePerformance == 1)
  {
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

  if (flush > 0)
  {
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
        H5Dclose(this->dataset);
        H5Sclose(this->dataspace);
        H5Pclose(this->cparms);
        H5Tclose(this->datatype);
        H5Gclose(this->groupDetector);
        H5Gclose(this->groupInstrument);
        H5Gclose(this->groupEntry);
        H5Fclose(this->file);
        this->file = 0;
        setIntegerParam(NDFileCapture, 0);
        setIntegerParam(NDWriteFile, 0);
        return asynError;
      }
    }
  }
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s wrote frame. dt=%.5fs (T=%.5fs)\n", 
    driverName, functionName, dt, period);

  this->nextRecord++;
  return(asynSuccess);
}

/** Read NDArray data from a HDF5 file; NOTE: not implemented yet.
  * \param[in] pArray Pointer to the address of an NDArray to read the data into.  */ 
asynStatus NDFileHDF5::readFile(NDArray **pArray)
{
  //static const char *functionName = "readFile";
  return asynError;
}


/** Closes the HDF5 file opened with NDFileHDF5::openFile */ 
asynStatus NDFileHDF5::closeFile()
{
  int storeAttributes, storePerformance;
  epicsTimeStamp now;
  double runtime = 0.0, writespeed = 0.0;
  epicsInt32 numCaptured;
  static const char *functionName = "closeFile";

  if (this->file == 0)
  {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
      "%s::%s file was not open! Ignoring close command.\n", 
      driverName, functionName);
    return asynError;
  }

  getIntegerParam(NDFileHDF5_storeAttributes, &storeAttributes);
  getIntegerParam(NDFileHDF5_storePerformance, &storePerformance);
  if (storeAttributes == 1)  this->closeAttributeDataset();
  if (storePerformance == 1) this->writePerformanceDataset();

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s closing HDF dataset %d\n", 
    driverName, functionName, this->dataset);
  H5Dclose(this->dataset);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s closing HDF dataspace %d\n", 
    driverName, functionName, this->dataspace);
  H5Sclose(this->dataspace);
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
  H5Gclose(this->groupDetector);
  H5Gclose(this->groupInstrument);
  H5Gclose(this->groupEntry);

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s closing HDF file %d\n", 
    driverName, functionName, this->file);
  H5Fclose(this->file);
  this->file = 0;

  epicsTimeGetCurrent(&now);
  runtime = epicsTimeDiffInSeconds(&now, &this->opents);
  getIntegerParam(NDFileNumCaptured, &numCaptured);
  writespeed = (numCaptured * this->frameSize)/runtime;
  setDoubleParam(NDFileHDF5_totalIoSpeed, writespeed);
  setDoubleParam(NDFileHDF5_totalRuntime, runtime);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s file closed! runtime=%.3f s overall acquisition performance=%.2f Mbit/s\n",
    driverName, functionName,
    runtime, writespeed);
  return asynSuccess;
}

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
  const char *functionName = "writeInt32";

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
    // if we are using the virtual dimensions we cannot allow setting a number of
    // frames to acquire which is larger than the product of all virtual dimension (n,X,Y) sizes
    // as there will not be a suitable location in the file to store the additional frames.
    if (numExtraDims > 0)
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
    } else
    {
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
  //const char *functionName = "NDFileHDF5";

  this->createParam(str_NDFileHDF5_nRowChunks,      asynParamInt32,   &NDFileHDF5_nRowChunks);
  this->createParam(str_NDFileHDF5_nColChunks,      asynParamInt32,   &NDFileHDF5_nColChunks);
  this->createParam(str_NDFileHDF5_extraDimSizeN,   asynParamInt32,   &NDFileHDF5_extraDimSizeN);
  this->createParam(str_NDFileHDF5_nFramesChunks,   asynParamInt32,   &NDFileHDF5_nFramesChunks);
  this->createParam(str_NDFileHDF5_chunkBoundaryAlign, asynParamInt32,&NDFileHDF5_chunkBoundaryAlign);
  this->createParam(str_NDFileHDF5_chunkBoundaryThreshold, asynParamInt32,&NDFileHDF5_chunkBoundaryThreshold);
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

  setIntegerParam(NDFileHDF5_nRowChunks,      0);
  setIntegerParam(NDFileHDF5_nColChunks,      0);
  setIntegerParam(NDFileHDF5_nFramesChunks,   0);
  setIntegerParam(NDFileHDF5_extraDimSizeN,   1);
  setIntegerParam(NDFileHDF5_chunkBoundaryAlign, 0);
  setIntegerParam(NDFileHDF5_chunkBoundaryAlign, 1);
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

  // initialise the linked list of attribute names
  ellInit(&this->attrList);

  this->hostname = (char*)calloc(MAXHOSTNAMELEN, sizeof(char));
  gethostname(this->hostname, MAXHOSTNAMELEN);
  //printf("hostname: %s\n", this->hostname);
}

/** Calculate the total number of frames that the current configured dimensions can contain.
 * Sets the NDFileNumCapture parameter to the total value so file saving will complete at this number.
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

unsigned int NDFileHDF5::calc_istorek()
{
    unsigned int retval = 0;
    unsigned int num_chunks = 1; // Number of chunks that fit in the full dataset
    double div_result = 0.0;
    int extradimsizes[MAXEXTRADIMS] = {0,0,0};
    int numExtraDims = 0;
    getIntegerParam(NDFileHDF5_nExtraDims,    &numExtraDims);

    getIntegerParam(NDFileHDF5_extraDimSizeN, &extradimsizes[2]);
    getIntegerParam(NDFileHDF5_extraDimSizeX, &extradimsizes[1]);
    getIntegerParam(NDFileHDF5_extraDimSizeY, &extradimsizes[0]);
    hsize_t maxdim = 0;
    int extradim = MAXEXTRADIMS - numExtraDims-1;
    if (numExtraDims == 0) getIntegerParam(NDFileNumCapture, &extradimsizes[2]);
    for (int i = 0; i<this->rank; i++)
    {
        maxdim = this->maxdims[i];
        if (maxdim == H5S_UNLIMITED) {
            maxdim = extradimsizes[extradim];
            extradim++;
        }
        div_result = (double)maxdim / (double)this->chunkdims[i];
        div_result = ceil(div_result);
        num_chunks *= (unsigned int)div_result;
    }
    retval = num_chunks/2;
    if (retval <= 0) retval = 1;
    return retval;
}

hsize_t NDFileHDF5::calc_chunk_cache_bytes()
{
    hsize_t nbytes = 0;
    epicsInt32 n_frames_chunk=0;
    getIntegerParam(NDFileHDF5_nFramesChunks, &n_frames_chunk);
    nbytes = this->maxdims[this->rank - 1] * this->maxdims[this->rank - 2]
             * this->bytesPerElement * n_frames_chunk;
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

hsize_t NDFileHDF5::calc_chunk_cache_slots()
{
    unsigned int long nslots = 1;
    unsigned int long num_chunks = 1;
    double div_result = 0.0;
    div_result = (double)this->maxdims[this->rank - 1] / (double)this->chunkdims[this->rank -1];
    num_chunks *= (unsigned int long)ceil(div_result);
    div_result = (double)this->maxdims[this->rank - 2] / (double)this->chunkdims[this->rank -2];
    num_chunks *= (unsigned int long)ceil(div_result);
    epicsInt32 n_frames_chunk=0, n_extra_dims=0, n_frames_capture=0;
    getIntegerParam(NDFileHDF5_nFramesChunks, &n_frames_chunk);
    getIntegerParam(NDFileHDF5_nExtraDims, &n_extra_dims);
    getIntegerParam(NDFileNumCapture, &n_frames_capture);
    div_result = (double)n_frames_capture / (double)n_frames_chunk;
    num_chunks *= (unsigned int long)ceil(div_result);
    for(int i=0; i < n_extra_dims; i++)
    {
        num_chunks *= (unsigned int long)this->virtualdims[i];
    }

    // number of slots have to be a prime number which is between 10 and 50 times
    // larger than the numer of chunks that can fit in the file/dataset.
    nslots = num_chunks * 50;
    while( !is_prime( nslots) )
        nslots++;
    return nslots;
}



asynStatus NDFileHDF5::configurePerformanceDataset()
{
  int numCaptureFrames;
  getIntegerParam(NDFileNumCapture, &numCaptureFrames);

  // only allocate new memory if we need more points than we've used before
  if (numCaptureFrames > this->numPerformancePoints)
  {
    this->numPerformancePoints = numCaptureFrames;
    if (this->performanceBuf != NULL) {free(this->performanceBuf); this->performanceBuf = NULL;}
    if (this->performanceBuf == NULL)
      this->performanceBuf = (epicsFloat64*)  calloc(5 * this->numPerformancePoints, sizeof(double));
  }
  this->performancePtr  = this->performanceBuf;

  // create the performance group in the file
  this->groupPerfomance = H5Gcreate(this->groupInstrument, "performance", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  return asynSuccess;
}

asynStatus NDFileHDF5::writePerformanceDataset()
{
  hsize_t dims[2];
  hid_t dataspace_id, dataset_id;
  epicsInt32 numCaptured;

  /* Create the data space for the second dataset. */
  getIntegerParam(NDFileNumCaptured, &numCaptured);
  dims[1] = 5;
  if (numCaptured < this->numPerformancePoints) dims[0] = numCaptured;
  else dims[0] = this->numPerformancePoints;
  dataspace_id = H5Screate_simple(2, dims, NULL);

  /* Create the second dataset in group "Group_A". */
  dataset_id = H5Dcreate2(this->groupPerfomance, "timestamp", H5T_NATIVE_DOUBLE, dataspace_id,
                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Write the second dataset. */
  H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE,
                    H5S_ALL, H5S_ALL,
                    H5P_DEFAULT, this->performanceBuf);

  /* Close the data space for the second dataset. */
  H5Sclose(dataspace_id);

  /* Close the second dataset */
  H5Dclose(dataset_id);

  /* Close the group. */
  H5Gclose(this->groupPerfomance);
  return asynSuccess;
}

/** Create the group of datasets to hold the NDArray attributes
 *
 */
asynStatus NDFileHDF5::createAttributeDataset()
{
  HDFAttributeNode *hdfAttrNode;
  NDAttribute *ndAttr = NULL;
  int extraDims;
  hsize_t hdfdims=1;
  int numCaptures = 1;
  hid_t hdfgroup;
  const char *attrNames[3] = {"description", "source", NULL};
  char *attrStrings[3] = {NULL,NULL,NULL};
  int i;
  NDAttrDataType_t ndAttrDataType; 
  size_t size;
  const char *functionName = "createAttributeDataset";

  getIntegerParam(NDFileHDF5_nExtraDims, &extraDims);
  getIntegerParam(NDFileNumCapture, &numCaptures);
  hdfdims = (hsize_t)numCaptures;

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Creating attribute datasets. extradims=%d attribute count=%d\n",
            driverName, functionName, extraDims, this->pFileAttributes->count());

  this->groupNDAttributes = H5Gcreate(this->groupDetector,   "NDAttributes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  this->groupEpicsPvData  = H5Gcreate(this->groupInstrument, "NDAttributes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if (strlen(this->hostname) > 0) {
    this->writeStringAttribute(this->groupNDAttributes, "hostname", this->hostname);
  }

  ndAttr = this->pFileAttributes->next(ndAttr); // get the first NDAttribute
  while(ndAttr != NULL)
  {
    hdfgroup = this->groupNDAttributes;
    if (ndAttr->sourceType == NDAttrSourceEPICSPV) hdfgroup = this->groupEpicsPvData;

    if (ndAttr->dataType < NDAttrString)
    {

      // allocate another name-nodes
      hdfAttrNode = (HDFAttributeNode*)calloc(1, sizeof(HDFAttributeNode));
      hdfAttrNode->attrName = epicsStrDup(ndAttr->pName); // copy the attribute name

      hdfAttrNode->offset     = 0;
      hdfAttrNode->hdfdims    = hdfdims;
      //hdfAttrNode->hdfrank  = extraDims + 1;
      hdfAttrNode->hdfrank    = 1;
      //hdfAttrNode->hdfdataspace = H5Screate_simple(hdfAttrNode->hdfrank, this->virtualdims, this->virtualdims);
      hdfAttrNode->hdfdataspace = H5Screate_simple(hdfAttrNode->hdfrank, &hdfAttrNode->hdfdims, NULL);
      hdfAttrNode->hdfcparm   = H5Pcreate(H5P_DATASET_CREATE);

      hdfAttrNode->hdfdatatype  = this->type_nd2hdf((NDDataType_t)ndAttr->dataType);
      H5Pset_fill_value (hdfAttrNode->hdfcparm, hdfAttrNode->hdfdatatype, this->ptrFillValue );

      hdfAttrNode->hdfdataset   = H5Dcreate2(hdfgroup, hdfAttrNode->attrName,
                                             hdfAttrNode->hdfdatatype, hdfAttrNode->hdfdataspace,
                                             H5P_DEFAULT, hdfAttrNode->hdfcparm, H5P_DEFAULT);

      // create a memory space of exactly one element dimension to use for writing slabs
      hdfAttrNode->elementSize  = 1;
      hdfAttrNode->hdfmemspace  = H5Screate_simple(hdfAttrNode->hdfrank, &hdfAttrNode->elementSize, NULL);

      // Write some description of the NDAttribute as a HDF attribute to the dataset
      attrStrings[0] = ndAttr->pDescription;
      attrStrings[1] = ndAttr->pSource;
      for (i=0; attrNames[i] != NULL; i++)
      {
        size = strlen(attrStrings[i]);
        if (size <= 0) continue;
        this->writeStringAttribute(hdfAttrNode->hdfdataset, attrNames[i], attrStrings[i]);
      }

      ellAdd(&this->attrList, (ELLNODE *)hdfAttrNode);
    }
     // if the NDArray attribute is a string we attach it as an HDF attribute to the meta data group.
    else if (ndAttr->dataType == NDAttrString)
    {
      // Write some description of the NDAttribute as a HDF attribute to the dataset
      ndAttr->getValueInfo(&ndAttrDataType, &size);
      attrStrings[0] = (char*)calloc(size, sizeof(char));
      ndAttr->getValue(ndAttrDataType, attrStrings[0], size);
      if (size > 0) this->writeStringAttribute(hdfgroup, ndAttr->pName, attrStrings[0]);
      free(attrStrings[0]);
    }
    ndAttr = this->pFileAttributes->next(ndAttr);
  }
  return asynSuccess;
}

/** Write the NDArray attributes to the file
 *
 */
asynStatus NDFileHDF5::writeAttributeDataset()
{
  asynStatus status = asynSuccess;
  HDFAttributeNode *hdfAttrNode = NULL;
  NDAttribute *ndAttr = NULL;
  //hsize_t elementSize = 1;
  void* datavalue;
  int ret;
  const char *functionName = "writeAttributeDataset";

  hdfAttrNode = (HDFAttributeNode*)ellFirst(&this->attrList);
  datavalue = calloc(8, sizeof(char));

  //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
  //  "%s::%s Writing attribute datasets. firstnode=%p num stored attributes=%d\n",
  //  driverName, functionName, hdfAttrNode, ellCount(&this->attrList));

  while(hdfAttrNode != NULL)
  {
    // find the named attribute in the NDAttributeList
    ndAttr = this->pFileAttributes->find(hdfAttrNode->attrName);
    if (ndAttr == NULL)
    {
      hdfAttrNode = (HDFAttributeNode*)ellNext((ELLNODE*)hdfAttrNode);
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s::%s WARNING: NDAttribute named \'%s\' not found\n",
        driverName, functionName, hdfAttrNode->attrName);
      continue;
    }
    // find the data based on datatype
    ret = ndAttr->getValue(ndAttr->dataType, datavalue, 8);
    if (ret == ND_ERROR) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
        "%s::%s: ERROR did not get data from NDAttribute \'%s\'\n",
        driverName, functionName, ndAttr->pName);
      memset(datavalue, 0, 8);
    }

    // Work with HDF5 library to select a suitable hyperslab (one element) and write the new data to it
    hdfAttrNode->hdffilespace = H5Dget_space(hdfAttrNode->hdfdataset);
//    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
//      "%s::%s: attribute=%s select_hyperslab offset=%lli fsize=%lli\n",
//      driverName, functionName, hdfAttrNode->attrName,
//      hdfAttrNode->offset,
//      elementSize);
    H5Sselect_hyperslab(hdfAttrNode->hdffilespace, H5S_SELECT_SET,
                                    &hdfAttrNode->offset, NULL,
                                    &hdfAttrNode->elementSize, NULL);

    // Write the data to the hyperslab.
    H5Dwrite(hdfAttrNode->hdfdataset, hdfAttrNode->hdfdatatype,
                         hdfAttrNode->hdfmemspace, hdfAttrNode->hdffilespace,
                         H5P_DEFAULT, datavalue);

    H5Sclose(hdfAttrNode->hdffilespace);
    hdfAttrNode->offset++;

    // Take the next attribute from the list
    hdfAttrNode = (HDFAttributeNode*)ellNext((ELLNODE*)hdfAttrNode);
  }
  return status;
}

asynStatus NDFileHDF5::closeAttributeDataset()
{
  asynStatus status = asynSuccess;
  HDFAttributeNode *hdfAttrNode;
  const char *functionName = "closeAttributeDataset";

  hdfAttrNode = (HDFAttributeNode*)ellFirst(&this->attrList);
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s Closing attribute datasets. firstnode=%p\n",
            driverName, functionName, hdfAttrNode);

  while(hdfAttrNode != NULL)
  {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s: closing attribute dataset \'%s\'\n",
              driverName, functionName, hdfAttrNode->attrName);
    H5Dclose(hdfAttrNode->hdfdataset);
    H5Sclose(hdfAttrNode->hdfmemspace);
    H5Sclose(hdfAttrNode->hdfdataspace);
    H5Pclose(hdfAttrNode->hdfcparm);
    free(hdfAttrNode->attrName);
    ellDelete(&this->attrList, (ELLNODE*)hdfAttrNode);
    hdfAttrNode = (HDFAttributeNode*)ellFirst(&this->attrList);
  }
  H5Gclose(this->groupNDAttributes);
  H5Gclose(this->groupEpicsPvData);
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

  return status;
}

asynStatus NDFileHDF5::writeRawdataAttribute()
{
  asynStatus status = asynSuccess;
  hid_t hdfattr;
  hid_t hdfattrdataspace;
  hsize_t hdfattrdims;
  epicsInt32 hdfattrval;
  char hdfattrname[DIMNAMESIZE];
  int i;
  const char *functionName = "writeDatasetAttribute";

  /* attach an attribute and write an integer data to it. ('signal', 1)  */
  /* First create the data space for the attribute. */
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    "%s::%s Adding a single constant attribute to \'data\'\n", 
    driverName, functionName);
  hdfattrdims = 1;
  hdfattrval = 1;
  hdfattrdataspace = H5Screate_simple(1, &hdfattrdims, NULL);
  hdfattr          = H5Acreate2(this->dataset, "signal",
                                H5T_NATIVE_INT32, hdfattrdataspace,
                                H5P_DEFAULT, H5P_DEFAULT);
  H5Awrite(hdfattr, H5T_NATIVE_INT32, (void*)&hdfattrval);
  H5Aclose(hdfattr);
  H5Sclose(hdfattrdataspace);

  /* attach an attribute and write an string of data to it. ('interpretation', 'image')  */
  this->writeStringAttribute(this->dataset, "interpretation", "image");

  /* Write the human-readable dimension names as an attribute to the rawdata set */
  for (i=0; i<this->rank; i++)
  {
    //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s   dim%d name: \'%s\'\n",
    //    driverName, functionName, i, this->ptrDimensionNames[i]);

    /* Create string attribute.  */
    epicsSnprintf(hdfattrname, DIMNAMESIZE, "dim%d", i);
    this->writeStringAttribute(this->dataset, hdfattrname, this->ptrDimensionNames[i]);
  }

  return status;
}

/** Extend the currently open file dataset to append one more frame
 * in the right dimension offsets.
 * This is to be called from writeFile() only
 */
void NDFileHDF5::extendDataSet()
{
  int i=0;
  bool growdims = true;
  bool growoffset = true;
  int extradims = 0;
  int nFramesCaptured=0;
  //const char *functionName = "extendDataSet";

  // Get the number of virtual dimensions from the plugin
  getIntegerParam(NDFileHDF5_nExtraDims, &extradims);

  // Add the n'th frame dimension (for multiple frames per scan point)
  extradims += 1;

  //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
  //  "%s::%s extradims=%d\n", 
  //  driverName, functionName, extradims);

  // first frame already has the offsets and dimensions preconfigured so
  // we dont need to increment anything here
  getIntegerParam(NDFileNumCaptured, &nFramesCaptured);
  if (nFramesCaptured <= 1) return;

  // in the simple case where dont use the extra X,Y dimensions we
  // just increment the n'th frame number
  if (extradims == 1)
  {
    this->dims[0]++;
    this->offset[0]++;
    return;
  }


  // run through the virtual dimensions in reverse order: n,X,Y
  // and increment, reset or ignore the offset of each dimension.
  for (i=extradims-1; i>=0; i--)
  {
    //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
    //  "%s::%s frame=%d i=%d growdims=%d dims[i]=%llu offset[i]=%llu virt[i]=%llu\n", 
    //  driverName, functionName,
    //  nFramesCaptured, i, (int)growdims, this->dims[i], this->offset[i], this->virtualdims[i]);
    if (this->dims[i] == this->virtualdims[i]) growdims = false;

    if (growoffset){
      this->offset[i]++;
      growoffset = false;
    }

    if (growdims){
      if (this->dims[i] < this->virtualdims[i]) {
        this->dims[i]++;
        growdims = false;
      }
    }

    if (this->offset[i] == this->virtualdims[i]) {
      this->offset[i] = 0;
      growoffset = true;
      growdims = true;
    }
  }

  return;
}

/** Configure the HDF5 dimension definitions based on NDArray dimensions.
 * Initially this implementation just copies the dimension sizes from the NDArray
 * in the same order. A last infinite length dimension is added if multiple frames
 * are to be stored in the same file (in Capture or Stream mode).
 */
asynStatus NDFileHDF5::configureDims(NDArray *pArray)
{
  int i=0,j=0, extradims = 0, ndims=0;
  //int numRowsInFrame = 0;
  //int chunkrows=0;
  int numCapture;
  asynStatus status = asynSuccess;
  char strdims[DIMSREPORTSIZE];
  const char *functionName = "configureDims";
  //strdims = (char*)calloc(DIMSREPORTSIZE, sizeof(char));

  if (this->multiFrameFile)
  {
    getIntegerParam(NDFileHDF5_nExtraDims, &extradims);
    extradims += 1;
  }

  ndims = pArray->ndims + extradims;

  //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
  //  "%s::%s multiframefile=%d ndims=%d extradims=%d maxdims=%p\n",
  //  driverName, functionName, (int)this->multiFrameFile, ndims, extradims, this->maxdims);

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

    //asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s::%s allocating dimension arrays\n",
    //        driverName, functionName);
    this->maxdims       = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->chunkdims     = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->framesize     = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->dims          = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->offset        = (hsize_t*)calloc(ndims,     sizeof(hsize_t));
    this->virtualdims   = (hsize_t*)calloc(extradims, sizeof(hsize_t));
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
      this->framesize[i]   = 1;
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
    this->framesize[i]  = pArray->dims[j].size;
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
  for (i = 0; i<3; i++)
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
      this->chunkdims[hdfdim] = user_chunking[i];
  }
  setIntegerParam(NDFileHDF5_nFramesChunks, user_chunking[2]);
  setIntegerParam(NDFileHDF5_nRowChunks,    user_chunking[1]);
  setIntegerParam(NDFileHDF5_nColChunks,    user_chunking[0]);

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

asynStatus NDFileHDF5::configureCompression()
{
  asynStatus status = asynSuccess;
  int compressionScheme;
  int szipNumPixels = 0;
  int nbitPrecision = 0;
  int nbitOffset = 0;
  int zLevel = 0;
  const char * functionName = "configureCompression";

  getIntegerParam(NDFileHDF5_compressionType, &compressionScheme);
  switch (compressionScheme)
  {
  case HDF5CompressNone:
    break;
  case HDF5CompressNumBits:
    getIntegerParam(NDFileHDF5_nbitsOffset, &nbitOffset);
    getIntegerParam(NDFileHDF5_nbitsPrecision, &nbitPrecision);
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
      "%s::%s Setting N-bit filter precision=%d bit offset=%d bit\n",
      driverName, functionName, nbitPrecision, nbitOffset);
    H5Tset_precision (this->datatype, nbitPrecision);
    H5Tset_offset (this->datatype, nbitOffset);
    H5Pset_nbit (this->cparms);

    // Finally read back the parameters we've just sent to HDF5
    nbitOffset = H5Tget_offset(this->datatype);
    nbitPrecision = (int)H5Tget_precision(this->datatype);
    setIntegerParam(NDFileHDF5_nbitsOffset, nbitOffset);
    setIntegerParam(NDFileHDF5_nbitsPrecision, nbitPrecision);
    break;
  case HDF5CompressSZip:
    getIntegerParam(NDFileHDF5_szipNumPixels, &szipNumPixels);
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
      "%s::%s Setting szip compression filter # pixels=%d\n",
      driverName, functionName, szipNumPixels);
    H5Pset_szip (this->cparms, H5_SZIP_NN_OPTION_MASK, szipNumPixels);
    break;
  case HDF5CompressZlib:
    getIntegerParam(NDFileHDF5_zCompressLevel, &zLevel);
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
      "%s::%s Setting zlib compression filter level=%d\n",
      driverName, functionName, zLevel);
    H5Pset_deflate(this->cparms, zLevel);
    break;
  }

  return status;
}

/** Translate the NDArray datatype to HDF5 datatypes */
hid_t NDFileHDF5::type_nd2hdf(NDDataType_t datatype)
{
  hid_t result;
  int fillvalue = 0;

  switch (datatype) {
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
        driverName, "type_nd2hdf", datatype);
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

  getIntegerParam(NDFileHDF5_nExtraDims, &extradims);
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
  new NDFileHDF5(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                 priority, stackSize);
  return(asynSuccess);
}


/** EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size",iocshArgInt};
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
