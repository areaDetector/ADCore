TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================



LIBRARY_IOC_WIN32 = szip
LIBRARY_IOC_vxWorks = szip

# OS-specific files in os/ARCH
INC_WIN32 += SZconfig.h
INC_WIN32 += rice.h
INC_WIN32 += ricehdf.h
INC_WIN32 += szip_adpt.h
INC_WIN32 += szlib.h
INC_vxWorks += SZconfig.h
INC_vxWorks += rice.h
INC_vxWorks += ricehdf.h
INC_vxWorks += szip_adpt.h
INC_vxWorks += szlib.h

LIB_SRCS += encoding.c
LIB_SRCS += rice.c
LIB_SRCS += sz_api.c

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

