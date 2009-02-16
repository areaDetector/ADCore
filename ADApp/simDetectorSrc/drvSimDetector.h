/* drvSimDetector.h
 *
 * This is a driver for a simulated area detector.
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  March 20, 2008
 *
 */

#ifndef DRV_SIMDETECTOR_H
#define DRV_SIMDETECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

int simDetectorConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType,
                      int maxBuffers, size_t maxMemory,
                      int priority, int stackSize);

#ifdef __cplusplus
}
#endif
#endif
