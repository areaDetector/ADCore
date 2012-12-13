TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for miscellaneous/asynPortDriver.cpp
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
CODE_CXXFLAGS=
endif

# The following flag is need to compile/link NDArray.cpp on Solaris
ifeq ($(GNU),NO)
  NDArray_CXXFLAGS_solaris += -features=tmplrefstatic
endif

DBD += ADSupport.dbd
DBD += commonDriverSupport.dbd

INC += NDArray.h
INC += PVAttribute.h
INC += paramAttribute.h
INC += asynNDArrayDriver.h
INC += ADDriver.h
INC += tinyxml.h
INC += tinystr.h

LIBRARY_IOC = ADBase
LIB_SRCS += NDArray.cpp
LIB_SRCS += asynNDArrayDriver.cpp
LIB_SRCS += ADDriver.cpp
LIB_SRCS += PVAttribute.cpp
LIB_SRCS += paramAttribute.cpp
LIB_SRCS += tinyxml.cpp tinystr.cpp tinyxmlparser.cpp tinyxmlerror.cpp
LIB_SRCS += parseAreaPrefixes.c

LIB_LIBS += asyn
LIB_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

