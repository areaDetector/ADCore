/** ADDriver.cpp
 *
 * This is the base class from which actual area detectors are derived.
 *
 * /author Author: Mark Rivers
 *         University of Chicago
 *
 * Created:  March 20, 2008
 *
 */

#include <stdio.h>

#include <epicsString.h>
#include <epicsThread.h>
#include <asynStandardInterfaces.h>

#include <asynPortDriver.h>

#include <epicsExport.h>
#include "ADDriver.h"

static const char *driverName = "ADDriver";

/** Set the shutter position.
  * This method will open (1) or close (0) the shutter if
  * ADShutterMode==ADShutterModeEPICS. Drivers will implement setShutter if they
  * support ADShutterModeDetector. If ADShutterMode=ADShutterModeDetector they will
  * control the shutter directly, else they will call this method.
  * \param[in] open 1 (open) or 0 (closed)
  */
void ADDriver::setShutter(int open)
{
    ADShutterMode_t shutterMode;
    int itemp;
    double delay;
    double shutterOpenDelay, shutterCloseDelay;

    getIntegerParam(paramSet->ADShutterMode, &itemp); shutterMode = (ADShutterMode_t)itemp;
    getDoubleParam(paramSet->ADShutterOpenDelay, &shutterOpenDelay);
    getDoubleParam(paramSet->ADShutterCloseDelay, &shutterCloseDelay);

    switch (shutterMode) {
        case ADShutterModeNone:
            break;
        case ADShutterModeEPICS:
            setIntegerParam(paramSet->ADShutterControlEPICS, open);
            callParamCallbacks();
            delay = shutterOpenDelay - shutterCloseDelay;
            epicsThreadSleep(delay);
            break;
        case ADShutterModeDetector:
            break;
    }
}

/** Sets an int32 parameter.
  * \param[in] pasynUser asynUser structure that contains the function code in pasynUser->reason.
  * \param[in] value The value for this parameter
  *
  * Takes action if the function code requires it.  Currently only ADShutterControl requires
  * action here.  This method is normally called from the writeInt32 method in derived classes, which
  * should set the value of the parameter in the parameter library. */
asynStatus ADDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function;
    int addr;
    const char *paramName;
    asynStatus status = asynSuccess;
    const char *functionName = "writeInt32";

    status = parseAsynUser(pasynUser, &function, &addr, &paramName);
    if (status != asynSuccess) return status;

    status = setIntegerParam(addr, function, value);

    if (function == paramSet->ADShutterControl) {
        setShutter(value);
    } else {
        /* If this parameter belongs to a base class call its method */
        if (function < FIRST_AD_PARAM) status = asynNDArrayDriver::writeInt32(pasynUser, value);
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, paramName=%s, value=%d\n",
              driverName, functionName, status, function, paramName, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, paramName=%s, value=%d\n",
              driverName, functionName, function, paramName, value);
    return status;
}


/** All of the arguments are simply passed to the constructor for the asynNDArrayDriver base class,
  * except numParams.  As of R3-0 numParams is no longer used in asynNDArrayDriver but we have left
  * it in here to avoid needing to change all drivers yet. In R5-0 we expect to remove maxBuffers and
  * maxMemory as well, so we will wait until then to change the ADDriver constructor arguments.
  * After calling the base class constructor this method sets reasonable default values for all of the parameters
  * defined in ADDriver.h.
  */
ADDriver::ADDriver(ADDriverParamSet* paramSet,
                   const char *portName, int maxAddr, int numParams, int maxBuffers, size_t maxMemory,
                   int interfaceMask, int interruptMask,
                   int asynFlags, int autoConnect, int priority, int stackSize):
    : asynNDArrayDriver(
        paramSet,
        portName, maxAddr, maxBuffers, maxMemory,
        interfaceMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynGenericPointerMask | asynDrvUserMask,
        interruptMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynGenericPointerMask,
        asynFlags, autoConnect, priority, stackSize),
    paramSet(paramSet)
{
    //char *functionName = "ADDriver";

    /* Here we set the values of read-only parameters and of read/write parameters that cannot
     * or should not get their values from the database.  Note that values set here will override
     * those in the database for output records because if asyn device support reads a value from
     * the driver with no error during initialization then it sets the output record to that value.
     * If a value is not set here then the read request will return an error (uninitialized).
     * Values set here will be overridden by values from save/restore if they exist. */
    setIntegerParam(paramSet->ADMaxSizeX, 1);
    setIntegerParam(paramSet->ADMaxSizeY, 1);
    setIntegerParam(paramSet->ADStatus, ADStatusIdle);
    setIntegerParam(paramSet->ADNumImagesCounter, 0);
    setIntegerParam(paramSet->ADNumExposuresCounter, 0);
    setDoubleParam(paramSet->ADTimeRemaining, 0.0);
    setIntegerParam(paramSet->ADShutterStatus, 0);

    setStringParam(paramSet->ADStatusMessage, "");
    setStringParam(paramSet->ADStringToServer, "");
    setStringParam(paramSet->ADStringFromServer, "");
}
