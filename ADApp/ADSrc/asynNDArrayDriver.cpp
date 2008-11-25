/*
 * asynNDArrayDriver.c
 * 
 * Base class that implements methods for asynStandardInterfaces with a parameter library.
 *
 * Author: Mark Rivers
 *
 * Created May 11, 2008
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <cantProceed.h>

#include "asynNDArrayDriver.h"

static const char *driverName = "asynNDArrayDriver";

/* asynGenericPointer interface methods */
asynStatus asynNDArrayDriver::readGenericPointer(asynUser *pasynUser, void *genericPointer)
{
    NDArray *pArray = (NDArray *)genericPointer;
    NDArray *myArray;
    int addr=0;
    NDArrayInfo_t arrayInfo;
    asynStatus status = asynSuccess;
    const char* functionName = "readNDArray";
    
    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    epicsMutexLock(this->mutexId);
    myArray = this->pArrays[addr];
    if (!myArray) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize, 
                    "%s:%s: error, no valid array available, pData=%p", 
                    driverName, functionName, pArray->pData);
        status = asynError;
    } else {
        pArray->ndims = myArray->ndims;
        memcpy(pArray->dims, myArray->dims, sizeof(pArray->dims));
        pArray->dataType = myArray->dataType;
        myArray->getInfo(&arrayInfo);
        if (arrayInfo.totalBytes > pArray->dataSize) arrayInfo.totalBytes = pArray->dataSize;
        memcpy(pArray->pData, myArray->pData, arrayInfo.totalBytes);
    }
    if (!status) 
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: error, maxBytes=%d, data=%p\n", 
              driverName, functionName, arrayInfo.totalBytes, pArray->pData);
    epicsMutexUnlock(this->mutexId);
    return status;
}

asynStatus asynNDArrayDriver::writeGenericPointer(asynUser *pasynUser, void *genericPointer)
{
    asynStatus status = asynSuccess;
    
    epicsMutexLock(this->mutexId);
    
    epicsMutexUnlock(this->mutexId);
    return status;
}


void asynNDArrayDriver::report(FILE *fp, int details)
{
    asynPortDriver::report(fp, details);
    if (details > 5) {
        if (this->pNDArrayPool) this->pNDArrayPool->report(details);
    }
}


/* Constructor */

asynNDArrayDriver::asynNDArrayDriver(const char *portName, int maxAddrIn, int paramTableSize, int maxBuffers, 
                                     size_t maxMemory, int interfaceMask, int interruptMask)
    : asynPortDriver(portName, maxAddrIn, paramTableSize, interfaceMask, interruptMask), pNDArrayPool(NULL)
{
    if ((maxBuffers > 0) && (maxMemory > 0)) this->pNDArrayPool = new NDArrayPool(maxBuffers, maxMemory);

    /* Allocate pArray pointer array */
    this->pArrays = (NDArray **)calloc(maxAddr, sizeof(NDArray *));
}

