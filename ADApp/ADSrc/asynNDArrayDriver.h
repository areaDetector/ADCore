#ifndef asynNDArrayDriver_H
#define asynNDArrayDriver_H

#include "asynPortDriver.h"
#include "NDArray.h"
/**
  *  This is the class from which both plugins and area detector drivers are indirectly derived.
  * asynNDArrayDriver inherits from asynPortDriver. It implements the asynGenericPointer functions, assuming
  * that these reference NDArray objects.
  */
class asynNDArrayDriver : public asynPortDriver {
public:
    asynNDArrayDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
                      int interfaceMask, int interruptMask,
                      int asynFlags, int autoConnect, int priority, int stackSize);
    virtual asynStatus readGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus writeGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual void report(FILE *fp, int details);

    NDArray **pArrays;             /**< Describe me */
    NDArrayPool *pNDArrayPool;     /**< Describe me too!!!! */
};

#endif
