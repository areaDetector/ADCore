/*
 * NDFileJPEG.h
 * Writes NDArrays to JPEG files.
 * Mark Rivers
 * May 9, 2009
 */

#ifndef DRV_NDFileJPEG_H
#define DRV_NDFileJPEG_H

#include "NDPluginFile.h"

#ifdef __cplusplus
// Force C interface for jpeg functions
extern "C" {
#endif
#include "jpeglib.h"
#ifdef __cplusplus
}
#endif

#define JPEG_BUF_SIZE 4096 /* choose an efficiently fwrite'able size */

/** Expanded data destination object for JPEG output */
typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */
  class NDFileJPEG *pNDFileJPEG;   /* Pointer to ourselves */
} jpegDestMgr;

#define NDFileJPEGQualityString  "JPEG_QUALITY"  /* (asynInt32, r/w) File quality */

/** Writes NDArrays in the JPEG file format, which is a lossy compression format.
  * This plugin was developed using the libjpeg library to write the file.
  */
class NDPLUGIN_API NDFileJPEG : public NDPluginFile {
public:
    NDFileJPEG(const char *portName, int queueSize, int blockingCallbacks,
               const char *NDArrayPort, int NDArrayAddr,
               int priority, int stackSize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();
    /* These should be private, but are called from C, must be public */
    void initDestination();
    boolean emptyOutputBuffer();
    void termDestination();

protected:
    int NDFileJPEGQuality;
    #define FIRST_NDFILE_JPEG_PARAM NDFileJPEGQuality

private:
    struct jpeg_compress_struct jpegInfo;
    struct jpeg_error_mgr jpegErr;
    NDColorMode_t colorMode;
    FILE *outFile;
    JOCTET jpegBuffer[JPEG_BUF_SIZE];
    jpegDestMgr destMgr;
};

#endif
