/**
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
/** This method copies an NDArray object from the asynNDArrayDriver to an NDArray.
  * The destination NDArray address is passed by the caller in the genericPointer argument. The caller
  * must allocate the memory for the array, and pass the size in NDArray->dataSize.
  * The method will limit the amount of data copied to the actual array size or the
  * input dataSize, whichever is smaller.
  * \param pasynUser Source NDArray is contained in this asynUser Object.
  * \param genericPointer Destination Address where NDArray is copied.  The user is responsible for creating space.
  */
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

/** This method currently does nothing.
  * Derived classes must implement this method as required.
  * \param pasynUser describe me
  * \param genericPointer describe me
  */
asynStatus asynNDArrayDriver::writeGenericPointer(asynUser *pasynUser, void *genericPointer)
{
    asynStatus status = asynSuccess;

    epicsMutexLock(this->mutexId);

    epicsMutexUnlock(this->mutexId);
    return status;
}

/** Report status of the driver.
  *This method calls the report function in the asynPortDriver base class. It then
  * calls the NDArrayPool->report() method if details >5.
  * \param fp describe me
  * \param details describe me
  */
void asynNDArrayDriver::report(FILE *fp, int details)
{
    asynPortDriver::report(fp, details);
    if (details > 5) {
        if (this->pNDArrayPool) this->pNDArrayPool->report(details);
    }
}


/* Constructor */
/** This is the constructor for the class.
  * portName, maxAddr, paramTableSize, interfaceMask and interruptMask are simply passed to the
  * asynPortDriver base class constructor. asynNDArray creates an NDArrayPool object to allocate NDArray
  * objects. maxBuffers and maxMemory are passed to the constructor for the NDArrayPool object.
  * \param portName describe me
  * \param maxAddr describe me
  * \param paramTableSize describe me
  * \param maxBuffers describe me
  * \param maxMemory describe me
  * \param interfaceMask describe me
  * \param interruptMask describe me
  * \param asynFlags describe me
  * \param autoConnect describe me
  * \param priority describe me
  * \param stackSize describe me
  */

asynNDArrayDriver::asynNDArrayDriver(const char *portName, int maxAddrIn, int paramTableSize, int maxBuffers,
                                     size_t maxMemory, int interfaceMask, int interruptMask,
                                     int asynFlags, int autoConnect, int priority, int stackSize)
    : asynPortDriver(portName, maxAddrIn, paramTableSize, interfaceMask, interruptMask,
                     asynFlags, autoConnect, priority, stackSize),
      pNDArrayPool(NULL)
{
    if ((maxBuffers > 0) && (maxMemory > 0)) this->pNDArrayPool = new NDArrayPool(maxBuffers, maxMemory);

    /* Allocate pArray pointer array */
    this->pArrays = (NDArray **)calloc(maxAddr, sizeof(NDArray *));
}

