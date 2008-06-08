#ifndef asynNDArrayBase_H
#define asynNDArrayBase_H

#include "asynPortDriver.h"
#include "NDArray.h"

class asynNDArrayBase : public asynPortDriver {
public:
    asynNDArrayBase(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
                    int interfaceMask, int interruptMask);
    virtual asynStatus readHandle(asynUser *pasynUser, void *handle);
    virtual asynStatus writeHandle(asynUser *pasynUser, void *handle);
    virtual void report(FILE *fp, int details);

    NDArray **pArrays;
    NDArrayPool *pNDArrayPool;
};

#endif
