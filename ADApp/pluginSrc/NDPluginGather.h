#ifndef NDPluginGather_H
#define NDPluginGather_H

#include <set>

#include "NDPluginDriver.h"

typedef enum {
    NDGatherCallbacksUnsorted,
    NDGatherCallbacksSorted
} NDGatherCallbacksSorted_t;

/* General parameters */
#define NDPluginGatherSortModeString  "GATHER_SORT_MODE"            /* (asynInt32,        r/w) sorted callback mode */
#define NDPluginGatherSortTimeString  "GATHER_SORT_TIME"            /* (asynFloat64,      r/w) sorted callback time */
#define NDPluginGatherListFreeString  "GATHER_LIST_FREE"            /* (asynInt32,        r/o) std::multilist free elements */

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

    void sortingTask();

protected:
    /* These methods override the virtual methods in the base class */
    virtual void processCallbacks(NDArray *pArray);
    virtual asynStatus connectToArrayPort(void);    
    virtual asynStatus setArrayInterrupt(int connect);

    int NDPluginGatherSortMode;
    #define FIRST_NDPLUGIN_GATHER_PARAM NDPluginGatherSortMode
    int NDPluginGatherSortTime;
    int NDPluginGatherListFree;
                                
private:
    int maxPorts_;
    NDGatherNDArraySource_t *NDArraySrc_;
    std::multiset<class sortedListElement> sortedNDArrayList_;
};
    
#endif
