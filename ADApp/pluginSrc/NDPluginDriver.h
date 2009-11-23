#ifndef NDPluginDriver_H
#define NDPluginDriver_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>
#include <asynStandardInterfaces.h>

#include "asynNDArrayDriver.h"

#define NDPluginDriverArrayPortString           "NDARRAY_PORT"          /**< (asynOctet,    r/w) The port for the NDArray interface */
#define NDPluginDriverArrayAddrString           "NDARRAY_ADDR"            /**< (asynInt32,    r/w) The address on the port */
#define NDPluginDriverPluginTypeString          "PLUGIN_TYPE"           /**< (asynOctet,    r/o) The type of plugin */
#define NDPluginDriverArrayCounterString        "ARRAY_COUNTER"         /**< (asynInt32,    r/w) Number of arrays processed */
#define NDPluginDriverDroppedArraysString       "DROPPED_ARRAYS"        /**< (asynInt32,    r/w) Number of dropped arrays */
#define NDPluginDriverEnableCallbacksString     "ENABLE_CALLBACKS"      /**< (asynInt32,    r/w) Enable callbacks from driver (1=Yes, 0=No) */
#define NDPluginDriverBlockingCallbacksString   "BLOCKING_CALLBACKS"    /**< (asynInt32,    r/w) Callbacks block (1=Yes, 0=No) */
#define NDPluginDriverMinCallbackTimeString     "MIN_CALLBACK_TIME"     /**< (asynFloat64,  r/w) Minimum time between calling processCallbacks 
                                                                         *  to execute plugin code */
#define NDPluginDriverUniqueIdString            "UNIQUE_ID"             /**< (asynInt32,    r/o) Unique ID number of array */
#define NDPluginDriverTimeStampString           "TIME_STAMP"            /**< (asynFloat64,  r/o) Time stamp of array */
#define NDPluginDriverDataTypeString            "DATA_TYPE"             /**< (asynInt32,    r/o) Data type of array */
#define NDPluginDriverColorModeString           "COLOR_MODE"            /**< (asynInt32,    r/o) Color mode of array (from colorMode array attribute if present) */
#define NDPluginDriverBayerPatternString        "BAYER_PATTERN"         /**< (asynInt32,    r/o) Bayer pattern of array  (from bayerPattern array attribute if present) */
#define NDPluginDriverNDimensionsString         "ARRAY_NDIMENSIONS"     /**< (asynInt32,    r/o) Number of dimensions in array */
#define NDPluginDriverDimensionsString          "ARRAY_DIMENSIONS"      /**< (asynInt32Array, r/o) Array dimensions */

/** Class from which actual plugin drivers are derived; derived from asynNDArrayDriver */
class NDPluginDriver : public asynNDArrayDriver {
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
    asynUser *pasynUserGenericPointer;          /**< asynUser for connecting to NDArray driver */
    void *asynGenericPointerPvt;                /**< Handle for connecting to NDArray driver */
    asynGenericPointer *pasynGenericPointer;    /**< asyn interface for connecting to NDArray driver */

protected:
    int NDPluginDriverArrayPort;
    #define FIRST_NDPLUGIN_PARAM NDPluginDriverArrayPort
    int NDPluginDriverArrayAddr;
    int NDPluginDriverPluginType;
    int NDPluginDriverArrayCounter;
    int NDPluginDriverDroppedArrays;
    int NDPluginDriverEnableCallbacks;
    int NDPluginDriverBlockingCallbacks;
    int NDPluginDriverMinCallbackTime;
    int NDPluginDriverUniqueId;
    int NDPluginDriverTimeStamp;
    int NDPluginDriverDataType;
    int NDPluginDriverColorMode;
    int NDPluginDriverBayerPattern;
    int NDPluginDriverNDimensions;
    int NDPluginDriverDimensions;
    #define LAST_NDPLUGIN_PARAM NDPluginDriverDimensions

private:
    virtual asynStatus setArrayInterrupt(int connect);
    
    /* The asyn interfaces we access as a client */
    void *asynGenericPointerInterruptPvt;

    /* Our data */
    epicsMessageQueueId msgQId;
    epicsTimeStamp lastProcessTime;
    int dimsPrev[ND_ARRAY_MAX_DIMS];
};
#define NUM_NDPLUGIN_PARAMS (&LAST_NDPLUGIN_PARAM - &FIRST_NDPLUGIN_PARAM + 1)

    
#endif
