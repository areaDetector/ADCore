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

paramList::paramList(int startVal, int nVals, asynStandardInterfaces *pasynInterfaces)
    : startVal(startVal), nVals(nVals), nFlags(0), pasynInterfaces(pasynInterfaces)
{
     vals = (paramVal *) calloc(nVals, sizeof(paramVal));
     flags = (int *) calloc(nVals, sizeof(int));
}

paramList::~paramList()
{
    free(vals);
    free(flags);
}

asynStatus paramList::setFlag(int index)
{
    asynStatus status = asynError;

    if (index >= 0 && index < this->nVals)
    {
        int i;
        /* See if we have already set the flag for this parameter */
        for (i=0; i<this->nFlags; i++) if (this->flags[i] == index) break;
        /* If not found add a flag */
        if (i == this->nFlags) this->flags[this->nFlags++] = index;
        status = asynSuccess;
    }
    return status;
}

asynStatus paramList::setInteger(int index, int value)
{
    asynStatus status = asynError;

    index -= this->startVal;
    if (index >= 0 && index < this->nVals)
    {
        if ( this->vals[index].type != paramInt ||
             this->vals[index].data.ival != value )
        {
            setFlag(index);
            this->vals[index].type = paramInt;
            this->vals[index].data.ival = value;
        }
        status = asynSuccess;
    }
    return status;
}

asynStatus paramList::setDouble(int index, double value)
{
    asynStatus status = asynError;

    index -= this->startVal;
    if (index >=0 && index < this->nVals)
    {
        if ( this->vals[index].type != paramDouble ||
             this->vals[index].data.dval != value )
        {
            setFlag(index);
            this->vals[index].type = paramDouble;
            this->vals[index].data.dval = value;
        }
        status = asynSuccess;
    }
    return status;
}

asynStatus paramList::setString(int index, const char *value)
{
    asynStatus status = asynError;

    index -= this->startVal;
    if (index >=0 && index < this->nVals)
    {
        if ( this->vals[index].type != paramString ||
             strcmp(this->vals[index].data.sval, value))
        {
            setFlag(index);
            this->vals[index].type = paramString;
            free(this->vals[index].data.sval);
            this->vals[index].data.sval = epicsStrDup(value);
        }
        status = asynSuccess;
    }
    return status;
}

asynStatus paramList::getInteger(int index, int *value)
{
    asynStatus status = asynError;

    index -= this->startVal;
    *value = 0;
    if (index >= 0 && index < this->nVals)
    {
        if (this->vals[index].type == paramInt) {
            *value = this->vals[index].data.ival;
            status = asynSuccess;
        }
    }
    return status;
}

asynStatus paramList::getDouble(int index, double *value)
{
    asynStatus status = asynError;

    index -= this->startVal;
    *value = 0.;
    if (index >= 0 && index < this->nVals)
    {
        if (this->vals[index].type == paramDouble) {
            *value = this->vals[index].data.dval;
            status = asynSuccess;
        }
    }
    return status;
}

asynStatus paramList::getString(int index, int maxChars, char *value)
{
    asynStatus status = asynError;

    index -= this->startVal;
    value[0]=0;
    if (index >= 0 && index < this->nVals)
    {
        if (this->vals[index].type == paramString) {
            strncpy(value, this->vals[index].data.sval, maxChars);
            status = asynSuccess;
        }
    }
    return status;
}

asynStatus paramList::intCallback(int command, int addr, int value)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    asynStandardInterfaces *pInterfaces = this->pasynInterfaces;
    int address;

    /* Pass int32 interrupts */
    if (!pInterfaces->int32InterruptPvt) return(asynError);
    pasynManager->interruptStart(pInterfaces->int32InterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynInt32Interrupt *pInterrupt = (asynInt32Interrupt *) pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &address);
        if ((command == pInterrupt->pasynUser->reason) &&
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 value);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pInterfaces->int32InterruptPvt);
    return(asynSuccess);
}

asynStatus paramList::doubleCallback(int command, int addr, double value)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    asynStandardInterfaces *pInterfaces = this->pasynInterfaces;
    int address;

    /* Pass float64 interrupts */
    if (!pInterfaces->float64InterruptPvt) return(asynError);
    pasynManager->interruptStart(pInterfaces->float64InterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynFloat64Interrupt *pInterrupt = (asynFloat64Interrupt *) pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &address);
        if ((command == pInterrupt->pasynUser->reason) &&
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 value);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pInterfaces->float64InterruptPvt);
    return(asynSuccess);
}

asynStatus paramList::stringCallback(int command, int addr, char *value)
{
    ELLLIST *pclientList;
    interruptNode *pnode;
    asynStandardInterfaces *pInterfaces = this->pasynInterfaces;
    int address;

    /* Pass octet interrupts */
    if (!pInterfaces->octetInterruptPvt) return(asynError);
    pasynManager->interruptStart(pInterfaces->octetInterruptPvt, &pclientList);
    pnode = (interruptNode *)ellFirst(pclientList);
    while (pnode) {
        asynOctetInterrupt *pInterrupt = (asynOctetInterrupt *) pnode->drvPvt;
        pasynManager->getAddr(pInterrupt->pasynUser, &address);
        if ((command == pInterrupt->pasynUser->reason) &&
            (address == addr)) {
            pInterrupt->callback(pInterrupt->userPvt, 
                                 pInterrupt->pasynUser,
                                 value, strlen(value), ASYN_EOM_END);
        }
        pnode = (interruptNode *)ellNext(&pnode->node);
    }
    pasynManager->interruptEnd(pInterfaces->octetInterruptPvt);
    return(asynSuccess);
}

asynStatus paramList::callCallbacks(int addr)
{
    int i, index;
    int command;
    asynStatus status = asynSuccess;

    for (i = 0; i < this->nFlags; i++)
    {
        index = this->flags[i];
        command = index + this->startVal;
        switch(this->vals[index].type) {
            case paramUndef:
                break;
            case paramInt:
                status = intCallback(command, addr, this->vals[index].data.ival);
                break;
            case paramDouble:
                status = doubleCallback(command, addr, this->vals[index].data.dval);
                break;
            case paramString:
                status = stringCallback(command, addr, this->vals[index].data.sval);
                break;
        }
    }
    this->nFlags=0;
    return(status);
}

asynStatus paramList::callCallbacks()
{
    return(callCallbacks(0));
}

void paramList::report()
{
    int i;

    printf( "Number of parameters is: %d\n", this->nVals );
    for (i=0; i<this->nVals; i++)
    {
        switch (this->vals[i].type)
        {
            case paramDouble:
                printf( "Parameter %d is a double, value %f\n", i+this->startVal, this->vals[i].data.dval );
                break;
            case paramInt:
                printf( "Parameter %d is an integer, value %d\n", i+this->startVal, this->vals[i].data.ival );
                break;
            case paramString:
                printf( "Parameter %d is a string, value %s\n", i+this->startVal, this->vals[i].data.sval );
                break;
            default:
                printf( "Parameter %d is undefined\n", i+this->startVal );
                break;
        }
    }
}

asynStatus asynParamBase::setIntegerParam(int index, int value)
{
    return this->params[0]->setInteger(index, value);
}

asynStatus asynParamBase::setIntegerParam(int list, int index, int value)
{
    return this->params[list]->setInteger(index, value);
}

asynStatus asynParamBase::setDoubleParam(int index, double value)
{
    return this->params[0]->setDouble(index, value);
}

asynStatus asynParamBase::setDoubleParam(int list, int index, double value)
{
    return this->params[list]->setDouble(index, value);
}

asynStatus asynParamBase::setStringParam(int index, const char *value)
{
    return this->params[0]->setString(index, value);
}

asynStatus asynParamBase::setStringParam(int list, int index, const char *value)
{
    return this->params[list]->setString(index, value);
}


asynStatus asynParamBase::getIntegerParam(int index, int *value)
{
    return this->params[0]->getInteger(index, value);
}

asynStatus asynParamBase::getIntegerParam(int list, int index, int *value)
{
    return this->params[list]->getInteger(index, value);
}

asynStatus asynParamBase::getDoubleParam(int index, double *value)
{
    return this->params[0]->getDouble(index, value);
}

asynStatus asynParamBase::getDoubleParam(int list, int index, double *value)
{
    return this->params[list]->getDouble(index, value);
}

asynStatus asynParamBase::getStringParam(int index, int maxChars, char *value)
{
    return this->params[0]->getString(index, maxChars, value);
}

asynStatus asynParamBase::getStringParam(int list, int index, int maxChars, char *value)
{
    return this->params[list]->getString(index, maxChars, value);
}

asynStatus asynParamBase::callParamCallbacks()
{
    return this->params[0]->callCallbacks();
}

asynStatus asynParamBase::callParamCallbacks(int list, int addr)
{
    return this->params[list]->callCallbacks(addr);
}

void asynParamBase::reportParams()
{
    int i;
    for (i=0; i<this->maxAddr; i++) this->params[i]->report();
}


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
    status = (asynStatus) getIntegerParam(addr, function, value);
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
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                "%s:writeInt32 not implemented", driverName);
    return(asynError);
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
    status = (asynStatus) getDoubleParam(addr, function, value);
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
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                "%s:writeFloat64 not implemented", driverName);
    return(asynError);
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
    status = (asynStatus)getStringParam(addr, function, maxChars, value);
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
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                "%s:writeOctet not implemented", driverName);
    return(asynError);
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
            this->params[addr]->report();
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

asynParamBase::asynParamBase(const char *portName, int maxAddr, int paramTableSize, int interfaceMask, int interruptMask)
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

    interfaceMask |= asynCommonMask;  /* Always need the asynCommon interface */
     /* Set addresses of asyn interfaces */
    if (interfaceMask & asynCommonMask)       pInterfaces->common.pinterface        = (void *)&ifaceCommon;
    if (interfaceMask & asynDrvUserMask)      pInterfaces->drvUser.pinterface       = (void *)&ifaceDrvUser;
    if (interfaceMask & asynInt32Mask)        pInterfaces->int32.pinterface         = (void *)&ifaceInt32;
    if (interfaceMask & asynFloat64Mask)      pInterfaces->float64.pinterface       = (void *)&ifaceFloat64;
    if (interfaceMask & asynOctetMask)        pInterfaces->octet.pinterface         = (void *)&ifaceOctet;
    if (interfaceMask & asynInt8ArrayMask)    pInterfaces->int8Array.pinterface     = (void *)&ifaceInt8Array;
    if (interfaceMask & asynInt16ArrayMask)   pInterfaces->int16Array.pinterface    = (void *)&ifaceInt16Array;
    if (interfaceMask & asynInt32ArrayMask)   pInterfaces->int32Array.pinterface    = (void *)&ifaceInt32Array;
    if (interfaceMask & asynFloat32ArrayMask) pInterfaces->float32Array.pinterface  = (void *)&ifaceFloat32Array;
    if (interfaceMask & asynFloat64ArrayMask) pInterfaces->float64Array.pinterface  = (void *)&ifaceFloat64Array;
    if (interfaceMask & asynHandleMask)       pInterfaces->handle.pinterface        = (void *)&ifaceHandle;

    /* Define which interfaces can generate interrupts */
    if (interruptMask & asynInt32Mask)        pInterfaces->int32CanInterrupt        = 1;
    if (interruptMask & asynFloat64Mask)      pInterfaces->float64CanInterrupt      = 1;
    if (interruptMask & asynOctetMask)        pInterfaces->octetCanInterrupt        = 1;
    if (interruptMask & asynInt8ArrayMask)    pInterfaces->int8ArrayCanInterrupt    = 1;
    if (interruptMask & asynInt16ArrayMask)   pInterfaces->int16ArrayCanInterrupt   = 1;
    if (interruptMask & asynInt32ArrayMask)   pInterfaces->int32ArrayCanInterrupt   = 1;
    if (interruptMask & asynFloat32ArrayMask) pInterfaces->float32ArrayCanInterrupt = 1;
    if (interruptMask & asynFloat64ArrayMask) pInterfaces->float64ArrayCanInterrupt = 1;
    if (interruptMask & asynHandleMask)       pInterfaces->handleCanInterrupt       = 1;

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

    /* Allocate space for the parameter objects */
    this->params = (paramList **) calloc(maxAddr, sizeof(paramList *));    
    /* Initialize the parameter library */
    for (addr=0; addr<maxAddr; addr++) {
        this->params[addr] = new paramList(0, paramTableSize, &this->asynStdInterfaces);
    }
}

