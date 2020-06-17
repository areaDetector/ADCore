#ifndef asynNDArrayDriver_H
#define asynNDArrayDriver_H

#include <epicsMutex.h>
#include <epicsEvent.h>

#include "asynPortDriver.h"
#include "NDArray.h"
#include "ADCoreVersion.h"
#include "asynNDArrayDriverParamSet.h"

/** Our param indexes are in the paramSet */
#define FIRST_ND_ARRAY_PARAM paramSet->FIRST_ND_ARRAY_PARAM_INDEX
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

/** This is the class from which NDArray drivers are derived; implements the asynGenericPointer functions
  * for NDArray objects.
  * For areaDetector, both plugins and detector drivers are indirectly derived from this class.
  * asynNDArrayDriver inherits from asynPortDriver.
  */
class epicsShareFunc asynNDArrayDriver : public asynPortDriver {
public:
    asynNDArrayDriver(asynNDArrayDriverParamSet* paramSet,
                      const char *portName, int maxAddr, int maxBuffers, size_t maxMemory,
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
