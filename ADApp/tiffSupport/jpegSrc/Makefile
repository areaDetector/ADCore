TOP=../../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

INC_WIN32 += jpeglib.h jconfig.h jmorecfg.h jerror.h 

LIBRARY_IOC_WIN32 = jpeg
ifeq ($(SHARED_LIBRARIES),YES)
  USR_CFLAGS_WIN32 += -DDLL_JPEG
  jpeg.dll: USR_CFLAGS_WIN32 += -DDLL_EXPORT
endif

jpeg_SRCS += jcapimin.c 
jpeg_SRCS += jcapistd.c 
jpeg_SRCS += jccoefct.c 
jpeg_SRCS += jccolor.c
jpeg_SRCS += jcdctmgr.c 
jpeg_SRCS += jchuff.c 
jpeg_SRCS += jcinit.c 
jpeg_SRCS += jcmainct.c 
jpeg_SRCS += jcmarker.c 
jpeg_SRCS += jcmaster.c
jpeg_SRCS += jcomapi.c 
jpeg_SRCS += jcparam.c 
jpeg_SRCS += jcphuff.c 
jpeg_SRCS += jcprepct.c 
jpeg_SRCS += jcsample.c 
jpeg_SRCS += jctrans.c 
jpeg_SRCS += jdapimin.c
jpeg_SRCS += jdapistd.c 
jpeg_SRCS += jdatadst.c 
jpeg_SRCS += jdatasrc.c 
jpeg_SRCS += jdcoefct.c 
jpeg_SRCS += jdcolor.c
jpeg_SRCS += jddctmgr.c 
jpeg_SRCS += jdhuff.c 
jpeg_SRCS += jdinput.c 
jpeg_SRCS += jdmainct.c 
jpeg_SRCS += jdmarker.c 
jpeg_SRCS += jdmaster.c
jpeg_SRCS += jdmerge.c 
jpeg_SRCS += jdphuff.c 
jpeg_SRCS += jdpostct.c 
jpeg_SRCS += jdsample.c 
jpeg_SRCS += jdtrans.c 
jpeg_SRCS += jerror.c 
jpeg_SRCS += jfdctflt.c
jpeg_SRCS += jfdctfst.c 
jpeg_SRCS += jfdctint.c
jpeg_SRCS += jidctflt.c 
jpeg_SRCS += jidctfst.c 
jpeg_SRCS += jidctint.c 
jpeg_SRCS += jidctred.c 
jpeg_SRCS += jquant1.c
jpeg_SRCS += jquant2.c 
jpeg_SRCS += jutils.c 
jpeg_SRCS += jmemmgr.c
jpeg_SRCS += jmemnobs.c

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

