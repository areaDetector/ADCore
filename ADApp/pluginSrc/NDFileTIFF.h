/*
 * NDFileTIFF.h
 * Writes NDArrays to TIFF files.
 * John Hammonds
 * April 17, 2009
 */

#ifndef DRV_NDFileTIFF_H
#define DRV_NDFileTIFF_H

#include "NDPluginFile.h"
#include "tiffio.h"

/* This version number is an attribute in the TIFF file to allow readers
 * to handle changes in the file contents */
#define NDTIFFFileVersion 1.0

/** Writes NDArrays in the TIFF file format.
    Tagged Image File Format is a file format for storing images.  The format was originally created by Aldus corporation and is
    currently developed by Adobe Systems Incorporated.  This plugin was developed using the libtiff library to write the file.
    The current version is only capable of writing 2-D images with one image per file.
    */

class epicsShareClass NDFileTIFF : public NDPluginFile {
public:
    NDFileTIFF(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int priority, int stackSize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();

protected:
    TIFF *tiff;
    NDColorMode_t colorMode;
    NDAttributeList *pFileAttributes;
    int numAttributes_;
    int *pAttributeId;
};

#endif
