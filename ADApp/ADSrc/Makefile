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

# Generate only these pvi ParamSet headers here
PVI_PARAMSETS = asynNDArrayDriverParamSet.h ADDriverParamSet.h
# Regenerate param set files from PVI YAML files if RULES_PVI exists
PVI_YAML_PATHS = $(TOP)/pvi
-include $(PVI)/configure/RULES_PVI

# The following flag is need to compile/link NDArray.cpp on Solaris
ifeq ($(GNU),NO)
  NDArray_CXXFLAGS_solaris += -features=tmplrefstatic
endif

USR_CFLAGS += -DUSE_TYPED_RSET
USR_CPPFLAGS += -DBUILDING_ADCore_API

DBD += ADSupport.dbd

INC += ADCoreAPI.h
INC += ADCoreVersion.h
INC += NDAttribute.h
INC += NDAttributeList.h
INC += NDArray.h
INC += Codec.h
INC += PVAttribute.h
INC += paramAttribute.h
INC += functAttribute.h
INC += asynNDArrayDriverParamSet.h
INC += asynNDArrayDriver.h
INC += ADDriverParamSet.h
INC += ADDriver.h
INC += CCDMultiTrack.h

LIBRARY_IOC = ADBase
LIB_SRCS += NDAttribute.cpp
LIB_SRCS += NDAttributeList.cpp
LIB_SRCS += NDArrayPool.cpp
LIB_SRCS += NDArray.cpp
LIB_SRCS += asynNDArrayDriver.cpp
LIB_SRCS += ADDriver.cpp
LIB_SRCS += paramAttribute.cpp
LIB_SRCS += CCDMultiTrack.cpp

ifeq ($(EPICS_LIBCOM_ONLY),YES)
  USR_CXXFLAGS += -DEPICS_LIBCOM_ONLY
else
  LIB_SRCS += PVAttribute.cpp
  LIB_SRCS += functAttribute.cpp
  LIB_SRCS += parseAreaPrefixes.c
  LIB_SRCS += myTimeStampSource.cpp
  LIB_SRCS += myAttributeFunctions.cpp
endif

LIB_LIBS += asyn
ifeq ($(EPICS_LIBCOM_ONLY),YES)
  LIB_LIBS += Com
else
  LIB_LIBS += $(EPICS_BASE_IOC_LIBS)
endif
ifeq ($(XML2_EXTERNAL),NO)
  LIB_LIBS += xml2
else
  ifdef XML2_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(XML2_INCLUDE))
  endif
  ifdef XML2_LIB
    xml2_DIR     = $(XML2_LIB)
    LIB_LIBS     += xml2
  else
    LIB_SYS_LIBS += xml2
  endif
endif

# Install commonDriverMakefile and commonLibraryMakefile in top-level cfg directory.
# Drivers currently look in $(TOP)/ADApp for these files so we leave them there but 
# need to install from this directory which requires using vpath.
vpath common%Makefile $(TOP)/ADApp/
CFG += commonLibraryMakefile
CFG += commonDriverMakefile

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

