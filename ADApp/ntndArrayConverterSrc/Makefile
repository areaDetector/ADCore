TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

ifeq ($(WITH_PVA), YES)
	LIBRARY_IOC += ntndArrayConverter
	INC += ntndArrayConverter.h
	LIB_SRCS += ntndArrayConverter.cpp
	LIB_LIBS += pvData
	LIB_LIBS += nt
endif

ifeq ($(WITH_PVXS), YES)
	LIBRARY_IOC += ntndArrayConverterPvxs
	INC += ntndArrayConverterPvxs.h
	LIB_SRCS += ntndArrayConverterPvxs.cpp
	LIB_LIBS += pvxs
endif

INC += ntndArrayConverterAPI.h
USR_CPPFLAGS += -DBUILDING_ntndArrayConverter_API
LIB_LIBS              += ADBase
LIB_LIBS              += asyn
LIB_LIBS              += $(EPICS_BASE_IOC_LIBS)
#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

