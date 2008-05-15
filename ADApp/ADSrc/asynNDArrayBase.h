#ifndef asynNDArrayBase_H
#define asynNDArrayBase_H

#include "asynParamBase.h"
#include "NDArray.h"

class asynNDArrayBase : public asynParamBase {
public:
    asynNDArrayBase(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory);
    virtual asynStatus readHandle(asynUser *pasynUser, void *handle);
    virtual asynStatus writeHandle(asynUser *pasynUser, void *handle);
    virtual void report(FILE *fp, int details);

    NDArray **pArrays;
    NDArrayPool *pNDArrayPool;
};

#endif
