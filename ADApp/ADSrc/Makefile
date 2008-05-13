TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += ADSupport.dbd

INC += ADParamLib.h
INC += ADUtils.h
INC += NDArray.h
INC += ADInterface.h
INC += asynParamBase.h
INC += asynNDArrayBase.h
INC += ADDriverBase.h

LIBRARY_IOC = ADBase
LIB_SRCS += NDArray.cpp
LIB_SRCS += ADParamLib.c
LIB_SRCS += ADUtils.c
LIB_SRCS += ADDriverBase.cpp
LIB_SRCS += asynParamBase.cpp
LIB_SRCS += asynNDArrayBase.cpp
LIB_LIBS += asyn
LIB_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

