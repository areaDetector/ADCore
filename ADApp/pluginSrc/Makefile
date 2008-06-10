TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += pluginSupport.dbd


LIBRARY_IOC = ADPlugin
LIB_SRCS += NDPluginDriver.cpp
LIB_SRCS += NDPluginFile.cpp
LIB_SRCS += drvNDFileEpics.c
LIB_SRCS += NDFileNetCDF.cpp
LIB_SRCS += NDPluginStdArrays.cpp
LIB_SRCS += drvNDStdArraysEpics.c
LIB_SRCS += NDPluginROI.cpp
LIB_SRCS += drvNDROIEpics.c
LIB_LIBS += ADBase
LIB_LIBS += netCDF

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

