TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += ADSupport.dbd

INC += ADParamLib.h
INC += ADUtils.h
INC += ADImageBuff.h
INC += asynADImage.h
INC += ADInterface.h

LIBRARY_IOC = ADBase
LIB_SRCS += ADParamLib.c
LIB_SRCS += ADUtils.c
LIB_SRCS += ADImageBuff.h
LIB_SRCS += ADImageBuffEpics.h
LIB_SRCS += asynADImageBase.c

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

