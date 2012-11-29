TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================


INC += napi.h

USR_CFLAGS += -DHDF5 -D_FILE_OFFSET_BITS=64

NeXus_LIBS_Darwin += hdf5 sz

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

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

