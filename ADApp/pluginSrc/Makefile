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

# We build 2 libraries: one for plugins that work on all platforms
# and one for ColorConvert that only works on Linux and Windows, because
# it uses the Prosilica library (at least for now)

DBD += pluginSupport.dbd
DBD += pluginColorConvertSupport.dbd

INC += NDPluginDriver.h

LIBRARY_IOC = NDPlugin
NDPlugin_SRCS += NDPluginDriver.cpp
NDPlugin_SRCS += NDPluginFile.cpp
NDPlugin_SRCS += NDFileNetCDF.cpp
NDPlugin_SRCS += NDPluginStdArrays.cpp
NDPlugin_SRCS += NDPluginROI.cpp
NDPlugin_SRCS += NDPluginProcess.cpp
NDPlugin_SRCS += NDPluginTransform.cpp
NDPlugin_SRCS_DEFAULT += NDFileTIFF.cpp NDFileJPEG.cpp NDFileNexus.cpp
NDPlugin_SRCS_vxWorks += -nil-
NDPlugin_LIBS += ADBase
NDPlugin_LIBS += netCDF
NDPlugin_LIBS += asyn
NDPlugin_LIBS += $(EPICS_BASE_IOC_LIBS)

LIBRARY_IOC_Linux = NDPluginColorConvert
LIBRARY_IOC_WIN32 = NDPluginColorConvert
NDPluginColorConvert_SRCS += NDPluginColorConvert.cpp
NDPluginColorConvert_LIBS += ADBase
NDPluginColorConvert_LIBS += asyn
NDPluginColorConvert_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

