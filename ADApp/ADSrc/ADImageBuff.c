/* ADImageBuff.c
 *
= * Image bufffer allocation utility.
 * 
 *
 * Mark Rivers
 * University of Chicago
 * April 5, 2008
 *
 */

#include <string.h>
#include <stdio.h>
#include <ellLib.h>

#include <epicsMutex.h>
#include <cantProceed.h>

#include "ADImageBuff.h"
#include "ADUtils.h"

typedef struct {
    ELLLIST freeList;
    epicsMutexId listLock;
    int maxImages;
    int numImages;
    size_t maxMemory;
    size_t memorySize;
    int numFree;
} ADImageBuffPvt_t;

static char *driverName = "ADImageBuff";

ADImageBuffPvt_t ADImageBuffPvt;
int ADImageBuffDebug = 0;

static int init(int maxImages, size_t maxMemory)
{
    int status=0;
    ADImageBuffPvt_t *pPvt = &ADImageBuffPvt;

    ellInit(&pPvt->freeList);
    pPvt->maxImages = maxImages;
    pPvt->maxMemory = maxMemory;
    pPvt->numImages = 0;
    pPvt->memorySize = 0;
    pPvt->numFree = 0;
    pPvt->listLock = epicsMutexCreate();
    
    return(status);
}
    
static  ADImage_t* alloc(int nx, int ny, int dataType, int dataSize, void *pData)
{
    ADImage_t *pImage;
    int bytesPerPixel;
    ADImageBuffPvt_t *pPvt = &ADImageBuffPvt;
    const char* functionName = "ADImageBuff::alloc:";
    
    if (!pPvt->listLock) {
        printf("%s: must call %s->init() first!\n", driverName, driverName);
        return NULL;
    }
    epicsMutexLock(pPvt->listLock);
    
    /* Find a free image */
    pImage = (ADImage_t *)ellFirst(&pPvt->freeList);
    
    if (!pImage) {
        /* We did not find a free image.  
         * Allocate a new one if we have not exceeded the limit */
        if (pPvt->numImages == pPvt->maxImages) {
            printf("%s: ERROR: reached limit of %d images\n", 
                   functionName, pPvt->maxImages);
        } else {
            pPvt->numImages++;
            pImage = callocMustSucceed(1, sizeof(ADImage_t),
                                       functionName);
            ellAdd(&pPvt->freeList, &pImage->node);
            pPvt->numFree++;
        }
    }
    
    if (pImage) {
        /* We have a frame */
        pImage->nx = nx;
        pImage->ny = ny;
        ADUtils->bytesPerPixel(dataType, &bytesPerPixel);
        if (dataSize == 0) dataSize = bytesPerPixel *nx *ny;
        pImage->dataType = dataType;

        /* If the caller passed a valid buffer use that, trust that its size is correct */
        if (pData) {
            pImage->pData = pData;
        } else {
            /* See if the current buffer is big enough */
            if (pImage->dataSize < dataSize) {
                /* No, we need to free the current buffer and allocate a new one */
                /* See if there is enough room */
                pPvt->memorySize -= pImage->dataSize;
                free(pImage->pData);
                if ((pPvt->memorySize + dataSize) > pPvt->maxMemory) {
                    printf("%s: ERROR: reached limit of %d memory\n", 
                           functionName, pPvt->maxMemory);
                    pImage = NULL;
                } else {
                    pImage->pData = callocMustSucceed(dataSize, 1,
                                                      functionName);
                    pImage->dataSize = dataSize;
                    pPvt->memorySize += dataSize;
                }
            }
        }
    }
    if (pImage) {
        if (ADImageBuffDebug) {
            printf("%s:alloc allocated pImage=%p\n", 
                driverName, pImage);
        }
        /* Set the reference count to 1, remove from free list */
        pImage->referenceCount = 1;
        ellDelete(&pPvt->freeList, &pImage->node);
        pPvt->numFree--;
    }
    epicsMutexUnlock(pPvt->listLock);
    return (pImage);
}

static int reserve(ADImage_t *pImage)
{
    ADImageBuffPvt_t *pPvt = &ADImageBuffPvt;
    
    if (!pPvt->listLock) {
        printf("%s: must call %s->init() first!\n", driverName, driverName);
        return -1;
    }
    epicsMutexLock(pPvt->listLock);
    pImage->referenceCount++;
    if (ADImageBuffDebug) {
        printf("%s:reserve reserved image pImage=%p, referenceCount=%d\n", 
            driverName, pImage, pImage->referenceCount);
    }
    epicsMutexUnlock(pPvt->listLock);
    return 0;
}

static int release(ADImage_t *pImage)
{   
    ADImageBuffPvt_t *pPvt = &ADImageBuffPvt;
    
    if (!pPvt->listLock) {
        printf("%s: must call %s->init() first!\n", driverName, driverName);
        return -1;
    }
    epicsMutexLock(pPvt->listLock);
    pImage->referenceCount--;
    if (ADImageBuffDebug) {
        printf("%s:release released image pImage=%p, referenceCount=%d\n", 
            driverName, pImage, pImage->referenceCount);
    }
    if (pImage->referenceCount == 0) {
        /* The last user has released this image, add it back to the free list */
        ellAdd(&pPvt->freeList, &pImage->node);
        pPvt->numFree++;
    }
    if (pImage->referenceCount < 0) {
        printf("%s:release ERROR, reference count < 0 pImage=%p\n",
            driverName, pImage);
    }
    epicsMutexUnlock(pPvt->listLock);
    return 0;
}

static int report(int details)
{
    ADImageBuffPvt_t *pPvt = &ADImageBuffPvt;
    
    if (!pPvt->listLock) {
        printf("%s: must call %s->init() first!\n", driverName, driverName);
        return -1;
    }
    printf("ADImageBuff:\n");
    printf("  numImages=%d, maxImages=%d\n", 
        pPvt->numImages, pPvt->maxImages);
    printf("  memorySize=%d, maxMemory=%d\n", 
        pPvt->memorySize, pPvt->maxMemory);
    printf("  numFree=%d\n", 
        pPvt->numFree);
    return 0;
}
   
static ADImageBuffSupport support =
{
    init,
    alloc,
    reserve,
    release,
    report,
};

ADImageBuffSupport *ADImageBuff = &support;

