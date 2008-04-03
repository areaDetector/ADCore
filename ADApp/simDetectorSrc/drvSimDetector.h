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

/* Note that the file format enum must agree with the mbbo/mbbi records in the simDetector.template file */
typedef enum {
   SimFormatBinary,
   SimFormatASCII
} SimFormat_t;

/* If we have any private driver parameters they begin with ADFirstDriverParam and should end
   with ADLastDriverParam, which is used for setting the size of the parameter library table */
typedef enum {
   SimGainX = ADFirstDriverParam,
   SimGainY,
   SimResetImage,
   ADLastDriverParam
} SimDetParam_t;

/* The command strings are the input to ADUtils->FindParam, which returns the corresponding parameter enum value */
static ADParamString_t SimDetParamString[] = {
    {SimGainX,      "SIM_GAINX"},  
    {SimGainY,      "SIM_GAINY"},  
    {SimResetImage, "RESET_IMAGE"}  
};

#define NUM_SIM_DET_PARAMS (sizeof(SimDetParamString)/sizeof(SimDetParamString[0]))


int simDetectorConfig(const char *portName, int maxSizeX, int maxSizeY, int dataType);

#ifdef __cplusplus
}
#endif
#endif
