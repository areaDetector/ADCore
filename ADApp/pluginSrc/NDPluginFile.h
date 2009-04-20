#ifndef NDPluginFile_H
#define NDPluginFile_H

#include <epicsTypes.h>

#include "NDPluginDriver.h"

/** Mask to open file for reading */
#define NDFileModeRead     0x01
/** Mask to open file for writing */
#define NDFileModeWrite    0x02
/** Mask to open file for appending */
#define NDFileModeAppend   0x04
/** Mask to open file to read or write multiple NDArrays in a single file */
#define NDFileModeMultiple 0x08
typedef int NDFileOpenMode_t;

/** Enums for plugin-specific parameters. There are currently no specific parameters for this driver yet.
  * It uses the ADStdDriverParams and NDPluginDriver params. */
typedef enum {
    NDPluginFileLastParam =
        NDPluginDriverLastParam
} NDPluginFileParam_t;


/** Base class for NDArray file writing plugins; actual file writing plugins inherit from this class.
  * This class handles the logic of single file per image, capture into buffer or streaming multiple images
  * to a single file.  
  * Derived classes must implement the 4 pure virtual functions: openFile, readFile, writeFile and closeFile. */
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


    /** Open a file; pure virtual function that must be implemented by derived classes.
      * \param[in] fileName  Absolute path name of the file to open.
      * \param[in] openMode Bit mask with one of the access mode bits NDFileModeRead, NDFileModeWrite. NDFileModeAppend.
      *           May also have the bit NDFileModeMultiple set if the file is to be opened to write or read multiple 
      *           NDArrays into a single file.
      * \param[in] pArray Pointer to an NDArray; this array does not contain data to be written or read.  
      *           Rather it can be used to determine the header information and data structure for the file.
      *           It is guaranteed that NDArrays pass to NDPluginFile::writeFile or NDPluginFile::readFile 
      *           will have the same data type, data dimensions and attributes as this array. */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray) = 0;

    /** Read NDArray data from a file; pure virtual function that must be implemented by derived classes.
      * \param[in] pArray Pointer to the address of an NDArray to read the data into.  */ 
    virtual asynStatus readFile(NDArray **pArray) = 0;

    /** Write NDArray data to a file; pure virtual function that must be implemented by derived classes.
      * \param[in] pArray Pointer to an NDArray to write to the file. This function can be called multiple
      *           times between the call to openFile and closeFile if the class set supportsMultipleArrays=1 and
      *           NDFileModeMultiple was set in openMode in the call to NDPluginFile::openFile. */ 
    virtual asynStatus writeFile(NDArray *pArray) = 0;

    /** Close the file opened with NDPluginFile::openFile; 
      * pure virtual function that must be implemented by derived classes. */ 
    virtual asynStatus closeFile() = 0;
    
    int supportsMultipleArrays; /**< This flag should be set to 1 if this plugin can write multiple NDArrays
                                  * to a single file. Used in capture and stream modes. */

private:
    asynStatus openFileBase(NDFileOpenMode_t openMode, NDArray *pArray);
    asynStatus readFileBase();
    asynStatus writeFileBase();
    asynStatus closeFileBase();
    asynStatus doCapture();
    NDArray **pCapture;
};

    
#endif
