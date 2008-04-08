/*  asynADImageBase.c
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

#include <epicsTypes.h>
#include <epicsStdio.h>
#include <cantProceed.h>

#define epicsExportSharedSymbols
#include <shareLib.h>
#include <asynDriver.h>

#include "ADInterface.h"
#include "asynADImage.h"

static asynStatus initialize(const char *portName, asynInterface *pADImageInterface);

static asynADImageBase ADImageBase = {initialize};
epicsShareDef asynADImageBase *pasynADImageBase = &ADImageBase;

static asynStatus writeDefault(void *drvPvt, asynUser *pasynUser, ADImage_t *pImage);
static asynStatus readDefault(void *drvPvt, asynUser *pasynUser, int maxBytes, ADImage_t *pImage);
static asynStatus registerInterruptUser(void *drvPvt, asynUser *pasynUser,
                    interruptCallbackADImage callback, void *userPvt, void **registrarPvt);
static asynStatus cancelInterruptUser(void *drvPvt, asynUser *pasynUser,
                    void *registrarPvt);


static asynStatus initialize(const char *portName, asynInterface *pdriver)
{
    asynADImage   *pasynADImage = (asynADImage *)pdriver->pinterface;

    if(!pasynADImage->write) pasynADImage->write = writeDefault;
    if(!pasynADImage->read) pasynADImage->read = readDefault;
    if(!pasynADImage->registerInterruptUser)
        pasynADImage->registerInterruptUser = registerInterruptUser;
    if(!pasynADImage->cancelInterruptUser)
        pasynADImage->cancelInterruptUser = cancelInterruptUser;
    return pasynManager->registerInterface(portName, pdriver);
}

static asynStatus writeDefault(void *drvPvt, asynUser *pasynUser, ADImage_t *pImage)
{
    const char *portName;
    asynStatus status;
    int        addr;
    
    status = pasynManager->getPortName(pasynUser,&portName);
    if(status!=asynSuccess) return status;
    status = pasynManager->getAddr(pasynUser,&addr);
    if(status!=asynSuccess) return status;
    epicsSnprintf(pasynUser->errorMessage,pasynUser->errorMessageSize,
        "write is not supported\n");
    asynPrint(pasynUser,ASYN_TRACE_ERROR,
        "%s %d write is not supported\n",portName,addr);
    return asynError;
}

static asynStatus readDefault(void *drvPvt, asynUser *pasynUser, int maxBytes, ADImage_t *pImage)
{
    const char *portName;
    asynStatus status;
    int        addr;
    
    status = pasynManager->getPortName(pasynUser,&portName);
    if(status!=asynSuccess) return status;
    status = pasynManager->getAddr(pasynUser,&addr);
    if(status!=asynSuccess) return status;
    epicsSnprintf(pasynUser->errorMessage,pasynUser->errorMessageSize,
        "write is not supported\n");
    asynPrint(pasynUser,ASYN_TRACE_ERROR,
        "%s %d read is not supported\n",portName,addr);
    return asynError;
}
    
static asynStatus registerInterruptUser(void *drvPvt,asynUser *pasynUser,
      interruptCallbackADImage callback, void *userPvt, void **registrarPvt)
{
    const char    *portName;
    asynStatus    status;
    int           addr;
    interruptNode *pinterruptNode;
    void          *pinterruptPvt;
    asynADImageInterrupt *pasynADImageInterrupt;
    
    status = pasynManager->getPortName(pasynUser,&portName);
    if(status!=asynSuccess) return status;
    status = pasynManager->getAddr(pasynUser,&addr);
    if(status!=asynSuccess) return status;
    status = pasynManager->getInterruptPvt(pasynUser, asynADImageType,
                                           &pinterruptPvt);
    if(status!=asynSuccess) return status;
    pinterruptNode = pasynManager->createInterruptNode(pinterruptPvt);
    if(status!=asynSuccess) return status;
    pasynADImageInterrupt = pasynManager->memMalloc(sizeof(asynADImageInterrupt));
    pinterruptNode->drvPvt = pasynADImageInterrupt;
    pasynADImageInterrupt->pasynUser =
                        pasynManager->duplicateAsynUser(pasynUser, NULL, NULL);
    pasynADImageInterrupt->addr = addr;
    pasynADImageInterrupt->callback = callback;
    pasynADImageInterrupt->userPvt = userPvt;
    *registrarPvt = pinterruptNode;
    asynPrint(pasynUser,ASYN_TRACE_FLOW,
        "%s %d registerInterruptUser\n",portName,addr);
    return pasynManager->addInterruptUser(pasynUser,pinterruptNode);
}

static asynStatus cancelInterruptUser(void *drvPvt, asynUser *pasynUser,
    void *registrarPvt)
{
    interruptNode *pinterruptNode = (interruptNode *)registrarPvt;
    asynStatus    status;
    const char    *portName;
    int           addr;
    asynADImageInterrupt *pasynADImageInterrupt = 
                                (asynADImageInterrupt *)pinterruptNode->drvPvt;

    status = pasynManager->getPortName(pasynUser,&portName);
    if(status!=asynSuccess) return status;
    status = pasynManager->getAddr(pasynUser,&addr);
    if(status!=asynSuccess) return status;
    asynPrint(pasynUser,ASYN_TRACE_FLOW,
        "%s %d cancelInterruptUser\n",portName,addr);
    status = pasynManager->removeInterruptUser(pasynUser,pinterruptNode);
    pasynManager->freeAsynUser(pasynADImageInterrupt->pasynUser);
    pasynManager->memFree(pasynADImageInterrupt, sizeof(asynADImageInterrupt));
    return status;
}
