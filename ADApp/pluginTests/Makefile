TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================


BOOSTDIR=/dls_sw/prod/tools/RHEL6-x86_64/boost/1-48-0/prefix

ifdef BOOSTDIR
boost_unit_test_framework_DIR=$(BOOSTDIR)/lib

PROD_IOC += NDPluginCircularBuff_test
NDPluginCircularBuff_test_SRCS += test_NDPluginCircularBuff.cpp
NDPluginCircularBuff_test_LIBS += asyn ADBase NDPlugin Com simDetector
NDPluginCircularBuff_test_LIBS += boost_unit_test_framework

endif # BOOSTDIR

include $(TOP)/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES
