#ifndef NDPluginStdArrays_H
#define NDPluginStdArrays_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/** Enums for plugin-specific parameters. */
typedef enum
{
    NDPluginStdArraysData           /* (asynXXXArray, r/w) Array data waveform */
        = NDPluginDriverLastParam,
    NDPluginStdArraysLastParam
} NDPluginStdArraysParam_t;

/** Converts NDArray callback data into standard asyn arrays (asynInt8Array, asynInt16Array, asynInt32Array,
  * asynFloat32Array or asynFloat64Array); normally used for putting NDArray data in EPICS waveform records.
  * It handles the data type conversion if the NDArray data type differs from the data type of the asyn interface.
  * It flattens the NDArrays to a single dimension because asyn and EPICS do not support multi-dimensional arrays. */
class NDPluginStdArrays : public NDPluginDriver {
public:
    NDPluginStdArrays(const char *portName, int queueSize, int blockingCallbacks, 
                      const char *NDArrayPort, int NDArrayAddr, size_t maxMemory,
                      int priority, int stackSize);

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
private:
    /* These methods are just for this class */
    template <typename epicsType> asynStatus readArray(asynUser *pasynUser, epicsType *value, 
                                        size_t nElements, size_t *nIn, NDDataType_t outputType);
                                        
};

    
#endif
