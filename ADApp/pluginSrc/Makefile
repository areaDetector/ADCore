TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# These flags are needed by PvApi.h, the Prosilca library used for Bayer to RGB color conversion
USR_CXXFLAGS_Linux += -D_LINUX -D_x86 -O0

DBD += pluginSupport.dbd

INC += NDPluginDriver.h

LIBRARY_IOC = ADPlugin
LIB_SRCS += NDPluginDriver.cpp
LIB_SRCS += NDPluginFile.cpp
LIB_SRCS += drvNDFileEpics.c
LIB_SRCS += NDFileNetCDF.cpp
LIB_SRCS += NDPluginStdArrays.cpp
LIB_SRCS += drvNDStdArraysEpics.c
LIB_SRCS += NDPluginROI.cpp
LIB_SRCS += drvNDROIEpics.c
LIB_SRCS_Linux += NDPluginColorConvert.cpp
LIB_SRCS_Linux += drvNDColorConvertEpics.c
LIB_SRCS_WIN32 += NDPluginColorConvert.cpp
LIB_SRCS_WIN32 += drvNDColorConvertEpics.c
LIB_LIBS += ADBase
LIB_LIBS += netCDF
LIB_LIBS += asyn
LIB_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

