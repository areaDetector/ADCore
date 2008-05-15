#ifndef ADDriverBase_H
#define ADDriverBase_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>
#include <asynStandardInterfaces.h>

#include "asynNDArrayBase.h"

class ADDriverBase : public asynNDArrayBase {
public:
    ADDriverBase(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory);
                 
    /* These are the methods that we override from asynParamBase */
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                     const char **pptypeName, size_t *psize);
                                     
    /* These are the methods that are new to this class */
    int createFileName(int maxChars, char *fullFileName);

    /* Our data */
};

    
#endif
