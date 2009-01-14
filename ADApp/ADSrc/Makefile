TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The following flag is need to compile/link NDArray.cpp on Solaris
ifeq ($(GNU),NO)
  NDArray_CXXFLAGS_solaris += -features=tmplrefstatic
endif

DBD += ADSupport.dbd

INC += NDArray.h
INC += ADStdDriverParams.h
INC += asynPortDriver.h
INC += asynNDArrayDriver.h
INC += ADDriver.h

LIBRARY_IOC = ADBase
LIB_SRCS += NDArray.cpp
LIB_SRCS += asynPortDriver.cpp
LIB_SRCS += asynNDArrayDriver.cpp
LIB_SRCS += ADDriver.cpp
LIB_LIBS += asyn
LIB_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

