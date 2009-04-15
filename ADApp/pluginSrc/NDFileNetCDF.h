/*
 * NDFileNetCDF.h
 * Writes NDArrays to netCDF files.
 * Mark Rivers
 * April 17, 2008
 */

#include "NDArray.h"

#ifndef DRV_NDFileNetCDF_H
#define DRV_NDFileNetCDF_H

/* This version number is an attribute in the netCDF file to allow readers
 * to handle changes in the file contents */
#define NDNetCDFFileVersion 2.0

typedef struct NDFileNetCDFState {
    int ncId;
    int arrayDataId;
    int uniqueIdId;
    int timeStampId;
    int nextRecord;
    int *pAttributeId;
} NDFileNetCDFState_t;


#ifdef __cplusplus
extern "C" {
#endif
int NDFileWriteNetCDF(const char *fileName, NDFileNetCDFState_t *pState, NDArray *pArray, 
                      int numArrays, int append, int close);
#ifdef __cplusplus
}
#endif
#endif
