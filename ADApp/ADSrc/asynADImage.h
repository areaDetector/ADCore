/*  asynADImage.h
 *
 ***********************************************************************
 * Copyright (c) 2002 The University of Chicago, as Operator of Argonne
 * National Laboratory, and the Regents of the University of
 * California, as Operator of Los Alamos National Laboratory, and
 * Berliner Elektronenspeicherring-Gesellschaft m.b.H. (BESSY).
 * asynDriver is distributed subject to a Software License Agreement
 * found in file LICENSE that is included with this distribution.
 ***********************************************************************
 *
 *  31-March-2008 Mark Rivers
 */

#ifndef asynADImageH
#define asynADImageH

#include <asynDriver.h>
#include <epicsTypes.h>
#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef void (*interruptCallbackADImage)(void *userPvt, asynUser *pasynUser,
              void *data, int dataType, int nx, int ny);
              
typedef struct asynADImageInterrupt {
    asynUser *pasynUser;
    int addr;
    interruptCallbackADImage callback;
    void *userPvt;
} asynADImageInterrupt;

#define asynADImageType "asynADImage"
typedef struct asynADImage {
    asynStatus (*write)(void *drvPvt, asynUser *pasynUser, void *value,
                    int dataType, int nx, int ny);
    asynStatus (*read)(void *drvPvt, asynUser *pasynUser, void *value,
                    int maxBytes, int *dataType, int *nx, int *ny);
    asynStatus (*registerInterruptUser)(void *drvPvt, asynUser *pasynUser,
                    interruptCallbackADImage callback, void *userPvt, void **registrarPvt);
    asynStatus (*cancelInterruptUser)(void *drvPvt, asynUser *pasynUser,
                    void *registrarPvt);
} asynADImage;

/* asynADImageBase does the following:
   calls  registerInterface for asynADImage.
   Implements registerInterruptUser and cancelInterruptUser
   Provides default implementations of all methods.
   registerInterruptUser and cancelInterruptUser can be called
   directly rather than via queueRequest.
*/

#define asynADImageBaseType "asynADImageBase"
typedef struct asynADImageBase {
    asynStatus (*initialize)(const char *portName,
                            asynInterface *pasynADImageInterface);
} asynADImageBase;

epicsShareExtern asynADImageBase *pasynADImageBase;


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* asynADImageH */
