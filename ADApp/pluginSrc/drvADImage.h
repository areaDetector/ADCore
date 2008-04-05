/*
 * drvADImage.h
 * 
 * Asyn driver for callbacks to asyn array interfaces for area detectors.
 * This is commonly used for EPICS waveform records.
 *
 * Author: Mark Rivers
 *
 * Created April 2, 2008
 */

#ifndef DRV_ADIMAGE_H
#define DRV_ADIMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

int drvADImageConfigure(const char *portName, int maxFrames, const char *imagePort, int imageAddr);

#ifdef __cplusplus
}
#endif
#endif
