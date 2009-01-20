/*
 * drvNDColorConvert.h
 * 
 * Asyn driver for callbacks to convert color modes
 * Author: Mark Rivers
 *
 * Created December 22, 2008
 */

#ifndef DRV_NDColorConvert_H
#define DRV_NDColorConvert_H

#ifdef __cplusplus
extern "C" {
#endif

int drvNDColorConvertConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                               const char *NDArrayPort, int NDArrayAddr, 
                               int maxBuffers, size_t maxMemory);

#ifdef __cplusplus
}
#endif
#endif
