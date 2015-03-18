#ifndef NDPluginCircularBuff_H
#define NDPluginCircularBuff_H

#include <epicsTypes.h>
#include <postfix.h>

#include "NDPluginDriver.h"
#include "NDArrayRing.h"

/* Param definitions */
#define NDCircBuffControlString             "CIRC_BUFF_CONTROL"               /* (asynInt32,        r/w) Run scope? */
#define NDCircBuffStatusString              "CIRC_BUFF_STATUS"                /* (asynOctetRead,    r/o) Scope status */
#define NDCircBuffTriggerAString            "CIRC_BUFF_TRIGGER_A"             /* (asynOctetWrite,   r/w) Trigger A attribute name */
#define NDCircBuffTriggerBString            "CIRC_BUFF_TRIGGER_B"             /* (asynOctetWrite,   r/w) Trigger B attribute name */
#define NDCircBuffTriggerAValString         "CIRC_BUFF_TRIGGER_A_VAL"         /* (asynFloat64,      r/o) Trigger A value */
#define NDCircBuffTriggerBValString         "CIRC_BUFF_TRIGGER_B_VAL"         /* (asynFloat64,      r/o) Trigger B value */
#define NDCircBuffTriggerCalcString         "CIRC_BUFF_TRIGGER_CALC"          /* (asynOctetWrite,   r/w) Trigger calculation expression */
#define NDCircBuffTriggerCalcValString      "CIRC_BUFF_TRIGGER_CALC_VAL"      /* (asynFloat64,   r/o) Trigger calculation value */
#define NDCircBuffPresetTriggerCountString  "CIRC_BUFF_PRESET_TRIGGER_COUNT"  /* (asynInt32,        r/w) Preset number of triggers 0=infinite*/
#define NDCircBuffActualTriggerCountString  "CIRC_BUFF_ACTUAL_TRIGGER_COUNT"  /* (asynInt32,        r/w) Actual number of triggers so far */
#define NDCircBuffPreTriggerString          "CIRC_BUFF_PRE_TRIGGER"           /* (asynInt32,        r/w) Number of pre-trigger images */
#define NDCircBuffPostTriggerString         "CIRC_BUFF_POST_TRIGGER"          /* (asynInt32,        r/w) Number of post-trigger images */
#define NDCircBuffCurrentImageString        "CIRC_BUFF_CURRENT_IMAGE"         /* (asynInt32,        r/o) Number of the current image */
#define NDCircBuffPostCountString           "CIRC_BUFF_POST_COUNT"            /* (asynInt32,        r/o) Number of the current post count image */
#define NDCircBuffSoftTriggerString         "CIRC_BUFF_SOFT_TRIGGER"          /* (asynInt32,        r/w) Force a soft trigger */
#define NDCircBuffTriggeredString           "CIRC_BUFF_TRIGGERED"             /* (asynInt32,        r/o) Have we had a trigger event */


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
    int NDCircBuffControl;
    #define FIRST_NDPLUGIN_CIRC_BUFF_PARAM NDCircBuffControl
    /* Scope */
    int NDCircBuffStatus;
    int NDCircBuffTriggerA;
    int NDCircBuffTriggerB;
    int NDCircBuffTriggerAVal;
    int NDCircBuffTriggerBVal;
    int NDCircBuffTriggerCalc;
    int NDCircBuffTriggerCalcVal;
    int NDCircBuffPresetTriggerCount;
    int NDCircBuffActualTriggerCount;
    int NDCircBuffPreTrigger;
    int NDCircBuffPostTrigger;
    int NDCircBuffCurrentImage;
    int NDCircBuffPostCount;
    int NDCircBuffSoftTrigger;
    int NDCircBuffTriggered;

    #define LAST_NDPLUGIN_CIRC_BUFF_PARAM NDCircBuffTriggered
                                
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
#define NUM_NDPLUGIN_CIRC_BUFF_PARAMS ((int)(&LAST_NDPLUGIN_CIRC_BUFF_PARAM - &FIRST_NDPLUGIN_CIRC_BUFF_PARAM + 1))
    
#endif

