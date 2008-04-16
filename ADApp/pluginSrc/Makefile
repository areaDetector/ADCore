TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += pluginSupport.dbd

INC += drvNDStdArrays.h
INC += drvADFile.h

LIBRARY_IOC = ADPlugin
LIB_SRCS += drvNDStdArrays.c
LIB_SRCS += drvNDStdArraysEpics.c
LIB_SRCS += drvADFile.c
LIB_SRCS += drvADFileEpics.c

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

