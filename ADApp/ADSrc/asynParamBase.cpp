/*
 * asynParamBase.c
 * 
 * Base class that implements methods for asynStandardInterfaces with a parameter library.
 *
 * Author: Mark Rivers
 *
 * Created April 27, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <cantProceed.h>

#include <asynStandardInterfaces.h>

#include "asynParamBase.h"

static char *driverName = "asynParamBase";

template <typename epicsType> 
asynStatus readArray(asynUser *pasynUser, epicsType *value, size_t nElements, size_t *nIn)
{
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                "%s:readArray not implemented", driverName);
    return(asynError);
}

template <typename epicsType> 
asynStatus writeArray(asynUser *pasynUser, epicsType *value, size_t nElements)
{
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                "%s:writeArray not implemented", driverName);
    return(asynError);
}


template <typename epicsType, typename interruptType> 
asynStatus doCallbacksArray(epicsType *value, size_t nElements,
                            int reason, int address, void *interruptPvt)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    int addr;

    pasynManager->interruptStart(interruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        interruptType *pInterrupt = (interruptType *)pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &addr);
        if ((pInterrupt->pasynUser->reason == reason) &&
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt,
                                 pInterrupt->pasynUser,
                                 value, nElements);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(interruptPvt);
    return(asynSuccess);
}

template <typename interruptType> 
void reportInterrupt(FILE *fp, void *interruptPvt, const char *interruptTypeString)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    
    if (interruptPvt) {
        pasynManager->interruptStart(interruptPvt, &pclientList);
        pnode = (interruptNode *)ellFirst(pclientList);
        while (pnode) {
            interruptType *pInterrupt = (interruptType *)pnode->drvPvt;
            fprintf(fp, "    %s callback client address=%p, addr=%d, reason=%d, userPvt=%p\n",
                    interruptTypeString, pInterrupt->callback, pInterrupt->addr,
                    pInterrupt->pasynUser->reason, pInterrupt->userPvt);
            pnode = (interruptNode *)ellNext(&pnode->node);
        }
        pasynManager->interruptEnd(interruptPvt);
    }
}

asynStatus asynParamBase::getAddress(asynUser *pasynUser, const char *functionName, int *address) 
{
    pasynManager->getAddr(pasynUser, address);
    if (*address > this->maxAddr-1) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
            "%s:%s: invalid address=%d, max=%d",
            driverName, functionName, *address, this->maxAddr-1);
        return(asynError);
    }
    return(asynSuccess);
}


/* asynInt32 interface methods */
static asynStatus readInt32(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 *value)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->readInt32(pasynUser, value));
}

asynStatus asynParamBase::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    static char *functionName = "readInt32";
    
    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);
    
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = (asynStatus) ADParam->getInteger(this->params[addr], function, value);
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, *value);
    else        
        asynPrint(this->pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, *value);
    epicsMutexUnlock(this->mutexId);
    return(status);
}

static asynStatus writeInt32(void *drvPvt, asynUser *pasynUser, 
                            epicsInt32 value)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->writeInt32(pasynUser, value));
}

asynStatus asynParamBase::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    const char* functionName = "writeInt32";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);

    /* Set the parameter in the parameter library. */
    status = (asynStatus) ADParam->setInteger(this->params[addr], function, value);
    /* Set the readback (N+1) entry in the parameter library too */
    status = (asynStatus) ADParam->setInteger(this->params[addr], function+1, value);

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus) ADParam->callCallbacksAddr(this->params[addr], addr);
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%d", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%d\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}

static asynStatus getBounds(void *drvPvt, asynUser *pasynUser,
                            epicsInt32 *low, epicsInt32 *high)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->getBounds(pasynUser, low, high));
}

asynStatus asynParamBase::getBounds(asynUser *pasynUser,
                                   epicsInt32 *low, epicsInt32 *high)
{
    /* This is only needed for the asynInt32 interface when the device uses raw units.
       Our interface is using engineering units. */
    *low = 0;
    *high = 65535;
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s::getBounds,low=%d, high=%d\n", 
              driverName, *low, *high);
    return(asynSuccess);
}


/* asynFloat64 interface methods */
static asynStatus readFloat64(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 *value)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->readFloat64(pasynUser, value));
}

asynStatus asynParamBase::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    static char *functionName = "readFloat64";
    
    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = (asynStatus) ADParam->getDouble(this->params[addr], function, value);
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%f", 
                  driverName, functionName, status, function, *value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, *value);
    epicsMutexUnlock(this->mutexId);
    return(status);
}

static asynStatus writeFloat64(void *drvPvt, asynUser *pasynUser,
                              epicsFloat64 value)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->writeFloat64(pasynUser, value));
}

asynStatus asynParamBase::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    const char* functionName = "writeFloat64";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);

    /* Set the parameter in the parameter library. */
    status = (asynStatus)ADParam->setDouble(this->params[addr], function, value);
    /* Set the readback (N+1) entry in the parameter library too */
    status = (asynStatus) ADParam->setDouble(this->params[addr], function+1, value);

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacksAddr(this->params[addr], addr);
    
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%f", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    epicsMutexUnlock(this->mutexId);
    return status;
}


/* asynOctet interface methods */
static asynStatus readOctet(void *drvPvt, asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->readOctet(pasynUser, value, maxChars, nActual, eomReason));
}

asynStatus asynParamBase::readOctet(asynUser *pasynUser,
                            char *value, size_t maxChars, size_t *nActual,
                            int *eomReason)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    const char *functionName = "readOctet";
   
    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);
    /* We just read the current value of the parameter from the parameter library.
     * Those values are updated whenever anything could cause them to change */
    status = (asynStatus)ADParam->getString(this->params[addr], function, maxChars, value);
    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%s", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *eomReason = ASYN_EOM_END;
    *nActual = strlen(value);
    epicsMutexUnlock(this->mutexId);
    return(status);
}

static asynStatus writeOctet(void *drvPvt, asynUser *pasynUser,
                              const char *value, size_t maxChars, size_t *nActual)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->writeOctet(pasynUser, value, maxChars, nActual));
}

asynStatus asynParamBase::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
    int function = pasynUser->reason;
    int addr=0;
    asynStatus status = asynSuccess;
    const char *functionName = "writeOctet";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);
    /* Set the parameter in the parameter library. */
    status = (asynStatus)ADParam->setString(this->params[addr], function, (char *)value);
    /* Set the readback (N+1) entry in the parameter library too */
    status = (asynStatus) ADParam->setString(this->params[addr], function+1, (char *)value);

    /* Do callbacks so higher layers see any changes */
    status = (asynStatus)ADParam->callCallbacksAddr(this->params[addr], addr);

    if (status) 
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                  "%s:%s: status=%d, function=%d, value=%s", 
                  driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:writeOctet: function=%d, value=%s\n", 
              driverName, functionName, function, value);
    *nActual = nChars;
    epicsMutexUnlock(this->mutexId);
    return status;
}


/* asynInt8Array interface methods */
static asynStatus readInt8Array(void *drvPvt, asynUser *pasynUser, epicsInt8 *value,
                                size_t nElements, size_t *nIn)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->readInt8Array(pasynUser, value, nElements, nIn));
}

asynStatus asynParamBase::readInt8Array(asynUser *pasynUser, epicsInt8 *value,
                                size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt8>(pasynUser, value, nElements, nIn));
}

static asynStatus writeInt8Array(void *drvPvt, asynUser *pasynUser, epicsInt8 *value,
                                size_t nElements)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->writeInt8Array(pasynUser, value, nElements));
}

asynStatus asynParamBase::writeInt8Array(asynUser *pasynUser, epicsInt8 *value,
                                size_t nElements)
{
    return(writeArray<epicsInt8>(pasynUser, value, nElements));
}

asynStatus asynParamBase::doCallbacksInt8Array(epicsInt8 *value,
                                size_t nElements, int reason, int addr)
{
    return(doCallbacksArray<epicsInt8, asynInt8ArrayInterrupt>(value, nElements, reason, addr,
                                        this->asynStdInterfaces.int8ArrayInterruptPvt));
}


/* asynInt16Array interface methods */
static asynStatus readInt16Array(void *drvPvt, asynUser *pasynUser, epicsInt16 *value,
                                size_t nElements, size_t *nIn)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->readInt16Array(pasynUser, value, nElements, nIn));
}

asynStatus asynParamBase::readInt16Array(asynUser *pasynUser, epicsInt16 *value,
                                size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt16>(pasynUser, value, nElements, nIn));
}

static asynStatus writeInt16Array(void *drvPvt, asynUser *pasynUser, epicsInt16 *value,
                                size_t nElements)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->writeInt16Array(pasynUser, value, nElements));
}

asynStatus asynParamBase::writeInt16Array(asynUser *pasynUser, epicsInt16 *value,
                                size_t nElements)
{
    return(writeArray<epicsInt16>(pasynUser, value, nElements));
}

asynStatus asynParamBase::doCallbacksInt16Array(epicsInt16 *value,
                                size_t nElements, int reason, int addr)
{
    return(doCallbacksArray<epicsInt16, asynInt16ArrayInterrupt>(value, nElements, reason, addr,
                                        this->asynStdInterfaces.int16ArrayInterruptPvt));
}


/* asynInt32Array interface methods */
static asynStatus readInt32Array(void *drvPvt, asynUser *pasynUser, epicsInt32 *value,
                                size_t nElements, size_t *nIn)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->readInt32Array(pasynUser, value, nElements, nIn));
}

asynStatus asynParamBase::readInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                size_t nElements, size_t *nIn)
{
    return(readArray<epicsInt32>(pasynUser, value, nElements, nIn));
}

static asynStatus writeInt32Array(void *drvPvt, asynUser *pasynUser, epicsInt32 *value,
                                size_t nElements)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->writeInt32Array(pasynUser, value, nElements));
}

asynStatus asynParamBase::writeInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                size_t nElements)
{
    return(writeArray<epicsInt32>(pasynUser, value, nElements));
}

asynStatus asynParamBase::doCallbacksInt32Array(epicsInt32 *value,
                                size_t nElements, int reason, int addr)
{
    return(doCallbacksArray<epicsInt32, asynInt32ArrayInterrupt>(value, nElements, reason, addr,
                                        this->asynStdInterfaces.int32ArrayInterruptPvt));
}


/* asynFloat32Array interface methods */
static asynStatus readFloat32Array(void *drvPvt, asynUser *pasynUser, epicsFloat32 *value,
                                size_t nElements, size_t *nIn)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->readFloat32Array(pasynUser, value, nElements, nIn));
}

asynStatus asynParamBase::readFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
                                size_t nElements, size_t *nIn)
{
    return(readArray<epicsFloat32>(pasynUser, value, nElements, nIn));
}

static asynStatus writeFloat32Array(void *drvPvt, asynUser *pasynUser, epicsFloat32 *value,
                                size_t nElements)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->writeFloat32Array(pasynUser, value, nElements));
}

asynStatus asynParamBase::writeFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
                                size_t nElements)
{
    return(writeArray<epicsFloat32>(pasynUser, value, nElements));
}

asynStatus asynParamBase::doCallbacksFloat32Array(epicsFloat32 *value,
                                size_t nElements, int reason, int addr)
{
    return(doCallbacksArray<epicsFloat32, asynFloat32ArrayInterrupt>(value, nElements, reason, addr,
                                        this->asynStdInterfaces.float32ArrayInterruptPvt));
}


/* asynFloat64Array interface methods */
static asynStatus readFloat64Array(void *drvPvt, asynUser *pasynUser, epicsFloat64 *value,
                                size_t nElements, size_t *nIn)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->readFloat64Array(pasynUser, value, nElements, nIn));
}

asynStatus asynParamBase::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value,
                                size_t nElements, size_t *nIn)
{
    return(readArray<epicsFloat64>(pasynUser, value, nElements, nIn));
}

static asynStatus writeFloat64Array(void *drvPvt, asynUser *pasynUser, epicsFloat64 *value,
                                size_t nElements)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->writeFloat64Array(pasynUser, value, nElements));
}

asynStatus asynParamBase::writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value,
                                size_t nElements)
{
    return(writeArray<epicsFloat64>(pasynUser, value, nElements));
}

asynStatus asynParamBase::doCallbacksFloat64Array(epicsFloat64 *value,
                                size_t nElements, int reason, int addr)
{
    return(doCallbacksArray<epicsFloat64, asynFloat64ArrayInterrupt>(value, nElements, reason, addr,
                                        this->asynStdInterfaces.float64ArrayInterruptPvt));
}

/* asynHandle interface methods */
static asynStatus readHandle(void *drvPvt, asynUser *pasynUser, void *handle)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->readHandle(pasynUser, handle));
}

asynStatus asynParamBase::readHandle(asynUser *pasynUser, void *handle)
{
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                "%s:readHandle not implemented", driverName);
    return(asynError);
}

static asynStatus writeHandle(void *drvPvt, asynUser *pasynUser, void *handle)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->writeHandle(pasynUser, handle));
}

asynStatus asynParamBase::writeHandle(asynUser *pasynUser, void *handle)
{
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                "%s:writeHandle not implemented", driverName);
    return(asynError);
}


asynStatus asynParamBase::doCallbacksHandle(void *handle, int reason, int address)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    int addr;

    pasynManager->interruptStart(this->asynStdInterfaces.handleInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynHandleInterrupt *pInterrupt = (asynHandleInterrupt *)pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &addr);
        if ((pInterrupt->pasynUser->reason == reason) &&
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt,
                                 pInterrupt->pasynUser,
                                 handle);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(this->asynStdInterfaces.handleInterruptPvt);
    return(asynSuccess);
}



asynStatus asynParamBase::findParam(asynParamString_t *paramTable, int numParams, 
                                    const char *paramName, int *param)
{
    int i;
    for (i=0; i < numParams; i++) {
        if (epicsStrCaseCmp(paramName, paramTable[i].paramString) == 0) {
            *param = paramTable[i].param;
            return(asynSuccess);
        }
    }
    return(asynError);
}

/* asynDrvUser interface methods */
static asynStatus drvUserCreate(void *drvPvt, asynUser *pasynUser,
                                 const char *drvInfo, 
                                 const char **pptypeName, size_t *psize)
{ 
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->drvUserCreate(pasynUser, drvInfo, pptypeName, psize));
}

asynStatus asynParamBase::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    static char *functionName = "drvUserCreate";
    
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s: entered", driverName, functionName);

    return(asynSuccess);

}

    
static asynStatus drvUserGetType(void *drvPvt, asynUser *pasynUser,
                                 const char **pptypeName, size_t *psize)
{ 
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->drvUserGetType(pasynUser, pptypeName, psize));
}

asynStatus asynParamBase::drvUserGetType(asynUser *pasynUser,
                                        const char **pptypeName, size_t *psize)
{
    /* This is not currently supported, because we can't get the strings for driver-specific commands */
    static char *functionName = "drvUserGetType";

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s: entered", driverName, functionName);

    *pptypeName = NULL;
    *psize = 0;
    return(asynError);
}

static asynStatus drvUserDestroy(void *drvPvt, asynUser *pasynUser)
{ 
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->drvUserDestroy(pasynUser));
}

asynStatus asynParamBase::drvUserDestroy(asynUser *pasynUser)
{
    static char *functionName = "drvUserDestroy";

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s: this=%p, pasynUser=%p\n",
              driverName, functionName, this, pasynUser);

    return(asynSuccess);
}


/* asynCommon interface methods */

static void report(void *drvPvt, FILE *fp, int details)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->report(fp, details));
}


void asynParamBase::report(FILE *fp, int details)
{
    int addr;
    asynStandardInterfaces *pInterfaces = &this->asynStdInterfaces;

    fprintf(fp, "Port: %s\n", this->portName);
    if (details >= 1) {
        /* Report interrupt clients */
        reportInterrupt<asynInt32Interrupt>       (fp, pInterfaces->int32InterruptPvt,        "int32");
        reportInterrupt<asynFloat64Interrupt>     (fp, pInterfaces->float64InterruptPvt,      "float64");
        reportInterrupt<asynOctetInterrupt>       (fp, pInterfaces->octetInterruptPvt,        "octet");
        reportInterrupt<asynInt8ArrayInterrupt>   (fp, pInterfaces->int8ArrayInterruptPvt,    "int8Array");
        reportInterrupt<asynInt16ArrayInterrupt>  (fp, pInterfaces->int16ArrayInterruptPvt,   "int16Array");
        reportInterrupt<asynInt32ArrayInterrupt>  (fp, pInterfaces->int32ArrayInterruptPvt,   "int32Array");
        reportInterrupt<asynFloat32ArrayInterrupt>(fp, pInterfaces->float32ArrayInterruptPvt, "float32Array");
        reportInterrupt<asynFloat64ArrayInterrupt>(fp, pInterfaces->float64ArrayInterruptPvt, "float64Array");
        reportInterrupt<asynHandleInterrupt>      (fp, pInterfaces->handleInterruptPvt,       "handle");
    }
    if (details > 5) {
        for (addr=0; addr<this->maxAddr; addr++) {
            ADParam->dump(this->params[addr]);
        }
    }
}

static asynStatus connect(void *drvPvt, asynUser *pasynUser)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->connect(pasynUser));
}

asynStatus asynParamBase::connect(asynUser *pasynUser)
{
    static char *functionName = "connect";
    
    pasynManager->exceptionConnect(pasynUser);
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s:, pasynUser=%p\n", 
              driverName, functionName, pasynUser);
    return(asynSuccess);
}


static asynStatus disconnect(void *drvPvt, asynUser *pasynUser)
{
    asynParamBase *pPvt = (asynParamBase *)drvPvt;
    
    return(pPvt->disconnect(pasynUser));
}

asynStatus asynParamBase::disconnect(asynUser *pasynUser)
{
    static char *functionName = "disconnect";
    
    pasynManager->exceptionDisconnect(pasynUser);
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s:, pasynUser=%p\n", 
              driverName, functionName, pasynUser);
    return(asynSuccess);
}


/* Structures with function pointers for each of the asyn interfaces */
static asynCommon ifaceCommon = {
    report,
    connect,
    disconnect
};

static asynInt32 ifaceInt32 = {
    writeInt32,
    readInt32,
    getBounds
};

static asynFloat64 ifaceFloat64 = {
    writeFloat64,
    readFloat64
};

static asynOctet ifaceOctet = {
    writeOctet,
    NULL,
    readOctet,
};

static asynInt8Array ifaceInt8Array = {
    writeInt8Array,
    readInt8Array
};

static asynInt16Array ifaceInt16Array = {
    writeInt16Array,
    readInt16Array
};

static asynInt32Array ifaceInt32Array = {
    writeInt32Array,
    readInt32Array
};

static asynFloat32Array ifaceFloat32Array = {
    writeFloat32Array,
    readFloat32Array
};

static asynFloat64Array ifaceFloat64Array = {
    writeFloat64Array,
    readFloat64Array
};

static asynHandle ifaceHandle = {
    writeHandle,
    readHandle
};

static asynDrvUser ifaceDrvUser = {
    drvUserCreate,
    drvUserGetType,
    drvUserDestroy
};


/* Constructor */

asynParamBase::asynParamBase(const char *portName, int maxAddr, int paramTableSize)
    : maxAddr(maxAddr)
{
    asynStatus status;
    char *functionName = "asynParamBase";
    asynStandardInterfaces *pInterfaces;
    int addr;

    /* Initialize some members to 0 */
    pInterfaces = &this->asynStdInterfaces;
    memset(pInterfaces, 0, sizeof(asynStdInterfaces));
       
    this->portName = epicsStrDup(portName);

    status = pasynManager->registerPort(portName,
                                        ASYN_MULTIDEVICE | ASYN_CANBLOCK,
                                        1,  /*  autoconnect */
                                        0,  /* medium priority */
                                        0); /* default stack size */
    if (status != asynSuccess) {
        printf("%s:%s: ERROR: Can't register port\n", driverName, functionName);
    }

    /* Create asynUser for debugging and for standardInterfacesBase */
    this->pasynUser = pasynManager->createAsynUser(0, 0);

     /* Set addresses of asyn interfaces */
    pInterfaces->common.pinterface        = (void *)&ifaceCommon;
    pInterfaces->drvUser.pinterface       = (void *)&ifaceDrvUser;
    pInterfaces->int32.pinterface         = (void *)&ifaceInt32;
    pInterfaces->float64.pinterface       = (void *)&ifaceFloat64;
    pInterfaces->octet.pinterface         = (void *)&ifaceOctet;
    pInterfaces->int8Array.pinterface     = (void *)&ifaceInt8Array;
    pInterfaces->int16Array.pinterface    = (void *)&ifaceInt16Array;
    pInterfaces->int32Array.pinterface    = (void *)&ifaceInt32Array;
    pInterfaces->float32Array.pinterface  = (void *)&ifaceFloat32Array;
    pInterfaces->float64Array.pinterface  = (void *)&ifaceFloat64Array;
    pInterfaces->handle.pinterface        = (void *)&ifaceHandle;

    /* Define which interfaces can generate interrupts */
    pInterfaces->int32CanInterrupt        = 1;
    pInterfaces->float64CanInterrupt      = 1;
    pInterfaces->octetCanInterrupt        = 1;
    pInterfaces->int8ArrayCanInterrupt    = 1;
    pInterfaces->int16ArrayCanInterrupt   = 1;
    pInterfaces->int32ArrayCanInterrupt   = 1;
    pInterfaces->float32ArrayCanInterrupt = 1;
    pInterfaces->float64ArrayCanInterrupt = 1;
    pInterfaces->handleCanInterrupt       = 1;

    status = pasynStandardInterfacesBase->initialize(portName, pInterfaces,
                                                     this->pasynUser, this);
    if (status != asynSuccess) {
        printf("%s:%s ERROR: Can't register interfaces: %s.\n",
               driverName, functionName, this->pasynUser->errorMessage);
        return;
    }

    /* Connect to our device for asynTrace */
    status = pasynManager->connectDevice(this->pasynUser, portName, 0);
    if (status != asynSuccess) {
        printf("%s:%s:, connectDevice failed\n", driverName, functionName);
        return;
    }

    /* Create the epicsMutex for locking access to data structures from other threads */
    this->mutexId = epicsMutexCreate();
    if (!this->mutexId) {
        printf("%s::%s epicsMutexCreate failure\n", driverName, functionName);
        return;
    }
    
    /* Allocate params pointer array */
    this->params = (PARAMS *)calloc(maxAddr, sizeof(PARAMS));

    /* Initialize the parameter library */
    for (addr=0; addr<maxAddr; addr++) {
        this->params[addr] = ADParam->create(0, paramTableSize, &this->asynStdInterfaces);
        if (!this->params[addr]) {
            printf("%s:%s: unable to create parameter library addr=%d\n", 
                driverName, functionName, addr);
            return;
        }
    }
}

