#ifndef AREA_DETECTOR_INTERFACE_H
#define AREA_DETECTOR_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <epicsTypes.h>

/**  EPICS Area Detector API

                 How use this API

This API is used to provide a general interface to area detectors
used by EPICS.  To provide EPICS support the driver must
implement the area_detector API by defining a table of function pointers
of type #ADDrvSET_t. 

However, there are a couple of things that should be born in mind
and these are outined in the following three subsections.

      Locking and multi-thread considerations

The driver writer can assume that any call to this library will be in
the context of a thread that can block for a reasonable length of
time. Hence, I/O to the detector can be synchronous. However, you
cannot assume that only one thread is active, so routines must lock
data structures when accessing them.

This is particularly important when calling the callback routine. All
asynchronous communication back to the upper level software is done
through the callback function set by ADSetXXXCallback(), where XXX
is Int32, Float64, or ImageData. The rule is
that the driver can assume that once this callback is completed all
side effects have been handled by the upper level software. However,
until it is called the driver must protect against updates.

The way this is normally implemented is that each detector has a mutex
semaphore that is used to lock the following three operations
together:

  A call to get information from the hardware. 
  This call often blocks while the data is returned from the detector.
  
  A call to update the parameters to represent the new hardware state.
  
  A callback to inform the upper level software that things have changed. 

Note, in implementing this the upper level software makes sure that
this API is never called in record processing context - so the
detector mutex is always called before the dbScanLock mutex. Getting this
out of order results in a deadlock.

                     Detector Parameters

Detector state information is accessed using the detector parameter
ADGetInteger(), ADSetInteger(), ADGetDouble(),
ADSetDouble(), ADGetString, ADSetString, and ADXXXCallbackFunc() routines (the latter
set with ADSetXXXCallback()). Parameters are identified by the
ADParam_t enumerated type. Since this is common to all detectors,
the ADParamLib library is provided in the areaDetector application for drivers
to use. Using this is optional, but it generally saves effort. A
specific driver's get and callback routines are typically just wrappers
around the corresponding ADParamLib routines, and the set routines are
also wrappers, but inevitably have some detector specific action to
pass the parameter value down to the detector.

                Background detector polling task.

Some detectors will need a background polling task to
update the state kept as parameters within the drivers. Typically,
there is one task per detector. If there is a state change this will result in
upper level software being called back in the context this task, so may be
a consideration in setting the task priority.

               Using the ADSetLog routine.

The ADSetLog() routine is provided to hook into higher level
EPICS reporting routines without compromising the EPICS independence
of this interface. Implementing is done most easily by copying an
existing driver.

*/

/** Forward declaration of DETECTOR_HDL, which is a pointer to an internal driver-dependent handle */
typedef struct ADHandle * DETECTOR_HDL;

#define AREA_DETECTOR_OK (0)
#define AREA_DETECTOR_ERROR (-1)

/* Enumeration of shutter status */
typedef enum
{
    ADShutterClosed, 
    ADShutterOpen
} ADShutterStatus_t;

/* Enumeration of image data types */
typedef enum
{
   ADInt8,
   ADUInt8,
   ADInt16,
   ADUInt16,
   ADInt32,
   ADUInt32,
   ADFloat32,
   ADFloat64
} ADDataType_t;

/* Enumeration of detector status */
typedef enum
{
   ADStatusIdle,
   ADStatusAcquire,
   ADStatusReadout,
   ASStatusCorrect,
   ADStatusSaving,
   ADStatusBusy,
   ADStatusAborting,
   ADStatusError,
} ADStatus_t;

typedef enum
{
   ADSingleFrame,
   ADMultipleFrame,
   ADContinuousFrame
} ADFrameMode_t;

/* The total number of commands that a driver can use, including those in the ADParam_t enum. */
#define MAX_DRIVER_COMMANDS 1000
/**

    This is an enumeration of parameters that affect the behaviour of the
    detector. They are used by the functions ADSetDouble(),
    ADSetInteger() and other associated functions.
*/

typedef enum
{
    ADManufacturer,    /* (string,  r/o) Detector manufacturer name */ 
    ADModel,           /* (string,  r/o) Detector model name */
    ADTemperature,     /* (double,  r/w) Detector temperature */
    ADGain,            /* (double,  r/w) Gain. */
    /* Parameters that control the detector binning */
    ADBinX,            /* (integer, r/w) Binning in the X direction */
    ADBinY,            /* (integer. r/w) Binning in the Y dieection */
    /* Parameters the control the region of the detector to be read out.
    * ADMinX, ADMinY, ADSizeX, and ADSizeY are in unbinned pixel units */
    ADMinX,            /* (integer, r/w) First pixel in the X direction.  0 is the first pixel on the detector */
    ADMinY,            /* (integer, r/w) First pixel in the Y direction.  0 is the first pixel on the detector */
    ADSizeX,           /* (integer, r/w) Size of the region to read in the X direction. */
    ADSizeY,           /* (integer, r/w) Size of the region to read in the Y direction. */
    ADMaxSizeX,        /* (integer, r/o) Maximum (sensor) size in the X direction. */
    ADMaxSizeY,        /* (integer, r/o) Maximum (sensro) size in the Y direction. */
    /* Parameters defining the size of the image data from the detector.
     * ADImageSizeX and ADImageSizeY are the actual dimensions of the image data, 
     * including effects of the region definition and binning */
    ADImageSizeX,      /* (integer, r/o) Size of the image data in the X direction */
    ADImageSizeY,      /* (integer, r/o) Size of the image data in the Y direction */
    ADImageSize,       /* (integer, r/o) Total size of image data in bytes */
    ADDataType,        /* (integer, r/w) Data type (ADDataType_t) */
    ADFrameMode,       /* (integer, r/w) Frame mode (ADFrameMode_t) */
    ADNumExposures,    /* (integer, r/w) Number of exposures per frame to acquire */
    ADNumFrames,       /* (integer, r/w) Number of frames to acquire in one acquisition sequence */
    ADAcquireTime,     /* (double,  r/w) Acquisition time per frame. */
    ADAcquirePeriod,   /* (double,  r/w) Acquisition period between frames */
    ADConnect,         /* (integer, r/w) Connection request and connection status */
    ADStatus,          /* (integer, r/o) Acquisition status (ADStatus_t)*/
    ADShutter,         /* (integer, r/w) Shutter control (ADShutterStatus_t) */
    ADAcquire,         /* (integer, r/w) Start(1) or Stop(0) acquisition */
    /* File name related parameters for saving data.
     * Drivers are not required to implement file saving, but if they do these parameters
     * should be used.
     * The driver will normally combine ADFilePath, ADFileName, and ADFileNumber into
     * a file name that order using the format specification in ADFileTemplate. 
     * For example ADFileTemplate might be "%s%s_%d.tif". */
    ADFilePath,        /* (string,  r/w) The file path. */
    ADFileName,        /* (string,  r/w) The file name. */
    ADFileNumber,      /* (integer, r/w) The next file number. */
    ADFileTemplate,    /* (string,  r/w) The format string. */
    ADAutoIncrement,   /* (integer, r/w) Autoincrement file number. 0=No, 1=Yes */
    ADFullFileName,    /* (string,  r/o) The actual complete file name for the last file saved. */
    ADFileFormat,      /* (integer, r/w) The data format to use for saving the file.  
                                         The values are enums that will be detector-specific. */
    ADAutoSave,        /* (integer, r/w) Automatically save files */
    ADWriteFile,       /* (integer, r/w) Manually save the most recent image to a file when value=1 */
    ADReadFile,        /* (integer, r/w) Manually read file when value=1 */
    
    ADLastStandardParam  /* The last command, used by the standard driver */
} ADParam_t;

#define ADFirstDriverParam ADLastStandardParam

/* EPICS EPICS driver support interface routines */

typedef void (*ADReportFunc)( int level );
/** Print the status of the detector to stdout
    This optional routine is intended to provide a debugging log about the detector
    and will typically be called by the EPICS dbior function. The level indicates
    the level of detail - typically level 0 prints a one line summary and higher
    levels provide increasing amount of detail.
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static void ADReport( int level );
#endif

typedef int (*ADInitFunc)( void );

/** EPICS driver support entry function initialisation routine.

    This optional routine is provided purely so the detector driver support entry table 
    (ADDrvSET_t) is compatible with an EPICS driver support entry table.
    Typically it won't be provided and the driver support entry table
    initialised with a null pointer. However, if provided, it will be called
    at iocInit.

    Returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADInit( void );
#endif

typedef enum
{
    ADTraceError   =0x0001,
    ADTraceIODevice=0x0002,
    ADTraceIOFilter=0x0004,
    ADTraceIODriver=0x0008,
    ADTraceFlow    =0x0010
} ADLogMask_t;

typedef int (*ADLogFunc)( void * userParam,
                          const ADLogMask_t logMask,
                          const char *pFormat, ...);

typedef int (*ADSetLogFunc)( DETECTOR_HDL pDetector, ADLogFunc logFunc, void * param );

/* Provide an external logging routine.

    This is an optional function which allows external software to hook
    the driver log routine into an external logging system. The
    external log function is a standard printf style routine with the
    exception that it has a first parameter that can be used to set external
    data on an detector by detector basis and a second parameter which is a message
    tracing indicator. This is set to be compatible with the asynTrace reasons
    - enabling tracing of errors, flow, and device filter and driver layers.
    infomational, minor, major or fatal.

    If pDetector is NULL, then this logging function and parameter should be used as
    a default - i.e. when logging is not taking place in the context of a single
    detector (a background polling task, for example).

    pDetector   [in] Pointer to detector handle returned by ADOpen.
    logFunc [in] Pointer to function of ADLogFunc type.
    param   [in] Pointer to the user parameter to be used for logging on this detector

    Returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADSetLog( DETECTOR_HDL pDetector, ADLogFunc logFunc, void * param );
#endif


/* Access Routines to open and close a connection to a detector. */

typedef DETECTOR_HDL (*ADOpenFunc)( int detector, char * param );

/** Initialise connection to a detector.

    This routine should open a connection to a detector
    and return a driver dependent handle that can be used for subsequent calls to
    other routines supported by the detector. The driver should support
    multiple opens on a single detector and process all calls from separate threads on a
    fifo basis.

    param     [in] Arbitrary, driver defined, parameter string.

    returns DETECTOR_HDL - pointer to handle for using to future calls to areaDetector routines
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static DETECTOR_HDL ADOpen( int detector, char * param );
#endif

typedef int (*ADCloseFunc)( DETECTOR_HDL pDetector );

/*  Close a connection to a detector.

    This routine should close the connection to a detector previously
    opened with ADOpen, and clean up all space specifically allocated to this 
    detector handle.

    pDetector  [in]   Pointer to detector handle returned by ADOpen.

    returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADClose( DETECTOR_HDL pDetector );
#endif

typedef int (*ADFindParamFunc)( DETECTOR_HDL pDetector, const char *paramString, int *function );

/*  Translates a parameter string to a driver-specific function code

    pDetector  [in]   Pointer to detector handle returned by ADOpen.
    paramString [in]  Parameter string for this parameter.
    function    [out] Function code corresponding to this paramString

    returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADFindParam( DETECTOR_HDL pDetector, const char *paramString, int *function );
#endif

/*  Callback Routines to handle callbacks of status information */

typedef void (*ADInt32CallbackFunc)    ( void *param, int function, int value );
typedef void (*ADFloat64CallbackFunc)  ( void *param, int function, double value );
typedef void (*ADStringCallbackFunc)   ( void *param, int function, char *value );
typedef void (*ADImageDataCallbackFunc)( void *param, void *value, ADDataType_t dataType, int nx, int ny );
typedef int (*ADSetInt32CallbackFunc)    ( DETECTOR_HDL pDetector, ADInt32CallbackFunc     callback, void *param);
typedef int (*ADSetFloat64CallbackFunc)  ( DETECTOR_HDL pDetector, ADFloat64CallbackFunc   callback, void *param );
typedef int (*ADSetStringCallbackFunc)   ( DETECTOR_HDL pDetector, ADStringCallbackFunc    callback, void *param );
typedef int (*ADSetImageDataCallbackFunc)( DETECTOR_HDL pDetector, ADImageDataCallbackFunc callback, void *param );

/*  Set a callback function to be called when detector information changes

    These routine set functions to be called by the driver if the detector data changes.
    
    Only one callback function of each data type is allowed per DETECTOR_HDL, and so subsequent calls to
    these functions using the original detector identifier will replace the original
    callback. Setting the callback function to a NULL pointer will delete the
    callback hook.

    pDetector [in]   Pointer to detector handle returned by ADOpen.
    callback  [in]   Pointer to a callback function of the appropriate type.
    param     [in]   Void pointer to parameter that should be used when calling the callback

    Returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADSetInt32Callback     ( DETECTOR_HDL pDetector, ADInt32CallbackFunc     callback, void *param );
static int ADSetFloat64Callback   ( DETECTOR_HDL pDetector, ADFloat64CallbackFunc   callback, void *param );
static int ADSetStringCallback    ( DETECTOR_HDL pDetector, ADStringCallbackFunc    callback, void *param );
static int ADSetImageDataCallback ( DETECTOR_HDL pDetector, ADImageDataCallbackFunc callback, void *param );
#endif


/* Routines that read and write detector control parameters */

typedef int (*ADGetIntegerFunc)( DETECTOR_HDL pDetector,  int function, int *value );

/*  Gets an integer parameter in the detector.

    pDetector [in]   Pointer to detector handle returned by ADOpen.
    function  [in]   One of the ADParam_t values, or a driver-specific function code, 
                     indicating which parameter to get.
    value     [in]   Value of the parameter.

    Returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure or not supported. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADGetInteger( DETECTOR_HDL pDetector, int function, int * value );
#endif

typedef int (*ADSetIntegerFunc)( DETECTOR_HDL pDetector,  int function, int value );

/*  Sets an integer parameter in the detector.

    pDetector [in]   Pointer to detector handle returned by ADOpen.
    function  [in]   One of the ADParam_t values, or a driver-specific function code, 
                     indicating which parameter to set.
    value     [in]   Value to be assigned to the parameter.

    Returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure or not supported. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADSetInteger( DETECTOR_HDL pDetector, int function, int value );
#endif

typedef int (*ADGetDoubleFunc)( DETECTOR_HDL pDetector, int function, double *value );

/*  Gets a double parameter in the detector.

    pDetector [in]   Pointer to detector handle returned by ADOpen.
    function  [in]   One of the ADParam_t values, or a driver-specific function code, 
                     indicating which parameter to get.
    value     [out]  Value of the parameter.

    Returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure or not supported. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADGetDouble( DETECTOR_HDL pDetector, int function, double * value );
#endif

typedef int (*ADSetDoubleFunc)( DETECTOR_HDL pDetector, int function, double value);

/*  Sets a double parameter in the detector.

    pDetector [in]   Pointer to detector handle returned by ADOpen.
    function  [in]   One of the ADParam_t values, or a driver-specific function code, 
                     indicating which parameter to set.
    value     [in]   Value to be assigned to the parameter.

    Returns:  0 (AREA_DETECTOR_OK) for success or non-zero for failure or not supported. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADSetDouble( DETECTOR_HDL pDetector, int function, double value );
#endif

typedef int (*ADGetStringFunc)( DETECTOR_HDL pDetector, int function, int maxChars, char *value );

/*  Gets a double parameter in the detector.

    pDetector [in]   Pointer to detector handle returned by ADOpen.
    function  [in]   One of the ADParam_t values, or a driver-specific function code, 
                     indicating which parameter to get.
    maxChars  [in]   Maximum length of string
    value     [out]  Value of the parameter.

    Returns:  0 (AREA_DETECTOR_OK) for success or non-zero for failure or not supported. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADGetString( DETECTOR_HDL pDetector, int function, int maxChars, char *value );
#endif

typedef int (*ADSetStringFunc)( DETECTOR_HDL pDetector, int function, const char *value);

/*  Sets a string parameter in the detector.

    pDetector [in]   Pointer to detector handle returned by ADOpen.
    function  [in]   One of the ADParam_t values, or a driver-specific function code, 
                     indicating which parameter to set.
    value     [in]   Value to be assigned to the parameter.

    Returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure or not supported. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADSetString( DETECTOR_HDL pDetector, int function, const char *value );
#endif

/* Routines to read and write image data */
typedef int (*ADGetImageFunc)( DETECTOR_HDL pDetector,  int maxBytes, void *buffer );

/*  Reads the data for the current image from the detector.

    pDetector [in]   Pointer to detector handle returned by ADOpen.
    maxBytes  [in]   The maximum number of bytes to return in buffer.
    buffer    [out]  The image data to be read from the detector.  
                     The data type of the pointer must be appropriate for the
                     current data type of the detector.

    Returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure or not supported. 
*/

/* Routines to read and write the image data */

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADGetImage( DETECTOR_HDL pDetector, int maxBytes, void *buffer );
#endif

typedef int (*ADSetImageFunc)( DETECTOR_HDL pDetector,  int maxBytes, void *buffer );

/*  Writes the data for the current image from the detector.

    pDetector [in]   Pointer to detector handle returned by ADOpen.
    maxBytes  [in]   The number of bytes in buffer.
    buffer    [in]   The image data to be sent to the detector.  
                     The data type of the pointer must be appropriate for the
                     current data type of the detector.

    Returns 0 (AREA_DETECTOR_OK) for success or non-zero for failure or not supported. 
*/

#ifdef DEFINE_AREA_DETECTOR_PROTOTYPES
static int ADSetImage( DETECTOR_HDL pDetector, int maxBytes, void *buffer );
#endif


/** The driver support entry table */

typedef struct
{
    int number;
    ADReportFunc          report;            /* Standard EPICS driver report function (optional) */
    ADInitFunc            init;              /* Standard EPICS dirver initialisation function (optional) */
    ADSetLogFunc          setLog;            /* Defines an external logging function (optional) */
    ADOpenFunc            open;              /* Driver open function */
    ADCloseFunc           close;             /* Driver close function */
    ADFindParamFunc       findParam;         /* Parameter lookup function */
    ADSetInt32CallbackFunc     setInt32Callback;      /* Provides a callback function the driver can call when an int32 value changes */
    ADSetFloat64CallbackFunc   setFloat64Callback;    /* Provides a callback function the driver can call when a float64 value changes */
    ADSetStringCallbackFunc    setStringCallback;     /* Provides a callback function the driver can call when a string value changes */
    ADSetImageDataCallbackFunc setImageDataCallback;  /* Provides a callback function the driver can call when there is new image data */
    ADGetIntegerFunc      getInteger;        /* Pointer to function to get an integer value */
    ADSetIntegerFunc      setInteger;        /* Pointer to function to set an integer value */
    ADGetDoubleFunc       getDouble;         /* Pointer to function to get a double value */
    ADSetDoubleFunc       setDouble;         /* Pointer to function to set a double value */
    ADGetStringFunc       getString;         /* Pointer to function to get a string value */
    ADSetStringFunc       setString;         /* Pointer to function to set a string value */
    ADGetImageFunc        getImage;          /* Pointer to function to getImage acquisition */
    ADSetImageFunc        setImage;          /* Pointer to function to setImage acquisition */
} ADDrvSet_t;

#ifdef __cplusplus
}
#endif
#endif
