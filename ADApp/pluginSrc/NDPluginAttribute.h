
#ifndef NDPluginAttribute_H
#define NDPluginAttribute_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/* General parameters */
#define NDPluginAttributeNameString       "ATTR_NAME"                /* (asynOctet,   r/w) Name of this attribute */
#define NDPluginAttributeAttrNameString   "ATTR_ATTRNAME"  /* (asynInt32,   r/w) Name of Attribute */
#define NDPluginAttributeResetString      "ATTR_RESET"     /* (asynInt32,   r/w) Clear the array data */
#define NDPluginAttributeUpdateString      "ATTR_UPDATE"     /* (asynInt32,   r/w) Update the data array */
#define NDPluginAttributeValString        "ATTR_VAL"       /* (asynFloat64,   r/o) Value of Attribute */
#define NDPluginAttributeValSumString        "ATTR_VAL_SUM"       /* (asynFloat64,   r/o) Integrated Value of Attribute */
#define NDPluginAttributeArrayString      "ATTR_ARRAY"       /* (asynFloat64Array,   r/o) Array of Attribute */
#define NDPluginAttributeDataTypeString   "ATTR_DATA_TYPE" /* (asynInt32,   r/w) Data type for Attribute.  -1 means automatic. */
#define NDPluginAttributeUpdatePeriodString        "ATTR_UPDATE_PERIOD"       /* (asynFloat64,   r/o) Update period for array */

/** Extract an Attribute from an NDArray and publish the value (and array of values) over channel access.  */
class NDPluginAttribute : public NDPluginDriver {
public:
    NDPluginAttribute(const char *portName, int queueSize, int blockingCallbacks, 
		      const char *NDArrayPort, int NDArrayAddr,
		      int maxBuffers, size_t maxMemory,
		      int priority, int stackSize, 
		      int maxTimeSeries, const char *attrName);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
    int NDPluginAttributeName;
    #define FIRST_NDPLUGIN_ATTR_PARAM NDPluginAttributeName

    int NDPluginAttributeAttrName;
    int NDPluginAttributeUpdate;
    int NDPluginAttributeReset;
    int NDPluginAttributeVal;
    int NDPluginAttributeValSum;
    int NDPluginAttributeArray;
    int NDPluginAttributeDataType;
    int NDPluginAttributeUpdatePeriod;

    #define LAST_NDPLUGIN_ATTR_PARAM NDPluginAttributeUpdatePeriod
                                
private:

    static const epicsInt32 MAX_ATTR_NAME_;

    int maxTimeSeries_;
    int currentPoint_;
    double *pTimeSeries_;
    epicsTimeStamp nowTime_;
    int arrayUpdate_;
    double nowTimeSecs_;
    double lastTimeSecs_;
    double valueSum_;

};
#define NUM_NDPLUGIN_ATTR_PARAMS (&LAST_NDPLUGIN_ATTR_PARAM - &FIRST_NDPLUGIN_ATTR_PARAM + 1)
    
#endif
