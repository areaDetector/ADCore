/*
 * drvNDFile.h
 * 
 * Asyn driver for callbacks to write data to files for area detectors.
 *
 * Author: Mark Rivers
 *
 * Created April 5, 2008
 */

#ifndef DRV_NDFile_H
#define DRV_NDFile_H

#ifdef __cplusplus
extern "C" {
#endif

int drvNDFileConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                       const char *NDArrayPort, int NDArrayAddr);

#ifdef __cplusplus
}
#endif
#endif
