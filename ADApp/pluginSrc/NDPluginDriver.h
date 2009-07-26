#ifndef NDPluginDriver_H
#define NDPluginDriver_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>
#include <asynStandardInterfaces.h>

#include "asynNDArrayDriver.h"

/** Parameters that are common to all NDArray plugins */
typedef enum
{
    NDPluginDriverArrayPort               /**< (asynOctet,    r/w) The port for the NDArray interface */
        = NDLastStdParam,
    NDPluginDriverArrayAddr,              /**< (asynInt32,    r/w) The address on the port */
    NDPluginDriverPluginType,             /**< (asynOctet,    r/w) The type of plugin */
    NDPluginDriverArrayCounter,           /**< (asynInt32,    r/w) Number of arrays processed */
    NDPluginDriverDroppedArrays,          /**< (asynInt32,    r/w) Number of dropped arrays */
    NDPluginDriverEnableCallbacks,        /**< (asynInt32,    r/w) Enable callbacks from driver (1=Yes, 0=No) */
    NDPluginDriverBlockingCallbacks,      /**< (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
    NDPluginDriverMinCallbackTime,        /**< (asynFloat64,  r/w) Minimum time between calling processCallbacks 
                                            *  to execute plugin code */
    NDPluginDriverUniqueId,               /**< (asynInt32,    r/o) Unique ID number of array */
    NDPluginDriverTimeStamp,              /**< (asynFloat64,  r/o) Time stamp of array */
    NDPluginDriverDataType,               /**< (asynInt32,    r/o) Data type of array */
    NDPluginDriverColorMode,              /**< (asynInt32,    r/o) Color mode of array (from colorMode array attribute if present) */
    NDPluginDriverBayerPattern,           /**< (asynInt32,    r/o) Bayer pattern of array  (from bayerPattern array attribute if present) */
    NDPluginDriverNDimensions,            /**< (asynInt32,    r/o) Number of dimensions in array */
    NDPluginDriverDimensions,             /**< (asynInt32Array, r/o) Array dimensions */
    NDPluginDriverLastParam
} NDPluginDriverParam_t;

/** Class from which actual plugin drivers are derived; derived from asynNDArrayDriver */
class NDPluginDriver : public asynNDArrayDriver {
public:
    NDPluginDriver(const char *portName, int queueSize, int blockingCallbacks, 
                   const char *NDArrayPort, int NDArrayAddr, int maxAddr, int paramTableSize,
                   int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask,
                   int asynFlags, int autoConnect, int priority, int stackSize);
                 
    /* These are the methods that we override from asynNDArrayDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                     const char **pptypeName, size_t *psize);
                                     
    /* These are the methods that are new to this class */
    virtual void driverCallback(asynUser *pasynUser, void *genericPointer);
    virtual void processTask(void);

protected:
    virtual void processCallbacks(NDArray *pArray);
    virtual asynStatus connectToArrayPort(void);    
    asynUser *pasynUserGenericPointer;          /**< asynUser for connecting to NDArray driver */
    void *asynGenericPointerPvt;                /**< Handle for connecting to NDArray driver */
    asynGenericPointer *pasynGenericPointer;    /**< asyn interface for connecting to NDArray driver */

private:
    virtual asynStatus setArrayInterrupt(int connect);
    
    /* The asyn interfaces we access as a client */
    void *asynGenericPointerInterruptPvt;

    /* Our data */
    epicsMessageQueueId msgQId;
    epicsTimeStamp lastProcessTime;
    int dimsPrev[ND_ARRAY_MAX_DIMS];
};

    
#endif
