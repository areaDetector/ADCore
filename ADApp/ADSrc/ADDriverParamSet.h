#ifndef ADDriverParamSet_H
#define ADDriverParamSet_H

#include "asynNDArrayDriverParamSet.h"

#define ADMaxSizeXString "MAX_SIZE_X"
#define ADMaxSizeYString "MAX_SIZE_Y"
#define ADBinXString "BIN_X"
#define ADBinYString "BIN_Y"
#define ADMinYString "MIN_Y"
#define ADMinXString "MIN_X"
#define ADReverseXString "REVERSE_X"
#define ADReverseYString "REVERSE_Y"
#define ADSizeXString "SIZE_X"
#define ADSizeYString "SIZE_Y"
#define ADGainString "GAIN"
#define ADShutterModeString "SHUTTER_MODE"
#define ADShutterStatusString "SHUTTER_STATUS"
#define ADShutterControlString "SHUTTER_CONTROL"
#define ADShutterOpenDelayString "SHUTTER_OPEN_DELAY"
#define ADShutterCloseDelayString "SHUTTER_CLOSE_DELAY"
#define ADAcquireTimeString "ACQ_TIME"
#define ADAcquirePeriodString "ACQ_PERIOD"
#define ADNumImagesCounterString "NIMAGES_COUNTER"
#define ADNumImagesString "NIMAGES"
#define ADNumExposuresString "NEXPOSURES"
#define ADImageModeString "IMAGE_MODE"
#define ADTriggerModeString "TRIGGER_MODE"
#define ADStatusString "STATUS"
#define ADStatusMessageString "STATUS_MESSAGE"
#define ADTimeRemainingString "TIME_REMAINING"
#define ADReadStatusString "READ_STATUS"
#define ADNumExposuresCounterString "NEXPOSURES_COUNTER"
#define ADStringToServerString "STRING_TO_SERVER"
#define ADStringFromServerString "STRING_FROM_SERVER"
#define ADShutterControlEPICSString "SHUTTER_CONTROL_EPICS"
#define ADTemperatureActualString "TEMPERATURE_ACTUAL"
#define ADFrameTypeString "FRAME_TYPE"
#define ADTemperatureString "TEMPERATURE"

const std::string ADBaseParamTree = \
"{\"parameters\":[{\"type\": \"Group\", \"name\": \"ADReadout\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"ArraySizeX\", \"label\": \"\", \"pv\": \"ArraySizeX_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ArraySizeY\", \"label\": \"\", \"pv\": \"ArraySizeY_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ArraySize\", \"label\": \"\", \"pv\": \"ArraySize_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"DataType\", \"label\": \"\", \"pv\": \"DataType\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"DataType_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ColorMode\", \"label\": \"\", \"pv\": \"ColorMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"ColorMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"MaxSizeX\", \"label\": \"\", \"pv\": \"MaxSizeX_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"MaxSizeY\", \"label\": \"\", \"pv\": \"MaxSizeY_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"BinX\", \"label\": \"\", \"pv\": \"BinX\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"BinX_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"BinY\", \"label\": \"\", \"pv\": \"BinY\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"BinY_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"MinY\", \"label\": \"\", \"pv\": \"MinY\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"MinY_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"MinX\", \"label\": \"\", \"pv\": \"MinX\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"MinX_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ReverseX\", \"label\": \"\", \"pv\": \"ReverseX\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ReverseX_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"ReverseY\", \"label\": \"\", \"pv\": \"ReverseY\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ReverseY_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"SizeX\", \"label\": \"\", \"pv\": \"SizeX\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"SizeX_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"SizeY\", \"label\": \"\", \"pv\": \"SizeY\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"SizeY_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"Gain\", \"label\": \"\", \"pv\": \"Gain\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"Gain_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADShutter\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"ShutterMode\", \"label\": \"\", \"pv\": \"ShutterMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"ShutterMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ShutterStatus\", \"label\": \"\", \"pv\": \"ShutterStatus_RBV\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"ShutterControl\", \"label\": \"\", \"pv\": \"ShutterControl\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ShutterControl_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"ShutterOpenDelay\", \"label\": \"\", \"pv\": \"ShutterOpenDelay\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ShutterOpenDelay_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ShutterCloseDelay\", \"label\": \"\", \"pv\": \"ShutterCloseDelay\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ShutterCloseDelay_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADCollect\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"NumQueuedArrays\", \"label\": \"\", \"pv\": \"NumQueuedArrays\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalW\", \"name\": \"WaitForPlugins\", \"label\": \"\", \"pv\": \"WaitForPlugins\", \"widget\": {\"type\": \"CheckBox\"}}, {\"type\": \"SignalRW\", \"name\": \"Acquire\", \"label\": \"\", \"pv\": \"Acquire\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"\", \"read_widget\": null}, {\"type\": \"SignalRW\", \"name\": \"ArrayCounter\", \"label\": \"\", \"pv\": \"ArrayCounter\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ArrayCounter_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ArrayCallbacks\", \"label\": \"\", \"pv\": \"ArrayCallbacks\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ArrayCallbacks_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"AcquireTime\", \"label\": \"\", \"pv\": \"AcquireTime\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"AcquireTime_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"AcquirePeriod\", \"label\": \"\", \"pv\": \"AcquirePeriod\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"AcquirePeriod_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"NumImagesCounter\", \"label\": \"\", \"pv\": \"NumImagesCounter_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NumImages\", \"label\": \"\", \"pv\": \"NumImages\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumImages_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NumExposures\", \"label\": \"\", \"pv\": \"NumExposures\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumExposures_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ImageMode\", \"label\": \"\", \"pv\": \"ImageMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"ImageMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"TriggerMode\", \"label\": \"\", \"pv\": \"TriggerMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"TriggerMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"DetectorState\", \"label\": \"\", \"pv\": \"DetectorState_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"StatusMessage\", \"label\": \"\", \"pv\": \"StatusMessage_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"TimeRemaining\", \"label\": \"\", \"pv\": \"TimeRemaining_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADBaseMisc\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalW\", \"name\": \"ReadStatus\", \"label\": \"\", \"pv\": \"ReadStatus\", \"widget\": {\"type\": \"CheckBox\"}}, {\"type\": \"SignalR\", \"name\": \"NumExposuresCounter\", \"label\": \"\", \"pv\": \"NumExposuresCounter_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"StringToServer\", \"label\": \"\", \"pv\": \"StringToServer_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"StringFromServer\", \"label\": \"\", \"pv\": \"StringFromServer_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ShutterControlEPICS\", \"label\": \"\", \"pv\": \"ShutterControlEPICS\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"TemperatureActual\", \"label\": \"\", \"pv\": \"TemperatureActual\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FrameType\", \"label\": \"\", \"pv\": \"FrameType\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"FrameType_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"Temperature\", \"label\": \"\", \"pv\": \"Temperature\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"Temperature_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADSetup\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"PortName\", \"label\": \"\", \"pv\": \"PortName_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"Manufacturer\", \"label\": \"\", \"pv\": \"Manufacturer_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"Model\", \"label\": \"\", \"pv\": \"Model_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"SerialNumber\", \"label\": \"\", \"pv\": \"SerialNumber_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"FirmwareVersion\", \"label\": \"\", \"pv\": \"FirmwareVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"SDKVersion\", \"label\": \"\", \"pv\": \"SDKVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"DriverVersion\", \"label\": \"\", \"pv\": \"DriverVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ADCoreVersion\", \"label\": \"\", \"pv\": \"ADCoreVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADAttrFile\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalW\", \"name\": \"NDAttributesFile\", \"label\": \"\", \"pv\": \"NDAttributesFile\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}}, {\"type\": \"SignalW\", \"name\": \"NDAttributesMacros\", \"label\": \"\", \"pv\": \"NDAttributesMacros\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"NDAttributesStatus\", \"label\": \"\", \"pv\": \"NDAttributesStatus\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFile\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"FileFormat\", \"label\": \"\", \"pv\": \"FileFormat\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"FileFormat_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFileBase\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"FilePathExists\", \"label\": \"\", \"pv\": \"FilePathExists_RBV\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"FilePath\", \"label\": \"\", \"pv\": \"FilePath\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FilePath_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"CreateDirectory\", \"label\": \"\", \"pv\": \"CreateDirectory\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"CreateDirectory_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FileName\", \"label\": \"\", \"pv\": \"FileName\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FileName_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FileNumber\", \"label\": \"\", \"pv\": \"FileNumber\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"\", \"read_widget\": null}, {\"type\": \"SignalRW\", \"name\": \"TempSuffix\", \"label\": \"\", \"pv\": \"TempSuffix\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"TempSuffix_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"LazyOpen\", \"label\": \"\", \"pv\": \"LazyOpen\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"LazyOpen_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"AutoIncrement\", \"label\": \"\", \"pv\": \"AutoIncrement\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"AutoIncrement_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"FileTemplate\", \"label\": \"\", \"pv\": \"FileTemplate\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FileTemplate_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"FullFileName\", \"label\": \"\", \"pv\": \"FullFileName_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"WriteFile\", \"label\": \"\", \"pv\": \"WriteFile\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"WriteFile_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"ReadFile\", \"label\": \"\", \"pv\": \"ReadFile\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ReadFile_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"AutoSave\", \"label\": \"\", \"pv\": \"AutoSave\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"AutoSave_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"NumCaptured\", \"label\": \"\", \"pv\": \"NumCaptured_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FileWriteMode\", \"label\": \"\", \"pv\": \"FileWriteMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"FileWriteMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NumCapture\", \"label\": \"\", \"pv\": \"NumCapture\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumCapture_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"Capture\", \"label\": \"\", \"pv\": \"Capture\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"Capture_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"DeleteDriverFile\", \"label\": \"\", \"pv\": \"DeleteDriverFile\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"DeleteDriverFile_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"WriteStatus\", \"label\": \"\", \"pv\": \"WriteStatus\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"WriteMessage\", \"label\": \"\", \"pv\": \"WriteMessage\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDArrayBaseMisc\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalW\", \"name\": \"EmptyFreeList\", \"label\": \"\", \"pv\": \"EmptyFreeList\", \"widget\": {\"type\": \"CheckBox\"}}, {\"type\": \"SignalR\", \"name\": \"AcquireBusyCB\", \"label\": \"\", \"pv\": \"AcquireBusyCB\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"BayerPattern\", \"label\": \"\", \"pv\": \"BayerPattern_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ArraySizeZ\", \"label\": \"\", \"pv\": \"ArraySizeZ_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"Codec\", \"label\": \"\", \"pv\": \"Codec_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"CompressedSize\", \"label\": \"\", \"pv\": \"CompressedSize_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"UniqueId\", \"label\": \"\", \"pv\": \"UniqueId_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"TimeStamp\", \"label\": \"\", \"pv\": \"TimeStamp_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"EpicsTSSec\", \"label\": \"\", \"pv\": \"EpicsTSSec_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"EpicsTSNsec\", \"label\": \"\", \"pv\": \"EpicsTSNsec_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolMaxMem\", \"label\": \"\", \"pv\": \"PoolMaxMem\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolUsedMem\", \"label\": \"\", \"pv\": \"PoolUsedMem\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolAllocBuffers\", \"label\": \"\", \"pv\": \"PoolAllocBuffers\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolFreeBuffers\", \"label\": \"\", \"pv\": \"PoolFreeBuffers\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NDimensions\", \"label\": \"\", \"pv\": \"NDimensions\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NDimensions_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"Dimensions\", \"label\": \"\", \"pv\": \"Dimensions\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"Dimensions_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}]}";

class ADDriverParamSet : public virtual asynNDArrayDriverParamSet {
public:
    ADDriverParamSet() {
        this->add(ADMaxSizeXString, asynParamInt32, &ADMaxSizeX);
        this->add(ADMaxSizeYString, asynParamInt32, &ADMaxSizeY);
        this->add(ADBinXString, asynParamInt32, &ADBinX);
        this->add(ADBinYString, asynParamInt32, &ADBinY);
        this->add(ADMinYString, asynParamInt32, &ADMinY);
        this->add(ADMinXString, asynParamInt32, &ADMinX);
        this->add(ADReverseXString, asynParamInt32, &ADReverseX);
        this->add(ADReverseYString, asynParamInt32, &ADReverseY);
        this->add(ADSizeXString, asynParamInt32, &ADSizeX);
        this->add(ADSizeYString, asynParamInt32, &ADSizeY);
        this->add(ADGainString, asynParamFloat64, &ADGain);
        this->add(ADShutterModeString, asynParamInt32, &ADShutterMode);
        this->add(ADShutterStatusString, asynParamInt32, &ADShutterStatus);
        this->add(ADShutterControlString, asynParamInt32, &ADShutterControl);
        this->add(ADShutterOpenDelayString, asynParamFloat64, &ADShutterOpenDelay);
        this->add(ADShutterCloseDelayString, asynParamFloat64, &ADShutterCloseDelay);
        this->add(ADAcquireTimeString, asynParamFloat64, &ADAcquireTime);
        this->add(ADAcquirePeriodString, asynParamFloat64, &ADAcquirePeriod);
        this->add(ADNumImagesCounterString, asynParamInt32, &ADNumImagesCounter);
        this->add(ADNumImagesString, asynParamInt32, &ADNumImages);
        this->add(ADNumExposuresString, asynParamInt32, &ADNumExposures);
        this->add(ADImageModeString, asynParamInt32, &ADImageMode);
        this->add(ADTriggerModeString, asynParamInt32, &ADTriggerMode);
        this->add(ADStatusString, asynParamInt32, &ADStatus);
        this->add(ADStatusMessageString, asynParamOctet, &ADStatusMessage);
        this->add(ADTimeRemainingString, asynParamFloat64, &ADTimeRemaining);
        this->add(ADReadStatusString, asynParamInt32, &ADReadStatus);
        this->add(ADNumExposuresCounterString, asynParamInt32, &ADNumExposuresCounter);
        this->add(ADStringToServerString, asynParamOctet, &ADStringToServer);
        this->add(ADStringFromServerString, asynParamOctet, &ADStringFromServer);
        this->add(ADShutterControlEPICSString, asynParamInt32, &ADShutterControlEPICS);
        this->add(ADTemperatureActualString, asynParamFloat64, &ADTemperatureActual);
        this->add(ADFrameTypeString, asynParamInt32, &ADFrameType);
        this->add(ADTemperatureString, asynParamFloat64, &ADTemperature);

        this->paramTree = ADBaseParamTree;
    }

    int ADMaxSizeX;
    #define FIRST_ADDRIVERPARAMSET_PARAM ADMaxSizeX
    int ADMaxSizeY;
    int ADBinX;
    int ADBinY;
    int ADMinY;
    int ADMinX;
    int ADReverseX;
    int ADReverseY;
    int ADSizeX;
    int ADSizeY;
    int ADGain;
    int ADShutterMode;
    int ADShutterStatus;
    int ADShutterControl;
    int ADShutterOpenDelay;
    int ADShutterCloseDelay;
    int ADAcquireTime;
    int ADAcquirePeriod;
    int ADNumImagesCounter;
    int ADNumImages;
    int ADNumExposures;
    int ADImageMode;
    int ADTriggerMode;
    int ADStatus;
    int ADStatusMessage;
    int ADTimeRemaining;
    int ADReadStatus;
    int ADNumExposuresCounter;
    int ADStringToServer;
    int ADStringFromServer;
    int ADShutterControlEPICS;
    int ADTemperatureActual;
    int ADFrameType;
    int ADTemperature;
};

#endif // ADDriverParamSet_H
