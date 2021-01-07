TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY_IOC += ntndArrayConverter
INC += ntndArrayConverterAPI.h
INC += ntndArrayConverter.h
LIB_SRCS += ntndArrayConverter.cpp

USR_CPPFLAGS += -DBUILDING_ntndArrayConverter_API

LIB_LIBS              += ADBase
LIB_LIBS              += pvData
LIB_LIBS              += nt
LIB_LIBS              += asyn
LIB_LIBS              += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

