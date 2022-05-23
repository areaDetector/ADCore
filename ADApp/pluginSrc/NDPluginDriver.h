#ifndef NDPluginDriver_H
#define NDPluginDriver_H

#include <set>
#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsThread.h>
#include <epicsTime.h>

#include <NDPluginAPI.h>

#include "NDPluginDriverParamSet.h"
#include "asynNDArrayDriver.h"

class Throttler;

// This class defines the object that is contained in the std::multilist for sorting output NDArrays
// It contains a pointer to the NDArray and the time that the object was added to the list
// It defines the < operator to use the NDArray::uniqueId field as the sort key

// We would like to hide this class definition in NDPluginDriver.cpp and just forward reference it here.
// That works on Visual Studio, and on gcc if instantiating plugins as heap variables with "new", but fails on gcc
// if instantiating plugins as automatic variables.
//class sortedListElement;

class sortedListElement {
    public:
        sortedListElement(NDArray *pArray, epicsTimeStamp time);
        friend bool operator<(const sortedListElement& lhs, const sortedListElement& rhs) {
            return (lhs.pArray_->uniqueId < rhs.pArray_->uniqueId);
        }
        NDArray *pArray_;
        epicsTimeStamp insertionTime_;
};

                                                                         *to execute plugin code */
/** Class from which actual plugin drivers are derived; derived from asynNDArrayDriver */
class NDPLUGIN_API NDPluginDriver : public asynNDArrayDriver, public epicsThreadRunable {
public:
    NDPluginDriver(NDPluginDriverParamSet* paramSet, const char *portName, int queueSize, int blockingCallbacks,
                   const char *NDArrayPort, int NDArrayAddr, int maxAddr,
                   int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask,
                   int asynFlags, int autoConnect, int priority, int stackSize, int maxThreads,
                   bool compressionAware = false);
    ~NDPluginDriver();

    /* These are the methods that we override from asynNDArrayDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                        size_t nElements, size_t *nIn);

    /* These are the methods that are new to this class */
    virtual void driverCallback(asynUser *pasynUser, void *genericPointer);
    virtual void run(void);
    virtual asynStatus start(void);
    void sortingTask();

protected:
    NDPluginDriverParamSet* paramSet;
    virtual void processCallbacks(NDArray *pArray) = 0;
    virtual void beginProcessCallbacks(NDArray *pArray);
    virtual asynStatus endProcessCallbacks(NDArray *pArray, bool copyArray=false, bool readAttributes=true);
    virtual asynStatus connectToArrayPort(void);
    virtual asynStatus setArrayInterrupt(int connect);

protected:
    #define FIRST_NDPLUGIN_PARAM paramSet->FIRST_NDPLUGINDRIVERPARAMSET_PARAM

    NDArray *pPrevInputArray_;
    bool throttled(NDArray *pArray);

private:
    void processTask();
    asynStatus createCallbackThreads();
    asynStatus startCallbackThreads();
    asynStatus deleteCallbackThreads();
    asynStatus createSortingThread();

    /* The asyn interfaces we access as a client */
    void *asynGenericPointerInterruptPvt_;

    /* Our data */
    int numThreads_;
    bool pluginStarted_;
    bool firstOutputArray_;
    asynUser *pasynUserGenericPointer_;          /**< asynUser for connecting to NDArray driver */
    void *asynGenericPointerPvt_;                /**< Handle for connecting to NDArray driver */
    asynGenericPointer *pasynGenericPointer_;    /**< asyn interface for connecting to NDArray driver */
    bool connectedToArrayPort_;
    std::vector<epicsThread*>pThreads_;
    epicsMessageQueue *pToThreadMsgQ_;
    epicsMessageQueue *pFromThreadMsgQ_;
    std::multiset<sortedListElement> sortedNDArrayList_;
    int prevUniqueId_;
    epicsThreadId sortingThreadId_;
    epicsTimeStamp lastProcessTime_;
    int dimsPrev_[ND_ARRAY_MAX_DIMS];
    bool compressionAware_;
    Throttler *throttler_;
};


#endif