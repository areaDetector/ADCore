#ifndef AsynNDArrayDriverParamSet_H
#define AsynNDArrayDriverParamSet_H

#include "asynParamSet.h"

#define NDPortNameSelfString "PORT_NAME_SELF"
#define NDADCoreVersionString "ADCORE_VERSION"
#define NDDriverVersionString "DRIVER_VERSION"
#define ADManufacturerString "MANUFACTURER"
#define ADModelString "MODEL"
#define ADSerialNumberString "SERIAL_NUMBER"
#define ADSDKVersionString "SDK_VERSION"
#define ADFirmwareVersionString "FIRMWARE_VERSION"
#define ADAcquireString "ACQUIRE"
#define ADAcquireBusyString "ACQUIRE_BUSY"
#define ADWaitForPluginsString "WAIT_FOR_PLUGINS"
#define NDArraySizeXString "ARRAY_SIZE_X"
#define NDArraySizeYString "ARRAY_SIZE_Y"
#define NDArraySizeZString "ARRAY_SIZE_Z"
#define NDArraySizeString "ARRAY_SIZE"
#define NDNDimensionsString "ARRAY_NDIMENSIONS"
#define NDDimensionsString "ARRAY_DIMENSIONS"
#define NDDataTypeString "DATA_TYPE"
#define NDColorModeString "COLOR_MODE"
#define NDUniqueIdString "UNIQUE_ID"
#define NDTimeStampString "TIME_STAMP"
#define NDEpicsTSSecString "EPICS_TS_SEC"
#define NDEpicsTSNsecString "EPICS_TS_NSEC"
#define NDBayerPatternString "BAYER_PATTERN"
#define NDCodecString "CODEC"
#define NDCompressedSizeString "COMPRESSED_SIZE"
#define NDArrayCounterString "ARRAY_COUNTER"
#define NDFilePathString "FILE_PATH"
#define NDFilePathExistsString "FILE_PATH_EXISTS"
#define NDFileNameString "FILE_NAME"
#define NDFileNumberString "FILE_NUMBER"
#define NDFileTemplateString "FILE_TEMPLATE"
#define NDAutoIncrementString "AUTO_INCREMENT"
#define NDFullFileNameString "FULL_FILE_NAME"
#define NDFileFormatString "FILE_FORMAT"
#define NDAutoSaveString "AUTO_SAVE"
#define NDWriteFileString "WRITE_FILE"
#define NDReadFileString "READ_FILE"
#define NDFileWriteModeString "WRITE_MODE"
#define NDFileWriteStatusString "WRITE_STATUS"
#define NDFileWriteMessageString "WRITE_MESSAGE"
#define NDFileNumCaptureString "NUM_CAPTURE"
#define NDFileNumCapturedString "NUM_CAPTURED"
#define NDFileCaptureString "CAPTURE"
#define NDFileDeleteDriverFileString "DELETE_DRIVER_FILE"
#define NDFileLazyOpenString "FILE_LAZY_OPEN"
#define NDFileCreateDirString "CREATE_DIR"
#define NDFileTempSuffixString "FILE_TEMP_SUFFIX"
#define NDAttributesFileString "ND_ATTRIBUTES_FILE"
#define NDAttributesStatusString "ND_ATTRIBUTES_STATUS"
#define NDAttributesMacrosString "ND_ATTRIBUTES_MACROS"
#define NDArrayDataString "ARRAY_DATA"
#define NDArrayCallbacksString "ARRAY_CALLBACKS"
#define NDPoolMaxBuffersString "POOL_MAX_BUFFERS"
#define NDPoolAllocBuffersString "POOL_ALLOC_BUFFERS"
#define NDPoolFreeBuffersString "POOL_FREE_BUFFERS"
#define NDPoolMaxMemoryString "POOL_MAX_MEMORY"
#define NDPoolUsedMemoryString "POOL_USED_MEMORY"
#define NDPoolEmptyFreeListString "POOL_EMPTY_FREELIST"
#define NDNumQueuedArraysString "NUM_QUEUED_ARRAYS"

/** asynNDArrayDriver param set */
class asynNDArrayDriverParamSet : public virtual asynParamSet {
public:
    asynNDArrayDriverParamSet() {
        std::cout << "asynNDArrayDriverParamSet" << std::endl;
        this->add(NDPortNameSelfString, asynParamOctet, &NDPortNameSelf);
        this->add(NDADCoreVersionString, asynParamOctet, &NDADCoreVersion);
        this->add(NDDriverVersionString, asynParamOctet, &NDDriverVersion);
        this->add(ADManufacturerString, asynParamOctet, &ADManufacturer);
        this->add(ADModelString, asynParamOctet, &ADModel);
        this->add(ADSerialNumberString, asynParamOctet, &ADSerialNumber);
        this->add(ADSDKVersionString, asynParamOctet, &ADSDKVersion);
        this->add(ADFirmwareVersionString, asynParamOctet, &ADFirmwareVersion);
        this->add(ADAcquireString, asynParamInt32, &ADAcquire);
        this->add(ADAcquireBusyString, asynParamInt32, &ADAcquireBusy);
        this->add(ADWaitForPluginsString, asynParamInt32, &ADWaitForPlugins);
        this->add(NDArraySizeXString, asynParamInt32, &NDArraySizeX);
        this->add(NDArraySizeYString, asynParamInt32, &NDArraySizeY);
        this->add(NDArraySizeZString, asynParamInt32, &NDArraySizeZ);
        this->add(NDArraySizeString, asynParamInt32, &NDArraySize);
        this->add(NDNDimensionsString, asynParamInt32, &NDNDimensions);
        this->add(NDDimensionsString, asynParamInt32, &NDDimensions);
        this->add(NDDataTypeString, asynParamInt32, &NDDataType);
        this->add(NDColorModeString, asynParamInt32, &NDColorMode);
        this->add(NDUniqueIdString, asynParamInt32, &NDUniqueId);
        this->add(NDTimeStampString, asynParamFloat64, &NDTimeStamp);
        this->add(NDEpicsTSSecString, asynParamInt32, &NDEpicsTSSec);
        this->add(NDEpicsTSNsecString, asynParamInt32, &NDEpicsTSNsec);
        this->add(NDBayerPatternString, asynParamInt32, &NDBayerPattern);
        this->add(NDCodecString, asynParamOctet, &NDCodec);
        this->add(NDCompressedSizeString, asynParamInt32, &NDCompressedSize);
        this->add(NDArrayCounterString, asynParamInt32, &NDArrayCounter);
        this->add(NDFilePathString, asynParamOctet, &NDFilePath);
        this->add(NDFilePathExistsString, asynParamInt32, &NDFilePathExists);
        this->add(NDFileNameString, asynParamOctet, &NDFileName);
        this->add(NDFileNumberString, asynParamInt32, &NDFileNumber);
        this->add(NDFileTemplateString, asynParamOctet, &NDFileTemplate);
        this->add(NDAutoIncrementString, asynParamInt32, &NDAutoIncrement);
        this->add(NDFullFileNameString, asynParamOctet, &NDFullFileName);
        this->add(NDFileFormatString, asynParamInt32, &NDFileFormat);
        this->add(NDAutoSaveString, asynParamInt32, &NDAutoSave);
        this->add(NDWriteFileString, asynParamInt32, &NDWriteFile);
        this->add(NDReadFileString, asynParamInt32, &NDReadFile);
        this->add(NDFileWriteModeString, asynParamInt32, &NDFileWriteMode);
        this->add(NDFileWriteStatusString, asynParamInt32, &NDFileWriteStatus);
        this->add(NDFileWriteMessageString, asynParamOctet, &NDFileWriteMessage);
        this->add(NDFileNumCaptureString, asynParamInt32, &NDFileNumCapture);
        this->add(NDFileNumCapturedString, asynParamInt32, &NDFileNumCaptured);
        this->add(NDFileCaptureString, asynParamInt32, &NDFileCapture);
        this->add(NDFileDeleteDriverFileString, asynParamInt32, &NDFileDeleteDriverFile);
        this->add(NDFileLazyOpenString, asynParamInt32, &NDFileLazyOpen);
        this->add(NDFileCreateDirString, asynParamInt32, &NDFileCreateDir);
        this->add(NDFileTempSuffixString, asynParamOctet, &NDFileTempSuffix);
        this->add(NDAttributesFileString, asynParamOctet, &NDAttributesFile);
        this->add(NDAttributesStatusString, asynParamInt32, &NDAttributesStatus);
        this->add(NDAttributesMacrosString, asynParamOctet, &NDAttributesMacros);
        this->add(NDArrayDataString, asynParamGenericPointer, &NDArrayData);
        this->add(NDArrayCallbacksString, asynParamInt32, &NDArrayCallbacks);
        this->add(NDPoolMaxBuffersString, asynParamInt32, &NDPoolMaxBuffers);
        this->add(NDPoolAllocBuffersString, asynParamInt32, &NDPoolAllocBuffers);
        this->add(NDPoolFreeBuffersString, asynParamInt32, &NDPoolFreeBuffers);
        this->add(NDPoolMaxMemoryString, asynParamFloat64, &NDPoolMaxMemory);
        this->add(NDPoolUsedMemoryString, asynParamFloat64, &NDPoolUsedMemory);
        this->add(NDPoolEmptyFreeListString, asynParamInt32, &NDPoolEmptyFreeList);
        this->add(NDNumQueuedArraysString, asynParamInt32, &NDNumQueuedArrays);
    }

    int NDPortNameSelf;
    #define FIRST_NDARRAY_PARAM_INDEX NDPortNameSelf
    int NDADCoreVersion;
    int NDDriverVersion;
    int ADManufacturer;
    int ADModel;
    int ADSerialNumber;
    int ADSDKVersion;
    int ADFirmwareVersion;
    int ADAcquire;
    int ADAcquireBusy;
    int ADWaitForPlugins;
    int NDArraySizeX;
    int NDArraySizeY;
    int NDArraySizeZ;
    int NDArraySize;
    int NDNDimensions;
    int NDDimensions;
    int NDDataType;
    int NDColorMode;
    int NDUniqueId;
    int NDTimeStamp;
    int NDEpicsTSSec;
    int NDEpicsTSNsec;
    int NDBayerPattern;
    int NDCodec;
    int NDCompressedSize;
    int NDArrayCounter;
    int NDFilePath;
    int NDFilePathExists;
    int NDFileName;
    int NDFileNumber;
    int NDFileTemplate;
    int NDAutoIncrement;
    int NDFullFileName;
    int NDFileFormat;
    int NDAutoSave;
    int NDWriteFile;
    int NDReadFile;
    int NDFileWriteMode;
    int NDFileWriteStatus;
    int NDFileWriteMessage;
    int NDFileNumCapture;
    int NDFileNumCaptured;
    int NDFileCapture;
    int NDFileDeleteDriverFile;
    int NDFileLazyOpen;
    int NDFileCreateDir;
    int NDFileTempSuffix;
    int NDAttributesFile;
    int NDAttributesStatus;
    int NDAttributesMacros;
    int NDArrayData;
    int NDArrayCallbacks;
    int NDPoolMaxBuffers;
    int NDPoolAllocBuffers;
    int NDPoolFreeBuffers;
    int NDPoolMaxMemory;
    int NDPoolUsedMemory;
    int NDPoolEmptyFreeList;
    int NDNumQueuedArrays;
};

#endif  // AsynNDArrayDriverParamSet_H
