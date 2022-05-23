#ifndef asynNDArrayDriver_H
#define asynNDArrayDriver_H

#include <epicsMutex.h>
#include <epicsEvent.h>

#include "asynNDArrayDriverParamSet.h"
#include "asynPortDriver.h"
#include "NDArray.h"
#include "ADCoreVersion.h"

/** Maximum length of a filename or any of its components */
#define MAX_FILENAME_LEN 256

/** Enumeration of file saving modes */
typedef enum {
    NDFileModeSingle,       /**< Write 1 array per file */
    NDFileModeCapture,      /**< Capture NDNumCapture arrays into memory, write them out when capture is complete.
                              *  Write all captured arrays to a single file if the file format supports this */
    NDFileModeStream        /**< Stream arrays continuously to a single file if the file format supports this */
} NDFileMode_t;

typedef enum {
    NDFileWriteOK,
    NDFileWriteError
} NDFileWriteStatus_t;

typedef enum {
    NDAttributesOK,
    NDAttributesFileNotFound,
    NDAttributesXMLSyntaxError,
    NDAttributesMacroError
} NDAttributesStatus_t;

/** Strings defining parameters that affect the behaviour of the detector.
  * These are the values passed to drvUserCreate.
  * The driver will place in pasynUser->reason an integer to be used when the
  * standard asyn interface methods are called. */
 /*                               String                 asyn interface  access   Description  */

/** ADCore version string */

/* These parameters were previously in ADDriver.h.
 * We moved them here so they can be used by other types of drivers
 * For consistency the #define and parameter names should begin with ND rather than AD but that would break
 * backwards compatibility. */

/* Parameters defining characteristics of the array data from the detector.
 * NDArraySizeX and NDArraySizeY are the actual dimensions of the array data,
 * including effects of the region definition and binning */

/* Statistics on number of arrays collected */

/* File name related parameters for saving data.
 * Drivers are not required to implement file saving, but if they do these parameters
 * should be used.
 * The driver will normally combine NDFilePath, NDFileName, and NDFileNumber into
 * a file name that order using the format specification in NDFileTemplate.
 * For example NDFileTemplate might be "%s%s_%d.tif" */


/* The detector array data */

/* NDArray Pool status and control */
#define NDPoolMaxBuffersString      "POOL_MAX_BUFFERS"

/* Queued arrays */

/** This is the class from which NDArray drivers are derived; implements the asynGenericPointer functions
  * for NDArray objects.
  * For areaDetector, both plugins and detector drivers are indirectly derived from this class.
  * asynNDArrayDriver inherits from asynPortDriver.
  */
class ADCORE_API asynNDArrayDriver : public asynPortDriver {
public:
    asynNDArrayDriver(asynNDArrayDriverParamSet* paramSet, const char *portName, int maxAddr, int maxBuffers, size_t maxMemory,
                      int interfaceMask, int interruptMask,
                      int asynFlags, int autoConnect, int priority, int stackSize);
    virtual ~asynNDArrayDriver();
    /* These are the methods that we override from asynPortDriver */
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    virtual asynStatus readGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus writeGenericPointer(asynUser *pasynUser, void *genericPointer);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus setIntegerParam(int index, int value);
    virtual asynStatus setIntegerParam(int list, int index, int value);
    virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    virtual void report(FILE *fp, int details);

    /* These are the methods that are new to this class */
    virtual asynStatus createFilePath(const char *path, int pathDepth);
    virtual asynStatus checkPath();
    virtual bool checkPath(std::string &filePath);
    virtual asynStatus createFileName(int maxChars, char *fullFileName);
    virtual asynStatus createFileName(int maxChars, char *filePath, char *fileName);
    virtual asynStatus readNDAttributesFile();
    virtual asynStatus getAttributes(NDAttributeList *pAttributeList);

    asynStatus incrementQueuedArrayCount();
    asynStatus decrementQueuedArrayCount();
    int getQueuedArrayCount();
    void updateQueuedArrayCount();

    class NDArrayPool *pNDArrayPool;     /**< An NDArrayPool pointer that is initialized to pNDArrayPoolPvt_ in the constructor.
                                     * Plugins change this pointer to the one passed in NDArray::pNDArrayPool */

protected:
    asynNDArrayDriverParamSet* paramSet;
    #define FIRST_NDARRAY_PARAM paramSet->FIRST_ASYNNDARRAYDRIVERPARAMSET_PARAM
    int NDPoolMaxBuffers;

    class NDArray **pArrays;             /**< An array of NDArray pointers used to store data in the driver */
    class NDAttributeList *pAttributeList;  /**< An NDAttributeList object used to obtain the current values of a set of attributes */
    int threadStackSize_;
    int threadPriority_;

private:
    NDArrayPool *pNDArrayPoolPvt_;
    epicsMutex *queuedArrayCountMutex_;
    epicsEventId queuedArrayEvent_;
    int queuedArrayCount_;

    bool queuedArrayUpdateRun_;
    epicsEventId queuedArrayUpdateDone_;

    friend class NDArrayPool;

};

#endif