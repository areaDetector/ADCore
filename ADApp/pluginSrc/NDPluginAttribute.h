
#ifndef NDPluginAttribute_H
#define NDPluginAttribute_H

#include <epicsTypes.h>

#include "NDPluginDriver.h"

/* General parameters */
#define NDPluginAttributeAttrNameString       "ATTR_ATTRNAME"         /* (asynInt32,        r/w) Name of Attribute */
#define NDPluginAttributeResetString          "ATTR_RESET"            /* (asynInt32,        r/w) Clear the sum data */
#define NDPluginAttributeValString            "ATTR_VAL"              /* (asynFloat64,      r/o) Value of Attribute */
#define NDPluginAttributeValSumString         "ATTR_VAL_SUM"          /* (asynFloat64,      r/o) Integrated Value of Attribute */

/** Extract an Attribute from an NDArray and publish the value (and array of values) over channel access.  */
class NDPLUGIN_API NDPluginAttribute : public NDPluginDriver {
public:
    NDPluginAttribute(const char *portName, int queueSize, int blockingCallbacks,
                      const char *NDArrayPort, int NDArrayAddr, int maxAttributes,
                      int maxBuffers, size_t maxMemory,
                      int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
    int NDPluginAttributeAttrName;
    #define FIRST_NDPLUGIN_ATTR_PARAM NDPluginAttributeAttrName
    int NDPluginAttributeReset;
    int NDPluginAttributeVal;
    int NDPluginAttributeValSum;

private:

    void doTimeSeriesCallbacks(NDArray *pArray);
    static const epicsInt32 MAX_ATTR_NAME_;
    static const char*      UNIQUE_ID_NAME_;
    static const char*      TIMESTAMP_NAME_;
    static const char*      EPICS_TS_SEC_NAME_;
    static const char*      EPICS_TS_NSEC_NAME_;

    int maxAttributes_;

};

#endif
