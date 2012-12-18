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

# These flags are needed by PvApi.h, the Prosilca library used for Bayer to RGB color conversion
USR_CXXFLAGS_Linux += -D_LINUX -D_x86 -O0
USR_CXXFLAGS_Darwin += -D_OSX -D_x86

# These flags are used for Linux and WIN32 to build the Bayer conversion in NDPluginColorConvert
USR_CXXFLAGS_Linux += -DHAVE_PVAPI
USR_CXXFLAGS_Darwin += -DHAVE_PVAPI
USR_CXXFLAGS_WIN32 += -DHAVE_PVAPI

# This flag is used to indicate that the TIFF library was built statically
USR_CXXFLAGS_WIN32 += -DLIBTIFF_STATIC

DBD += NDPluginSupport.dbd
DBD += NDFileJPEG.dbd
DBD += NDFileMagick.dbd
DBD += NDFileNetCDF.dbd
DBD += NDFileNexus.dbd
DBD += NDFileHDF5.dbd
DBD += NDFileTIFF.dbd
DBD += NDFileNull.dbd
DBD += NDPluginColorConvert.dbd
DBD += NDPluginOverlay.dbd
DBD += NDPluginProcess.dbd
DBD += NDPluginROI.dbd
DBD += NDPluginStats.dbd
DBD += NDPluginStdArrays.dbd
DBD += NDPluginTransform.dbd

INC += NDPluginDriver.h
INC += NDPluginFile.h

LIBRARY_IOC += NDPlugin
NDPlugin_SRCS += NDPluginDriver.cpp
NDPlugin_SRCS += NDPluginFile.cpp
NDPlugin_SRCS += NDFileNetCDF.cpp
NDPlugin_SRCS += NDPluginStdArrays.cpp
NDPlugin_SRCS += NDPluginStats.cpp
NDPlugin_SRCS += NDPluginProcess.cpp
NDPlugin_SRCS += NDPluginROI.cpp
NDPlugin_SRCS += NDPluginTransform.cpp
NDPlugin_SRCS += NDPluginOverlay.cpp
NDPlugin_SRCS += NDPluginColorConvert.cpp
NDPlugin_SRCS_DEFAULT += NDFileTIFF.cpp NDFileJPEG.cpp NDFileNexus.cpp NDFileHDF5.cpp NDFileNull.cpp
NDPlugin_SRCS_vxWorks += NDFileDummy.cpp

LIBRARY_IOC += NDFileMagick
NDFileMagick_SRCS_DEFAULT += NDFileMagick.cpp 
NDFileMagick_SRCS_vxWorks += NDFileDummyMagick.cpp

include $(AREA_DETECTOR)/ADApp/commonDriverMakefile

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

