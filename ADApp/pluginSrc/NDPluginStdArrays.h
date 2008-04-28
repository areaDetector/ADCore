#ifndef NDPluginStdArrays_H
#define NDPluginStdArrays_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "ADInterface.h"
#include "NDPluginBase.h"

typedef enum
{
    NDPluginStdArraysData           /* (asynXXXArray, r/w) Array data waveform */
        = NDPluginBaseLastParam,
    NDPluginStdArraysLastParam
} NDPluginStdArraysParam_t;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t NDPluginStdArraysParamString[] = {
    {NDPluginStdArraysData,               "STD_ARRAY_DATA"}
};

#define NUM_ND_PLUGIN_STD_ARRAYS_PARAMS (sizeof(NDPluginStdArraysParamString)/sizeof(NDPluginStdArraysParamString[0]))

class NDPluginStdArrays : public NDPluginBase {
public:
    NDPluginStdArrays(const char *portName, int queueSize, int blockingCallbacks, 
                      const char *NDArrayPort, int NDArrayAddr);

    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray_t *pArray);
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
                                        size_t nElements, size_t *nIn, int outputType);                            
};

    
#endif
