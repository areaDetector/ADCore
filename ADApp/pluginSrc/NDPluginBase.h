#ifndef NDPluginBase_H
#define NDPluginBase_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "ADParamLib.h"
#include "NDArrayBuff.h"

typedef enum
{
    NDPluginBaseArrayPort           /* (asynOctet,    r/w) The port for the NDArray interface */
        = ADFirstDriverParam,
    NDPluginBaseArrayAddr,          /* (asynInt32,    r/w) The address on the port */
    NDPluginBaseArrayCounter,       /* (asynInt32,    r/w) Number of arrays processed */
    NDPluginBaseDroppedArrays,      /* (asynInt32,    r/w) Number of dropped arrays */
    NDPluginBaseEnableCallbacks,    /* (asynInt32,    r/w) Enable callbacks from driver (1=Yes, 0=No) */
    NDPluginBaseBlockingCallbacks,  /* (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
    NDPluginBaseMinCallbackTime,     /* (asynFloat64,  r/w) Minimum time between file writes */
    NDPluginBaseLastParam
} NDPluginBaseParam_t;

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t NDPluginBaseParamString[] = {
    {NDPluginBaseArrayPort,         "NDARRAY_PORT" },
    {NDPluginBaseArrayAddr,         "NDARRAY_ADDR" },
    {NDPluginBaseArrayCounter,      "ARRAY_COUNTER"},
    {NDPluginBaseDroppedArrays,     "DROPPED_ARRAYS" },
    {NDPluginBaseEnableCallbacks,   "ENABLE_CALLBACKS" },
    {NDPluginBaseBlockingCallbacks, "BLOCKING_CALLBACKS" },
    {NDPluginBaseMinCallbackTime,   "MIN_CALLBACK_TIME" }
};

#define NUM_ND_PLUGIN_BASE_PARAMS (sizeof(NDPluginBaseParamString)/sizeof(NDPluginBaseParamString[0]))

class NDPluginBase {
public:
    NDPluginBase(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int paramTableSize);
    virtual void processCallbacks(NDArray_t *pArray);
    virtual void driverCallback(asynUser *pasynUser, void *handle);
    virtual void processTask(void);
    virtual asynStatus setArrayInterrupt(int connect);
    virtual asynStatus connectToArrayPort(void);
    virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars,
                         size_t *nActual, int *eomReason);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    virtual asynStatus readNDArray(asynUser *pasynUser, void *handle);
    virtual asynStatus writeNDArray(asynUser *pasynUser, void *handle);
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                     const char **pptypeName, size_t *psize);
    virtual asynStatus drvUserGetType(asynUser *pasynUser,
                                        const char **pptypeName, size_t *psize);
    virtual asynStatus drvUserDestroy(asynUser *pasynUser);
    virtual void report(FILE *fp, int details);
    virtual asynStatus connect(asynUser *pasynUser);
    virtual asynStatus disconnect(asynUser *pasynUser);
   
    char *portName;
    epicsMutexId mutexId;
    epicsMessageQueueId msgQId;
    PARAMS params;
    NDArray_t *pArray;
    epicsTimeStamp lastProcessTime;

    /* The asyn interfaces this driver implements */
    asynStandardInterfaces asynStdInterfaces;
    
    /* The asyn interfaces we access as a client */
    asynHandle *pasynHandle;
    void *asynHandlePvt;
    void *asynHandleInterruptPvt;
    asynUser *pasynUserHandle;
    
    /* asynUser connected to ourselves for asynTrace */
    asynUser *pasynUser;
};

    
#endif
