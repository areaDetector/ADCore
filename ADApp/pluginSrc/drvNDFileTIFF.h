/*
 * drvNDFileTIFF.h
 *
 * Asyn driver for callbacks to write data to TIFF files for area detectors.
 *
 * Author: John Hammonds
 *
 * Created April 17, 2009
 */

#ifndef DRV_NDFileTIFF_H
#define DRV_NDFileTIFF_H

#ifdef __cplusplus
extern "C" {
#endif

int drvNDFileTIFFConfigure(const char *portName, int queueSize, int blockingCallbacks,
                             const char *NDArrayPort, int NDArrayAddr,
                             int priority, int stackSize);

#ifdef __cplusplus
}
#endif
#endif
