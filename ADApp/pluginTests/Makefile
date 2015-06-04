TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The ADTestUtility library contain a number of helper functions to generate test
# data, asyn port names, record output NDArrays etc etc.
# This utility library can be used by external modules that provides unittests.
# It depends on asyn, and areaDetector libs - but we dont
# need to do the linking here as long as the final test executable binary links.
# The ADTestUtility does not depend on boost!
LIBRARY_IOC_WIN32 += ADTestUtility
LIBRARY_IOC_Linux += ADTestUtility
ADTestUtility_SRCS += testingutilities.cpp
ADTestUtility_SRCS += AsynPortClientContainer.cpp
ADTestUtility_SRCS += SimulatedDetectorWrapper.cpp
ADTestUtility_SRCS += HDF5PluginWrapper.cpp
ADTestUtility_SRCS += HDF5FileReader.cpp


# The plugin-test executable with the actual unittests depend on the boost 
# unittest framework so we can only build it if boost has been configured
ifdef BOOST
  PROD_IOC_WIN32 += plugin-test
  PROD_IOC_Linux += plugin-test
  plugin-test_SRCS += plugin-test.cpp
  plugin-test_SRCS += test_NDPluginCircularBuff.cpp
  
  plugin-test_SRCS += test_NDFileHDF5.cpp
  USR_INCLUDES += $(HDF5_INCLUDE)
  USR_INCLUDES += $(SZ_INCLUDE)
  USR_INCLUDES += $(XML2_INCLUDE)

  # Add tests for new plugins like this:
  #plugin-test_SRCS += test_<plugin name>.cpp
  
  plugin-test_LIBS += boost_unit_test_framework simDetector ADTestUtility
  boost_unit_test_framework_DIR=$(BOOST_LIB)
endif

include $(TOP)/ADApp/commonDriverMakefile

include $(TOP)/configure/RULES
