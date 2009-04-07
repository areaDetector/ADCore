#ifndef asynNDArrayDriver_H
#define asynNDArrayDriver_H

#include "asynPortDriver.h"
#include "NDArray.h"
/**
  *  asynNDArrayDriver inherits from asynPortDriver. It implements the asynGenericPointer functions, assuming that these reference NDArray objects. This is the class from which both plugins and area detector drivers are indirectly derived.
  */
class asynNDArrayDriver : public asynPortDriver {
public:
    /**
       This is the constructor for the class. portName, maxAddr, paramTableSize, interfaceMask and interruptMask are simply passed to the asynPortDriver base class constructor. asynNDArray creates an NDArrayPool object to allocate NDArray objects. maxBuffers and maxMemory are passed to the constructor for the NDArrayPool object.
       \param portName describe me
       \param maxAddr describe me
       \param paramTableSize describe me
       \param maxBuffers describe me
       \param maxMemory describe me
       \param interfaceMask describe me
       \param interruptMask describe me
       \param asynFlags describe me
       \param autoConnect describe me
       \param priority describe me
       \param stackSize describe me
      */
    asynNDArrayDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
                      int interfaceMask, int interruptMask,
                      int asynFlags, int autoConnect, int priority, int stackSize);
    /**
       This method copies an NDArray object from the asynNDArrayDriver to an NDArray
       whose address is passed by the caller in the genericPointer argument. The caller
       must allocate the memory for the array, and pass the size in NDArray->dataSize.
       The method will limit the amount of data copied to the actual array size or the
       input dataSize, whichever is smaller.
       \param pasynUser describe me
       \param genericPointer describe me
     */
    virtual asynStatus readGenericPointer(asynUser *pasynUser, void *genericPointer);
	/**
       This method currently does nothing. Derived classes must implement this method
       as required.
       \param pasynUser describe me
       \param genericPointer describe me
	  */
    virtual asynStatus writeGenericPointer(asynUser *pasynUser, void *genericPointer);
 	/**
	   This method calls the report function in the asynPortDriver base class. It then
	   calls the NDArrayPool->report() method if details >5.
       \param fp describe me
       \param details describe me
 	  */
    virtual void report(FILE *fp, int details);

    /**
      Please describe me
      */
    NDArray **pArrays;
    /**
      Please describe me too
      */
    NDArrayPool *pNDArrayPool;
};

#endif
