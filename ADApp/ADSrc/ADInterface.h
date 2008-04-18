#ifndef AREA_DETECTOR_INTERFACE_H
#define AREA_DETECTOR_INTERFACE_H

#include <ellLib.h>
#include <epicsMutex.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILENAME_LEN 256

/**             EPICS Area Detector API

                  Mark Rivers
              University of Chicago
                April 2, 2008

Detector drivers are written to use the standard asyn interfaces asynInt32, asynFloat64 and asynOctet.
They must implement those interfaces, and additionally must implement the asynADImage interface
for reading and writing images.  

Whenever any parameter changes the driver must perform the callbacks
on those interfaces.  This can be done conveniently by using the ADParamLib library.


*/

#define AREA_DETECTOR_OK (0)
#define AREA_DETECTOR_ERROR (-1)

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
    
    ADManufacturer,    /* (asynOctet,    r/o) Detector manufacturer name */ 
    ADModel,           /* (asynOctet,    r/o) Detector model name */
    ADGain,            /* (asynFloat64,  r/w) Gain. */

    /* Parameters that control the detector binning */
    ADBinX,            /* (asynInt32,    r/w) Binning in the X direction */
    ADBinY,            /* (asynInt32.    r/w) Binning in the Y dieection */

    /* Parameters the control the region of the detector to be read out.
    * ADMinX, ADMinY, ADSizeX, and ADSizeY are in unbinned pixel units */
    ADMinX,            /* (asynInt32,    r/w) First pixel in the X direction.  0 is the first pixel on the detector */
    ADMinY,            /* (asynInt32,    r/w) First pixel in the Y direction.  0 is the first pixel on the detector */
    ADSizeX,           /* (asynInt32,    r/w) Size of the region to read in the X direction. */
    ADSizeY,           /* (asynInt32,    r/w) Size of the region to read in the Y direction. */
    ADMaxSizeX,        /* (asynInt32,    r/o) Maximum (sensor) size in the X direction. */
    ADMaxSizeY,        /* (asynInt32,    r/o) Maximum (sensro) size in the Y direction. */

    /* Parameters defining the size of the image data from the detector.
     * ADImageSizeX and ADImageSizeY are the actual dimensions of the image data, 
     * including effects of the region definition and binning */
    ADImageSizeX,      /* (asynInt32,    r/o) Size of the image data in the X direction */
    ADImageSizeY,      /* (asynInt32,    r/o) Size of the image data in the Y direction */
    ADImageSize,       /* (asynInt32,    r/o) Total size of image data in bytes */
    ADDataType,        /* (asynInt32,    r/w) Data type (ADDataType_t) */
    ADImageMode,       /* (asynInt32,    r/w) Image mode (ADImageMode_t) */
    ADTriggerMode,     /* (asynInt32,    r/w) Trigger mode (ADTriggerMode_t) */
    ADNumExposures,    /* (asynInt32,    r/w) Number of exposures per image to acquire */
    ADNumImages,       /* (asynInt32,    r/w) Number of images to acquire in one acquisition sequence */
    ADAcquireTime,     /* (asynFloat64,  r/w) Acquisition time per image. */
    ADAcquirePeriod,   /* (asynFloat64,  r/w) Acquisition period between images */
    ADStatus,          /* (asynInt32,    r/o) Acquisition status (ADStatus_t) */
    ADShutter,         /* (asynInt32,    r/w) Shutter control (ADShutterStatus_t) */
    ADAcquire,         /* (asynInt32,    r/w) Start(1) or Stop(0) acquisition */

    /* Statistics on number of imagess collected and the image rate. */
    ADImageCounter,    /* (asynInt32,    r/w) Number of images acquired since last reset */
 
    /* File name related parameters for saving data.
     * Drivers are not required to implement file saving, but if they do these parameters
     * should be used.
     * The driver will normally combine ADFilePath, ADFileName, and ADFileNumber into
     * a file name that order using the format specification in ADFileTemplate. 
     * For example ADFileTemplate might be "%s%s_%d.tif". */
    ADFilePath,        /* (asynOctet,    r/w) The file path. */
    ADFileName,        /* (asynOctet,    r/w) The file name. */
    ADFileNumber,      /* (asynInt32,    r/w) The next file number. */
    ADFileTemplate,    /* (asynOctet,    r/w) The format asynOctet. */
    ADAutoIncrement,   /* (asynInt32,    r/w) Autoincrement file number. 0=No, 1=Yes */
    ADFullFileName,    /* (asynOctet,    r/o) The actual complete file name for the last file saved. */
    ADFileFormat,      /* (asynInt32,    r/w) The data format to use for saving the file.  
                                              The values are enums that will be detector-specific. */
    ADAutoSave,        /* (asynInt32,    r/w) Automatically save files */
    ADWriteFile,       /* (asynInt32,    r/w) Manually save the most recent image to a file when value=1 */
    ADReadFile,        /* (asynInt32,    r/w) Manually read file when value=1 */
    
    ADFirstDriverParam  /* The last parameter, used by the standard driver */
                        /* Drivers that use ADParamLib should begin their parameters with this value. */
} ADParam_t;

typedef struct {
    int param;
    char *paramString;
} ADParamString_t;

#ifdef DEFINE_STANDARD_PARAM_STRINGS
/* The parameter strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in the drivers parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
 
static ADParamString_t ADStandardParamString[] = {
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

    {ADImageSizeX,     "IMAGE_SIZE_X"},
    {ADImageSizeY,     "IMAGE_SIZE_Y"},
    {ADImageSize,      "IMAGE_SIZE"  },
    {ADDataType,       "DATA_TYPE"   },
    {ADImageMode,      "IMAGE_MODE"  },
    {ADNumExposures,   "NEXPOSURES"  },
    {ADNumImages,      "NIMAGES"     },
    {ADAcquireTime,    "ACQ_TIME"    },
    {ADAcquirePeriod,  "ACQ_PERIOD"  },
    {ADStatus,         "STATUS"      },
    {ADTriggerMode,    "TRIGGER_MODE"},
    {ADShutter,        "SHUTTER"     },
    {ADAcquire,        "ACQUIRE"     },

    {ADImageCounter,   "IMAGE_COUNTER" }, 

    {ADFilePath,       "FILE_PATH"     },
    {ADFileName,       "FILE_NAME"     },
    {ADFileNumber,     "FILE_NUMBER"   },
    {ADFileTemplate,   "FILE_TEMPLATE" },
    {ADAutoIncrement,  "AUTO_INCREMENT"},
    {ADFullFileName,   "FULL_FILE_NAME"},
    {ADFileFormat,     "FILE_FORMAT"   },
    {ADAutoSave,       "AUTO_SAVE"     },
    {ADWriteFile,      "WRITE_FILE"    },
    {ADReadFile,       "READ_FILE"     },
};

#define NUM_AD_STANDARD_PARAMS (sizeof(ADStandardParamString)/sizeof(ADStandardParamString[0]))
#endif

#ifdef __cplusplus
}
#endif
#endif
