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

ifeq ($(SHARED_LIBRARIES),YES)
  USR_CFLAGS_WIN32 += -DDLL_NEXUS
  NeXus.dll: USR_CFLAGS_WIN32 += -DDLL_EXPORT
endif

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

ifeq ($(OS_CLASS), $(filter $(OS_CLASS), Linux Darwin solaris))
  ifdef HDF5_LIB
    hdf5_DIR             = $(HDF5_LIB)
    LIB_LIBS            += hdf5
  else
    LIB_SYS_LIBS        += hdf5
  endif
  ifdef SZIP
    ifdef SZIP_LIB
      sz_DIR             = $(SZIP_LIB)
      LIB_LIBS          += sz
    else
      LIB_SYS_LIBS      += sz
    endif
  endif
endif

LIBRARY_IOC_WIN32    += NeXus
LIBRARY_IOC_cygwin32 += NeXus
LIBRARY_IOC_Linux    += NeXus
LIBRARY_IOC_Darwin   += NeXus

NeXus_SRCS += napi.c
NeXus_SRCS += napi5.c
NeXus_SRCS += napiu.c
NeXus_SRCS += nxdataset.c
NeXus_SRCS += nxio.c
NeXus_SRCS += nxstack.c
NeXus_SRCS += nxxml.c
NeXus_SRCS += stptok.c

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

