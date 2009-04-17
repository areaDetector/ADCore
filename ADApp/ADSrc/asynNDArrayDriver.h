#ifndef asynNDArrayDriver_H
#define asynNDArrayDriver_H

#include "asynPortDriver.h"
#include "NDArray.h"
/** This is the class from which NDArray drivers are derived; implements the asynGenericPointer functions, assuming
  * that these reference NDArray objects. 
  * For areaDetector, both plugins and detector drivers are indirectly derived from this class.
  * asynNDArrayDriver inherits from asynPortDriver.
  */
class asynNDArrayDriver : public asynPortDriver {
public:
    asynNDArrayDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
                      int interfaceMask, int interruptMask,
                      int asynFlags, int autoConnect, int priority, int stackSize);
    virtual asynStatus readGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus writeGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual void report(FILE *fp, int details);

protected:
    NDArray **pArrays;             /**< An array of NDArray pointers used to store data in the driver */
    NDArrayPool *pNDArrayPool;     /**< An NDArrayPool object used to allocate and manipulate NDArray objects */
};

#endif
