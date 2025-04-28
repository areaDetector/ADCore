TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY_IOC += NDPlugin
DBD         += NDPluginSupport.dbd

# Use HDF5 API V2
USR_CXXFLAGS_Linux += -DH5Gopen_vers=2

# json.hpp requires C++11
NDPluginBadPixel_CXXFLAGS_Linux += -std=c++11
NDPluginBadPixel_CXXFLAGS_Darwin += -std=c++11

ifneq ($(SHARED_LIBRARIES), YES)
   USR_CFLAGS_WIN32 += -DLIBXML_STATIC
   USR_CXXFLAGS_WIN32 += -DLIBXML_STATIC
endif

# This flag is what switches to declspec(dllexport) when building the library
USR_CPPFLAGS += -DBUILDING_NDPlugin_API

INC      += NDPluginAPI.h
INC      += NDPluginDriver.h
LIB_SRCS += NDPluginDriver.cpp
LIB_SRCS += throttler.cpp

NDPluginSupport_DBD += NDPluginAttribute.dbd
INC      += NDPluginAttribute.h
LIB_SRCS += NDPluginAttribute.cpp

ifeq ($(WITH_JSON), YES)
  NDPluginSupport_DBD += NDPluginBadPixel.dbd
  INC      += NDPluginBadPixel.h
  INC      += json.hpp
  LIB_SRCS += NDPluginBadPixel.cpp
endif

NDPluginSupport_DBD += NDPluginCircularBuff.dbd
INC      += NDArrayRing.h
INC      += NDPluginCircularBuff.h
LIB_SRCS += NDPluginCircularBuff.cpp
LIB_SRCS += NDArrayRing.cpp

NDPluginSupport_DBD += NDPluginColorConvert.dbd
INC      += NDPluginColorConvert.h
LIB_SRCS += NDPluginColorConvert.cpp

NDPluginSupport_DBD += NDPluginFFT.dbd
INC      += NDPluginFFT.h
LIB_SRCS += NDPluginFFT.cpp
LIB_SRCS += fft.c

NDPluginSupport_DBD += NDPluginGather.dbd
INC      += NDPluginGather.h
LIB_SRCS += NDPluginGather.cpp

NDPluginSupport_DBD += NDPluginOverlay.dbd
INC      += NDPluginOverlay.h
INC      += NDPluginOverlayTextFont.h
LIB_SRCS += NDPluginOverlay.cpp
LIB_SRCS += NDPluginOverlayTextFont.cpp

NDPluginSupport_DBD += NDPluginProcess.dbd
INC      += NDPluginProcess.h
LIB_SRCS += NDPluginProcess.cpp

NDPluginSupport_DBD += NDPluginROI.dbd
INC      += NDPluginROI.h
LIB_SRCS += NDPluginROI.cpp

NDPluginSupport_DBD += NDPluginROIStat.dbd
INC      += NDPluginROIStat.h
LIB_SRCS += NDPluginROIStat.cpp

NDPluginSupport_DBD += NDPluginScatter.dbd
INC      += NDPluginScatter.h
LIB_SRCS += NDPluginScatter.cpp

NDPluginSupport_DBD += NDPluginStats.dbd
INC      += NDPluginStats.h
LIB_SRCS += NDPluginStats.cpp

NDPluginSupport_DBD += NDPluginStdArrays.dbd
INC      += NDPluginStdArrays.h
LIB_SRCS += NDPluginStdArrays.cpp

NDPluginSupport_DBD += NDPluginTimeSeries.dbd
INC      += NDPluginTimeSeries.h
LIB_SRCS += NDPluginTimeSeries.cpp

NDPluginSupport_DBD += NDPluginTransform.dbd
INC      += NDPluginTransform.h
LIB_SRCS += NDPluginTransform.cpp

NDPluginSupport_DBD += NDPluginAttrPlot.dbd
INC      += NDPluginAttrPlot.h CircularBuffer.h
LIB_SRCS += NDPluginAttrPlot.cpp

NDPluginSupport_DBD += NDPluginCodec.dbd
INC      += NDPluginCodec.h
LIB_SRCS += NDPluginCodec.cpp

DBD      += NDPosPlugin.dbd
INC      += NDPosPlugin.h
INC      += NDPosPluginFileReader.h
LIB_SRCS += NDPosPlugin.cpp
LIB_SRCS += NDPosPluginFileReader.cpp

INC      += NDPluginFile.h
LIB_SRCS += NDPluginFile.cpp

DBD      += NDFileNull.dbd
INC      += NDFileNull.h
LIB_SRCS += NDFileNull.cpp

ifeq ($(WITH_GRAPHICSMAGICK),YES)
  ifeq ($(GRAPHICSMAGICK_PREFIX_SYMBOLS),YES)
    USR_CXXFLAGS += -DPREFIX_MAGICK_SYMBOLS
  endif
  DBD      += NDFileMagick.dbd
  INC      += NDFileMagick.h
  LIB_SRCS += NDFileMagick.cpp
  ifdef GRAPHICSMAGICK_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(GRAPHICSMAGICK_INCLUDE))
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
  ifdef HDF5_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(HDF5_INCLUDE))
  endif
  ifdef SZIP_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(SZIP_INCLUDE))
  endif
endif

ifeq ($(WITH_JPEG),YES)
  DBD      += NDFileJPEG.dbd
  INC      += NDFileJPEG.h
  LIB_SRCS += NDFileJPEG.cpp
  USR_CXXFLAGS += -DHAVE_JPEG
  ifdef JPEG_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(JPEG_INCLUDE))
  endif
endif

ifeq ($(WITH_NETCDF),YES)
  DBD      += NDFileNetCDF.dbd
  INC      += NDFileNetCDF.h
  LIB_SRCS += NDFileNetCDF.cpp
  ifdef NETCDF_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(NETCDF_INCLUDE))
  endif
endif

ifeq ($(WITH_NEXUS),YES)
  DBD      += NDFileNexus.dbd
  INC      += NDFileNexus.h
  LIB_SRCS += NDFileNexus.cpp
  ifdef NEXUS_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(NEXUS_INCLUDE))
  endif
endif

ifeq ($(WITH_TIFF),YES)
  DBD      += NDFileTIFF.dbd
  INC      += NDFileTIFF.h
  LIB_SRCS += NDFileTIFF.cpp
  ifeq ($(SHARED_LIBRARIES),NO)
    # This flag is used to indicate that the TIFF library was built statically
    USR_CXXFLAGS_WIN32 += -DLIBTIFF_STATIC
  endif
  ifdef TIFF_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(TIFF_INCLUDE))
  endif
endif

ifeq ($(WITH_PVA), YES)
  DBD += NDPluginPva.dbd
  INC += NDPluginPva.h
  LIB_SRCS += NDPluginPva.cpp
endif

ifeq ($(WITH_PVXS), YES)
  DBD += NDPluginPvxs.dbd
  INC += NDPluginPvxs.h
  LIB_SRCS += NDPluginPvxs.cpp
endif

ifeq ($(WITH_BLOSC), YES)
  USR_CXXFLAGS += -DHAVE_BLOSC
endif

ifeq ($(WITH_BITSHUFFLE), YES)
  USR_CXXFLAGS += -DHAVE_BITSHUFFLE
endif

ifdef BLOSC_INCLUDE
  USR_INCLUDES += $(addprefix -I, $(BLOSC_INCLUDE))
endif

ifdef BITSHUFFLE_INCLUDE
  USR_INCLUDES += $(addprefix -I, $(BITSHUFFLE_INCLUDE))
endif

ifdef HDF5_INCLUDE
  USR_INCLUDES += $(addprefix -I, $(HDF5_INCLUDE))
endif

ifdef SZIP_INCLUDE
  USR_INCLUDES += $(addprefix -I, $(HDF5_INCLUDE))
endif

ifdef XML2_INCLUDE
  USR_INCLUDES += $(addprefix -I, $(XML2_INCLUDE))
endif

NDPlugin_SYS_LIBS_WIN32 += ws2_32
NDPlugin_SYS_LIBS_WIN32 += user32

# This tests the problem with forward referencing class sortedListElement if it is
# forwarded referenced in NDPluginDriver.h and defined in NDPluginDriver.cpp
#LIB_SRCS += test_forward_reference.cpp


# The following gets rid of the -fno-implicit-templates flag on vxWorks,
# so we get automatic template instantiation.
# This is what we want for the source files that define template functions.
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
  CODE_CXXFLAGS=
endif

# Hack to allow multiple definition of _Unwind_Resume produced by MingW on Linux.
ifeq (mingw,$(findstring mingw, $(T_A)))
  USR_LDFLAGS += -Wl,-allow-multiple-definition
endif

include $(TOP)/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

