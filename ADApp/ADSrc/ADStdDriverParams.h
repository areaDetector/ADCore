#ifndef AD_STD_DRIVER_PARAMS_H
#define AD_STD_DRIVER_PARAMS_H

#include "asynPortDriver.h"
#include "NDStdDriverParams.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    ADStatusIdle,       /**< Detector is idle */
    ADStatusAcquire,    /**< Detector is acquiring */
    ADStatusReadout,    /**< Detector is reading out */
    ADStatusCorrect,    /**< Detector is correcting data */
    ADStatusSaving,     /**< Detector is saving data */
    ADStatusAborting,   /**< Detector is aborting an operation */
    ADStatusError,      /**< Detector has encountered an error */
    ADStatusWaiting     /**< Detector is waiting for something, typically for the acquire period to elapse */
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

/** Enumeration of parameters that affect the behaviour of the detector. 
  * These are the values that asyn will place in pasynUser->reason when the
  * standard asyn interface methods are called. */
typedef enum
{
    /*    Name          asyn interface  access   Description  */
    ADManufacturer      /**< (asynOctet,    r/o) Detector manufacturer name */
      = NDLastStdParam, 
    ADModel,            /**< (asynOctet,    r/o) Detector model name */

    ADGain,             /**< (asynFloat64,  r/w) Gain */

    /* Parameters that control the detector binning */
    ADBinX,             /**< (asynInt32,    r/w) Binning in the X direction */
    ADBinY,             /**< (asynInt32,    r/w) Binning in the Y direction */

    /* Parameters the control the region of the detector to be read out.
    * ADMinX, ADMinY, ADSizeX, and ADSizeY are in unbinned pixel units */
    ADMinX,             /**< (asynInt32,    r/w) First pixel in the X direction; 0 is the first pixel on the detector */
    ADMinY,             /**< (asynInt32,    r/w) First pixel in the Y direction; 0 is the first pixel on the detector */
    ADSizeX,            /**< (asynInt32,    r/w) Size of the region to read in the X direction */
    ADSizeY,            /**< (asynInt32,    r/w) Size of the region to read in the Y direction */
    ADMaxSizeX,         /**< (asynInt32,    r/o) Maximum (sensor) size in the X direction */
    ADMaxSizeY,         /**< (asynInt32,    r/o) Maximum (sensor) size in the Y direction */

    /* Parameters that control the orientation of the image */
    ADReverseX,         /**< (asynInt32,    r/w) Reverse image in the X direction (0=No, 1=Yes) */
    ADReverseY,         /**< (asynInt32,    r/w) Reverse image in the Y direction (0=No, 1=Yes) */

    /* Parameters defining the acquisition parameters. */
    ADFrameType,        /**< (asynInt32,    r/w) Frame type (ADFrameType_t) */
    ADImageMode,        /**< (asynInt32,    r/w) Image mode (ADImageMode_t) */
    ADTriggerMode,      /**< (asynInt32,    r/w) Trigger mode (ADTriggerMode_t) */
    ADNumExposures,     /**< (asynInt32,    r/w) Number of exposures per image to acquire */
    ADNumImages,        /**< (asynInt32,    r/w) Number of images to acquire in one acquisition sequence */
    ADAcquireTime,      /**< (asynFloat64,  r/w) Acquisition time per image */
    ADAcquirePeriod,    /**< (asynFloat64,  r/w) Acquisition period between images */
    ADStatus,           /**< (asynInt32,    r/o) Acquisition status (ADStatus_t) */
    ADAcquire,          /**< (asynInt32,    r/w) Start(1) or Stop(0) acquisition */

    /* Shutter parameters */
    ADShutterControl,   /**< (asynInt32,    r/w) (ADShutterStatus_t) Open (1) or Close(0) shutter */
    ADShutterControlEPICS, /**< (asynInt32,  r/w) (ADShutterStatus_t) Open (1) or Close(0) EPICS shutter */
    ADShutterStatus,    /**< (asynInt32,    r/o) (ADShutterStatus_t) Shutter Open (1) or Closed(0) */
    ADShutterMode,      /**< (asynInt32,    r/w) (ADShutterMode_t) Use EPICS or detector shutter */
    ADShutterOpenDelay, /**< (asynFloat64,  r/w) Time for shutter to open */
    ADShutterCloseDelay,/**< (asynFloat64,  r/w) Time for shutter to close */

    /* Temperature parameters */
    ADTemperature,      /**< (asynFloat64,  r/w) Detector temperature */

    /* Statistics on number of images collected and the image rate */
    ADNumImagesCounter, /**< (asynInt32,    r/w) Number of images collected in current acquisition sequence */
    ADNumExposuresCounter, /**< (asynInt32, r/w) Number of exposures collected for current image */
    ADTimeRemaining,    /**< (asynFloat64,  r/w) Acquisition time remaining */

    /* Status reading */
    ADReadStatus,      /**< (asynInt32,     r/w) Write 1 to force a read of detector status */

    /* Status message strings */
    ADStatusMessage,    /**< (asynOctet,    r/o) Status message */
    ADStringToServer,   /**< (asynOctet,    r/o) String sent to server for message-based drivers */
    ADStringFromServer, /**< (asynOctet,    r/o) String received from server for message-based drivers */

    ADLastStdParam      /**< The last standard driver parameter; 
                          * Drivers must begin their detector-specific parameter enums with this value */
} ADStdDriverParam_t;

/** If DEFINE_AD_STANDARD_PARAMS is true then these parameter strings are defined
  * for the userParam argument for asyn device support links
  * The asynDrvUser interface in the drivers parses these strings and puts the
  * corresponding enum value in pasynUser->reason */
#ifdef DEFINE_AD_STANDARD_PARAMS
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
#endif

#ifdef __cplusplus
}
#endif
#endif
