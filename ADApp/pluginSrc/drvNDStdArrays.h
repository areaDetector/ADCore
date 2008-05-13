/*
 * drvNDStdArrays.h
 * 
 * Asyn driver for callbacks to standard asyn array interfaces for NDArray drivers.
 * This is commonly used for EPICS waveform records.
 *
 * Author: Mark Rivers
 *
 * Created April 2, 2008
 */

#ifndef drvNDStdArrays_H
#define drvNDStdArrays_H

#ifdef __cplusplus
extern "C" {
#endif

int drvNDStdArraysConfigure(const char *portName, int queueSize, int blockingCallbacks, 
                            const char *NDArrayPort, int NDArrayAddr, size_t maxMemory);

#ifdef __cplusplus
}
#endif
#endif
