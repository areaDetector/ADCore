#ifndef NDPluginGather_H
#define NDPluginGather_H

#include "NDPluginDriver.h"

/* General parameters */
#define NDPluginGatherDummyString          "GATHER_DUMMY"            /* (asynInt32,        r/w) dummy parameter */

typedef struct {
    void *asynGenericPointerInterruptPvt;        /**< InterruptPvt for connecting to NDArray driver interupts */
    asynUser *pasynUserGenericPointer;           /**< asynUser for connecting to NDArray driver */
    void *asynGenericPointerPvt;                 /**< Handle for connecting to NDArray driver */
    asynGenericPointer *pasynGenericPointer;     /**< asyn interface for connecting to NDArray driver */
    bool connectedToArrayPort;
} NDGatherNDArraySource_t;

/** A plugin that subscribes to callbacks from multiple ports, not just a single port  */
class epicsShareClass NDPluginGather : public NDPluginDriver {
public:
    NDPluginGather(const char *portName, int queueSize, int blockingCallbacks, 
                   int maxPorts, 
                   int maxBuffers, size_t maxMemory,
                   int priority, int stackSize);

protected:
    /* These methods override the virtual methods in the base class */
    virtual void processCallbacks(NDArray *pArray);
    virtual asynStatus connectToArrayPort(void);    
    virtual asynStatus setArrayInterrupt(int connect);
    int NDPluginGatherDummy;
    #define FIRST_NDPLUGIN_GATHER_PARAM NDPluginGatherDummy
                                
private:
    int maxPorts_;
    NDGatherNDArraySource_t *NDArraySrc_;
};
    
#endif
