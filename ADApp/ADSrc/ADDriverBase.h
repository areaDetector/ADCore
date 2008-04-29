#ifndef ADDriverBase_H
#define ADDriverBase_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>
#include <asynStandardInterfaces.h>

#include "asynParamBase.h"

class ADDriverBase : public asynParamBase {
public:
    ADDriverBase(const char *portName, int maxAddr, int paramTableSize);
                 
    /* These are the methods that we override from asynParamBase */
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                     const char **pptypeName, size_t *psize);
                                     
    /* These are the methods that are new to this class */

    /* Our data */
};

    
#endif
