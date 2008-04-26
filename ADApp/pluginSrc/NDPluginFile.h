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

/* The command strings are the userParam argument for asyn device support links
 * The asynDrvUser interface in this driver parses these strings and puts the
 * corresponding enum value in pasynUser->reason */
static ADParamString_t NDPluginFileParamString[] = {
    {NDPluginFileWriteMode,         "WRITE_MODE" },
    {NDPluginFileNumCapture,        "NUM_CAPTURE" },
    {NDPluginFileNumCaptured,       "NUM_CAPTURED" },
    {NDPluginFileCapture,           "CAPTURE" },
};

#define NUM_ND_PLUGIN_FILE_PARAMS (sizeof(NDPluginFileParamString)/sizeof(NDPluginFileParamString[0]))

class NDPluginFile : public NDPluginBase {
public:
    NDPluginFile(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr);
    void processCallbacks(NDArray_t *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                          size_t *nActual);
    asynStatus writeNDArray(asynUser *pasynUser, void *handle);
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                             const char **pptypeName, size_t *psize);
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
