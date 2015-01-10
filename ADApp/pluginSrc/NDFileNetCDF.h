/*
 * NDFileNetCDF.h
 * Writes NDArrays to netCDF files.
 * Mark Rivers
 * April 17, 2008
 */

#ifndef DRV_NDFileNetCDF_H
#define DRV_NDFileNetCDF_H

#include "NDPluginFile.h"

/** This version number is an attribute in the netCDF file to allow readers
 * to handle changes in the file contents */
#define NDNetCDFFileVersion 3.0

/** Writes NDArrays to files in the netCDF file format.
  * netCDF is an open-source, portable, self-describing binary format supported by Unidata at UCAR
  * (http://www.unidata.ucar.edu/software/netcdf).
  * The netCDF format supports arrays of any dimension and all of the data types supported by NDArray.
  * It can store multiple NDArrays in a single file, so it sets NDPluginFile::supportsMultipleArrays to 1.
  * If also can store all of the attributes associated with an NDArray.
  * This class implements the 4 pure virtual functions from 
  * NDPluginFile: openFile, readFile, writeFile and closeFile. */
class epicsShareClass NDFileNetCDF : public NDPluginFile {
public:
    NDFileNetCDF(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int priority, int stackSize);
                 
    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();

private:
    int ncId;
    int arrayDataId;
    int uniqueIdId;
    int timeStampId;
    int epicsTSSecId;
    int epicsTSNsecId;
    int nextRecord;
    int *pAttributeId;
    NDAttributeList *pFileAttributes;
};

#define NUM_NDFILE_NETCDF_PARAMS 0
#endif
