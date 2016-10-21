TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY_IOC += NDPlugin

# Persuade travis (ubuntu 12.04) to use HDF5 API V2 (1.8 rather than default 1.6)
USR_CXXFLAGS_Linux += -DH5_NO_DEPRECATED_SYMBOLS -DH5Gopen_vers=2

DBD      += NDPluginSupport.dbd
INC      += NDPluginDriver.h
LIB_SRCS += NDPluginDriver.cpp

DBD      += NDPluginCircularBuff.dbd
INC      += NDArrayRing.h
INC      += NDPluginCircularBuff.h
LIB_SRCS += NDPluginCircularBuff.cpp
LIB_SRCS += NDArrayRing.cpp

DBD      += NDPluginAttribute.dbd
INC      += NDPluginAttribute.h
LIB_SRCS += NDPluginAttribute.cpp

DBD      += NDPluginColorConvert.dbd
INC      += NDPluginColorConvert.h
LIB_SRCS += NDPluginColorConvert.cpp

DBD      += NDPluginFFT.dbd
INC      += NDPluginFFT.h
LIB_SRCS += NDPluginFFT.cpp
LIB_SRCS += fft.c

DBD      += NDPluginOverlay.dbd
INC      += NDPluginOverlay.h
INC      += NDPluginOverlayTextFont.h
LIB_SRCS += NDPluginOverlay.cpp
LIB_SRCS += NDPluginOverlayTextFont.cpp

DBD      += NDPluginProcess.dbd
INC      += NDPluginProcess.h
LIB_SRCS += NDPluginProcess.cpp

DBD      += NDPluginROI.dbd
INC      += NDPluginROI.h
LIB_SRCS += NDPluginROI.cpp

DBD      += NDPluginROIStat.dbd
INC      += NDPluginROIStat.h
LIB_SRCS += NDPluginROIStat.cpp

DBD      += NDPluginStats.dbd
INC      += NDPluginStats.h
LIB_SRCS += NDPluginStats.cpp

DBD      += NDPluginStdArrays.dbd
INC      += NDPluginStdArrays.h
LIB_SRCS += NDPluginStdArrays.cpp

DBD      += NDPluginTimeSeries.dbd
INC      += NDPluginTimeSeries.h
LIB_SRCS += NDPluginTimeSeries.cpp

DBD      += NDPluginTransform.dbd
INC      += NDPluginTransform.h
LIB_SRCS += NDPluginTransform.cpp

ifeq ($(WITH_XML2),YES)
DBD      += NDPosPlugin.dbd
INC      += NDPosPlugin.h
INC      += NDPosPluginFileReader.h
LIB_SRCS += NDPosPlugin.cpp 
LIB_SRCS += NDPosPluginFileReader.cpp
endif

INC      += NDPluginFile.h
LIB_SRCS += NDPluginFile.cpp

DBD      += NDFileNull.dbd
INC      += NDFileNull.h
LIB_SRCS += NDFileNull.cpp

ifeq ($(WITH_GRAPHICSMAGICK),YES)
  DBD      += NDFileMagick.dbd
  INC      += NDFileMagick.h
  LIB_SRCS += NDFileMagick.cpp
  ifdef GRAPHICS_MAGICK_INCLUDE
    USR_INCLUDES += $(GRAPHICS_MAGICK_INCLUDE)
  endif
endif

ifeq ($(WITH_HDF5),YES)
  DBD      += NDFileHDF5.dbd
  INC      += NDFileHDF5.h
  INC      += NDFileHDF5Dataset.h
  INC      += NDFileHDF5AttributeDataset.h
  INC      += NDFileHDF5Layout.h
  INC      += NDFileHDF5LayoutXML.h
  INC      += NDFileHDF5VersionCheck.h
  LIB_SRCS += NDFileHDF5.cpp 
  LIB_SRCS += NDFileHDF5Dataset.cpp 
  LIB_SRCS += NDFileHDF5AttributeDataset.cpp 
  LIB_SRCS += NDFileHDF5LayoutXML.cpp 
  LIB_SRCS += NDFileHDF5Layout.cpp 
endif

ifeq ($(WITH_JPEG),YES)
  DBD      += NDFileJPEG.dbd
  INC      += NDFileJPEG.h
  LIB_SRCS += NDFileJPEG.cpp 
endif

ifeq ($(WITH_NETCDF),YES)
  DBD      += NDFileNetCDF.dbd
  INC      += NDFileNetCDF.h
  LIB_SRCS += NDFileNetCDF.cpp
endif

ifeq ($(WITH_NEXUS),YES)
  DBD      += NDFileNexus.dbd
  INC      += NDFileNexus.h
  LIB_SRCS += NDFileNexus.cpp 
endif

ifeq ($(WITH_TIFF),YES)
  DBD      += NDFileTIFF.dbd
  INC      += NDFileTIFF.h
  LIB_SRCS += NDFileTIFF.cpp 
  ifeq ($(SHARED_LIBRARIES),NO)
    # This flag is used to indicate that the TIFF library was built statically
    USR_CXXFLAGS_WIN32 += -DLIBTIFF_STATIC
  endif
endif

ifeq ($(WITH_EPICS_V4), YES)
  DBD += NDPluginPva.dbd
  INC += NDPluginPva.h
  LIB_SRCS += NDPluginPva.cpp
endif

ifdef HDF5_INCLUDE
  USR_INCLUDES += $(HDF5_INCLUDE)
endif
ifdef SZIP_INCLUDE
  USR_INCLUDES += $(SZIP_INCLUDE)
endif
ifdef XML2_INCLUDE
  USR_INCLUDES += $(XML2_INCLUDE)
endif

NDPlugin_SYS_LIBS_WIN32 += ws2_32
NDPlugin_SYS_LIBS_WIN32 += user32

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for the source files that define template functions.
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
  CODE_CXXFLAGS=
endif

include $(TOP)/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

