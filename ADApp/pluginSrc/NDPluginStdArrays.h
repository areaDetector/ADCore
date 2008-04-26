#ifndef NDPluginStdArrays_H
#define NDPluginStdArrays_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "ADInterface.h"
#include "NDPluginBase.h"

typedef enum
{
    NDPluginStdArraysNDimensions    /* (asynInt32,    r/o) Number of dimensions in array */
        = NDPluginBaseLastParam,
    NDPluginStdArraysDimensions,    /* (asynInt32Array, r/o) Array dimensions */
    NDPluginStdArraysData,          /* (asynXXXArray, r/w) Array data waveform */
    NDPluginStdArraysLastParam
} NDPluginStdArraysParam_t;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t NDPluginStdArraysParamString[] = {
    {NDPluginStdArraysNDimensions,        "ARRAY_NDIMENSIONS"},
    {NDPluginStdArraysDimensions,         "ARRAY_DIMENSIONS"},
    {NDPluginStdArraysData,               "ARRAY_DATA"}
};

#define NUM_ND_PLUGIN_STD_ARRAYS_PARAMS (sizeof(NDPluginStdArraysParamString)/sizeof(NDPluginStdArraysParamString[0]))

class NDPluginStdArrays : public NDPluginBase {
public:
    NDPluginStdArrays(const char *portName, int queueSize, int blockingCallbacks, 
                      const char *NDArrayPort, int NDArrayAddr);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray_t *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                             const char **pptypeName, size_t *psize);
                             
    /* These methods are unique to this class */
    asynStatus readInt8Array(asynUser *pasynUser, epicsInt8 *value, size_t nElements, size_t *nIn);
    asynStatus readInt16Array(asynUser *pasynUser, epicsInt16 *value, size_t nElements, size_t *nIn);
    asynStatus readInt32Array(asynUser *pasynUser,    epicsInt32 *value, size_t nElements, size_t *nIn);
    asynStatus readFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements, size_t *nIn);
    asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);

private:
    int dimsPrev[ND_ARRAY_MAX_DIMS];
};

    
#endif
