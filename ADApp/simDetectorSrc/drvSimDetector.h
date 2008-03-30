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

extern ADDrvSet_t ADSimDetector;
int simDetectorSetup(int num_cameras);
int simDetectorConfig(int camera, int maxSizeX, int maxSizeY, int dataType);

#ifdef __cplusplus
}
#endif
#endif
