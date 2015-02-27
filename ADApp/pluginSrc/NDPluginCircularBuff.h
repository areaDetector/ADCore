#ifndef NDPluginCircularBuff_H
#define NDPluginCircularBuff_H

#include <deque>

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/* Param definitions */
#define NDPluginCircularBuffControlString            "CIRCULAR_BUFF_CONTROL"       /* (asynInt32,        r/w) Run scope? */
#define NDPluginCircularBuffStatusString             "CIRCULAR_BUFF_STATUS"        /* (asynOctetRead,    r/o) Scope status */
#define NDPluginCircularBuffPreTriggerString         "CIRCULAR_BUFF_PRE_TRIGGER"   /* (asynInt32,        r/w) Number of pre-trigger images */
#define NDPluginCircularBuffPostTriggerString        "CIRCULAR_BUFF_POST_TRIGGER"  /* (asynInt32,        r/w) Number of post-trigger images */
#define NDPluginCircularBuffCurrentImageString       "CIRCULAR_BUFF_CURRENT_IMAGE" /* (asynInt32,        r/o) Number of the current image */
#define NDPluginCircularBuffPostCountString          "CIRCULAR_BUFF_POST_COUNT"    /* (asynInt32,        r/o) Number of the current post count image */
#define NDPluginCircularBuffSoftTriggerString        "CIRCULAR_BUFF_SOFT_TRIGGER"  /* (asynInt32,        r/w) Force a soft trigger */
#define NDPluginCircularBuffTriggeredString          "CIRCULAR_BUFF_TRIGGERED"     /* (asynInt32,        r/o) Have we had a trigger event */

#define NDPluginCircularBuffTriggeredAttribute       "ExternalTrigger"

/** Performs a scope like capture.  Records a quantity
  * of pre-trigger and post-trigger images
  */
class epicsShareClass NDPluginCircularBuff : public NDPluginDriver {
public:
    NDPluginCircularBuff(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    
    //template <typename epicsType> asynStatus doProcessCircularBuffT(NDArray *pArray);
    //asynStatus doProcessCircularBuff(NDArray *pArray);
   
protected:
    int NDPluginCircularBuffControl;
    #define FIRST_NDPLUGIN_CIRCULAR_BUFF_PARAM NDPluginCircularBuffControl
    /* Scope */
    int NDPluginCircularBuffStatus;
    int NDPluginCircularBuffPreTrigger;
    int NDPluginCircularBuffPostTrigger;
    int NDPluginCircularBuffCurrentImage;
    int NDPluginCircularBuffPostCount;
    int NDPluginCircularBuffSoftTrigger;
    int NDPluginCircularBuffTriggered;

    #define LAST_NDPLUGIN_CIRCULAR_BUFF_PARAM NDPluginCircularBuffTriggered
                                
private:

    std::deque<NDArray *> *preBuffer_;
    NDArray *pOldArray_;
    int previousTrigger_;
    int maxBuffers_;
};
#define NUM_NDPLUGIN_CIRCULAR_BUFF_PARAMS ((int)(&LAST_NDPLUGIN_CIRCULAR_BUFF_PARAM - &FIRST_NDPLUGIN_CIRCULAR_BUFF_PARAM + 1))
    
#endif

