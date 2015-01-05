TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for miscellaneous/asynPortDriver.cpp
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
CODE_CXXFLAGS=
endif

PROD_IOC_Linux  += simDetectorNoIOCApp
PROD_IOC_WIN32  += simDetectorNoIOCApp
PROD_IOC_Darwin  += simDetectorNoIOCApp
PROD_SRCS += simDetectorNoIOC.cpp

PROD_LIBS += simDetector

ifeq ($(EPICS_LIBCOM_ONLY),YES)
  PROD_LIBS += ADBase
  PROD_LIBS += NDPlugin
  PROD_LIBS += netCDF
  ifneq (vxWorks, $(findstring vxWorks, $(T_A)))
    PROD_LIBS += NeXus
  endif
  PROD_LIBS += asyn
  PROD_LIBS += Com
else
  include $(ADCORE)/ADApp/commonDriverMakefile
endif

USR_INCLUDES += $(HDF5_INCLUDE)
USR_INCLUDES += $(SZ_INCLUDE)
USR_INCLUDES += $(XML2_INCLUDE)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

