/**
 * asynNDArrayDriver.cpp
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
/** This method copies an NDArray object from the asynNDArrayDriver to an NDArray pointer passed in by the caller.
  * The destination NDArray address is passed by the caller in the genericPointer argument. The caller
  * must allocate the memory for the array, and pass the size in NDArray->dataSize.
  * The method will limit the amount of data copied to the actual array size or the
  * input dataSize, whichever is smaller.
  * \param[in] pasynUser Used to obtain the addr for the NDArray to be copied from, and for asynTrace output.
  * \param[out] genericPointer Pointer to an NDArray. The NDArray must have been previously allocated by the caller.
  * The NDArray from the asynNDArrayDriver will be copied into the NDArray pointed to by genericPointer.
  */
asynStatus asynNDArrayDriver::readGenericPointer(asynUser *pasynUser, void *genericPointer)
{
    NDArray *pArray = (NDArray *)genericPointer;
    NDArray *myArray;
    NDArrayInfo_t arrayInfo;
    int addr;
    asynStatus status = asynSuccess;
    const char* functionName = "readNDArray";

    status = getAddress(pasynUser, functionName, &addr); if (status != asynSuccess) return(status);
    this->lock();
    myArray = this->pArrays[addr];
    if (!myArray) {
        epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                    "%s:%s: error, no valid array available, pData=%p",
                    driverName, functionName, pArray->pData);
        status = asynError;
    } else {
        this->pNDArrayPool->copy(myArray, pArray, 0);
        myArray->getInfo(&arrayInfo);
        if (arrayInfo.totalBytes > pArray->dataSize) arrayInfo.totalBytes = pArray->dataSize;
        memcpy(pArray->pData, myArray->pData, arrayInfo.totalBytes);
    }
    if (!status)
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: error, maxBytes=%d, data=%p\n",
              driverName, functionName, arrayInfo.totalBytes, pArray->pData);
    this->unlock();
    return status;
}

/** This method currently does nothing, but it should be implemented in this base class.
  * Derived classes can implement this method as required.
  * \param[in] pasynUser Used to obtain the addr for the NDArray to be copied to, and for asynTrace output.
  * \param[in] genericPointer Pointer to an NDArray. 
  * The NDArray pointed to by genericPointer will be copied into the NDArray in asynNDArrayDriver .
  */
asynStatus asynNDArrayDriver::writeGenericPointer(asynUser *pasynUser, void *genericPointer)
{
    asynStatus status = asynSuccess;

    this->lock();

    this->unlock();
    return status;
}

/** Report status of the driver.
  * This method calls the report function in the asynPortDriver base class. It then
  * calls the NDArrayPool->report() method if details >5.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >5 then NDArrayPool::report is called.
  */
void asynNDArrayDriver::report(FILE *fp, int details)
{
    asynPortDriver::report(fp, details);
    if (details > 5) {
        if (this->pNDArrayPool) this->pNDArrayPool->report(details);
    }
}


/** This is the constructor for the asynNDArrayDriver class.
  * portName, maxAddr, paramTableSize, interfaceMask, interruptMask, asynFlags, autoConnect, priority and stackSize
  * are simply passed to the asynPortDriver base class constructor. 
  * asynNDArray creates an NDArrayPool object to allocate NDArray
  * objects. maxBuffers and maxMemory are passed to the constructor for the NDArrayPool object.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxAddrIn The maximum  number of asyn addr addresses this driver supports. 1 is minimum.
  * \param[in] paramTableSize The number of parameters that this driver supports.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is allowed to allocate.
  *            Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is allowed to allocate.
  *            Set this to -1 to allow an unlimited amount of memory.
  * \param[in] interfaceMask The asyn interfaces that this driver supports.
  * \param[in] interruptMask The asyn interfaces that can generate interrupts (callbacks)
  * \param[in] asynFlags Flags when creating the asyn port driver.  Includes ASYN_CANBLOCK and ASYN_MULTIDEVICE.
  * \param[in] autoConnect The autoConnect flag for the asyn port driver.
  * \param[in] priority The thread priority for the asynPort driver thread if ASYN_CANBLOCK is set.
  * \param[in] stackSize The stack size for the asynPort driver thread if ASYN_CANBLOCK is set.
  */

asynNDArrayDriver::asynNDArrayDriver(const char *portName, int maxAddrIn, int paramTableSize, int maxBuffers,
                                     size_t maxMemory, int interfaceMask, int interruptMask,
                                     int asynFlags, int autoConnect, int priority, int stackSize)
    : asynPortDriver(portName, maxAddrIn, paramTableSize, interfaceMask, interruptMask,
                     asynFlags, autoConnect, priority, stackSize),
      pNDArrayPool(NULL)
{
    if ((maxBuffers != 0) || (maxMemory != 0)) this->pNDArrayPool = new NDArrayPool(maxBuffers, maxMemory);

    /* Allocate pArray pointer array */
    this->pArrays = (NDArray **)calloc(maxAddr, sizeof(NDArray *));
}

