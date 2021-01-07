#ifndef NDPluginGather_H
#define NDPluginGather_H

#include <set>

#include "NDPluginDriver.h"

typedef struct {
    void *asynGenericPointerInterruptPvt;        /**< InterruptPvt for connecting to NDArray driver interupts */
    asynUser *pasynUserGenericPointer;           /**< asynUser for connecting to NDArray driver */
    void *asynGenericPointerPvt;                 /**< Handle for connecting to NDArray driver */
    asynGenericPointer *pasynGenericPointer;     /**< asyn interface for connecting to NDArray driver */
    bool connectedToArrayPort;
} NDGatherNDArraySource_t;

/** A plugin that subscribes to callbacks from multiple ports, not just a single port  */
class NDPLUGIN_API NDPluginGather : public NDPluginDriver {
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


private:
    int maxPorts_;
    NDGatherNDArraySource_t *NDArraySrc_;
};

#endif
