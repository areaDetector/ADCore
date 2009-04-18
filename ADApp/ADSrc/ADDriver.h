#ifndef ADDriver_H
#define ADDriver_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>
#include <asynStandardInterfaces.h>

#include "asynNDArrayDriver.h"

/** Class from which areaDetector drivers are directly derived. */
class ADDriver : public asynNDArrayDriver {
public:
    /* This is the constructor for the class. */
    ADDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
             int interfaceMask, int interruptMask,
             int asynFlags, int autoConnect, int priority, int stackSize);

    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                                     const char **pptypeName, size_t *psize);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

    /* These are the methods that are new to this class */
    int createFileName(int maxChars, char *fullFileName);
    void setShutter(int open);
};


#endif
