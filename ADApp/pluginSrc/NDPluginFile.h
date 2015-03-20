#ifndef NDPluginFile_H
#define NDPluginFile_H

#include <epicsTypes.h>
#include <epicsMutex.h>

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

#define FILEPLUGIN_NAME        "FilePluginFileName"
#define FILEPLUGIN_NUMBER      "FilePluginFileNumber"
#define FILEPLUGIN_DESTINATION "FilePluginDestination"

/** Base class for NDArray file writing plugins; actual file writing plugins inherit from this class.
  * This class handles the logic of single file per image, capture into buffer or streaming multiple images
  * to a single file.  
  * Derived classes must implement the 4 pure virtual functions: openFile, readFile, writeFile and closeFile. */
class epicsShareClass NDPluginFile : public NDPluginDriver {
public:
    NDPluginFile(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxAddr, int numParams,
                 int maxBuffers, size_t maxMemory, int interfaceMask, int interruptMask,
                 int asynFlags, int autoConnect, int priority, int stackSize);
                 
    /* These methods override those in the base class */
    virtual void processCallbacks(NDArray *pArray);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeNDArray(asynUser *pasynUser, void *genericPointer);

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
    
    int supportsMultipleArrays; /**< Derived classes must set this flag to 0/1 if they cannot/can write 
                                  * multiple NDArrays to a single file. Used in capture and stream modes. */

private:
    asynStatus openFileBase(NDFileOpenMode_t openMode, NDArray *pArray);
    asynStatus readFileBase();
    asynStatus writeFileBase();
    asynStatus closeFileBase();
    asynStatus doCapture(int capture);
    void       freeCaptureBuffer(int numCapture);
    void       doNDArrayCallbacks(NDArray *pArray);
    asynStatus attrFileNameCheck();
    asynStatus attrFileNameSet();
    bool attrIsProcessingRequired(NDAttributeList* pAttrList);
    void registerInitFrameInfo(NDArray *pArray); /**< Grab a copy of the NDArrayInfo_t structure for future reference */
    bool isFrameValid(NDArray *pArray); /**< Compare pArray dimensions and datatype against latched NDArrayInfo_t structure */

    NDArray **pCapture;
    int captureBufferSize;
    epicsMutexId fileMutexId;
    bool useAttrFilePrefix;
    bool lazyOpen;
    NDArrayInfo_t *ndArrayInfoInit; /**< The NDArray information at file open time.
                                      *  Used to check against changes in incoming frames dimensions or datatype */
};

#define NUM_NDPLUGIN_FILE_PARAMS 0
    
#endif
