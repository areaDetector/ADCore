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

#include "ADInterface.h"
#include "NDArrayBuff.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int  (*setParamDefaults)    (void *params);
    int  (*createFileName)      (void *params, int maxChars, char *fullFileName);
} ADUtilsSupport;

extern ADUtilsSupport *ADUtils;

#ifdef __cplusplus
}
#endif
#endif
