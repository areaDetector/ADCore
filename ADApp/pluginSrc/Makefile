TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += pluginSupport.dbd

INC += drvNDStdArrays.h
INC += drvNDFile.h

LIBRARY_IOC = ADPlugin
LIB_SRCS += drvNDStdArrays.c
LIB_SRCS += drvNDStdArraysEpics.c
LIB_SRCS += drvNDFile.c
LIB_SRCS += drvNDFileEpics.c
LIB_SRCS += NDFileNetCDF.c
LIB_SRCS += drvNDROI.c
LIB_SRCS += drvNDROIEpics.c
LIB_LIBS += netCDF

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

