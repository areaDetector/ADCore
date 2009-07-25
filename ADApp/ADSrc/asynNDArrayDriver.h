#ifndef asynNDArrayDriver_H
#define asynNDArrayDriver_H

#include "asynPortDriver.h"
#include "NDArray.h"
#include "PVAttribute.h"


/** Maximum length of a filename or any of its components */
#define MAX_FILENAME_LEN 256

/** Enumeration of file saving modes */
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

    PVAttributesFile,   /**< (asynOctet,    r/w) Attributes file name */

    /* The detector array data */
    NDArrayData,        /**< (asynGenericPointer,   r/w) NDArray data */
    NDArrayCallbacks,   /**< (asynInt32,    r/w) Do callbacks with array data (0=No, 1=Yes) */

    NDLastStdParam      /**< The last standard ND driver parameter; 
                          *  Derived classes must begin their specific parameter enums with this value */
} NDStdDriverParam_t;

#define NUM_ND_STANDARD_PARAMS (sizeof(NDStdDriverParamString)/sizeof(NDStdDriverParamString[0]))


/** This is the class from which NDArray drivers are derived; implements the asynGenericPointer functions 
  * for NDArray objects. 
  * For areaDetector, both plugins and detector drivers are indirectly derived from this class.
  * asynNDArrayDriver inherits from asynPortDriver.
  */
class asynNDArrayDriver : public asynPortDriver {
public:
    asynNDArrayDriver(const char *portName, int maxAddr, int paramTableSize, int maxBuffers, size_t maxMemory,
                      int interfaceMask, int interruptMask,
                      int asynFlags, int autoConnect, int priority, int stackSize);

    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    virtual asynStatus readGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus writeGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                                     const char **pptypeName, size_t *psize);
    virtual void report(FILE *fp, int details);

    /* These are the methods that are new to this class */
    virtual int createFileName(int maxChars, char *fullFileName);
    virtual int createFileName(int maxChars, char *filePath, char *fileName);
    virtual int readPVAttributesFile(const char *fileName);
    virtual int getAttributes(NDArray *pArray);
    virtual NDArray* getAttributesCopy(NDArray *pArray, bool release);

protected:
    NDArray **pArrays;             /**< An array of NDArray pointers used to store data in the driver */
    NDArrayPool *pNDArrayPool;     /**< An NDArrayPool object used to allocate and manipulate NDArray objects */
    class PVAttributeList *pPVAttributeList;  /**< A PVAttributeList object used to obtain the current values of a set of
                                          *  EPICS PVs */
};

#endif
