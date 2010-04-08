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

LIBRARY_IOC = simDetector
LIB_SRCS += simDetector.cpp
LIB_LIBS += NDPlugin ADBase netCDF
LIB_LIBS += calc busy sscan autosave
LIB_LIBS += asyn
LIB_LIBS += $(EPICS_BASE_IOC_LIBS)

#==================================
PROD_IOC += simDetectorApp
# <name>.dbd will be created from <name>Include.dbd
DBD += simDetectorApp.dbd
DBD += simDetectorSupport.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
simDetectorApp_SRCS_DEFAULT += simDetectorApp_registerRecordDeviceDriver.cpp simDetectorAppMain.cpp
simDetectorApp_SRCS_vxWorks += simDetectorApp_registerRecordDeviceDriver.cpp

# Add locally compiled object code
simDetectorApp_LIBS += simDetector
simDetectorApp_LIBS += NDPlugin ADBase netCDF
simDetectorApp_LIBS_Linux += PvAPI ImageLib
simDetectorApp_LIBS_WIN32 += PvAPI ImageLib
simDetectorApp_LIBS += calc busy sscan autosave 
simDetectorApp_LIBS += asyn
simDetectorApp_LIBS += $(EPICS_BASE_IOC_LIBS)

simDetectorApp_SYS_LIBS_cygwin32 += rpc
simDetectorApp_LIBS_WIN32        += libtiff jpeg libNeXus.dll
simDetectorApp_LIBS_cygwin32     += libtiff jpeg libNeXus.dll

# To use versions of these libraries supplied with areaDetector use this line
simDetectorApp_LIBS_Linux        += NeXus mfhdf df hdf5 mxml
# To use system versions of these libraries use this line
#simDetectorApp_SYS_LIBS_Linux    += NeXus mfhdf df hdf5 mxml

# To use versions of these libraries supplied with areaDetector use this line
simDetectorApp_LIBS_Linux        += tiff jpeg z
# To use system versions of these libraries use this line
#simDetectorApp_SYS_LIBS_Linux    += tiff jpeg z

simDetectorApp_LIBS_solaris        += NeXus hdf5 jpeg z
simDetectorApp_SYS_LIBS_solaris    += tiff

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

