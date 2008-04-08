/*
 * drvADFile.h
 * 
 * Asyn driver for callbacks to write data to files for area detectors.
 *
 * Author: Mark Rivers
 *
 * Created April 5, 2008
 */

#ifndef DRV_ADFILE_H
#define DRV_ADFILE_H

#ifdef __cplusplus
extern "C" {
#endif

int drvADFileConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                       const char *imagePort, int imageAddr);

#ifdef __cplusplus
}
#endif
#endif
