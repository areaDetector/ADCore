#ifndef ADDriver_H
#define ADDriver_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>

#include "ADDriverParamSet.h"
#include "asynNDArrayDriver.h"


/** Success code; generally asyn status codes are used instead where possible */
#define AREA_DETECTOR_OK 0
/** Failure code; generally asyn status codes are used instead where possible */
#define AREA_DETECTOR_ERROR -1

/** Enumeration of shutter status */
typedef enum
{
    ADShutterClosed,    /**< Shutter closed */
    ADShutterOpen       /**< Shutter open */
} ADShutterStatus_t;

/** Enumeration of shutter modes */
typedef enum
{
    ADShutterModeNone,      /**< Don't use shutter */
    ADShutterModeEPICS,     /**< Shutter controlled by EPICS PVs */
    ADShutterModeDetector   /**< Shutter controlled directly by detector */
} ADShutterMode_t;

/** Enumeration of detector status */
typedef enum
{
    ADStatusIdle,         /**< Detector is idle */
    ADStatusAcquire,      /**< Detector is acquiring */
    ADStatusReadout,      /**< Detector is reading out */
    ADStatusCorrect,      /**< Detector is correcting data */
    ADStatusSaving,       /**< Detector is saving data */
    ADStatusAborting,     /**< Detector is aborting an operation */
    ADStatusError,        /**< Detector has encountered an error */
    ADStatusWaiting,      /**< Detector is waiting for something, typically for the acquire period to elapse */
    ADStatusInitializing, /**< Detector is initializing, typically at startup */
    ADStatusDisconnected, /**< Detector is not connected */
    ADStatusAborted       /**< Detector aquisition has been aborted.*/
} ADStatus_t;

/** Enumeration of image collection modes */
typedef enum
{
    ADImageSingle,      /**< Collect a single image per Acquire command */
    ADImageMultiple,    /**< Collect ADNumImages images per Acquire command */
    ADImageContinuous   /**< Collect images continuously until Acquire is set to 0 */
} ADImageMode_t;

/* Enumeration of frame types */
typedef enum
{
    ADFrameNormal,          /**< Normal frame type */
    ADFrameBackground,      /**< Background frame type */
    ADFrameFlatField,       /**< Flat field (no sample) frame type */
    ADFrameDoubleCorrelation      /**< Double correlation frame type, used to remove zingers */
} ADFrameType_t;

/* Enumeration of trigger modes */
typedef enum
{
    ADTriggerInternal,      /**< Internal trigger from detector */
    ADTriggerExternal       /**< External trigger input */
} ADTriggerMode_t;

/** Strings defining parameters that affect the behaviour of the detector.
  * These are the values passed to drvUserCreate.
  * The driver will place in pasynUser->reason an integer to be used when the
  * standard asyn interface methods are called. */
 /*                               String                 asyn interface  access   Description  */


    /* Parameters that control the detector binning */

    /* Parameters the control the region of the detector to be read out.
    * ADMinX, ADMinY, ADSizeX, and ADSizeY are in unbinned pixel units */

    /* Parameters that control the orientation of the image */

    /* Parameters defining the acquisition parameters. */

    /* Shutter parameters */

    /* Temperature parameters */

    /* Statistics on number of images collected and the image rate */

    /* Status reading */

    /* Status message strings */

/** Class from which areaDetector drivers are directly derived. */
class ADCORE_API ADDriver : public asynNDArrayDriver {
public:
    /* This is the constructor for the class. */
    ADDriver(ADDriverParamSet* paramSet, const char *portName, int maxAddr, int numParams, int maxBuffers, size_t maxMemory,
             int interfaceMask, int interruptMask,
             int asynFlags, int autoConnect, int priority, int stackSize);

    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus connect(asynUser *pasynUser);

    /* These are the methods that are new to this class */
    virtual void setShutter(int open);

protected:
    ADDriverParamSet* paramSet;
    #define FIRST_AD_PARAM paramSet->FIRST_ADDRIVERPARAMSET_PARAM
    
    bool deviceIsReachable;
};

#endif