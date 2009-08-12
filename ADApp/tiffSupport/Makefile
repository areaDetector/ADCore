TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# This directory contains files to support libtiff and libjpeg on WIN32, so users don't need to install it.
# These files were obtained from the distribution at
# http://gnuwin32.sourceforge.net/packages/tiff.htm
# http://gnuwin32.sourceforge.net/packages/jpeg.htm
# 
INC_WIN32    += tiff.h tiffio.h tiffvers.h tiffconf.h 
INC_cygwin32 += tiff.h tiffio.h tiffvers.h tiffconf.h 
INC_WIN32    += jpeglib.h jconfig.h jmorecfg.h jerror.h 
INC_cygwin32 += jpeglib.h jconfig.h jmorecfg.h jerror.h 
# Use these lines to install local versions of these libraries
# Comment them out to use system versions of the libraries
INC_Linux    += tiff.h tiffio.h tiffvers.h tiffconf.h 
INC_Linux    += jpeglib.h jconfig.h jmorecfg.h jerror.h 

LIB_INSTALLS_WIN32    += ../libtiff.lib  ../jpeg.lib
LIB_INSTALLS_cygwin32 += ../libtiff.lib  ../jpeg.lib
# Use this line to install local versions of these libraries
# Comment it out to use system versions of the libraries
LIB_INSTALLS_Linux    += ../libtiff.a    ../libjpeg.a  ../libz.a
BIN_INSTALLS_WIN32    += ../libtiff3.dll ../jpeg62.dll ../zlib1.dll
BIN_INSTALLS_cygwin32 += ../libtiff3.dll ../jpeg62.dll ../zlib1.dll

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

