TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for the source files that define template functions.
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
CODE_CXXFLAGS=
endif

# Persuade travis (ubuntu 12.04) to use HDF5 API V2 (1.8 rather than default 1.6)
USR_CXXFLAGS_Linux += -DH5_NO_DEPRECATED_SYMBOLS -DH5Gopen_vers=2

# This flag is used to indicate that the TIFF library was built statically
USR_CXXFLAGS_WIN32 += -DLIBTIFF_STATIC

DBD += NDPluginSupport.dbd
DBD += NDFileHDF5.dbd
DBD += NDFileJPEG.dbd
DBD += NDFileMagick.dbd
DBD += NDFileNetCDF.dbd
DBD += NDFileNexus.dbd
DBD += NDFileNull.dbd
DBD += NDFileTIFF.dbd
DBD += NDPluginAttribute.dbd
DBD += NDPluginCircularBuff.dbd
DBD += NDPluginColorConvert.dbd
DBD += NDPluginFFT.dbd
DBD += NDPluginOverlay.dbd
DBD += NDPluginProcess.dbd
DBD += NDPluginROI.dbd
DBD += NDPluginROIStat.dbd
DBD += NDPluginStats.dbd
DBD += NDPluginStdArrays.dbd
DBD += NDPluginTimeSeries.dbd
DBD += NDPluginTransform.dbd
DBD += NDPosPlugin.dbd

INC += NDArrayRing.h
INC += NDFileHDF5.h
INC += NDFileHDF5Dataset.h
INC += NDFileHDF5AttributeDataset.h
INC += NDFileHDF5Layout.h
INC += NDFileHDF5LayoutXML.h
INC += NDFileHDF5VersionCheck.h
INC += NDFileJPEG.h
INC += NDFileMagick.h
INC += NDFileNetCDF.h
INC += NDFileNexus.h
INC += NDFileNull.h
INC += NDFileTIFF.h
INC += NDPluginAttribute.h
INC += NDPluginCircularBuff.h
INC += NDPluginColorConvert.h
INC += NDPluginDriver.h
INC += NDPluginFFT.h
INC += NDPluginFile.h
INC += NDPluginOverlay.h
INC += NDPluginOverlayTextFont.h
INC += NDPluginProcess.h
INC += NDPluginROI.h
INC += NDPluginROIStat.h
INC += NDPluginStats.h
INC += NDPluginStdArrays.h
INC += NDPluginTimeSeries.h
INC += NDPluginTransform.h
INC += NDPosPlugin.h
INC += NDPosPluginFileReader.h

LIBRARY_IOC += NDPlugin
NDPlugin_SRCS += fft.c
NDPlugin_SRCS += NDArrayRing.cpp
NDPlugin_SRCS_DEFAULT += NDFileTIFF.cpp NDFileJPEG.cpp NDFileNexus.cpp NDFileHDF5.cpp NDFileHDF5Dataset.cpp 
NDPlugin_SRCS_DEFAULT += NDFileHDF5AttributeDataset.cpp NDFileHDF5LayoutXML.cpp NDFileHDF5Layout.cpp NDFileNull.cpp
NDPlugin_SRCS_DEFAULT += NDPosPlugin.cpp NDPosPluginFileReader.cpp
NDPlugin_SRCS_vxWorks += NDFileDummy.cpp
NDPlugin_SRCS += NDFileNetCDF.cpp
NDPlugin_SRCS += NDPluginAttribute.cpp
NDPlugin_SRCS += NDPluginCircularBuff.cpp
NDPlugin_SRCS += NDPluginColorConvert.cpp
NDPlugin_SRCS += NDPluginDriver.cpp
NDPlugin_SRCS += NDPluginFFT.cpp
NDPlugin_SRCS += NDPluginFile.cpp
NDPlugin_SRCS += NDPluginOverlay.cpp
NDPlugin_SRCS += NDPluginOverlayTextFont.cpp
NDPlugin_SRCS += NDPluginProcess.cpp
NDPlugin_SRCS += NDPluginROI.cpp
NDPlugin_SRCS += NDPluginROIStat.cpp
NDPlugin_SRCS += NDPluginStats.cpp
NDPlugin_SRCS += NDPluginStdArrays.cpp
NDPlugin_SRCS += NDPluginTimeSeries.cpp
NDPlugin_SRCS += NDPluginTransform.cpp

NDPlugin_SYS_LIBS_WIN32 += Ws2_32
NDPlugin_SYS_LIBS_WIN32 += User32

ifeq ($(USE_GRAPHICSMAGICK), YES)
  USR_INCLUDES += $(GRAPHICS_MAGICK_INCLUDE)
  NDPlugin_SRCS += NDFileMagick.cpp
else 
  NDPlugin_SRCS += NDFileMagickStub.cpp
endif

ifeq ($(EPICS_V4), YES)
  DBD += NDPluginPva.dbd
  INC += NDPluginPva.h
  NDPlugin_SRCS += NDPluginPva.cpp
endif

USR_INCLUDES += $(HDF5_INCLUDE)
USR_INCLUDES += $(SZ_INCLUDE)
USR_INCLUDES += $(XML2_INCLUDE)

include $(TOP)/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

