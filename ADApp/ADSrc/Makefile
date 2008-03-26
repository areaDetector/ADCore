TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += ADSupport.dbd

INC += ADParamLib.h
INC += ADUtils.h
INC += ADInterface.h

LIBRARY_IOC = areaDetector
LIB_SRCS += ADParamLib.c
LIB_SRCS += ADUtils.c
LIB_SRCS += drvADAsyn.c

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

