#ifndef NDFileHDF5ParamSet_H
#define NDFileHDF5ParamSet_H

#include "NDPluginFileParamSet.h"

#define NDFileHDF5_compressionTypeString "HDF5_compressionType"
#define NDFileHDF5_SWMRSupportedString "HDF5_SWMRSupported"
#define NDFileHDF5_nbitsPrecisionString "HDF5_nbitsPrecision"
#define NDFileHDF5_nbitsOffsetString "HDF5_nbitsOffset"
#define NDFileHDF5_SWMRModeString "HDF5_SWMRMode"
#define NDFileHDF5_SWMRRunningString "HDF5_SWMRRunning"
#define NDFileHDF5_szipNumPixelsString "HDF5_szipNumPixels"
#define NDFileHDF5_SWMRCbCounterString "HDF5_SWMRCbCounter"
#define NDFileHDF5_zCompressLevelString "HDF5_zCompressLevel"
#define NDFileHDF5_SWMRFlushNowString "HDF5_SWMRFlushNow"
#define NDFileHDF5_jpegQualityString "HDF5_jpegQuality"
#define NDFileHDF5_chunkSizeAutoString "HDF5_chunkSizeAuto"
#define NDFileHDF5_storePerformanceString "HDF5_storePerformance"
#define NDFileHDF5_nColChunksString "HDF5_nColChunks"
#define NDFileHDF5_storeAttributesString "HDF5_storeAttributes"
#define NDFileHDF5_bloscShuffleTypeString "HDF5_bloscShuffleType"
#define NDFileHDF5_nRowChunksString "HDF5_nRowChunks"
#define NDFileHDF5_totalRuntimeString "HDF5_totalRuntime"
#define NDFileHDF5_bloscCompressorString "HDF5_bloscCompressor"
#define NDFileHDF5_chunkSize2String "HDF5_chunkSize2"
#define NDFileHDF5_totalIoSpeedString "HDF5_totalIoSpeed"
#define NDFileHDF5_bloscCompressLevelString "HDF5_bloscCompressLevel"
#define NDFileHDF5_nFramesChunksString "HDF5_nFramesChunks"
#define NDFileHDF5_chunkBoundaryAlignString "HDF5_chunkBoundaryAlign"
#define NDFileHDF5_layoutErrorMsgString "HDF5_layoutErrorMsg"
#define NDFileHDF5_layoutValidString "HDF5_layoutValid"
#define NDFileHDF5_chunkBoundaryThresholdString "HDF5_chunkBoundaryThreshold"
#define NDFileHDF5_flushNthFrameString "HDF5_flushNthFrame"
#define NDFileHDF5_layoutFilenameString "HDF5_layoutFilename"
#define NDFileHDF5_fillValueString "HDF5_fillValue"
#define NDFileHDF5_chunkSize3String "HDF5_chunkSize3"
#define NDFileHDF5_chunkSize4String "HDF5_chunkSize4"
#define NDFileHDF5_chunkSize5String "HDF5_chunkSize5"
#define NDFileHDF5_chunkSize6String "HDF5_chunkSize6"
#define NDFileHDF5_chunkSize7String "HDF5_chunkSize7"
#define NDFileHDF5_chunkSize8String "HDF5_chunkSize8"
#define NDFileHDF5_chunkSize9String "HDF5_chunkSize9"
#define NDFileHDF5_nExtraDimsString "HDF5_nExtraDims"
#define NDFileHDF5_dimAttDatasetsString "HDF5_dimAttDatasets"
#define NDFileHDF5_ExtraDimSizeNString "HDF5_extraDimSizeN"
#define NDFileHDF5_extraDimNameNString "HDF5_extraDimNameN"
#define NDFileHDF5_ExtraDimSizeXString "HDF5_extraDimSizeX"
#define NDFileHDF5_extraDimChunkXString "HDF5_extraDimChunkX"
#define NDFileHDF5_extraDimNameXString "HDF5_extraDimNameX"
#define NDFileHDF5_ExtraDimSizeYString "HDF5_extraDimSizeY"
#define NDFileHDF5_extraDimChunkYString "HDF5_extraDimChunkY"
#define NDFileHDF5_extraDimNameYString "HDF5_extraDimNameY"
#define NDFileHDF5_ExtraDimSize3String "HDF5_extraDimSize3"
#define NDFileHDF5_extraDimChunk3String "HDF5_extraDimChunk3"
#define NDFileHDF5_extraDimName3String "HDF5_extraDimName3"
#define NDFileHDF5_ExtraDimSize4String "HDF5_extraDimSize4"
#define NDFileHDF5_extraDimChunk4String "HDF5_extraDimChunk4"
#define NDFileHDF5_extraDimName4String "HDF5_extraDimName4"
#define NDFileHDF5_ExtraDimSize5String "HDF5_extraDimSize5"
#define NDFileHDF5_extraDimChunk5String "HDF5_extraDimChunk5"
#define NDFileHDF5_extraDimName5String "HDF5_extraDimName5"
#define NDFileHDF5_ExtraDimSize6String "HDF5_extraDimSize6"
#define NDFileHDF5_extraDimChunk6String "HDF5_extraDimChunk6"
#define NDFileHDF5_extraDimName6String "HDF5_extraDimName6"
#define NDFileHDF5_ExtraDimSize7String "HDF5_extraDimSize7"
#define NDFileHDF5_extraDimChunk7String "HDF5_extraDimChunk7"
#define NDFileHDF5_extraDimName7String "HDF5_extraDimName7"
#define NDFileHDF5_ExtraDimSize8String "HDF5_extraDimSize8"
#define NDFileHDF5_extraDimChunk8String "HDF5_extraDimChunk8"
#define NDFileHDF5_extraDimName8String "HDF5_extraDimName8"
#define NDFileHDF5_ExtraDimSize9String "HDF5_extraDimSize9"
#define NDFileHDF5_extraDimChunk9String "HDF5_extraDimChunk9"
#define NDFileHDF5_extraDimName9String "HDF5_extraDimName9"
#define NDFileHDF5_posRunningString "HDF5_posRunning"
#define NDFileHDF5_posIndexDimNString "HDF5_posIndexDimN"
#define NDFileHDF5_posNameDimNString "HDF5_posNameDimN"
#define NDFileHDF5_posIndexDimXString "HDF5_posIndexDimX"
#define NDFileHDF5_posNameDimXString "HDF5_posNameDimX"
#define NDFileHDF5_posIndexDimYString "HDF5_posIndexDimY"
#define NDFileHDF5_posNameDimYString "HDF5_posNameDimY"
#define NDFileHDF5_posIndexDim3String "HDF5_posIndexDim3"
#define NDFileHDF5_posNameDim3String "HDF5_posNameDim3"
#define NDFileHDF5_posIndexDim4String "HDF5_posIndexDim4"
#define NDFileHDF5_posNameDim4String "HDF5_posNameDim4"
#define NDFileHDF5_posIndexDim5String "HDF5_posIndexDim5"
#define NDFileHDF5_posNameDim5String "HDF5_posNameDim5"
#define NDFileHDF5_posIndexDim6String "HDF5_posIndexDim6"
#define NDFileHDF5_posNameDim6String "HDF5_posNameDim6"
#define NDFileHDF5_posIndexDim7String "HDF5_posIndexDim7"
#define NDFileHDF5_posNameDim7String "HDF5_posNameDim7"
#define NDFileHDF5_posIndexDim8String "HDF5_posIndexDim8"
#define NDFileHDF5_posNameDim8String "HDF5_posNameDim8"
#define NDFileHDF5_posIndexDim9String "HDF5_posIndexDim9"
#define NDFileHDF5_posNameDim9String "HDF5_posNameDim9"
#define NDFileHDF5_NDAttributeChunkString "HDF5_NDAttributeChunk"
#define NDFileHDF5_extraDimChunkNString "HDF5_extraDimChunkN"

const std::string NDFileHDF5ParamTree = \
"{\"parameters\":[{\"type\": \"Group\", \"name\": \"NDFileHDF5\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"Compression\", \"label\": \"\", \"pv\": \"Compression\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"Compression_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"SWMRSupported\", \"label\": \"\", \"pv\": \"SWMRSupported_RBV\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"NumDataBits\", \"label\": \"\", \"pv\": \"NumDataBits\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumDataBits_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"DataBitsOffset\", \"label\": \"\", \"pv\": \"DataBitsOffset\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"DataBitsOffset_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"SWMRMode\", \"label\": \"\", \"pv\": \"SWMRMode\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"SWMRMode_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"SWMRActive\", \"label\": \"\", \"pv\": \"SWMRActive_RBV\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"SZipNumPixels\", \"label\": \"\", \"pv\": \"SZipNumPixels\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"SZipNumPixels_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"SWMRCbCounter\", \"label\": \"\", \"pv\": \"SWMRCbCounter_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ZLevel\", \"label\": \"\", \"pv\": \"ZLevel\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ZLevel_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalW\", \"name\": \"FlushNow\", \"label\": \"\", \"pv\": \"FlushNow\", \"widget\": {\"type\": \"CheckBox\"}}, {\"type\": \"SignalRW\", \"name\": \"JPEGQuality\", \"label\": \"\", \"pv\": \"JPEGQuality\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"JPEGQuality_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ChunkSizeAuto\", \"label\": \"\", \"pv\": \"ChunkSizeAuto\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ChunkSizeAuto_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"StorePerform\", \"label\": \"\", \"pv\": \"StorePerform\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"StorePerform_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"NumColChunks\", \"label\": \"\", \"pv\": \"NumColChunks\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumColChunks_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"StoreAttr\", \"label\": \"\", \"pv\": \"StoreAttr\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"StoreAttr_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"BloscShuffle\", \"label\": \"\", \"pv\": \"BloscShuffle\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"BloscShuffle_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NumRowChunks\", \"label\": \"\", \"pv\": \"NumRowChunks\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumRowChunks_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"RunTime\", \"label\": \"\", \"pv\": \"RunTime\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"BloscCompressor\", \"label\": \"\", \"pv\": \"BloscCompressor\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"BloscCompressor_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ChunkSize2\", \"label\": \"\", \"pv\": \"ChunkSize2\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ChunkSize2_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"IOSpeed\", \"label\": \"\", \"pv\": \"IOSpeed\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"BloscLevel\", \"label\": \"\", \"pv\": \"BloscLevel\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"BloscLevel_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NumFramesChunks\", \"label\": \"\", \"pv\": \"NumFramesChunks\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumFramesChunks_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"BoundaryAlign\", \"label\": \"\", \"pv\": \"BoundaryAlign\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"BoundaryAlign_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"XMLErrorMsg\", \"label\": \"\", \"pv\": \"XMLErrorMsg_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"XMLValid\", \"label\": \"\", \"pv\": \"XMLValid_RBV\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"BoundaryThreshold\", \"label\": \"\", \"pv\": \"BoundaryThreshold\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"BoundaryThreshold_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NumFramesFlush\", \"label\": \"\", \"pv\": \"NumFramesFlush\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumFramesFlush_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"XMLFileName\", \"label\": \"\", \"pv\": \"XMLFileName\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"XMLFileName_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FillValue\", \"label\": \"\", \"pv\": \"FillValue\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FillValue_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFileHDF5ChunkingFull\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"ChunkSize3\", \"label\": \"\", \"pv\": \"ChunkSize3\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ChunkSize3_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ChunkSize4\", \"label\": \"\", \"pv\": \"ChunkSize4\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ChunkSize4_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ChunkSize5\", \"label\": \"\", \"pv\": \"ChunkSize5\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ChunkSize5_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ChunkSize6\", \"label\": \"\", \"pv\": \"ChunkSize6\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ChunkSize6_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ChunkSize7\", \"label\": \"\", \"pv\": \"ChunkSize7\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ChunkSize7_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ChunkSize8\", \"label\": \"\", \"pv\": \"ChunkSize8\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ChunkSize8_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ChunkSize9\", \"label\": \"\", \"pv\": \"ChunkSize9\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ChunkSize9_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFileHDF5ExtraDims\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"NumExtraDims\", \"label\": \"\", \"pv\": \"NumExtraDims\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumExtraDims_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"DimAttDatasets\", \"label\": \"\", \"pv\": \"DimAttDatasets\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"DimAttDatasets_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSizeN\", \"label\": \"\", \"pv\": \"ExtraDimSizeN\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSizeN_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimNameN\", \"label\": \"\", \"pv\": \"ExtraDimNameN_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSizeX\", \"label\": \"\", \"pv\": \"ExtraDimSizeX\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSizeX_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunkX\", \"label\": \"\", \"pv\": \"ExtraDimChunkX\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunkX_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimNameX\", \"label\": \"\", \"pv\": \"ExtraDimNameX_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSizeY\", \"label\": \"\", \"pv\": \"ExtraDimSizeY\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSizeY_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunkY\", \"label\": \"\", \"pv\": \"ExtraDimChunkY\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunkY_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimNameY\", \"label\": \"\", \"pv\": \"ExtraDimNameY_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSize3\", \"label\": \"\", \"pv\": \"ExtraDimSize3\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSize3_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunk3\", \"label\": \"\", \"pv\": \"ExtraDimChunk3\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunk3_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimName3\", \"label\": \"\", \"pv\": \"ExtraDimName3_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSize4\", \"label\": \"\", \"pv\": \"ExtraDimSize4\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSize4_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunk4\", \"label\": \"\", \"pv\": \"ExtraDimChunk4\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunk4_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimName4\", \"label\": \"\", \"pv\": \"ExtraDimName4_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSize5\", \"label\": \"\", \"pv\": \"ExtraDimSize5\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSize5_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunk5\", \"label\": \"\", \"pv\": \"ExtraDimChunk5\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunk5_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimName5\", \"label\": \"\", \"pv\": \"ExtraDimName5_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSize6\", \"label\": \"\", \"pv\": \"ExtraDimSize6\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSize6_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunk6\", \"label\": \"\", \"pv\": \"ExtraDimChunk6\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunk6_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimName6\", \"label\": \"\", \"pv\": \"ExtraDimName6_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSize7\", \"label\": \"\", \"pv\": \"ExtraDimSize7\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSize7_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunk7\", \"label\": \"\", \"pv\": \"ExtraDimChunk7\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunk7_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimName7\", \"label\": \"\", \"pv\": \"ExtraDimName7_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSize8\", \"label\": \"\", \"pv\": \"ExtraDimSize8\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSize8_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunk8\", \"label\": \"\", \"pv\": \"ExtraDimChunk8\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunk8_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimName8\", \"label\": \"\", \"pv\": \"ExtraDimName8_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimSize9\", \"label\": \"\", \"pv\": \"ExtraDimSize9\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimSize9_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunk9\", \"label\": \"\", \"pv\": \"ExtraDimChunk9\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunk9_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExtraDimName9\", \"label\": \"\", \"pv\": \"ExtraDimName9_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFileHDF5Positions\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"PositionMode\", \"label\": \"\", \"pv\": \"PositionMode\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"PositionMode_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDimN\", \"label\": \"\", \"pv\": \"PosIndexDimN\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDimN_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDimN\", \"label\": \"\", \"pv\": \"PosNameDimN\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDimN_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDimX\", \"label\": \"\", \"pv\": \"PosIndexDimX\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDimX_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDimX\", \"label\": \"\", \"pv\": \"PosNameDimX\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDimX_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDimY\", \"label\": \"\", \"pv\": \"PosIndexDimY\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDimY_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDimY\", \"label\": \"\", \"pv\": \"PosNameDimY\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDimY_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDim3\", \"label\": \"\", \"pv\": \"PosIndexDim3\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDim3_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDim3\", \"label\": \"\", \"pv\": \"PosNameDim3\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDim3_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDim4\", \"label\": \"\", \"pv\": \"PosIndexDim4\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDim4_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDim4\", \"label\": \"\", \"pv\": \"PosNameDim4\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDim4_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDim5\", \"label\": \"\", \"pv\": \"PosIndexDim5\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDim5_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDim5\", \"label\": \"\", \"pv\": \"PosNameDim5\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDim5_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDim6\", \"label\": \"\", \"pv\": \"PosIndexDim6\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDim6_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDim6\", \"label\": \"\", \"pv\": \"PosNameDim6\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDim6_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDim7\", \"label\": \"\", \"pv\": \"PosIndexDim7\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDim7_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDim7\", \"label\": \"\", \"pv\": \"PosNameDim7\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDim7_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDim8\", \"label\": \"\", \"pv\": \"PosIndexDim8\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDim8_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDim8\", \"label\": \"\", \"pv\": \"PosNameDim8\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDim8_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosIndexDim9\", \"label\": \"\", \"pv\": \"PosIndexDim9\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosIndexDim9_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"PosNameDim9\", \"label\": \"\", \"pv\": \"PosNameDim9\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"PosNameDim9_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFileHDF5Misc\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"NDAttributeChunk\", \"label\": \"\", \"pv\": \"NDAttributeChunk\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NDAttributeChunk_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ExtraDimChunkN\", \"label\": \"\", \"pv\": \"ExtraDimChunkN\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ExtraDimChunkN_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDPluginBase\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"PluginType\", \"label\": \"\", \"pv\": \"PluginType_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NDArrayPort\", \"label\": \"\", \"pv\": \"NDArrayPort\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NDArrayPort_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NDArrayAddress\", \"label\": \"\", \"pv\": \"NDArrayAddress\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NDArrayAddress_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"EnableCallbacks\", \"label\": \"\", \"pv\": \"EnableCallbacks\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"EnableCallbacks_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"MinCallbackTime\", \"label\": \"\", \"pv\": \"MinCallbackTime\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"MinCallbackTime_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"QueueFree\", \"label\": \"\", \"pv\": \"QueueFree\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"QueueSize\", \"label\": \"\", \"pv\": \"QueueSize\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"QueueSize_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ExecutionTime\", \"label\": \"\", \"pv\": \"ExecutionTime_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"DroppedArrays\", \"label\": \"\", \"pv\": \"DroppedArrays\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"DroppedArrays_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalW\", \"name\": \"ProcessPlugin\", \"label\": \"\", \"pv\": \"ProcessPlugin\", \"widget\": {\"type\": \"CheckBox\"}}]}, {\"type\": \"Group\", \"name\": \"NDPluginBaseFull\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"DroppedOutputArrays\", \"label\": \"\", \"pv\": \"DroppedOutputArrays\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"DroppedOutputArrays_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"MaxByteRate\", \"label\": \"\", \"pv\": \"MaxByteRate\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"MaxByteRate_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"BlockingCallbacks\", \"label\": \"\", \"pv\": \"BlockingCallbacks\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"BlockingCallbacks_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"NumThreads\", \"label\": \"\", \"pv\": \"NumThreads\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumThreads_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"MaxThreads\", \"label\": \"\", \"pv\": \"MaxThreads_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"SortMode\", \"label\": \"\", \"pv\": \"SortMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"SortMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"SortTime\", \"label\": \"\", \"pv\": \"SortTime\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"SortTime_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"SortFree\", \"label\": \"\", \"pv\": \"SortFree\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"SortSize\", \"label\": \"\", \"pv\": \"SortSize\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"SortSize_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"DisorderedArrays\", \"label\": \"\", \"pv\": \"DisorderedArrays\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"DisorderedArrays_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADSetup\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"PortName\", \"label\": \"\", \"pv\": \"PortName_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"Manufacturer\", \"label\": \"\", \"pv\": \"Manufacturer_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"Model\", \"label\": \"\", \"pv\": \"Model_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"SerialNumber\", \"label\": \"\", \"pv\": \"SerialNumber_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"FirmwareVersion\", \"label\": \"\", \"pv\": \"FirmwareVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"SDKVersion\", \"label\": \"\", \"pv\": \"SDKVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"DriverVersion\", \"label\": \"\", \"pv\": \"DriverVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ADCoreVersion\", \"label\": \"\", \"pv\": \"ADCoreVersion_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADReadout\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"ArraySizeX\", \"label\": \"\", \"pv\": \"ArraySizeX_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ArraySizeY\", \"label\": \"\", \"pv\": \"ArraySizeY_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ArraySize\", \"label\": \"\", \"pv\": \"ArraySize_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"DataType\", \"label\": \"\", \"pv\": \"DataType\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"DataType_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ColorMode\", \"label\": \"\", \"pv\": \"ColorMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"ColorMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"ADCollect\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"NumQueuedArrays\", \"label\": \"\", \"pv\": \"NumQueuedArrays\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalW\", \"name\": \"WaitForPlugins\", \"label\": \"\", \"pv\": \"WaitForPlugins\", \"widget\": {\"type\": \"CheckBox\"}}, {\"type\": \"SignalRW\", \"name\": \"Acquire\", \"label\": \"\", \"pv\": \"Acquire\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"\", \"read_widget\": null}, {\"type\": \"SignalRW\", \"name\": \"ArrayCounter\", \"label\": \"\", \"pv\": \"ArrayCounter\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"ArrayCounter_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"ArrayCallbacks\", \"label\": \"\", \"pv\": \"ArrayCallbacks\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ArrayCallbacks_RBV\", \"read_widget\": {\"type\": \"LED\"}}]}, {\"type\": \"Group\", \"name\": \"ADAttrFile\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalW\", \"name\": \"NDAttributesFile\", \"label\": \"\", \"pv\": \"NDAttributesFile\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}}, {\"type\": \"SignalW\", \"name\": \"NDAttributesMacros\", \"label\": \"\", \"pv\": \"NDAttributesMacros\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"NDAttributesStatus\", \"label\": \"\", \"pv\": \"NDAttributesStatus\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFile\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalRW\", \"name\": \"FileFormat\", \"label\": \"\", \"pv\": \"FileFormat\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"FileFormat_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDFileBase\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalR\", \"name\": \"FilePathExists\", \"label\": \"\", \"pv\": \"FilePathExists_RBV\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"FilePath\", \"label\": \"\", \"pv\": \"FilePath\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FilePath_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"CreateDirectory\", \"label\": \"\", \"pv\": \"CreateDirectory\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"CreateDirectory_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FileName\", \"label\": \"\", \"pv\": \"FileName\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FileName_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FileNumber\", \"label\": \"\", \"pv\": \"FileNumber\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"\", \"read_widget\": null}, {\"type\": \"SignalRW\", \"name\": \"TempSuffix\", \"label\": \"\", \"pv\": \"TempSuffix\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"TempSuffix_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"LazyOpen\", \"label\": \"\", \"pv\": \"LazyOpen\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"LazyOpen_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"AutoIncrement\", \"label\": \"\", \"pv\": \"AutoIncrement\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"AutoIncrement_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"FileTemplate\", \"label\": \"\", \"pv\": \"FileTemplate\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"FileTemplate_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"FullFileName\", \"label\": \"\", \"pv\": \"FullFileName_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"WriteFile\", \"label\": \"\", \"pv\": \"WriteFile\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"WriteFile_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"ReadFile\", \"label\": \"\", \"pv\": \"ReadFile\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"ReadFile_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"AutoSave\", \"label\": \"\", \"pv\": \"AutoSave\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"AutoSave_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"NumCaptured\", \"label\": \"\", \"pv\": \"NumCaptured_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"FileWriteMode\", \"label\": \"\", \"pv\": \"FileWriteMode\", \"widget\": {\"type\": \"ComboBox\"}, \"read_pv\": \"FileWriteMode_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NumCapture\", \"label\": \"\", \"pv\": \"NumCapture\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NumCapture_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"Capture\", \"label\": \"\", \"pv\": \"Capture\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"Capture_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalRW\", \"name\": \"DeleteDriverFile\", \"label\": \"\", \"pv\": \"DeleteDriverFile\", \"widget\": {\"type\": \"CheckBox\"}, \"read_pv\": \"DeleteDriverFile_RBV\", \"read_widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"WriteStatus\", \"label\": \"\", \"pv\": \"WriteStatus\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"WriteMessage\", \"label\": \"\", \"pv\": \"WriteMessage\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}, {\"type\": \"Group\", \"name\": \"NDArrayBaseMisc\", \"label\": \"\", \"layout\": {\"type\": \"Grid\", \"labelled\": true}, \"children\": [{\"type\": \"SignalW\", \"name\": \"EmptyFreeList\", \"label\": \"\", \"pv\": \"EmptyFreeList\", \"widget\": {\"type\": \"CheckBox\"}}, {\"type\": \"SignalR\", \"name\": \"AcquireBusyCB\", \"label\": \"\", \"pv\": \"AcquireBusyCB\", \"widget\": {\"type\": \"LED\"}}, {\"type\": \"SignalR\", \"name\": \"BayerPattern\", \"label\": \"\", \"pv\": \"BayerPattern_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"ArraySizeZ\", \"label\": \"\", \"pv\": \"ArraySizeZ_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"Codec\", \"label\": \"\", \"pv\": \"Codec_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"CompressedSize\", \"label\": \"\", \"pv\": \"CompressedSize_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"UniqueId\", \"label\": \"\", \"pv\": \"UniqueId_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"TimeStamp\", \"label\": \"\", \"pv\": \"TimeStamp_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"EpicsTSSec\", \"label\": \"\", \"pv\": \"EpicsTSSec_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"EpicsTSNsec\", \"label\": \"\", \"pv\": \"EpicsTSNsec_RBV\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolMaxMem\", \"label\": \"\", \"pv\": \"PoolMaxMem\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolUsedMem\", \"label\": \"\", \"pv\": \"PoolUsedMem\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolAllocBuffers\", \"label\": \"\", \"pv\": \"PoolAllocBuffers\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalR\", \"name\": \"PoolFreeBuffers\", \"label\": \"\", \"pv\": \"PoolFreeBuffers\", \"widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"NDimensions\", \"label\": \"\", \"pv\": \"NDimensions\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"NDimensions_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}, {\"type\": \"SignalRW\", \"name\": \"Dimensions\", \"label\": \"\", \"pv\": \"Dimensions\", \"widget\": {\"type\": \"TextWrite\", \"lines\": 1}, \"read_pv\": \"Dimensions_RBV\", \"read_widget\": {\"type\": \"TextRead\", \"lines\": 1}}]}]}";

class NDFileHDF5ParamSet : public virtual NDPluginFileParamSet {
public:
    NDFileHDF5ParamSet() {
        this->add(NDFileHDF5_compressionTypeString, asynParamInt32, &NDFileHDF5_compressionType);
        this->add(NDFileHDF5_SWMRSupportedString, asynParamInt32, &NDFileHDF5_SWMRSupported);
        this->add(NDFileHDF5_nbitsPrecisionString, asynParamInt32, &NDFileHDF5_nbitsPrecision);
        this->add(NDFileHDF5_nbitsOffsetString, asynParamInt32, &NDFileHDF5_nbitsOffset);
        this->add(NDFileHDF5_SWMRModeString, asynParamInt32, &NDFileHDF5_SWMRMode);
        this->add(NDFileHDF5_SWMRRunningString, asynParamInt32, &NDFileHDF5_SWMRRunning);
        this->add(NDFileHDF5_szipNumPixelsString, asynParamInt32, &NDFileHDF5_szipNumPixels);
        this->add(NDFileHDF5_SWMRCbCounterString, asynParamInt32, &NDFileHDF5_SWMRCbCounter);
        this->add(NDFileHDF5_zCompressLevelString, asynParamInt32, &NDFileHDF5_zCompressLevel);
        this->add(NDFileHDF5_SWMRFlushNowString, asynParamInt32, &NDFileHDF5_SWMRFlushNow);
        this->add(NDFileHDF5_jpegQualityString, asynParamInt32, &NDFileHDF5_jpegQuality);
        this->add(NDFileHDF5_chunkSizeAutoString, asynParamInt32, &NDFileHDF5_chunkSizeAuto);
        this->add(NDFileHDF5_storePerformanceString, asynParamInt32, &NDFileHDF5_storePerformance);
        this->add(NDFileHDF5_nColChunksString, asynParamInt32, &NDFileHDF5_nColChunks);
        this->add(NDFileHDF5_storeAttributesString, asynParamInt32, &NDFileHDF5_storeAttributes);
        this->add(NDFileHDF5_bloscShuffleTypeString, asynParamInt32, &NDFileHDF5_bloscShuffleType);
        this->add(NDFileHDF5_nRowChunksString, asynParamInt32, &NDFileHDF5_nRowChunks);
        this->add(NDFileHDF5_totalRuntimeString, asynParamFloat64, &NDFileHDF5_totalRuntime);
        this->add(NDFileHDF5_bloscCompressorString, asynParamInt32, &NDFileHDF5_bloscCompressor);
        this->add(NDFileHDF5_chunkSize2String, asynParamInt32, &NDFileHDF5_chunkSize2);
        this->add(NDFileHDF5_totalIoSpeedString, asynParamFloat64, &NDFileHDF5_totalIoSpeed);
        this->add(NDFileHDF5_bloscCompressLevelString, asynParamInt32, &NDFileHDF5_bloscCompressLevel);
        this->add(NDFileHDF5_nFramesChunksString, asynParamInt32, &NDFileHDF5_nFramesChunks);
        this->add(NDFileHDF5_chunkBoundaryAlignString, asynParamInt32, &NDFileHDF5_chunkBoundaryAlign);
        this->add(NDFileHDF5_layoutErrorMsgString, asynParamOctet, &NDFileHDF5_layoutErrorMsg);
        this->add(NDFileHDF5_layoutValidString, asynParamInt32, &NDFileHDF5_layoutValid);
        this->add(NDFileHDF5_chunkBoundaryThresholdString, asynParamInt32, &NDFileHDF5_chunkBoundaryThreshold);
        this->add(NDFileHDF5_flushNthFrameString, asynParamInt32, &NDFileHDF5_flushNthFrame);
        this->add(NDFileHDF5_layoutFilenameString, asynParamOctet, &NDFileHDF5_layoutFilename);
        this->add(NDFileHDF5_fillValueString, asynParamFloat64, &NDFileHDF5_fillValue);
        this->add(NDFileHDF5_chunkSize3String, asynParamInt32, &NDFileHDF5_chunkSize3);
        this->add(NDFileHDF5_chunkSize4String, asynParamInt32, &NDFileHDF5_chunkSize4);
        this->add(NDFileHDF5_chunkSize5String, asynParamInt32, &NDFileHDF5_chunkSize5);
        this->add(NDFileHDF5_chunkSize6String, asynParamInt32, &NDFileHDF5_chunkSize6);
        this->add(NDFileHDF5_chunkSize7String, asynParamInt32, &NDFileHDF5_chunkSize7);
        this->add(NDFileHDF5_chunkSize8String, asynParamInt32, &NDFileHDF5_chunkSize8);
        this->add(NDFileHDF5_chunkSize9String, asynParamInt32, &NDFileHDF5_chunkSize9);
        this->add(NDFileHDF5_nExtraDimsString, asynParamInt32, &NDFileHDF5_nExtraDims);
        this->add(NDFileHDF5_dimAttDatasetsString, asynParamInt32, &NDFileHDF5_dimAttDatasets);
        this->add(NDFileHDF5_ExtraDimSizeNString, asynParamInt32, &NDFileHDF5_ExtraDimSizeN);
        this->add(NDFileHDF5_extraDimNameNString, asynParamOctet, &NDFileHDF5_extraDimNameN);
        this->add(NDFileHDF5_ExtraDimSizeXString, asynParamInt32, &NDFileHDF5_ExtraDimSizeX);
        this->add(NDFileHDF5_extraDimChunkXString, asynParamInt32, &NDFileHDF5_extraDimChunkX);
        this->add(NDFileHDF5_extraDimNameXString, asynParamOctet, &NDFileHDF5_extraDimNameX);
        this->add(NDFileHDF5_ExtraDimSizeYString, asynParamInt32, &NDFileHDF5_ExtraDimSizeY);
        this->add(NDFileHDF5_extraDimChunkYString, asynParamInt32, &NDFileHDF5_extraDimChunkY);
        this->add(NDFileHDF5_extraDimNameYString, asynParamOctet, &NDFileHDF5_extraDimNameY);
        this->add(NDFileHDF5_ExtraDimSize3String, asynParamInt32, &NDFileHDF5_ExtraDimSize3);
        this->add(NDFileHDF5_extraDimChunk3String, asynParamInt32, &NDFileHDF5_extraDimChunk3);
        this->add(NDFileHDF5_extraDimName3String, asynParamOctet, &NDFileHDF5_extraDimName3);
        this->add(NDFileHDF5_ExtraDimSize4String, asynParamInt32, &NDFileHDF5_ExtraDimSize4);
        this->add(NDFileHDF5_extraDimChunk4String, asynParamInt32, &NDFileHDF5_extraDimChunk4);
        this->add(NDFileHDF5_extraDimName4String, asynParamOctet, &NDFileHDF5_extraDimName4);
        this->add(NDFileHDF5_ExtraDimSize5String, asynParamInt32, &NDFileHDF5_ExtraDimSize5);
        this->add(NDFileHDF5_extraDimChunk5String, asynParamInt32, &NDFileHDF5_extraDimChunk5);
        this->add(NDFileHDF5_extraDimName5String, asynParamOctet, &NDFileHDF5_extraDimName5);
        this->add(NDFileHDF5_ExtraDimSize6String, asynParamInt32, &NDFileHDF5_ExtraDimSize6);
        this->add(NDFileHDF5_extraDimChunk6String, asynParamInt32, &NDFileHDF5_extraDimChunk6);
        this->add(NDFileHDF5_extraDimName6String, asynParamOctet, &NDFileHDF5_extraDimName6);
        this->add(NDFileHDF5_ExtraDimSize7String, asynParamInt32, &NDFileHDF5_ExtraDimSize7);
        this->add(NDFileHDF5_extraDimChunk7String, asynParamInt32, &NDFileHDF5_extraDimChunk7);
        this->add(NDFileHDF5_extraDimName7String, asynParamOctet, &NDFileHDF5_extraDimName7);
        this->add(NDFileHDF5_ExtraDimSize8String, asynParamInt32, &NDFileHDF5_ExtraDimSize8);
        this->add(NDFileHDF5_extraDimChunk8String, asynParamInt32, &NDFileHDF5_extraDimChunk8);
        this->add(NDFileHDF5_extraDimName8String, asynParamOctet, &NDFileHDF5_extraDimName8);
        this->add(NDFileHDF5_ExtraDimSize9String, asynParamInt32, &NDFileHDF5_ExtraDimSize9);
        this->add(NDFileHDF5_extraDimChunk9String, asynParamInt32, &NDFileHDF5_extraDimChunk9);
        this->add(NDFileHDF5_extraDimName9String, asynParamOctet, &NDFileHDF5_extraDimName9);
        this->add(NDFileHDF5_posRunningString, asynParamInt32, &NDFileHDF5_posRunning);
        this->add(NDFileHDF5_posIndexDimNString, asynParamOctet, &NDFileHDF5_posIndexDimN);
        this->add(NDFileHDF5_posNameDimNString, asynParamOctet, &NDFileHDF5_posNameDimN);
        this->add(NDFileHDF5_posIndexDimXString, asynParamOctet, &NDFileHDF5_posIndexDimX);
        this->add(NDFileHDF5_posNameDimXString, asynParamOctet, &NDFileHDF5_posNameDimX);
        this->add(NDFileHDF5_posIndexDimYString, asynParamOctet, &NDFileHDF5_posIndexDimY);
        this->add(NDFileHDF5_posNameDimYString, asynParamOctet, &NDFileHDF5_posNameDimY);
        this->add(NDFileHDF5_posIndexDim3String, asynParamOctet, &NDFileHDF5_posIndexDim3);
        this->add(NDFileHDF5_posNameDim3String, asynParamOctet, &NDFileHDF5_posNameDim3);
        this->add(NDFileHDF5_posIndexDim4String, asynParamOctet, &NDFileHDF5_posIndexDim4);
        this->add(NDFileHDF5_posNameDim4String, asynParamOctet, &NDFileHDF5_posNameDim4);
        this->add(NDFileHDF5_posIndexDim5String, asynParamOctet, &NDFileHDF5_posIndexDim5);
        this->add(NDFileHDF5_posNameDim5String, asynParamOctet, &NDFileHDF5_posNameDim5);
        this->add(NDFileHDF5_posIndexDim6String, asynParamOctet, &NDFileHDF5_posIndexDim6);
        this->add(NDFileHDF5_posNameDim6String, asynParamOctet, &NDFileHDF5_posNameDim6);
        this->add(NDFileHDF5_posIndexDim7String, asynParamOctet, &NDFileHDF5_posIndexDim7);
        this->add(NDFileHDF5_posNameDim7String, asynParamOctet, &NDFileHDF5_posNameDim7);
        this->add(NDFileHDF5_posIndexDim8String, asynParamOctet, &NDFileHDF5_posIndexDim8);
        this->add(NDFileHDF5_posNameDim8String, asynParamOctet, &NDFileHDF5_posNameDim8);
        this->add(NDFileHDF5_posIndexDim9String, asynParamOctet, &NDFileHDF5_posIndexDim9);
        this->add(NDFileHDF5_posNameDim9String, asynParamOctet, &NDFileHDF5_posNameDim9);
        this->add(NDFileHDF5_NDAttributeChunkString, asynParamInt32, &NDFileHDF5_NDAttributeChunk);
        this->add(NDFileHDF5_extraDimChunkNString, asynParamInt32, &NDFileHDF5_extraDimChunkN);

        this->paramTree = NDFileHDF5ParamTree;
    }

    int NDFileHDF5_compressionType;
    #define FIRST_NDFILEHDF5PARAMSET_PARAM NDFileHDF5_compressionType
    int NDFileHDF5_SWMRSupported;
    int NDFileHDF5_nbitsPrecision;
    int NDFileHDF5_nbitsOffset;
    int NDFileHDF5_SWMRMode;
    int NDFileHDF5_SWMRRunning;
    int NDFileHDF5_szipNumPixels;
    int NDFileHDF5_SWMRCbCounter;
    int NDFileHDF5_zCompressLevel;
    int NDFileHDF5_SWMRFlushNow;
    int NDFileHDF5_jpegQuality;
    int NDFileHDF5_chunkSizeAuto;
    int NDFileHDF5_storePerformance;
    int NDFileHDF5_nColChunks;
    int NDFileHDF5_storeAttributes;
    int NDFileHDF5_bloscShuffleType;
    int NDFileHDF5_nRowChunks;
    int NDFileHDF5_totalRuntime;
    int NDFileHDF5_bloscCompressor;
    int NDFileHDF5_chunkSize2;
    int NDFileHDF5_totalIoSpeed;
    int NDFileHDF5_bloscCompressLevel;
    int NDFileHDF5_nFramesChunks;
    int NDFileHDF5_chunkBoundaryAlign;
    int NDFileHDF5_layoutErrorMsg;
    int NDFileHDF5_layoutValid;
    int NDFileHDF5_chunkBoundaryThreshold;
    int NDFileHDF5_flushNthFrame;
    int NDFileHDF5_layoutFilename;
    int NDFileHDF5_fillValue;
    int NDFileHDF5_chunkSize3;
    int NDFileHDF5_chunkSize4;
    int NDFileHDF5_chunkSize5;
    int NDFileHDF5_chunkSize6;
    int NDFileHDF5_chunkSize7;
    int NDFileHDF5_chunkSize8;
    int NDFileHDF5_chunkSize9;
    int NDFileHDF5_nExtraDims;
    int NDFileHDF5_dimAttDatasets;
    int NDFileHDF5_ExtraDimSizeN;
    int NDFileHDF5_extraDimNameN;
    int NDFileHDF5_ExtraDimSizeX;
    int NDFileHDF5_extraDimChunkX;
    int NDFileHDF5_extraDimNameX;
    int NDFileHDF5_ExtraDimSizeY;
    int NDFileHDF5_extraDimChunkY;
    int NDFileHDF5_extraDimNameY;
    int NDFileHDF5_ExtraDimSize3;
    int NDFileHDF5_extraDimChunk3;
    int NDFileHDF5_extraDimName3;
    int NDFileHDF5_ExtraDimSize4;
    int NDFileHDF5_extraDimChunk4;
    int NDFileHDF5_extraDimName4;
    int NDFileHDF5_ExtraDimSize5;
    int NDFileHDF5_extraDimChunk5;
    int NDFileHDF5_extraDimName5;
    int NDFileHDF5_ExtraDimSize6;
    int NDFileHDF5_extraDimChunk6;
    int NDFileHDF5_extraDimName6;
    int NDFileHDF5_ExtraDimSize7;
    int NDFileHDF5_extraDimChunk7;
    int NDFileHDF5_extraDimName7;
    int NDFileHDF5_ExtraDimSize8;
    int NDFileHDF5_extraDimChunk8;
    int NDFileHDF5_extraDimName8;
    int NDFileHDF5_ExtraDimSize9;
    int NDFileHDF5_extraDimChunk9;
    int NDFileHDF5_extraDimName9;
    int NDFileHDF5_posRunning;
    int NDFileHDF5_posIndexDimN;
    int NDFileHDF5_posNameDimN;
    int NDFileHDF5_posIndexDimX;
    int NDFileHDF5_posNameDimX;
    int NDFileHDF5_posIndexDimY;
    int NDFileHDF5_posNameDimY;
    int NDFileHDF5_posIndexDim3;
    int NDFileHDF5_posNameDim3;
    int NDFileHDF5_posIndexDim4;
    int NDFileHDF5_posNameDim4;
    int NDFileHDF5_posIndexDim5;
    int NDFileHDF5_posNameDim5;
    int NDFileHDF5_posIndexDim6;
    int NDFileHDF5_posNameDim6;
    int NDFileHDF5_posIndexDim7;
    int NDFileHDF5_posNameDim7;
    int NDFileHDF5_posIndexDim8;
    int NDFileHDF5_posNameDim8;
    int NDFileHDF5_posIndexDim9;
    int NDFileHDF5_posNameDim9;
    int NDFileHDF5_NDAttributeChunk;
    int NDFileHDF5_extraDimChunkN;
};

#endif // NDFileHDF5ParamSet_H
