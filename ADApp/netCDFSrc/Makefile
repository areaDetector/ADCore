TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================



LIBRARY_IOC = netCDF
NETCDF = $(TOP)/ADApp/netCDFSrc
USR_CFLAGS += -DHAVE_CONFIG_H

SRC_DIRS += $(NETCDF)/inc
INC += netcdf.h

SRC_DIRS += $(NETCDF)/libsrc
LIB_SRCS += attr.c
LIB_SRCS += dim.c
LIB_SRCS += lookup3.c
LIB_SRCS += nc.c
LIB_SRCS += nc3dispatch.c
LIB_SRCS += nclistmgr.c
LIB_SRCS += ncx.c
LIB_SRCS += posixio.c
LIB_SRCS += putget.c
LIB_SRCS += string.c
LIB_SRCS += utf8proc.c
LIB_SRCS += v1hpg.c
LIB_SRCS += var.c

SRC_DIRS += $(NETCDF)/libdispatch
LIB_SRCS += att.c
LIB_SRCS += dim1.c
LIB_SRCS += dispatch.c
LIB_SRCS += error.c
LIB_SRCS += file.c
LIB_SRCS += nc_uri.c
LIB_SRCS += var1.c

SRC_DIRS += $(NETCDF)/liblib
LIB_SRCS += stub.c

PROD_IOC += test_big_classic
PROD_LIBS += netCDF

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

