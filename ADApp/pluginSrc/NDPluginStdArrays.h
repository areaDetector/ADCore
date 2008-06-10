#ifndef NDPluginStdArrays_H
#define NDPluginStdArrays_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

typedef enum
{
    NDPluginStdArraysData           /* (asynXXXArray, r/w) Array data waveform */
        = NDPluginDriverLastParam,
    NDPluginStdArraysLastParam
} NDPluginStdArraysParam_t;

class NDPluginStdArrays : public NDPluginDriver {
public:
    NDPluginStdArrays(const char *portName, int queueSize, int blockingCallbacks, 
                      const char *NDArrayPort, int NDArrayAddr,
                      size_t maxMemory);

    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    virtual asynStatus readInt8Array(asynUser *pasynUser, epicsInt8 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus readInt16Array(asynUser *pasynUser, epicsInt16 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus readFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value,
                                        size_t nElements, size_t *nIn);
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                        const char **pptypeName, size_t *psize);

    /* These methods are just for this class */
    template <typename epicsType> asynStatus readArray(asynUser *pasynUser, epicsType *value, 
                                        size_t nElements, size_t *nIn, NDDataType_t outputType);
                                        
};

    
#endif
