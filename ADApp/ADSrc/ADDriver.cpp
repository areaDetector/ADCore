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

#include "ADDriver.h"

static const char *driverName = "ADDriver";

static asynParamString_t ADStdDriverParamString[] = {
    {ADManufacturer,   "MANUFACTURER"},
    {ADModel,          "MODEL"       },

    {ADGain,           "GAIN"        },

    {ADBinX,           "BIN_X"       },
    {ADBinY,           "BIN_Y"       },

    {ADMinX,           "MIN_X"       },
    {ADMinY,           "MIN_Y"       },
    {ADSizeX,          "SIZE_X"      },
    {ADSizeY,          "SIZE_Y"      },
    {ADMaxSizeX,       "MAX_SIZE_X"  },
    {ADMaxSizeY,       "MAX_SIZE_Y"  },
    {ADReverseX,       "REVERSE_X"   },
    {ADReverseY,       "REVERSE_Y"   },

    {ADFrameType,      "FRAME_TYPE"  },
    {ADImageMode,      "IMAGE_MODE"  },
    {ADNumExposures,   "NEXPOSURES"  },
    {ADNumExposuresCounter, "NEXPOSURES_COUNTER"  },
    {ADNumImages,      "NIMAGES"     },
    {ADNumImagesCounter, "NIMAGES_COUNTER"},
    {ADAcquireTime,    "ACQ_TIME"    },
    {ADAcquirePeriod,  "ACQ_PERIOD"  },
    {ADTimeRemaining,  "TIME_REMAINING"},
    {ADStatus,         "STATUS"      },
    {ADTriggerMode,    "TRIGGER_MODE"},
    {ADAcquire,        "ACQUIRE"     },

    {ADShutterControl,   "SHUTTER_CONTROL"},
    {ADShutterControlEPICS, "SHUTTER_CONTROL_EPICS"},
    {ADShutterStatus,    "SHUTTER_STATUS"},
    {ADShutterMode,      "SHUTTER_MODE"        },
    {ADShutterOpenDelay, "SHUTTER_OPEN_DELAY"  },
    {ADShutterCloseDelay,"SHUTTER_CLOSE_DELAY" },

    {ADTemperature,    "TEMPERATURE" },

    {ADReadStatus,     "READ_STATUS"     },

    {ADStatusMessage,  "STATUS_MESSAGE"     },
    {ADStringToServer, "STRING_TO_SERVER"   },
    {ADStringFromServer,"STRING_FROM_SERVER"},
};

#define NUM_AD_STANDARD_PARAMS (sizeof(ADStdDriverParamString)/sizeof(ADStdDriverParamString[0]))

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
    double delay;
    double shutterOpenDelay, shutterCloseDelay;

    getIntegerParam(ADShutterMode, (int *)&shutterMode);
    getDoubleParam(ADShutterOpenDelay, &shutterOpenDelay);
    getDoubleParam(ADShutterCloseDelay, &shutterCloseDelay);

    switch (shutterMode) {
        case ADShutterModeNone:
            break;
        case ADShutterModeEPICS:
            setIntegerParam(ADShutterControlEPICS, open);
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
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    const char *functionName = "writeInt32";

    status = setIntegerParam(function, value);

    switch (function) {
    case ADShutterControl:
        setShutter(value);
        break;
    default:
        break;
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, value=%d\n",
              driverName, functionName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%d\n",
              driverName, functionName, function, value);
    return status;
}


/** Sets pasynUser->reason to one of the enum values for the parameters defined in ADStdDriverParams.h
  * if the drvInfo field matches one the strings defined in that file.
  * Simply calls asynPortDriver::drvUserCreateParam with the parameter table for this driver.
  * \param[in] pasynUser pasynUser structure that driver modifies
  * \param[in] drvInfo String containing information about what driver function is being referenced
  * \param[out] pptypeName Location in which driver puts a copy of drvInfo.
  * \param[out] psize Location where driver puts size of param 
  * \return Returns asynSuccess if a matching string was found, asynError if not found. */
asynStatus ADDriver::drvUserCreate(asynUser *pasynUser,
                                       const char *drvInfo, 
                                       const char **pptypeName, size_t *psize)
{
    asynStatus status;
    //const char *functionName = "drvUserCreate";
    
    /* See if this is one of our standard parameters */
    status = this->drvUserCreateParam(pasynUser, drvInfo, pptypeName, psize, 
                                      ADStdDriverParamString, NUM_AD_STANDARD_PARAMS);
                                      
    /* If not then see if it is a base class parameter */
    if (status) status = asynNDArrayDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
    return(status);
}



/** All of the arguments are simply passed to
  * the constructor for the asynNDArrayDriver base class. After calling the base class
  * constructor this method sets reasonable default values for all of the parameters
  * defined in ADStdDriverParams.h.
  */
ADDriver::ADDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
                   int interfaceMask, int interruptMask,
                   int asynFlags, int autoConnect, int priority, int stackSize)

    : asynNDArrayDriver(portName, maxAddr, paramTableSize, maxBuffers, maxMemory,
          interfaceMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynGenericPointerMask | asynDrvUserMask,
          interruptMask | asynInt32Mask | asynFloat64Mask | asynOctetMask | asynGenericPointerMask,
          asynFlags, autoConnect, priority, stackSize)

{
    //char *functionName = "ADDriver";

    /* Set some default values for parameters */
    setStringParam(ADManufacturer, "Unknown");
    setStringParam(ADModel,        "Unknown");
    setDoubleParam (ADGain,         1.0);
    setIntegerParam(ADBinX,         1);
    setIntegerParam(ADBinY,         1);
    setIntegerParam(ADMinX,         0);
    setIntegerParam(ADMinY,         0);
    setIntegerParam(ADSizeX,        1);
    setIntegerParam(ADSizeY,        1);
    setIntegerParam(ADMaxSizeX,     1);
    setIntegerParam(ADMaxSizeY,     1);
    setIntegerParam(ADReverseX,     0);
    setIntegerParam(ADReverseY,     0);
    setIntegerParam(ADFrameType,    ADFrameNormal);
    setIntegerParam(ADImageMode,    ADImageContinuous);
    setIntegerParam(ADTriggerMode,  0);
    setIntegerParam(ADNumExposures, 1);
    setIntegerParam(ADNumImages,    1);
    setDoubleParam (ADAcquireTime,  1.0);
    setDoubleParam (ADAcquirePeriod,0.0);
    setIntegerParam(ADStatus,       ADStatusIdle);
    setIntegerParam(ADAcquire,      0);
    setIntegerParam(ADNumImagesCounter, 0);
    setIntegerParam(ADNumExposuresCounter, 0);
    setDoubleParam( ADTimeRemaining, 0.0);
    setIntegerParam(ADShutterControl, 0);
    setIntegerParam(ADShutterStatus, 0);
    setIntegerParam(ADShutterMode,   0);
    setDoubleParam (ADShutterOpenDelay, 0.0);
    setDoubleParam (ADShutterCloseDelay, 0.0);
    setDoubleParam (ADTemperature, 0.0);

    setStringParam (ADStatusMessage,  "");
    setStringParam (ADStringToServer, "");
    setStringParam (ADStringFromServer,  "");
}
