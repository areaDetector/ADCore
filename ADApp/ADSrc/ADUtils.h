/* ADUtils.h
 *
 * This file defines some utility functions that can be used by area detector
 * drivers.
 *
 * Mark Rivers
 * University of Chicago
 * March 5, 2008
 *
 */
 
#ifndef AD_UTILS_H
#define AD_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ADInterface.h>


typedef struct
{
    int  (*setParamDefaults)    (void *params);
    int  (*bytesPerPixel)       (ADDataType_t dataType, int *size);
    int  (*convertImage)        (void *imageIn, 
                                 int dataTypeIn,
                                 int sizeXIn, int sizeYIn,
                                 void *imageOut,
                                 int dataTypeOut,
                                 int binX, int binY,
                                 int startX, int startY,
                                 int regionSizeX, int regionSizeY,
                                 int *SizeXOut, int *sizeYOut);
    int  (*createFileName)       (void *params, int maxChars, char *fullFileName);
} ADUtilsSupport;

extern ADUtilsSupport *ADUtils;

#ifdef __cplusplus
}
#endif
#endif
