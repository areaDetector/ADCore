#ifndef ND_STD_DRIVER_PARAMS_H
#define ND_STD_DRIVER_PARAMS_H

#include "asynPortDriver.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum length of a filename or any of its components */
#define MAX_FILENAME_LEN 256

/* Enumeration of file saving modes */
typedef enum {
    NDFileModeSingle,       /**< Write 1 array per file */
    NDFileModeCapture,      /**< Capture NDNumCapture arrays into memory, write them out when capture is complete.
                              *  Write all captured arrays to a single file if the file format supports this */
    NDFileModeStream        /**< Stream arrays continuously to a single file if the file format supports this */
} NDFileMode_t;


/** Enumeration of parameters that affect the behaviour of the detector. 
  * These are the values that asyn will place in pasynUser->reason when the
  * standard asyn interface methods are called. */
typedef enum
{
    /*    Name          asyn interface  access   Description  */
    NDPortNameSelf,     /**< (asynOctet,    r/o) Asyn port name of this driver instance */

    /* Parameters defining the size of the array data from the detector.
     * NDArraySizeX and NDArraySizeY are the actual dimensions of the array data,
     * including effects of the region definition and binning */
    NDArraySizeX,       /**< (asynInt32,    r/o) Size of the array data in the X direction */
    NDArraySizeY,       /**< (asynInt32,    r/o) Size of the array data in the Y direction */
    NDArraySizeZ,       /**< (asynInt32,    r/o) Size of the array data in the Z direction */
    NDArraySize,        /**< (asynInt32,    r/o) Total size of array data in bytes */
    NDDataType,         /**< (asynInt32,    r/w) Data type (NDDataType_t) */
    NDColorMode,        /**< (asynInt32,    r/w) Color mode (NDColorMode_t) */

    /* Statistics on number of arrays collected */
    NDArrayCounter,     /**< (asynInt32,    r/w) Number of arrays since last reset */

    /* File name related parameters for saving data.
     * Drivers are not required to implement file saving, but if they do these parameters
     * should be used.
     * The driver will normally combine NDFilePath, NDFileName, and NDFileNumber into
     * a file name that order using the format specification in NDFileTemplate.
     * For example NDFileTemplate might be "%s%s_%d.tif" */
    NDFilePath,         /**< (asynOctet,    r/w) The file path */
    NDFileName,         /**< (asynOctet,    r/w) The file name */
    NDFileNumber,       /**< (asynInt32,    r/w) The next file number */
    NDFileTemplate,     /**< (asynOctet,    r/w) The file format template; C-style format string */
    NDAutoIncrement,    /**< (asynInt32,    r/w) Autoincrement file number; 0=No, 1=Yes */
    NDFullFileName,     /**< (asynOctet,    r/o) The actual complete file name for the last file saved */
    NDFileFormat,       /**< (asynInt32,    r/w) The data format to use for saving the file.  */
    NDAutoSave,         /**< (asynInt32,    r/w) Automatically save files */
    NDWriteFile,        /**< (asynInt32,    r/w) Manually save the most recent array to a file when value=1 */
    NDReadFile,         /**< (asynInt32,    r/w) Manually read file when value=1 */
    NDFileWriteMode,    /**< (asynInt32,    r/w) File saving mode (NDFileMode_t) */
    NDFileNumCapture,   /**< (asynInt32,    r/w) Number of arrays to capture */
    NDFileNumCaptured,  /**< (asynInt32,    r/o) Number of arrays already captured */
    NDFileCapture,      /**< (asynInt32,    r/w) Start or stop capturing arrays */

    /* The detector array data */
    NDArrayData,        /**< (asynGenericPointer,   r/w) NDArray data */
    NDArrayCallbacks,   /**< (asynInt32,    r/w) Do callbacks with array data (0=No, 1=Yes) */

    NDLastStdParam      /**< The last standard ND driver parameter; 
                          *  Derived classes must begin their specific parameter enums with this value */
} NDStdDriverParam_t;

/** If DEFINE_ND_STANDARD_PARAMS is true then these parameter strings are defined
  * for the userParam argument for asyn device support links
  * The asynDrvUser interface in the drivers parses these strings and puts the
  * corresponding enum value in pasynUser->reason */
#ifdef DEFINE_ND_STANDARD_PARAMS
static asynParamString_t NDStdDriverParamString[] = {
    {NDPortNameSelf,   "PORT_NAME_SELF"},
    {NDArraySizeX,     "ARRAY_SIZE_X"},
    {NDArraySizeY,     "ARRAY_SIZE_Y"},
    {NDArraySizeZ,     "ARRAY_SIZE_Z"},
    {NDArraySize,      "ARRAY_SIZE"  },
    {NDDataType,       "DATA_TYPE"   },
    {NDColorMode,      "COLOR_MODE"  },

    {NDArrayCounter,   "ARRAY_COUNTER" },

    {NDFilePath,       "FILE_PATH"     },
    {NDFileName,       "FILE_NAME"     },
    {NDFileNumber,     "FILE_NUMBER"   },
    {NDFileTemplate,   "FILE_TEMPLATE" },
    {NDAutoIncrement,  "AUTO_INCREMENT"},
    {NDFullFileName,   "FULL_FILE_NAME"},
    {NDFileFormat,     "FILE_FORMAT"   },
    {NDAutoSave,       "AUTO_SAVE"     },
    {NDWriteFile,      "WRITE_FILE"    },
    {NDReadFile,       "READ_FILE"     },
    {NDFileWriteMode,  "WRITE_MODE"    },
    {NDFileNumCapture, "NUM_CAPTURE"   },
    {NDFileNumCaptured,"NUM_CAPTURED"  },
    {NDFileCapture,    "CAPTURE"       },

    {NDArrayData,      "NDARRAY_DATA"  },
    {NDArrayCallbacks, "ARRAY_CALLBACKS"  }
};

#define NUM_ND_STANDARD_PARAMS (sizeof(NDStdDriverParamString)/sizeof(NDStdDriverParamString[0]))
#endif

#ifdef __cplusplus
}
#endif
#endif
