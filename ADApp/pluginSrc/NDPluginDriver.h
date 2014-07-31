#ifndef NDPluginDriver_H
#define NDPluginDriver_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>

#include "asynNDArrayDriver.h"

#define NDPluginDriverArrayPortString           "NDARRAY_PORT"          /**< (asynOctet,    r/w) The port for the NDArray interface */
#define NDPluginDriverArrayAddrString           "NDARRAY_ADDR"          /**< (asynInt32,    r/w) The address on the port */
#define NDPluginDriverPluginTypeString          "PLUGIN_TYPE"           /**< (asynOctet,    r/o) The type of plugin */
#define NDPluginDriverDroppedArraysString       "DROPPED_ARRAYS"        /**< (asynInt32,    r/w) Number of dropped arrays */
#define NDPluginDriverQueueSizeString           "QUEUE_SIZE"            /**< (asynInt32,    r/w) Total queue elements */ 
#define NDPluginDriverQueueFreeString           "QUEUE_FREE"            /**< (asynInt32,    r/w) Free queue elements */
#define NDPluginDriverEnableCallbacksString     "ENABLE_CALLBACKS"      /**< (asynInt32,    r/w) Enable callbacks from driver (1=Yes, 0=No) */
#define NDPluginDriverBlockingCallbacksString   "BLOCKING_CALLBACKS"    /**< (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
#define NDPluginDriverMinCallbackTimeString     "MIN_CALLBACK_TIME"     /**< (asynFloat64,  r/w) Minimum time between calling processCallbacks 
                                                                         *  to execute plugin code */

/** Class from which actual plugin drivers are derived; derived from asynNDArrayDriver */
class epicsShareClass NDPluginDriver : public asynNDArrayDriver {
public:
    NDPluginDriver(const char *portName, int queueSize, int blockingCallbacks, 
                   const char *NDArrayPort, int NDArrayAddr, int maxAddr, int numParams,
                   int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask,
                   int asynFlags, int autoConnect, int priority, int stackSize);
                 
    /* These are the methods that we override from asynNDArrayDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                        size_t nElements, size_t *nIn);
                                     
    /* These are the methods that are new to this class */
    virtual void driverCallback(asynUser *pasynUser, void *genericPointer);
    virtual void processTask(void);

protected:
    virtual void processCallbacks(NDArray *pArray);
    virtual asynStatus connectToArrayPort(void);    

protected:
    int NDPluginDriverArrayPort;
    #define FIRST_NDPLUGIN_PARAM NDPluginDriverArrayPort
    int NDPluginDriverArrayAddr;
    int NDPluginDriverPluginType;
    int NDPluginDriverDroppedArrays;
    int NDPluginDriverQueueSize;
    int NDPluginDriverQueueFree;
    int NDPluginDriverEnableCallbacks;
    int NDPluginDriverBlockingCallbacks;
    int NDPluginDriverMinCallbackTime;
    #define LAST_NDPLUGIN_PARAM NDPluginDriverMinCallbackTime

private:
    virtual asynStatus setArrayInterrupt(int connect);
    
    /* The asyn interfaces we access as a client */
    void *asynGenericPointerInterruptPvt;

    /* Our data */
    asynUser *pasynUserGenericPointer;          /**< asynUser for connecting to NDArray driver */
    void *asynGenericPointerPvt;                /**< Handle for connecting to NDArray driver */
    asynGenericPointer *pasynGenericPointer;    /**< asyn interface for connecting to NDArray driver */
    bool connectedToArrayPort;
    epicsMessageQueueId msgQId;
    epicsTimeStamp lastProcessTime;
    int dimsPrev[ND_ARRAY_MAX_DIMS];
};
#define NUM_NDPLUGIN_PARAMS ((int)(&LAST_NDPLUGIN_PARAM - &FIRST_NDPLUGIN_PARAM + 1))

    
#endif
