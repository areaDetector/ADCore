/*
 * NDFileNetCDF.h
 * Writes NDArrays to netCDF files.
 *
 * Mark Rivers
 * April 17, 2008
 */

#include "NDArray.h"

#ifndef DRV_NDFileNetCDF_H
#define DRV_NDFileNetCDF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NDFileNetCDFState {
    int ncId;
    int arrayDataId;
    int uniqueIdId;
    int timeStampId;
    int nextRecord;
} NDFileNetCDFState_t;


int NDFileWriteNetCDF(const char *fileName, NDFileNetCDFState_t *pState, NDArray *pArray, 
                      int numArrays, int append, int close);

#ifdef __cplusplus
}
#endif
#endif
