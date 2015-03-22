TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

PROD_IOC += plugin-test
plugin-test_SRCS += plugin-test.cpp
plugin-test_SRCS += test_NDPluginCircularBuff.cpp
#plugin-test_SRCS += test_.cpp

plugin-test_LIBS += boost_unit_test_framework simDetector
boost_unit_test_framework_DIR=$(BOOST_LIB)

include $(TOP)/ADApp/commonDriverMakefile

include $(TOP)/configure/RULES
