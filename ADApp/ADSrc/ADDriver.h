#ifndef ADDriver_H
#define ADDriver_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>
#include <asynStandardInterfaces.h>

#include "asynNDArrayDriver.h"

/** This is the class from which area detector drivers are directly derived.  ADDriver
    inherits from asynNDArrayDriver.
  */
class ADDriver : public asynNDArrayDriver {
public:
/** This is the constructor for the class. */
    ADDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
             int interfaceMask, int interruptMask,
             int asynFlags, int autoConnect, int priority, int stackSize);

    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                                     const char **pptypeName, size_t *psize);

    /* These are the methods that are new to this class */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    int createFileName(int maxChars, char *fullFileName);
    void setShutter(int open);

    /* Our data */
};


#endif
