TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

ifdef BOOST
boost_unit_test_framework_DIR=$(BOOST_LIB)

PROD_IOC += NDPluginCircularBuff_test
NDPluginCircularBuff_test_SRCS += test_NDPluginCircularBuff.cpp
NDPluginCircularBuff_test_LIBS += asyn ADBase NDPlugin Com simDetector
NDPluginCircularBuff_test_LIBS += boost_unit_test_framework

endif # BOOST

include $(TOP)/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES
