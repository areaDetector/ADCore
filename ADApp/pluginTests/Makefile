TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# Persuade travis (ubuntu 12.04) to use HDF5 API V2 (1.8 rather than default 1.6)
USR_CXXFLAGS_Linux += -DH5_NO_DEPRECATED_SYMBOLS -DH5Dopen_vers=2 -DH5Gcreate_vers=2

# The plugin-test executable with the actual unittests depend on the boost 
# unittest framework so we can only build it if boost has been configured
ifeq ($(WITH_BOOST),YES)
  # The ADTestUtility library contain a number of helper functions to generate test
  # data, asyn port names, record output NDArrays etc etc.
  # This utility library can be used by external modules that provides unittests.
  LIBRARY_IOC_Linux += ADTestUtility
  LIBRARY_IOC_Darwin += ADTestUtility
  ADTestUtility_SRCS += testingutilities.cpp
  ADTestUtility_SRCS += AsynPortClientContainer.cpp
  ADTestUtility_SRCS += AsynException.cpp
  
  ifeq ($(WITH_HDF5),YES)
    ADTestUtility_SRCS += HDF5PluginWrapper.cpp
    ADTestUtility_SRCS += HDF5FileReader.cpp
  endif
  ADTestUtility_SRCS += PosPluginWrapper.cpp
  ADTestUtility_SRCS += TimeSeriesPluginWrapper.cpp
  ADTestUtility_SRCS += FFTPluginWrapper.cpp

  PROD_IOC_Linux += plugin-test
  PROD_IOC_Darwin += plugin-test
  plugin-test_SRCS += plugin-test.cpp
  plugin-test_SRCS += test_NDPluginCircularBuff.cpp
  ifeq ($(WITH_HDF5),YES)
    plugin-test_SRCS += test_NDFileHDF5.cpp
    plugin-test_SRCS += test_NDFileHDF5AttributeDataset.cpp
    plugin-test_SRCS += test_NDFileHDF5ExtraDimensions.cpp
  endif
  plugin-test_SRCS += test_NDPosPlugin.cpp
  plugin-test_SRCS += test_NDPluginTimeSeries.cpp
  plugin-test_SRCS += test_NDPluginFFT.cpp

  # Add tests for new plugins like this:
  #plugin-test_SRCS += test_<plugin name>.cpp
  
  plugin-test_LIBS += ADTestUtility
  ifdef BOOST_LIB
    boost_unit_test_framework_DIR=$(BOOST_LIB)
    plugin-test_LIBS += boost_unit_test_framework
  else
    plugin-test_SYS_LIBS += boost_unit_test_framework
  endif

  USR_INCLUDES += $(HDF5_INCLUDE)
  USR_INCLUDES += $(SZ_INCLUDE)
  USR_INCLUDES += $(XML2_INCLUDE)
  USR_INCLUDES += $(BOOST_INCLUDE)

endif

ifeq ($(WITH_HDF5),YES)
  PROD += test_SWMR_works
  PROD += test_SWMR_fail
  PROD += test_SWMR_fail_min
  test_SWMR_works_SRCS += test_SWMR_works.c
  test_SWMR_fail_SRCS  += test_SWMR_fail.c
  test_SWMR_fail_min_SRCS  += test_SWMR_fail_min.c
endif

include $(ADCORE)/ADApp/commonDriverMakefile

include $(TOP)/configure/RULES
