TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += areaDetectorSupport.dbd

INC += ADParamLib.h
INC += areaDetectorInterface.h

LIBRARY_IOC = areaDetector
LIB_SRCS += ADParamLib.c
LIB_SRCS += drvAreaDetectorAsyn.c

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

