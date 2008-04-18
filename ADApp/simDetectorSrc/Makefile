TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#USR_CFLAGS += 
#USR_CXXFLAGS +=

LIBRARY_IOC = simDetector
LIB_SRCS += drvSimDetector.c
LIB_SRCS += drvSimDetectorEpics.c
LIB_LIBS += ADBase ADPlugin
LIB_LIBS += sscan autosave
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
simDetectorApp_LIBS += ADBase ADPlugin netCDF
simDetectorApp_LIBS += sscan autosave 
simDetectorApp_LIBS += asyn
simDetectorApp_LIBS += $(EPICS_BASE_IOC_LIBS)

simDetectorCCDApp_SYS_LIBS_cygwin32 += rpc

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

