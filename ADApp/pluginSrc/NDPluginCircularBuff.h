#ifndef NDPluginCircularBuff_H
#define NDPluginCircularBuff_H

#include <epicsTypes.h>
#include <postfix.h>

#include "NDPluginDriver.h"
#include "NDArrayRing.h"

typedef struct NDCircularBuff {
    size_t  nElements;
    size_t  preTrigger;
    size_t  postTrigger;
} NDCircularBuff_t;

/* Param definitions */
#define NDPluginCircularBuffControlString            "CIRCULAR_BUFF_CONTROL"       /* (asynInt32,        r/w) Run scope? */
#define NDPluginCircularBuffStatusString             "CIRCULAR_BUFF_STATUS"        /* (asynOctetRead,    r/o) Scope status */
#define NDPluginCircularBuffTriggerAString           "CIRCULAR_BUFF_TRIGGER_A"     /* (asynOctetWrite,   r/w) Trigger A attribute name */
#define NDPluginCircularBuffTriggerBString           "CIRCULAR_BUFF_TRIGGER_B"     /* (asynOctetWrite,   r/w) Trigger B attribute name */
#define NDPluginCircularBuffTriggerAValString        "CIRCULAR_BUFF_TRIGGER_A_VAL" /* (asynFloat64,      r/o) Trigger A value */
#define NDPluginCircularBuffTriggerBValString        "CIRCULAR_BUFF_TRIGGER_B_VAL" /* (asynFloat64,      r/o) Trigger B value */
#define NDPluginCircularBuffTriggerCalcString        "CIRCULAR_BUFF_TRIGGER_CALC"  /* (asynOctetWrite,   r/w) Trigger calculation expression */
#define NDPluginCircularBuffTriggerCalcValString     "CIRCULAR_BUFF_TRIGGER_CALC_VAL" /* (asynFloat64,   r/o) Trigger calculation value */
#define NDPluginCircularBuffPreTriggerString         "CIRCULAR_BUFF_PRE_TRIGGER"   /* (asynInt32,        r/w) Number of pre-trigger images */
#define NDPluginCircularBuffPostTriggerString        "CIRCULAR_BUFF_POST_TRIGGER"  /* (asynInt32,        r/w) Number of post-trigger images */
#define NDPluginCircularBuffCurrentImageString       "CIRCULAR_BUFF_CURRENT_IMAGE" /* (asynInt32,        r/o) Number of the current image */
#define NDPluginCircularBuffPostCountString          "CIRCULAR_BUFF_POST_COUNT"    /* (asynInt32,        r/o) Number of the current post count image */
#define NDPluginCircularBuffSoftTriggerString        "CIRCULAR_BUFF_SOFT_TRIGGER"  /* (asynInt32,        r/w) Force a soft trigger */
#define NDPluginCircularBuffTriggeredString          "CIRCULAR_BUFF_TRIGGERED"     /* (asynInt32,        r/o) Have we had a trigger event */


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
    asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t nChars, size_t *nActual);
    
    //template <typename epicsType> asynStatus doProcessCircularBuffT(NDArray *pArray);
    //asynStatus doProcessCircularBuff(NDArray *pArray);
   
protected:
    int NDPluginCircularBuffControl;
    #define FIRST_NDPLUGIN_CIRCULAR_BUFF_PARAM NDPluginCircularBuffControl
    /* Scope */
    int NDPluginCircularBuffStatus;
    int NDPluginCircularBuffTriggerA;
    int NDPluginCircularBuffTriggerB;
    int NDPluginCircularBuffTriggerAVal;
    int NDPluginCircularBuffTriggerBVal;
    int NDPluginCircularBuffTriggerCalc;
    int NDPluginCircularBuffTriggerCalcVal;
    int NDPluginCircularBuffPreTrigger;
    int NDPluginCircularBuffPostTrigger;
    int NDPluginCircularBuffCurrentImage;
    int NDPluginCircularBuffPostCount;
    int NDPluginCircularBuffSoftTrigger;
    int NDPluginCircularBuffTriggered;

    #define LAST_NDPLUGIN_CIRCULAR_BUFF_PARAM NDPluginCircularBuffTriggered
                                
private:

    asynStatus calculateTrigger(NDArray *pArray, int *trig);
    NDArrayRing *preBuffer_;
    NDArray *pOldArray_;
    int previousTrigger_;
    int maxBuffers_;
    char triggerCalcInfix_[MAX_INFIX_SIZE];
    char triggerCalcPostfix_[MAX_POSTFIX_SIZE];
    double triggerCalcArgs_[CALCPERFORM_NARGS];
};
#define NUM_NDPLUGIN_CIRCULAR_BUFF_PARAMS ((int)(&LAST_NDPLUGIN_CIRCULAR_BUFF_PARAM - &FIRST_NDPLUGIN_CIRCULAR_BUFF_PARAM + 1))
    
#endif

