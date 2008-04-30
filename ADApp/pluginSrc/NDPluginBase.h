#ifndef NDPluginBase_H
#define NDPluginBase_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>
#include <asynStandardInterfaces.h>

#include "asynParamBase.h"

typedef enum
{
    NDPluginBaseArrayPort           /* (asynOctet,    r/w) The port for the NDArray interface */
        = ADFirstDriverParam,
    NDPluginBaseArrayAddr,          /* (asynInt32,    r/w) The address on the port */
    NDPluginBaseArrayCounter,       /* (asynInt32,    r/w) Number of arrays processed */
    NDPluginBaseDroppedArrays,      /* (asynInt32,    r/w) Number of dropped arrays */
    NDPluginBaseEnableCallbacks,    /* (asynInt32,    r/w) Enable callbacks from driver (1=Yes, 0=No) */
    NDPluginBaseBlockingCallbacks,  /* (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
    NDPluginBaseMinCallbackTime,    /* (asynFloat64,  r/w) Minimum time between file writes */
    NDPluginBaseUniqueId,           /* (asynInt32,    r/o) Unique ID number of array */
    NDPluginBaseTimeStamp,          /* (asynFloat64,  r/o) Time stamp of array */
    NDPluginBaseDataType,           /* (asynInt32,    r/o) Data type of array */
    NDPluginBaseNDimensions,        /* (asynInt32,    r/o) Number of dimensions in array */
    NDPluginBaseDimensions,         /* (asynInt32Array, r/o) Array dimensions */
    NDPluginBaseLastParam
} NDPluginBaseParam_t;

class NDPluginBase : public asynParamBase {
public:
    NDPluginBase(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxAddr, int paramTableSize);
                 
    /* These are the methods that we override from asynParamBase */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                     const char **pptypeName, size_t *psize);
                                     
    /* These are the methods that are new to this class */
    virtual void processCallbacks(NDArray_t *pArray);
    virtual void driverCallback(asynUser *pasynUser, void *handle);
    virtual void processTask(void);
    virtual asynStatus setArrayInterrupt(int connect);
    virtual asynStatus connectToArrayPort(void);

    /* The asyn interfaces we access as a client */
    asynHandle *pasynHandle;
    void *asynHandlePvt;
    void *asynHandleInterruptPvt;
    asynUser *pasynUserHandle;

    /* Our data */
    epicsMessageQueueId msgQId;
    epicsTimeStamp lastProcessTime;
    int dimsPrev[ND_ARRAY_MAX_DIMS];
};

    
#endif
