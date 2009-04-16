/*
 * drvNDFileNetCDF.h
 * 
 * Asyn driver for callbacks to write data to netCDF files for area detectors.
 *
 * Author: Mark Rivers
 *
 * Created April 5, 2008
 */

#ifndef DRV_NDFileNetCDF_H
#define DRV_NDFileNetCDF_H

#ifdef __cplusplus
extern "C" {
#endif

int drvNDFileNetCDFConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                             const char *NDArrayPort, int NDArrayAddr,
                             int priority, int stackSize);

#ifdef __cplusplus
}
#endif
#endif
