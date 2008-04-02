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

int drvADImageConfigure(const char *portName, const char *detectorPortName);
