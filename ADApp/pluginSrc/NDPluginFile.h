#ifndef NDPluginFile_H
#define NDPluginFile_H

#include <epicsTypes.h>

#include "NDPluginDriver.h"

#define NDFileModeRead     0x01
#define NDFileModeWrite    0x02
#define NDFileModeAppend   0x04
#define NDFileModeMultiple 0x08
typedef int NDFileOpenMode_t;

/* There are currently no specific parameters for this driver yet.
 * It uses the ADStdDriverParams and NDPluginDriver params */
typedef enum {
    NDPluginFileLastParam =
        NDPluginDriverLastParam
} NDPluginFileParam_t;

class NDPluginFile : public NDPluginDriver {
public:
    NDPluginFile(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int priority, int stackSize);
                 
    /* These methods override those in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeNDArray(asynUser *pasynUser, void *genericPointer);
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                             const char **pptypeName, size_t *psize);

    /* These methods are new to this class and must be implemented by derived classes because
     * they are pure virtual functions */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray) = 0;
    virtual asynStatus readFile(NDArray **pArray) = 0;
    virtual asynStatus writeFile(NDArray *pArray) = 0;
    virtual asynStatus closeFile() = 0;
    
    int supportsMultipleArrays;

private:
    asynStatus openFileBase(NDFileOpenMode_t openMode, NDArray *pArray);
    asynStatus readFileBase();
    asynStatus writeFileBase();
    asynStatus closeFileBase();
    asynStatus doCapture();
    NDArray **pCapture;
};

    
#endif
