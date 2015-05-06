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

INC += ADCoreVersion.h
INC += NDAttribute.h
INC += NDAttributeList.h
INC += NDArray.h
INC += PVAttribute.h
INC += paramAttribute.h
INC += functAttribute.h
INC += asynNDArrayDriver.h
INC += ADDriver.h
INC += tinyxml.h
INC += tinystr.h

LIBRARY_IOC = ADBase
LIB_SRCS += NDAttribute.cpp
LIB_SRCS += NDAttributeList.cpp
LIB_SRCS += NDArrayPool.cpp
LIB_SRCS += NDArray.cpp
LIB_SRCS += asynNDArrayDriver.cpp
LIB_SRCS += ADDriver.cpp
LIB_SRCS += paramAttribute.cpp
ifeq ($(EPICS_LIBCOM_ONLY),YES)
  USR_CXXFLAGS += -DEPICS_LIBCOM_ONLY
else
  LIB_SRCS += PVAttribute.cpp
  LIB_SRCS += functAttribute.cpp
  LIB_SRCS += parseAreaPrefixes.c
  LIB_SRCS += myTimeStampSource.cpp
  LIB_SRCS += myAttributeFunctions.cpp
endif
LIB_SRCS += tinyxml.cpp tinystr.cpp tinyxmlparser.cpp tinyxmlerror.cpp

LIB_LIBS += asyn
ifeq ($(EPICS_LIBCOM_ONLY),YES)
  LIB_LIBS += Com
else
  LIB_LIBS += $(EPICS_BASE_IOC_LIBS)
endif

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

