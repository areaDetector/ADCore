/*
 * NDFileNetCDF.h
 * Writes NDArrays to netCDF files.
 * Mark Rivers
 * April 17, 2008
 */

#ifndef DRV_NDFileNetCDF_H
#define DRV_NDFileNetCDF_H

#include "NDPluginFile.h"

/* This version number is an attribute in the netCDF file to allow readers
 * to handle changes in the file contents */
#define NDNetCDFFileVersion 2.0

class NDFileNetCDF : public NDPluginFile {
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
    int nextRecord;
    int *pAttributeId;
};

#endif
