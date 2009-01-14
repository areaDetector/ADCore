TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

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
NDPlugin_SRCS += drvNDFileEpics.c
NDPlugin_SRCS += NDFileNetCDF.cpp
NDPlugin_SRCS += NDPluginStdArrays.cpp
NDPlugin_SRCS += drvNDStdArraysEpics.c
NDPlugin_SRCS += NDPluginROI.cpp
NDPlugin_SRCS += drvNDROIEpics.c
NDPlugin_LIBS += ADBase
NDPlugin_LIBS += netCDF
NDPlugin_LIBS += asyn
NDPlugin_LIBS += $(EPICS_BASE_IOC_LIBS)

LIBRARY_IOC_Linux = NDPluginColorConvert
LIBRARY_IOC_WIN32 = NDPluginColorConvert
NDPluginColorConvert_SRCS += NDPluginColorConvert.cpp
NDPluginColorConvert_SRCS += drvNDColorConvertEpics.c
NDPluginColorConvert_LIBS += ADBase
NDPluginColorConvert_LIBS += asyn
NDPluginColorConvert_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

