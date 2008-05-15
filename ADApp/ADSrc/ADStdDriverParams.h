#ifndef AD_STD_DRIVER_PARAMS_H
#define AD_STD_DRIVER_PARAMS_H

#include <ellLib.h>
#include <epicsMutex.h>

#include "asynParamBase.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILENAME_LEN 256
#define AREA_DETECTOR_OK 0
#define AREA_DETECTOR_ERROR -1

/* Enumeration of shutter status */
typedef enum
{
    ADShutterClosed, 
    ADShutterOpen
} ADShutterStatus_t;

/* Enumeration of detector status */
typedef enum
{
    ADStatusIdle,
    ADStatusAcquire,
    ADStatusReadout,
    ASStatusCorrect,
    ADStatusSaving,
    ADStatusAborting,
    ADStatusError,
} ADStatus_t;

typedef enum
{
    ADImageSingle,
    ADImageMultiple,
    ADImageContinuous
} ADImageMode_t;

typedef enum
{
    ADTriggerInternal,
    ADTriggerExternal
} ADTriggerMode_t;

    
/**

    This is an enumeration of parameters that affect the behaviour of the
    detector. These are the values that asyn will place in pasynUser->reason when the
    standard asyn interface methods are called.
*/

typedef enum
{
    /*    Name          asyn interface  access   Description  */
    
    ADManufacturer_RBV,    /* (asynOctet,    r/o) Detector manufacturer name */ 
    ADModel_RBV,           /* (asynOctet,    r/o) Detector model name */

    ADGain,                /* (asynFloat64,  r/w) Gain. */
    ADGain_RBV,            /* (asynFloat64,  r/w) Gain. */

    /* Parameters that control the detector binning */
    ADBinX,                /* (asynInt32,    r/w) Binning in the X direction */
    ADBinX_RBV,            /* (asynInt32,    r/w) Binning in the X direction */
    ADBinY,                /* (asynInt32.    r/w) Binning in the Y direction */
    ADBinY_RBV,            /* (asynInt32.    r/w) Binning in the Y direction */

    /* Parameters the control the region of the detector to be read out.
    * ADMinX, ADMinY, ADSizeX, and ADSizeY are in unbinned pixel units */
    ADMinX,                /* (asynInt32,    r/w) First pixel in the X direction.  0 is the first pixel on the detector */
    ADMinX_RBV,            /* (asynInt32,    r/w) First pixel in the X direction.  0 is the first pixel on the detector */
    ADMinY,                /* (asynInt32,    r/w) First pixel in the Y direction.  0 is the first pixel on the detector */
    ADMinY_RBV,            /* (asynInt32,    r/w) First pixel in the Y direction.  0 is the first pixel on the detector */
    ADSizeX,               /* (asynInt32,    r/w) Size of the region to read in the X direction. */
    ADSizeX_RBV,           /* (asynInt32,    r/w) Size of the region to read in the X direction. */
    ADSizeY,               /* (asynInt32,    r/w) Size of the region to read in the Y direction. */
    ADSizeY_RBV,           /* (asynInt32,    r/w) Size of the region to read in the Y direction. */
    ADMaxSizeX_RBV,        /* (asynInt32,    r/o) Maximum (sensor) size in the X direction. */
    ADMaxSizeY_RBV,        /* (asynInt32,    r/o) Maximum (sensor) size in the Y direction. */

    /* Parameters that control the orientation of the image */
    ADReverseX,            /* (asynInt32,    r/w) Reverse image in the X direction (0=No, 1=Yes) */
    ADReverseX_RBV,        /* (asynInt32,    r/w) Reverse image in the X direction (0=No, 1=Yes) */
    ADReverseY,            /* (asynInt32.    r/w) Reverse image in the Y direction (0=No, 1=Yes) */
    ADReverseY_RBV,        /* (asynInt32.    r/w) Reverse image in the Y direction (0=No, 1=Yes) */

    /* Parameters defining the size of the image data from the detector.
     * ADImageSizeX and ADImageSizeY are the actual dimensions of the image data, 
     * including effects of the region definition and binning */
    ADImageSizeX_RBV,      /* (asynInt32,    r/o) Size of the image data in the X direction */
    ADImageSizeY_RBV,      /* (asynInt32,    r/o) Size of the image data in the Y direction */
    ADImageSize_RBV,       /* (asynInt32,    r/o) Total size of image data in bytes */
    ADDataType,            /* (asynInt32,    r/w) Data type (ADDataType_t) */
    ADDataType_RBV,        /* (asynInt32,    r/w) Data type (ADDataType_t) */
    ADImageMode,           /* (asynInt32,    r/w) Image mode (ADImageMode_t) */
    ADImageMode_RBV,       /* (asynInt32,    r/w) Image mode (ADImageMode_t) */
    ADTriggerMode,         /* (asynInt32,    r/w) Trigger mode (ADTriggerMode_t) */
    ADTriggerMode_RBV,     /* (asynInt32,    r/w) Trigger mode (ADTriggerMode_t) */
    ADNumExposures,        /* (asynInt32,    r/w) Number of exposures per image to acquire */
    ADNumExposures_RBV,    /* (asynInt32,    r/w) Number of exposures per image to acquire */
    ADNumImages,           /* (asynInt32,    r/w) Number of images to acquire in one acquisition sequence */
    ADNumImages_RBV,       /* (asynInt32,    r/w) Number of images to acquire in one acquisition sequence */
    ADAcquireTime,         /* (asynFloat64,  r/w) Acquisition time per image. */
    ADAcquireTime_RBV,     /* (asynFloat64,  r/w) Acquisition time per image. */
    ADAcquirePeriod,       /* (asynFloat64,  r/w) Acquisition period between images */
    ADAcquirePeriod_RBV,   /* (asynFloat64,  r/w) Acquisition period between images */
    ADStatus_RBV,          /* (asynInt32,    r/o) Acquisition status (ADStatus_t) */
    ADShutter,             /* (asynInt32,    r/w) Shutter control (ADShutterStatus_t) */
    ADShutter_RBV,         /* (asynInt32,    r/w) Shutter control (ADShutterStatus_t) */
    ADAcquire,             /* (asynInt32,    r/w) Start(1) or Stop(0) acquisition */
    ADAcquire_RBV,         /* (asynInt32,    r/w) Start(1) or Stop(0) acquisition */

    /* Statistics on number of images collected and the image rate. */
    ADImageCounter,        /* (asynInt32,    r/w) Number of images acquired since last reset */
    ADImageCounter_RBV,    /* (asynInt32,    r/w) Number of images acquired since last reset */
 
    /* File name related parameters for saving data.
     * Drivers are not required to implement file saving, but if they do these parameters
     * should be used.
     * The driver will normally combine ADFilePath, ADFileName, and ADFileNumber into
     * a file name that order using the format specification in ADFileTemplate. 
     * For example ADFileTemplate might be "%s%s_%d.tif". */
    ADFilePath,            /* (asynOctet,    r/w) The file path. */
    ADFilePath_RBV,        /* (asynOctet,    r/w) The file path. */
    ADFileName,            /* (asynOctet,    r/w) The file name. */
    ADFileName_RBV,        /* (asynOctet,    r/w) The file name. */
    ADFileNumber,          /* (asynInt32,    r/w) The next file number. */
    ADFileNumber_RBV,      /* (asynInt32,    r/w) The next file number. */
    ADFileTemplate,        /* (asynOctet,    r/w) The format asynOctet. */
    ADFileTemplate_RBV,    /* (asynOctet,    r/w) The format asynOctet. */
    ADAutoIncrement,       /* (asynInt32,    r/w) Autoincrement file number. 0=No, 1=Yes */
    ADAutoIncrement_RBV,   /* (asynInt32,    r/w) Autoincrement file number. 0=No, 1=Yes */
    ADFullFileName_RBV,    /* (asynOctet,    r/o) The actual complete file name for the last file saved. */
    ADFileFormat,          /* (asynInt32,    r/w) The data format to use for saving the file.  */
    ADFileFormat_RBV,      /* (asynInt32,    r/w) The data format to use for saving the file.  */
    ADAutoSave,            /* (asynInt32,    r/w) Automatically save files */
    ADAutoSave_RBV,        /* (asynInt32,    r/w) Automatically save files */
    ADWriteFile,           /* (asynInt32,    r/w) Manually save the most recent image to a file when value=1 */
    ADWriteFile_RBV,       /* (asynInt32,    r/w) Manually save the most recent image to a file when value=1 */
    ADReadFile,            /* (asynInt32,    r/w) Manually read file when value=1 */
    ADReadFile_RBV,        /* (asynInt32,    r/w) Manually read file when value=1 */

    NDArrayData,           /* (asynHandle,   r/w) NDArray data */
    
    ADFirstDriverParam     /* The last parameter, used by the standard driver */
                           /* Drivers that use ADParamLib should begin their parameters with this value. */
} ADStdDriverParam_t;

#ifdef DEFINE_AD_STANDARD_PARAMS
/* The parameter strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in the drivers parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
 
static asynParamString_t ADStdDriverParamString[] = {
    {ADManufacturer_RBV,   "MANUFACTURER_RBV"},  
    {ADModel_RBV,          "MODEL_RBV"       },  

    {ADGain,               "GAIN"        },
    {ADGain_RBV,           "GAIN_RBV"        },

    {ADBinX,               "BIN_X"       },
    {ADBinX_RBV,           "BIN_X_RBV"       },
    {ADBinY,               "BIN_Y"       },
    {ADBinY_RBV,           "BIN_Y_RBV"       },

    {ADMinX,               "MIN_X"       },
    {ADMinX_RBV,           "MIN_X_RBV"       },
    {ADMinY,               "MIN_Y"       },
    {ADMinY_RBV,           "MIN_Y_RBV"       },
    {ADSizeX,              "SIZE_X"      },
    {ADSizeX_RBV,          "SIZE_X_RBV"      },
    {ADSizeY,              "SIZE_Y"      },
    {ADSizeY_RBV,          "SIZE_Y_RBV"      },
    {ADMaxSizeX_RBV,       "MAX_SIZE_X_RBV"  },
    {ADMaxSizeY_RBV,       "MAX_SIZE_Y_RBV"  },
    {ADReverseX,           "REVERSE_X"   },
    {ADReverseX_RBV,       "REVERSE_X_RBV"   },
    {ADReverseY,           "REVERSE_Y"   },
    {ADReverseY_RBV,       "REVERSE_Y_RBV"   },

    {ADImageSizeX_RBV,     "IMAGE_SIZE_X_RBV"},
    {ADImageSizeY_RBV,     "IMAGE_SIZE_Y_RBV"},
    {ADImageSize_RBV,      "IMAGE_SIZE_RBV"  },
    {ADDataType,           "DATA_TYPE"   },
    {ADDataType_RBV,       "DATA_TYPE_RBV"   },
    {ADImageMode,          "IMAGE_MODE"  },
    {ADImageMode_RBV,      "IMAGE_MODE_RBV"  },
    {ADNumExposures,       "NEXPOSURES"  },
    {ADNumExposures_RBV,   "NEXPOSURES_RBV"  },
    {ADNumImages,          "NIMAGES"     },
    {ADNumImages_RBV,      "NIMAGES_RBV"     },
    {ADAcquireTime,        "ACQ_TIME"    },
    {ADAcquireTime_RBV,    "ACQ_TIME_RBV"    },
    {ADAcquirePeriod,      "ACQ_PERIOD"  },
    {ADAcquirePeriod_RBV,  "ACQ_PERIOD_RBV"  },
    {ADStatus_RBV,         "STATUS_RBV"      },
    {ADTriggerMode,        "TRIGGER_MODE"},
    {ADTriggerMode_RBV,    "TRIGGER_MODE_RBV"},
    {ADShutter,            "SHUTTER"     },
    {ADShutter_RBV,        "SHUTTER_RBV"     },
    {ADAcquire,            "ACQUIRE"     },
    {ADAcquire_RBV,        "ACQUIRE_RBV"     },

    {ADImageCounter,       "IMAGE_COUNTER" }, 
    {ADImageCounter_RBV,   "IMAGE_COUNTER_RBV" }, 

    {ADFilePath,           "FILE_PATH"     },
    {ADFilePath_RBV,       "FILE_PATH_RBV"     },
    {ADFileName,           "FILE_NAME"     },
    {ADFileName_RBV,       "FILE_NAME_RBV"     },
    {ADFileNumber,         "FILE_NUMBER"   },
    {ADFileNumber_RBV,     "FILE_NUMBER_RBV"   },
    {ADFileTemplate,       "FILE_TEMPLATE" },
    {ADFileTemplate_RBV,   "FILE_TEMPLATE_RBV" },
    {ADAutoIncrement,      "AUTO_INCREMENT"},
    {ADAutoIncrement_RBV,  "AUTO_INCREMENT_RBV"},
    {ADFullFileName_RBV,   "FULL_FILE_NAME_RBV"},
    {ADFileFormat,         "FILE_FORMAT"   },
    {ADFileFormat_RBV,     "FILE_FORMAT_RBV"   },
    {ADAutoSave,           "AUTO_SAVE"     },
    {ADAutoSave_RBV,       "AUTO_SAVE_RBV"     },
    {ADWriteFile,          "WRITE_FILE"    },
    {ADWriteFile_RBV,      "WRITE_FILE_RBV"    },
    {ADReadFile,           "READ_FILE"     },
    {ADReadFile_RBV,       "READ_FILE_RBV"     },

    {NDArrayData,      "NDARRAY_DATA"  },
};

#define NUM_AD_STANDARD_PARAMS (sizeof(ADStdDriverParamString)/sizeof(ADStdDriverParamString[0]))
#endif

#ifdef __cplusplus
}
#endif
#endif
