/* ADImageBuff.h
 *
 * Image bufffer allocation utility.
 * 
 *
 * Mark Rivers
 * University of Chicago
 * April 5, 2008
 *
 */
 
#ifndef AD_IMAGEBUFF_H
#define AD_IMAGEBUFF_H

#include <epicsMutex.h>
#include <ellLib.h>

#include "ADInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int         (*init)     (int maxImages, size_t maxMemory);
    ADImage_t*  (*alloc)    (int nx, int ny, int dataType, int dataSize, void *pData);
    int         (*reserve)  (ADImage_t *pImage); 
    int         (*release)  (ADImage_t *pImage);
    int         (*report)   (int details);     
} ADImageBuffSupport;

extern ADImageBuffSupport *ADImageBuff;

#ifdef __cplusplus
}
#endif
#endif
