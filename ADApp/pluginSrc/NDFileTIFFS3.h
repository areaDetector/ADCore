/*
 * NDFileTIFF.h
 * Writes NDArrays to TIFF files.
 * John Hammonds
 * April 17, 2009
 */

#ifndef DRV_NDFileTIFFS3_H
#define DRV_NDFileTIFFS3_H

#include "NDFileTIFF.h"

/** Writes NDArrays in the TIFF file format. To an Amazon S3
    Tagged Image File Format is a file format for storing images.  The format was originally created by Aldus corporation and is
    currently developed by Adobe Systems Incorporated.  This plugin was developed using the libtiff library to write the file.
    The current version is only capable of writing 2-D images with one image per file.
    */

class epicsShareClass NDFileTIFFS3 : public NDFileTIFF {
  public:
     NDFileTIFFS3(const char *portName, int queueSize, int blockingCallbacks,
                  const char *NDArrayPort, int NDArrayAddr,
                  const char *endpoint, int awslog,
                  int priority, int stackSize);

    virtual ~NDFileTIFFS3();    
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus closeFile();

  private:
    Aws::SDKOptions options;
    std::shared_ptr<Aws::IOStream> awsStream;
    std::shared_ptr<Aws::S3::S3Client> s3Client;
    char keyName[256];
    bool awsLogging = false;
// 
//     /* The methods that this class implements */
//     virtual asynStatus readFile(NDArray **pArray);
//     virtual asynStatus writeFile(NDArray *pArray);
//     virtual asynStatus closeFile();
// 
};

class NDFileTIFFS3_AWSContext : public Aws::Client::AsyncCallerContext {
  public:
    NDFileTIFFS3_AWSContext()
      : Aws::Client::AsyncCallerContext() {}

    void SetTIFFS3(NDFileTIFFS3 *c) {
      tiffS3 = c;
    }
    NDFileTIFFS3* GetTIFFS3(void) {
      return tiffS3;
    }
  private:
    NDFileTIFFS3 *tiffS3;
};


#endif
