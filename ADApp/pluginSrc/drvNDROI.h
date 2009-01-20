/*
 * drvNDROI.h
 * 
 * Asyn driver for ROI plugin for NDArray callbacks.
 *
 * Author: Mark Rivers
 *
 * Created April 2, 2008
 */

#ifndef drvNDROI_H
#define drvNDROI_H

#ifdef __cplusplus
extern "C" {
#endif

int drvNDROIConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                      const char *NDArrayPort, int NDArrayAddr, int maxROIs,
                      int maxBuffers, size_t maxMemory);

#ifdef __cplusplus
}
#endif
#endif
