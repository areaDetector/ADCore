#ifndef NDPluginGather_H
#define NDPluginGather_H

#include "NDPluginDriver.h"

/* General parameters */
#define NDPluginGatherDummyString          "GATHER_DUMMY"            /* (asynInt32,        r/w) dummy parameter */

/** A plugin that subscribes to callbacks from multiple ports, not just a single port  */
class epicsShareClass NDPluginGather : public NDPluginDriver {
public:
    NDPluginGather(const char *portName, int queueSize, int blockingCallbacks, 
                   int maxPorts, 
                   int maxBuffers, size_t maxMemory,
                   int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

protected:
    int NDPluginGatherDummy;
    #define FIRST_NDPLUGIN_GATHER_PARAM NDPluginGatherDummy
                                
private:
};
    
#endif
