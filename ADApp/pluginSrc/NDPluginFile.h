#ifndef NDPluginFile_H
#define NDPluginFile_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"
#include "NDFileNetCDF.h"

/* Note that the file format enum must agree with the mbbo/mbbi records in the NDFile.template file */
typedef enum {
    NDFileFormatNetCDF
} NDPluginFileFormat_t;

/* There are currently no specific parameters for this driver yet.
 * It uses the ADStdDriverParams and NDPluginDriver params */
typedef enum {
    NDPluginFileLastParam =
        NDPluginDriverLastParam
} NDPluginFileParam_t;

class NDPluginFile : public NDPluginDriver {
public:
    NDPluginFile(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr);
                 
    /* These methods override those in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeNDArray(asynUser *pasynUser, void *genericPointer);
    asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                             const char **pptypeName, size_t *psize);

    /* These methods are new to this class */
    asynStatus readFile(void);
    asynStatus writeFile(void);
    asynStatus doCapture(void);

private:
    NDArray *pCaptureNext;
    NDArray *pCapture;
    NDFileNetCDFState_t netCDFState;
    epicsMutexId fileMutexId;
};

    
#endif
