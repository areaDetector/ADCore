#ifndef NDPluginScatter_H
#define NDPluginScatter_H

#include "NDPluginDriver.h"

/* General parameters */
#define NDPluginScatterMethodString          "SCATTER_METHOD"            /* (asynInt32,        r/w) Algorithm for scatter */

/** Extract an Attribute from an NDArray and publish the value (and array of values) over channel access.  */
class epicsShareClass NDPluginScatter : public NDPluginDriver {
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
