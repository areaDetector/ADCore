#ifndef AsynNDArrayDriverParamSet_H
#define AsynNDArrayDriverParamSet_H

#include "asynParamSet.h"

#define NDArrayDataString "ARRAY_DATA"
#define NDPortNameSelfString "PORT_NAME_SELF"
#define ADManufacturerString "MANUFACTURER"
#define ADModelString "MODEL"
#define ADSerialNumberString "SERIAL_NUMBER"
#define ADFirmwareVersionString "FIRMWARE_VERSION"
#define ADSDKVersionString "SDK_VERSION"
#define NDDriverVersionString "DRIVER_VERSION"
#define NDADCoreVersionString "ADCORE_VERSION"
#define NDArraySizeXString "ARRAY_SIZE_X"
#define NDArraySizeYString "ARRAY_SIZE_Y"
#define NDArraySizeString "ARRAY_SIZE"
#define NDDataTypeString "DATA_TYPE"
#define NDColorModeString "COLOR_MODE"
#define NDNumQueuedArraysString "NUM_QUEUED_ARRAYS"
#define ADWaitForPluginsString "WAIT_FOR_PLUGINS"
#define ADAcquireString "ACQUIRE"
#define NDArrayCounterString "ARRAY_COUNTER"
#define NDArrayCallbacksString "ARRAY_CALLBACKS"
#define NDAttributesFileString "ND_ATTRIBUTES_FILE"
#define NDAttributesMacrosString "ND_ATTRIBUTES_MACROS"
#define NDAttributesStatusString "ND_ATTRIBUTES_STATUS"
#define NDFileFormatString "FILE_FORMAT"
#define NDFilePathExistsString "FILE_PATH_EXISTS"
#define NDFilePathString "FILE_PATH"
#define NDFileCreateDirString "CREATE_DIR"
#define NDFileNameString "FILE_NAME"
#define NDFileNumberString "FILE_NUMBER"
#define NDFileTempSuffixString "FILE_TEMP_SUFFIX"
#define NDFileLazyOpenString "FILE_LAZY_OPEN"
#define NDAutoIncrementString "AUTO_INCREMENT"
#define NDFileTemplateString "FILE_TEMPLATE"
#define NDFullFileNameString "FULL_FILE_NAME"
#define NDWriteFileString "WRITE_FILE"
#define NDReadFileString "READ_FILE"
#define NDAutoSaveString "AUTO_SAVE"
#define NDFileNumCapturedString "NUM_CAPTURED"
#define NDFileWriteModeString "WRITE_MODE"
#define NDFileNumCaptureString "NUM_CAPTURE"
#define NDFileCaptureString "CAPTURE"
#define NDFileDeleteDriverFileString "DELETE_DRIVER_FILE"
#define NDFileWriteStatusString "WRITE_STATUS"
#define NDFileWriteMessageString "WRITE_MESSAGE"
#define NDPoolEmptyFreeListString "POOL_EMPTY_FREELIST"
#define ADAcquireBusyString "ACQUIRE_BUSY"
#define NDBayerPatternString "BAYER_PATTERN"
#define NDArraySizeZString "ARRAY_SIZE_Z"
#define NDCodecString "CODEC"
#define NDCompressedSizeString "COMPRESSED_SIZE"
#define NDUniqueIdString "UNIQUE_ID"
#define NDTimeStampString "TIME_STAMP"
#define NDEpicsTSSecString "EPICS_TS_SEC"
#define NDEpicsTSNsecString "EPICS_TS_NSEC"
#define NDPoolMaxMemoryString "POOL_MAX_MEMORY"
#define NDPoolUsedMemoryString "POOL_USED_MEMORY"
#define NDPoolAllocBuffersString "POOL_ALLOC_BUFFERS"
#define NDPoolFreeBuffersString "POOL_FREE_BUFFERS"
#define NDNDimensionsString "ARRAY_NDIMENSIONS"
#define NDDimensionsString "ARRAY_DIMENSIONS"

const std::string NDArrayBaseParamTree = \
"{\"parameters\":[{\"type\": \"Group\", \"name\": \"ADSetup\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"PortName\", \"label\": \"\", \"pv\": \"PortName_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"Manufacturer\", \"label\": \"\", \"pv\": \"Manufacturer_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"Model\", \"label\": \"\", \"pv\": \"Model_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"SerialNumber\", \"label\": \"\", \"pv\": \"SerialNumber_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"FirmwareVersion\", \"label\": \"\", \"pv\": \"FirmwareVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"SDKVersion\", \"label\": \"\", \"pv\": \"SDKVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"DriverVersion\", \"label\": \"\", \"pv\": \"DriverVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ADCoreVersion\", \"label\": \"\", \"pv\": \"ADCoreVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADReadout\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"ArraySizeX\", \"label\": \"\", \"pv\": \"ArraySizeX_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ArraySizeY\", \"label\": \"\", \"pv\": \"ArraySizeY_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ArraySize\", \"label\": \"\", \"pv\": \"ArraySize_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"DataType\", \"label\": \"\", \"pv\": \"DataType\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"DataType_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ColorMode\", \"label\": \"\", \"pv\": \"ColorMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"ColorMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADCollect\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"NumQueuedArrays\", \"label\": \"\", \"pv\": \"NumQueuedArrays\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalW\", \"name\": \"WaitForPlugins\", \"label\": \"\", \"pv\": \"WaitForPlugins\", \"widget\": {\"type\": \"CheckBox\"}}, {\"type\": \"SignalRW\", \"name\": \"Acquire\", \"label\": \"\", \"pv\": \"Acquire\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"\", \"read_widget\": null}, {\"type\": \"SignalRW\", \"name\": \"ArrayCounter\", \"label\": \"\", \"pv\": \"ArrayCounter\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ArrayCounter_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ArrayCallbacks\", \"label\": \"\", \"pv\": \"ArrayCallbacks\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ArrayCallbacks_RBV\", \"read_widget\": {\"type\": \"LED\"}}]}, {\"type\": \"Group\", \"name\": \"ADAttrFile\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalW\", \"name\": \"NDAttributesFile\", \"label\": \"\", \"pv\": \"NDAttributesFile\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}}, {\"type\": \"SignalW\", \"name\": \"NDAttributesMacros\", \"label\": \"\", \"pv\": \"NDAttributesMacros\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"NDAttributesStatus\", \"label\": \"\", \"pv\": \"NDAttributesStatus\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFile\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"FileFormat\", \"label\": \"\", \"pv\": \"FileFormat\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"FileFormat_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFileBase\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"FilePathExists\", \"label\": \"\", \"pv\": \"FilePathExists_RBV\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"FilePath\", \"label\": \"\", \"pv\": \"FilePath\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FilePath_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"CreateDirectory\", \"label\": \"\", \"pv\": \"CreateDirectory\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"CreateDirectory_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FileName\", \"label\": \"\", \"pv\": \"FileName\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FileName_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FileNumber\", \"label\": \"\", \"pv\": \"FileNumber\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"\", \"read_widget\": null}, {\"type\": \"SignalRW\", \"name\": \"TempSuffix\", \"label\": \"\", \"pv\": \"TempSuffix\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"TempSuffix_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"LazyOpen\", \"label\": \"\", \"pv\": \"LazyOpen\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"LazyOpen_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"AutoIncrement\", \"label\": \"\", \"pv\": \"AutoIncrement\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"AutoIncrement_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"FileTemplate\", \"label\": \"\", \"pv\": \"FileTemplate\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FileTemplate_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"FullFileName\", \"label\": \"\", \"pv\": \"FullFileName_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"WriteFile\", \"label\": \"\", \"pv\": \"WriteFile\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"WriteFile_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"ReadFile\", \"label\": \"\", \"pv\": \"ReadFile\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ReadFile_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"AutoSave\", \"label\": \"\", \"pv\": \"AutoSave\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"AutoSave_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"NumCaptured\", \"label\": \"\", \"pv\": \"NumCaptured_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FileWriteMode\", \"label\": \"\", \"pv\": \"FileWriteMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"FileWriteMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NumCapture\", \"label\": \"\", \"pv\": \"NumCapture\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumCapture_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"Capture\", \"label\": \"\", \"pv\": \"Capture\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"Capture_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"DeleteDriverFile\", \"label\": \"\", \"pv\": \"DeleteDriverFile\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"DeleteDriverFile_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"WriteStatus\", \"label\": \"\", \"pv\": \"WriteStatus\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"WriteMessage\", \"label\": \"\", \"pv\": \"WriteMessage\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDArrayBaseMisc\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalW\", \"name\": \"EmptyFreeList\", \"label\": \"\", \"pv\": \"EmptyFreeList\", \"widget\": {\"type\": \"CheckBox\"}}, {\"type\": \"SignalR\", \"name\": \"AcquireBusyCB\", \"label\": \"\", \"pv\": \"AcquireBusyCB\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"BayerPattern\", \"label\": \"\", \"pv\": \"BayerPattern_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ArraySizeZ\", \"label\": \"\", \"pv\": \"ArraySizeZ_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"Codec\", \"label\": \"\", \"pv\": \"Codec_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"CompressedSize\", \"label\": \"\", \"pv\": \"CompressedSize_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"UniqueId\", \"label\": \"\", \"pv\": \"UniqueId_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"TimeStamp\", \"label\": \"\", \"pv\": \"TimeStamp_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"EpicsTSSec\", \"label\": \"\", \"pv\": \"EpicsTSSec_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"EpicsTSNsec\", \"label\": \"\", \"pv\": \"EpicsTSNsec_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolMaxMem\", \"label\": \"\", \"pv\": \"PoolMaxMem\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolUsedMem\", \"label\": \"\", \"pv\": \"PoolUsedMem\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolAllocBuffers\", \"label\": \"\", \"pv\": \"PoolAllocBuffers\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolFreeBuffers\", \"label\": \"\", \"pv\": \"PoolFreeBuffers\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NDimensions\", \"label\": \"\", \"pv\": \"NDimensions\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NDimensions_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"Dimensions\", \"label\": \"\", \"pv\": \"Dimensions\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"Dimensions_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}]}";

class asynNDArrayDriverParamSet : public virtual asynParamSet {
public:
    asynNDArrayDriverParamSet() {
        this->add(NDArrayDataString, asynParamGenericPointer, &NDArrayData);
        this->add(NDPortNameSelfString, asynParamOctet, &NDPortNameSelf);
        this->add(ADManufacturerString, asynParamOctet, &ADManufacturer);
        this->add(ADModelString, asynParamOctet, &ADModel);
        this->add(ADSerialNumberString, asynParamOctet, &ADSerialNumber);
        this->add(ADFirmwareVersionString, asynParamOctet, &ADFirmwareVersion);
        this->add(ADSDKVersionString, asynParamOctet, &ADSDKVersion);
        this->add(NDDriverVersionString, asynParamOctet, &NDDriverVersion);
        this->add(NDADCoreVersionString, asynParamOctet, &NDADCoreVersion);
        this->add(NDArraySizeXString, asynParamInt32, &NDArraySizeX);
        this->add(NDArraySizeYString, asynParamInt32, &NDArraySizeY);
        this->add(NDArraySizeString, asynParamInt32, &NDArraySize);
        this->add(NDDataTypeString, asynParamInt32, &NDDataType);
        this->add(NDColorModeString, asynParamInt32, &NDColorMode);
        this->add(NDNumQueuedArraysString, asynParamInt32, &NDNumQueuedArrays);
        this->add(ADWaitForPluginsString, asynParamInt32, &ADWaitForPlugins);
        this->add(ADAcquireString, asynParamInt32, &ADAcquire);
        this->add(NDArrayCounterString, asynParamInt32, &NDArrayCounter);
        this->add(NDArrayCallbacksString, asynParamInt32, &NDArrayCallbacks);
        this->add(NDAttributesFileString, asynParamOctet, &NDAttributesFile);
        this->add(NDAttributesMacrosString, asynParamOctet, &NDAttributesMacros);
        this->add(NDAttributesStatusString, asynParamInt32, &NDAttributesStatus);
        this->add(NDFileFormatString, asynParamInt32, &NDFileFormat);
        this->add(NDFilePathExistsString, asynParamInt32, &NDFilePathExists);
        this->add(NDFilePathString, asynParamOctet, &NDFilePath);
        this->add(NDFileCreateDirString, asynParamInt32, &NDFileCreateDir);
        this->add(NDFileNameString, asynParamOctet, &NDFileName);
        this->add(NDFileNumberString, asynParamInt32, &NDFileNumber);
        this->add(NDFileTempSuffixString, asynParamOctet, &NDFileTempSuffix);
        this->add(NDFileLazyOpenString, asynParamInt32, &NDFileLazyOpen);
        this->add(NDAutoIncrementString, asynParamInt32, &NDAutoIncrement);
        this->add(NDFileTemplateString, asynParamOctet, &NDFileTemplate);
        this->add(NDFullFileNameString, asynParamOctet, &NDFullFileName);
        this->add(NDWriteFileString, asynParamInt32, &NDWriteFile);
        this->add(NDReadFileString, asynParamInt32, &NDReadFile);
        this->add(NDAutoSaveString, asynParamInt32, &NDAutoSave);
        this->add(NDFileNumCapturedString, asynParamInt32, &NDFileNumCaptured);
        this->add(NDFileWriteModeString, asynParamInt32, &NDFileWriteMode);
        this->add(NDFileNumCaptureString, asynParamInt32, &NDFileNumCapture);
        this->add(NDFileCaptureString, asynParamInt32, &NDFileCapture);
        this->add(NDFileDeleteDriverFileString, asynParamInt32, &NDFileDeleteDriverFile);
        this->add(NDFileWriteStatusString, asynParamInt32, &NDFileWriteStatus);
        this->add(NDFileWriteMessageString, asynParamOctet, &NDFileWriteMessage);
        this->add(NDPoolEmptyFreeListString, asynParamInt32, &NDPoolEmptyFreeList);
        this->add(ADAcquireBusyString, asynParamInt32, &ADAcquireBusy);
        this->add(NDBayerPatternString, asynParamInt32, &NDBayerPattern);
        this->add(NDArraySizeZString, asynParamInt32, &NDArraySizeZ);
        this->add(NDCodecString, asynParamOctet, &NDCodec);
        this->add(NDCompressedSizeString, asynParamInt32, &NDCompressedSize);
        this->add(NDUniqueIdString, asynParamInt32, &NDUniqueId);
        this->add(NDTimeStampString, asynParamFloat64, &NDTimeStamp);
        this->add(NDEpicsTSSecString, asynParamInt32, &NDEpicsTSSec);
        this->add(NDEpicsTSNsecString, asynParamInt32, &NDEpicsTSNsec);
        this->add(NDPoolMaxMemoryString, asynParamFloat64, &NDPoolMaxMemory);
        this->add(NDPoolUsedMemoryString, asynParamFloat64, &NDPoolUsedMemory);
        this->add(NDPoolAllocBuffersString, asynParamInt32, &NDPoolAllocBuffers);
        this->add(NDPoolFreeBuffersString, asynParamInt32, &NDPoolFreeBuffers);
        this->add(NDNDimensionsString, asynParamInt32, &NDNDimensions);
        this->add(NDDimensionsString, asynParamInt32, &NDDimensions);

        this->paramTree = NDArrayBaseParamTree;
    }

    int NDArrayData;
    #define FIRST_ASYNNDARRAYDRIVERPARAMSET_PARAM NDArrayData
    int NDPortNameSelf;
    int ADManufacturer;
    int ADModel;
    int ADSerialNumber;
    int ADFirmwareVersion;
    int ADSDKVersion;
    int NDDriverVersion;
    int NDADCoreVersion;
    int NDArraySizeX;
    int NDArraySizeY;
    int NDArraySize;
    int NDDataType;
    int NDColorMode;
    int NDNumQueuedArrays;
    int ADWaitForPlugins;
    int ADAcquire;
    int NDArrayCounter;
    int NDArrayCallbacks;
    int NDAttributesFile;
    int NDAttributesMacros;
    int NDAttributesStatus;
    int NDFileFormat;
    int NDFilePathExists;
    int NDFilePath;
    int NDFileCreateDir;
    int NDFileName;
    int NDFileNumber;
    int NDFileTempSuffix;
    int NDFileLazyOpen;
    int NDAutoIncrement;
    int NDFileTemplate;
    int NDFullFileName;
    int NDWriteFile;
    int NDReadFile;
    int NDAutoSave;
    int NDFileNumCaptured;
    int NDFileWriteMode;
    int NDFileNumCapture;
    int NDFileCapture;
    int NDFileDeleteDriverFile;
    int NDFileWriteStatus;
    int NDFileWriteMessage;
    int NDPoolEmptyFreeList;
    int ADAcquireBusy;
    int NDBayerPattern;
    int NDArraySizeZ;
    int NDCodec;
    int NDCompressedSize;
    int NDUniqueId;
    int NDTimeStamp;
    int NDEpicsTSSec;
    int NDEpicsTSNsec;
    int NDPoolMaxMemory;
    int NDPoolUsedMemory;
    int NDPoolAllocBuffers;
    int NDPoolFreeBuffers;
    int NDNDimensions;
    int NDDimensions;
};

#endif // AsynNDArrayDriverParamSet_H
