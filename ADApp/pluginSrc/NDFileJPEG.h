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

/** Writes NDArrays in the JPEG file format, which is a lossy compression format.
    This plugin was developed using the libjpeg library to write the file.
  */

/** Enums for plugin-specific parameters. */
typedef enum
{
    NDFileJPEGQuality           /* (asynInt32, r/w) File quality */
        = NDPluginFileLastParam,
    NDFileJPEGLastParam
} NDFileJPEGParam_t;


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

private:
    struct jpeg_compress_struct jpegInfo;
    struct jpeg_error_mgr jpegErr;
    NDColorMode_t colorMode;
    FILE *outFile;
};

#endif
