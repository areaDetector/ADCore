TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for the source files that define template functions.
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
CODE_CXXFLAGS=
endif

# These flags are needed by PvApi.h, the Prosilca library used for Bayer to RGB color conversion
USR_CXXFLAGS_Linux += -D_LINUX -D_x86 -O0
USR_CXXFLAGS_Darwin += -D_OSX -D_x86

# We no longer set the -DHAVE_PVAPI flag to build the Bayer conversion in NDPluginColorConvert
# because PvAPI binary is no longer included.  This can be done in CONFIG_SITE if the library is available.
#USR_CXXFLAGS_Linux += -DHAVE_PVAPI
#USR_CXXFLAGS_Darwin += -DHAVE_PVAPI
#USR_CXXFLAGS_WIN32 += -DHAVE_PVAPI

# This flag is used to indicate that the TIFF library was built statically
USR_CXXFLAGS_WIN32 += -DLIBTIFF_STATIC

ifeq ($(STATIC_BUILD), NO)
  USR_CXXFLAGS_WIN32 += -DH5_BUILT_AS_DYNAMIC_LIB
endif

DBD += NDPluginSupport.dbd
DBD += NDFileJPEG.dbd
DBD += NDFileMagick.dbd
DBD += NDFileNetCDF.dbd
DBD += NDFileNexus.dbd
DBD += NDFileHDF5.dbd
DBD += NDFileTIFF.dbd
DBD += NDFileNull.dbd
DBD += NDPluginColorConvert.dbd
DBD += NDPluginOverlay.dbd
DBD += NDPluginProcess.dbd
DBD += NDPluginROI.dbd
DBD += NDPluginStats.dbd
DBD += NDPluginStdArrays.dbd
DBD += NDPluginTransform.dbd
DBD += NDPluginAttribute.dbd

INC += NDPluginDriver.h
INC += NDPluginFile.h

LIBRARY_IOC += NDPlugin
NDPlugin_SRCS += NDPluginDriver.cpp
NDPlugin_SRCS += NDPluginFile.cpp
NDPlugin_SRCS += NDFileNetCDF.cpp
NDPlugin_SRCS += NDPluginStdArrays.cpp
NDPlugin_SRCS += NDPluginStats.cpp
NDPlugin_SRCS += NDPluginProcess.cpp
NDPlugin_SRCS += NDPluginROI.cpp
NDPlugin_SRCS += NDPluginTransform.cpp
NDPlugin_SRCS += NDPluginAttribute.cpp
NDPlugin_SRCS += NDPluginOverlay.cpp
NDPlugin_SRCS += NDPluginOverlayTextFont.cpp
NDPlugin_SRCS += NDPluginColorConvert.cpp
NDPlugin_SRCS_DEFAULT += NDFileTIFF.cpp NDFileJPEG.cpp NDFileNexus.cpp NDFileHDF5.cpp NDFileHDF5Dataset.cpp NDFileHDF5LayoutXML.cpp NDFileHDF5Layout.cpp NDFileNull.cpp
NDPlugin_SRCS_vxWorks += NDFileDummy.cpp
NDPlugin_SYS_LIBS_WIN32 += Ws2_32
NDPlugin_SYS_LIBS_WIN32 += User32

ifeq ($(USE_GRAPHICSMAGICK), YES)
  USR_INCLUDES += $(GRAPHICS_MAGICK_INCLUDE)
  NDPlugin_SRCS += NDFileMagick.cpp
else 
  NDPlugin_SRCS += NDFileMagickStub.cpp
endif

USR_INCLUDES += $(HDF5_INCLUDE)
USR_INCLUDES += $(SZ_INCLUDE)
# Point to libxml2 header files
# TODO: enable supporting a windows version of libxml2
USR_INCLUDES += -I/usr/include/libxml2

include $(TOP)/ADApp/commonLibraryMakefile

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

