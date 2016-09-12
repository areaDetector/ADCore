TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================



LIBRARY_IOC_WIN32 = zlib

ifeq ($(SHARED_LIBRARIES),YES)
  USR_CFLAGS_WIN32 += -DZLIB_DLL
endif

# OS-specific files in os/ARCH
INC_WIN32 += zlib.h
INC_WIN32 += zconf.h

LIB_SRCS += adler32.c
LIB_SRCS += compress.c
LIB_SRCS += crc32.c
LIB_SRCS += deflate.c
LIB_SRCS += gzclose.c
LIB_SRCS += gzlib.c
LIB_SRCS += gzread.c
LIB_SRCS += gzwrite.c
LIB_SRCS += infback.c
LIB_SRCS += inffast.c
LIB_SRCS += inflate.c
LIB_SRCS += inftrees.c
LIB_SRCS += trees.c
LIB_SRCS += uncompr.c
LIB_SRCS += zutil.c


include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

