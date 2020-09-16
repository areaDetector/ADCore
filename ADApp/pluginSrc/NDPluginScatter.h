#ifndef NDPluginScatter_H
#define NDPluginScatter_H

#include "NDPluginDriver.h"

/* General parameters */
#define NDPluginScatterMethodString          "SCATTER_METHOD"            /* (asynInt32,        r/w) Algorithm for scatter */

/** A plugin that does callbacks in round-robin fashion rather than passing every NDArray to every callback client  */
class NDPLUGIN_API NDPluginScatter : public NDPluginDriver {
public:
    NDPluginScatter(const char *portName, int queueSize, int blockingCallbacks,
                      const char *NDArrayPort, int NDArrayAddr,
                      int maxBuffers, size_t maxMemory,
                      int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

protected:
    int NDPluginScatterMethod;
    #define FIRST_NDPLUGIN_SCATTER_PARAM NDPluginScatterMethod

private:
    int nextClient_;
    asynStatus doNDArrayCallbacks(NDArray *pArray, int reason, int addr);
};

#endif
