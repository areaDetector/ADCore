#ifndef ADDriver_H
#define ADDriver_H

#include <epicsTypes.h>
#include <epicsMessageQueue.h>
#include <epicsTime.h>
#include <asynStandardInterfaces.h>

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
#define ADManufacturerString        "MANUFACTURER"          /**< (asynOctet,    r/o) Detector manufacturer name */
#define ADModelString               "MODEL"                 /**< (asynOctet,    r/o) Detector model name */

#define ADGainString                "GAIN"                  /**< (asynFloat64,  r/w) Gain */

    /* Parameters that control the detector binning */
#define ADBinXString                "BIN_X"                 /**< (asynInt32,    r/w) Binning in the X direction */
#define ADBinYString                "BIN_Y"                 /**< (asynInt32,    r/w) Binning in the Y direction */

    /* Parameters the control the region of the detector to be read out.
    * ADMinX, ADMinY, ADSizeX, and ADSizeY are in unbinned pixel units */
#define ADMinXString                "MIN_X"                 /**< (asynInt32,    r/w) First pixel in the X direction; 0 is the first pixel on the detector */
#define ADMinYString                "MIN_Y"                 /**< (asynInt32,    r/w) First pixel in the Y direction; 0 is the first pixel on the detector */
#define ADSizeXString               "SIZE_X"                /**< (asynInt32,    r/w) Size of the region to read in the X direction */
#define ADSizeYString               "SIZE_Y"                /**< (asynInt32,    r/w) Size of the region to read in the Y direction */
#define ADMaxSizeXString            "MAX_SIZE_X"            /**< (asynInt32,    r/o) Maximum (sensor) size in the X direction */
#define ADMaxSizeYString            "MAX_SIZE_Y"            /**< (asynInt32,    r/o) Maximum (sensor) size in the Y direction */

    /* Parameters that control the orientation of the image */
#define ADReverseXString            "REVERSE_X"             /**< (asynInt32,    r/w) Reverse image in the X direction (0=No, 1=Yes) */
#define ADReverseYString            "REVERSE_Y"             /**< (asynInt32,    r/w) Reverse image in the Y direction (0=No, 1=Yes) */

    /* Parameters defining the acquisition parameters. */
#define ADFrameTypeString           "FRAME_TYPE"            /**< (asynInt32,    r/w) Frame type (ADFrameType_t) */
#define ADImageModeString           "IMAGE_MODE"            /**< (asynInt32,    r/w) Image mode (ADImageMode_t) */
#define ADTriggerModeString         "TRIGGER_MODE"          /**< (asynInt32,    r/w) Trigger mode (ADTriggerMode_t) */
#define ADNumExposuresString        "NEXPOSURES"            /**< (asynInt32,    r/w) Number of exposures per image to acquire */
#define ADNumImagesString           "NIMAGES"               /**< (asynInt32,    r/w) Number of images to acquire in one acquisition sequence */
#define ADAcquireTimeString         "ACQ_TIME"              /**< (asynFloat64,  r/w) Acquisition time per image */
#define ADAcquirePeriodString       "ACQ_PERIOD"            /**< (asynFloat64,  r/w) Acquisition period between images */
#define ADStatusString              "STATUS"                /**< (asynInt32,    r/o) Acquisition status (ADStatus_t) */
#define ADAcquireString             "ACQUIRE"               /**< (asynInt32,    r/w) Start(1) or Stop(0) acquisition */

    /* Shutter parameters */
#define ADShutterControlString      "SHUTTER_CONTROL"       /**< (asynInt32,    r/w) (ADShutterStatus_t) Open (1) or Close(0) shutter */
#define ADShutterControlEPICSString "SHUTTER_CONTROL_EPICS" /**< (asynInt32, r/o) (ADShutterStatus_t) Open (1) or Close(0) EPICS shutter */
#define ADShutterStatusString       "SHUTTER_STATUS"        /**< (asynInt32,    r/o) (ADShutterStatus_t) Shutter Open (1) or Closed(0) */
#define ADShutterModeString         "SHUTTER_MODE"          /**< (asynInt32,    r/w) (ADShutterMode_t) Use EPICS or detector shutter */
#define ADShutterOpenDelayString    "SHUTTER_OPEN_DELAY"    /**< (asynFloat64,  r/w) Time for shutter to open */
#define ADShutterCloseDelayString   "SHUTTER_CLOSE_DELAY"   /**< (asynFloat64,  r/w) Time for shutter to close */

    /* Temperature parameters */
#define ADTemperatureString         "TEMPERATURE"           /**< (asynFloat64,  r/w) Detector temperature */
#define ADTemperatureActualString   "TEMPERATURE_ACTUAL"    /**< (asynFloat64,  r/o) Actual detector temperature */

    /* Statistics on number of images collected and the image rate */
#define ADNumImagesCounterString    "NIMAGES_COUNTER"       /**< (asynInt32,    r/o) Number of images collected in current acquisition sequence */
#define ADNumExposuresCounterString "NEXPOSURES_COUNTER"    /**< (asynInt32, r/o) Number of exposures collected for current image */
#define ADTimeRemainingString       "TIME_REMAINING"        /**< (asynFloat64,  r/o) Acquisition time remaining */

    /* Status reading */
#define ADReadStatusString          "READ_STATUS"           /**< (asynInt32,     r/w) Write 1 to force a read of detector status */

    /* Status message strings */
#define ADStatusMessageString       "STATUS_MESSAGE"        /**< (asynOctet,    r/o) Status message */
#define ADStringToServerString      "STRING_TO_SERVER"      /**< (asynOctet,    r/o) String sent to server for message-based drivers */
#define ADStringFromServerString    "STRING_FROM_SERVER"    /**< (asynOctet,    r/o) String received from server for message-based drivers */

/** Class from which areaDetector drivers are directly derived. */
class epicsShareClass ADDriver : public asynNDArrayDriver {
public:
    /* This is the constructor for the class. */
    ADDriver(const char *portName, int maxAddr, int numParams, int maxBuffers, size_t maxMemory,
             int interfaceMask, int interruptMask,
             int asynFlags, int autoConnect, int priority, int stackSize);

    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

    /* These are the methods that are new to this class */
    virtual void setShutter(int open);

protected:
    int ADManufacturer;
    #define FIRST_AD_PARAM ADManufacturer
    int ADModel;
    int ADGain;
    int ADBinX;
    int ADBinY;
    int ADMinX;
    int ADMinY;
    int ADSizeX;
    int ADSizeY;
    int ADMaxSizeX;
    int ADMaxSizeY;
    int ADReverseX;
    int ADReverseY;
    int ADFrameType;
    int ADImageMode;
    int ADNumExposures;
    int ADNumExposuresCounter;
    int ADNumImages;
    int ADNumImagesCounter;
    int ADAcquireTime;
    int ADAcquirePeriod;
    int ADTimeRemaining;
    int ADStatus;
    int ADTriggerMode;
    int ADAcquire;
    int ADShutterControl;
    int ADShutterControlEPICS;
    int ADShutterStatus;
    int ADShutterMode;
    int ADShutterOpenDelay;
    int ADShutterCloseDelay;
    int ADTemperature;
    int ADTemperatureActual;
    int ADReadStatus;
    int ADStatusMessage;
    int ADStringToServer;
    int ADStringFromServer; 
    #define LAST_AD_PARAM ADStringFromServer   
};
#define NUM_AD_PARAMS ((int)(&LAST_AD_PARAM - &FIRST_AD_PARAM + 1))

#endif
