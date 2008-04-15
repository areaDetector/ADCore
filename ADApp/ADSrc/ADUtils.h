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

#include "ADInterface.h"

typedef struct
{
    int  (*setParamDefaults)    (void *params);
    int  (*createFileName)      (void *params, int maxChars, char *fullFileName);
    void (*handleCallback)      (void *handleInterruptPvt, void *handle);
    int  (*findParam)           (ADParamString_t *paramTable, int numParams, 
                                 const char *paramName, int *param);
} ADUtilsSupport;

extern ADUtilsSupport *ADUtils;

#ifdef __cplusplus
}
#endif
#endif
