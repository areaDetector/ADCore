TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# Only build static version of ADTestUtility (missing dllimport/export)
SHARED_LIBRARIES=NO

# Use HDF5 API V2
USR_CXXFLAGS_Linux += -DH5Dopen_vers=2 -DH5Gcreate_vers=2
USR_CFLAGS_Linux   += -DH5Dopen_vers=2 -DH5Gcreate_vers=2

ifeq ($(SHARED_LIBRARIES), YES)
   USR_CXXFLAGS_WIN32 += -DH5_BUILT_AS_DYNAMIC_LIB
   USR_CFLAGS_WIN32 += -DH5_BUILT_AS_DYNAMIC_LIB
endif

# The plugin-test executable with the actual unittests depend on the boost 
# unittest framework so we can only build it if boost has been configured
ifeq ($(WITH_BOOST),YES)
  # The ADTestUtility library contain a number of helper functions to generate test
  # data, asyn port names, record output NDArrays etc etc.
  # This utility library can be used by external modules that provides unittests.
  USR_CXXFLAGS_Linux += -std=c++11
  LIBRARY_IOC_Linux += ADTestUtility
  LIBRARY_IOC_Darwin += ADTestUtility
  LIBRARY_IOC_WIN32 += ADTestUtility
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
  ADTestUtility_SRCS += AttrPlotPluginWrapper.cpp
  ADTestUtility_SRCS += ROIPluginWrapper.cpp
  ADTestUtility_SRCS += OverlayPluginWrapper.cpp

  PROD_IOC_Linux += plugin-test
  PROD_IOC_Darwin += plugin-test
  PROD_IOC_WIN32 += plugin-test
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
  plugin-test_SRCS += test_NDPluginAttrPlot.cpp
  plugin-test_SRCS += test_NDPluginROI.cpp
  plugin-test_SRCS += test_NDPluginOverlay.cpp
  plugin-test_SRCS += test_NDArrayPool.cpp

  # Add tests for new plugins like this:
  #plugin-test_SRCS += test_<plugin name>.cpp

  USR_LDFLAGS_WIN32 += /SUBSYSTEM:CONSOLE
  #USR_LDFLAGS_WIN32 += /VERBOSE
  
  ifdef BOOST_LIB
    boost_unit_test_framework_DIR=$(BOOST_LIB)
    plugin-test_LIBS_Linux += boost_unit_test_framework
    plugin-test_LIBS_Darwin += boost_unit_test_framework
	USR_LDFLAGS_WIN32 += /LIBPATH:$(BOOST_LIB)
    LIB_LIBS_WIN32 += libboost_unit_test_framework-vc141-mt-s-x64-1_69
  else
    plugin-test_SYS_LIBS += boost_unit_test_framework
  endif

  # Link order matters when doing a static build
  plugin-test_LIBS += ADTestUtility

  ifdef HDF5_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(HDF5_INCLUDE))
  endif
  ifdef SZIP_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(SZIP_INCLUDE))
  endif
  ifdef XML2_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(XML2_INCLUDE))
  endif
  ifdef BOOST_INCLUDE
    USR_INCLUDES += $(addprefix -I, $(BOOST_INCLUDE))
  endif
  ifeq ($(BOOST_USE_STATIC_LINK),YES)
	USR_CXXFLAGS_Linux += -DBOOST_USE_STATIC_LINK
	USR_CFLAGS_Linux += -DBOOST_USE_STATIC_LINK
	USR_CXXFLAGS_WIN32 += -DBOOST_USE_STATIC_LINK
	USR_CFLAGS_WIN32 += -DBOOST_USE_STATIC_LINK
  endif
endif

## hdf5-1.10.1 seems to have fixed these SWMR problems
## We keep the test files but don't  build them for now
#ifeq ($(WITH_HDF5),YES)
#  USR_INCLUDES += $(addprefix -I, $(HDF5_INCLUDE))
#  PROD += test_SWMR_works
#  PROD += test_SWMR_fail
#  PROD += test_SWMR_fail_min
#  test_SWMR_works_SRCS += test_SWMR_works.c
#  test_SWMR_fail_SRCS  += test_SWMR_fail.c
#  test_SWMR_fail_min_SRCS  += test_SWMR_fail_min.c
#endif

include $(TOP)/ADApp/commonDriverMakefile

include $(TOP)/configure/RULES
