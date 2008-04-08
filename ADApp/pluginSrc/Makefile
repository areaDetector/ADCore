TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += ADPluginSupport.dbd

INC += drvADImage.h
INC += drvADFile.h

LIBRARY_IOC = ADPlugin
LIB_SRCS += drvADImage.c
LIB_SRCS += drvADImageEpics.c
LIB_SRCS += drvADFile.c
LIB_SRCS += drvADFileEpics.c

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

