#ifndef NDPluginFile_H
#define NDPluginFile_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "ADInterface.h"
#include "NDPluginBase.h"
#include "NDFileNetCDF.h"

/* Note that the file format enum must agree with the mbbo/mbbi records in the NDFile.template file */
typedef enum {
    NDFileFormatNetCDF,
} NDPluginFileFormat_t;

typedef enum {
    NDPluginFileModeSingle,
    NDPluginFileModeCapture,
    NDPluginFileModeStream
} NDPluginFileMode_t;

typedef enum
{
    NDPluginFileWriteMode           /* (asynInt32,    r/w) File saving mode (NDFileMode_t) */
        = NDPluginBaseLastParam,
    NDPluginFileNumCapture,         /* (asynInt32,    r/w) Number of arrays to capture */
    NDPluginFileNumCaptured,        /* (asynInt32,    r/w) Number of arrays already captured */
    NDPluginFileCapture,            /* (asynInt32,    r/w) Start or stop capturing arrays */
    NDPluginFileLastParam
} NDPluginFileParam_t;

class NDPluginFile : public NDPluginBase {
public:
    NDPluginFile(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr);
                 
    /* These methods override those in the base class */
    void processCallbacks(NDArray_t *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeNDArray(asynUser *pasynUser, void *handle);
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                             const char **pptypeName, size_t *psize);

    /* These methods are new to this class */
    asynStatus readFile(void);
    asynStatus writeFile(void);
    asynStatus doCapture(void);

private:
    NDArray_t *pCaptureNext;
    NDArray_t *pCapture;
    NDFileNetCDFState_t netCDFState;
    epicsMutexId fileMutexId;
};

    
#endif
