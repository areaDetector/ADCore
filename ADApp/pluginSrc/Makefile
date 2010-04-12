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
# These flags are used for Linux and WIN32 to build the Bayer conversion in NDPluginColorConvert
USR_CXXFLAGS_Linux += -DHAVE_PVAPI
USR_CXXFLAGS_WIN32 += -DHAVE_PVAPI

DBD += pluginSupport.dbd

INC += NDPluginDriver.h
INC += NDPluginFile.h

LIBRARY_IOC = NDPlugin
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
NDPlugin_SRCS_DEFAULT += NDFileTIFF.cpp NDFileJPEG.cpp NDFileNexus.cpp
NDPlugin_SRCS_vxWorks += NDFileDummy.cpp
NDPlugin_LIBS += ADBase
NDPlugin_LIBS += netCDF
NDPlugin_LIBS += asyn
NDPlugin_LIBS += $(EPICS_BASE_IOC_LIBS)

NDPlugin_LIBS_solaris += NeXus hdf5_cpp hdf5

NDPlugin_SYS_LIBS_solaris   += tiff

# use libs supplied with areaDetector (in tiffSupport)
NDPlugin_LIBS_solaris   += jpeg sz z

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

