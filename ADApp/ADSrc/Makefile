TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += ADSupport.dbd

INC += ADParamLib.h
INC += ADUtils.h
INC += NDArrayBuff.h
INC += ADInterface.h
INC += asynParamBase.h
INC += ADDriverBase.h

LIBRARY_IOC = ADBase
LIB_SRCS += ADParamLib.c
LIB_SRCS += ADUtils.c
LIB_SRCS += NDArrayBuff.c
LIB_SRCS += NDArrayBuffEpics.c
LIB_SRCS += ADDriverBase.cpp
LIB_SRCS += asynParamBase.cpp
LIB_LIBS += asyn
LIB_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

