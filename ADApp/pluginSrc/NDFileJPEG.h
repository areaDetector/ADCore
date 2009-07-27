/*
 * NDFileJPEG.h
 * Writes NDArrays to JPEG files.
 * Mark Rivers
 * May 9, 2009
 */

#ifndef DRV_NDFileJPEG_H
#define DRV_NDFileJPEG_H

#include "NDPluginFile.h"
#include "jpeglib.h"

#define JPEG_BUF_SIZE 4096 /* choose an efficiently fwrite'able size */

/** Expanded data destination object for JPEG output */
typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */
  class NDFileJPEG *pNDFileJPEG;   /* Pointer to ourselves */
} jpegDestMgr;

/** Enums for plugin-specific parameters. */
typedef enum
{
    NDFileJPEGQuality           /* (asynInt32, r/w) File quality */
        = NDPluginFileLastParam,
    NDFileJPEGLastParam
} NDFileJPEGParam_t;


/** Writes NDArrays in the JPEG file format, which is a lossy compression format.
  * This plugin was developed using the libjpeg library to write the file.
  */
class NDFileJPEG : public NDPluginFile {
public:
    NDFileJPEG(const char *portName, int queueSize, int blockingCallbacks,
               const char *NDArrayPort, int NDArrayAddr,
               int priority, int stackSize);

    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo, 
                                        const char **pptypeName, size_t *psize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();
    /* These should be private, but are called from C, must be public */
    void initDestination();
    boolean emptyOutputBuffer();
    void termDestination();

private:
    struct jpeg_compress_struct jpegInfo;
    struct jpeg_error_mgr jpegErr;
    NDColorMode_t colorMode;
    FILE *outFile;
    JOCTET jpegBuffer[JPEG_BUF_SIZE];
    jpegDestMgr destMgr;
};

#endif
