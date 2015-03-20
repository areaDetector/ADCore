TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================


INC += napi.h
INC += napiconfig.h
INC += nxconfig.h

USR_CFLAGS += -DHDF5 -D_FILE_OFFSET_BITS=64

# Travis/ubuntu 12.04 tweak: persuade the hdf5 library build to use API v18 over v16
USR_CFLAGS += -DH5_NO_DEPRECATED_SYMBOLS -DH5Gopen_vers=2

ifeq ($(HDF5_STATIC_BUILD), NO)
  USR_CXXFLAGS_WIN32    += -DH5_BUILT_AS_DYNAMIC_LIB
  USR_CFLAGS_WIN32      += -DH5_BUILT_AS_DYNAMIC_LIB
  LIB_LIBS_WIN32        += hdf5 szip zlib
else
  USR_CXXFLAGS_WIN32    += -DH5_BUILT_AS_STATIC_LIB
  USR_CFLAGS_WIN32      += -DH5_BUILT_AS_STATIC_LIB
  LIB_LIBS_WIN32        += libhdf5 libszip libzlib
endif
USR_INCLUDES += $(HDF5_INCLUDE)

LIB_SYS_LIBS_cygwin32 += libhdf5
LIB_SYS_LIBS_cygwin32 += libz
LIB_LIBS_Linux        += hdf5
LIB_LIBS_Darwin       += hdf5
ifdef SZIP
  LIB_LIBS_Linux        += sz
  LIB_LIBS_Darwin       += sz
endif  

LIBRARY_IOC_WIN32    += NeXus
LIBRARY_IOC_cygwin32 += NeXus
LIBRARY_IOC_Linux    += NeXus
LIBRARY_IOC_Darwin   += NeXus

LIB_SRCS += napi.c
LIB_SRCS += napi5.c
LIB_SRCS += napiu.c
LIB_SRCS += nxdataset.c
LIB_SRCS += nxio.c
LIB_SRCS += nxstack.c
LIB_SRCS += nxxml.c
LIB_SRCS += stptok.c

hdf5_DIR = $(HDF5_LIB)
sz_DIR   = $(SZIP_LIB)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

