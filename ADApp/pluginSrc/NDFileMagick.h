/*
 * NDFileMagick.h
 * Writes NDArrays to any file format supported by ImageMagick
 * Mark Rivers
 * September 20, 2010 
 */

#ifndef DRV_NDFileMagick_H
#define DRV_NDFileMagick_H

#include "NDPluginFile.h"
#include "Magick++.h"
using namespace Magick;
using namespace std;
    
#define NDFileMagickQualityString      "MAGICK_QUALITY"        /* (asynInt32, r/w) File quality */
#define NDFileMagickCompressTypeString "MAGICK_COMPRESS_TYPE"  /* (asynInt32, r/w) Compression type */
#define NDFileMagickBitDepthString     "MAGICK_BIT_DEPTH"      /* (asynInt32, r/w) Bit depth */

/** Writes NDArrays to files using the GraphicsMagick library; can write many different file formats.
  */
class epicsShareClass NDFileMagick : public NDPluginFile {
public:
    NDFileMagick(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int priority, int stackSize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();

protected:
    int NDFileMagickQuality;
    #define FIRST_NDFILE_MAGICK_PARAM NDFileMagickQuality
    int NDFileMagickCompressType;
    int NDFileMagickBitDepth;
    #define LAST_NDFILE_MAGICK_PARAM NDFileMagickBitDepth

private:
    size_t sizeX;
    size_t sizeY;
    StorageType storageType;
    NDColorMode_t colorMode;
    ImageType imageType;
    string colorMap;
    Image image;
    char fileName[MAX_FILENAME_LEN];
};
#define NUM_NDFILE_MAGICK_PARAMS ((int)(&LAST_NDFILE_MAGICK_PARAM - &FIRST_NDFILE_MAGICK_PARAM + 1))
#endif
