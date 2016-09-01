TOP=../../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

INC_WIN32 += tiff.h tiffio.h tiffvers.h tiffconf.h 

LIBRARY_IOC_WIN32 = tiff
ifeq ($(SHARED_LIBRARIES),YES)
  USR_CFLAGS_WIN32 += -DDLL_TIFF
  tiff.dll: USR_CFLAGS_WIN32 += -DDLL_EXPORT
endif

LIB_SYS_LIBS_WIN32 += user32

tiff_SRCS += tif_aux.c
tiff_SRCS += tif_close.c
tiff_SRCS += tif_codec.c
tiff_SRCS += tif_color.c
tiff_SRCS += tif_compress.c
tiff_SRCS += tif_dir.c
tiff_SRCS += tif_dirinfo.c
tiff_SRCS += tif_dirread.c
tiff_SRCS += tif_dirwrite.c
tiff_SRCS += tif_dumpmode.c
tiff_SRCS += tif_error.c
tiff_SRCS += tif_extension.c
tiff_SRCS += tif_fax3.c
tiff_SRCS += tif_fax3sm.c
tiff_SRCS += tif_getimage.c
tiff_SRCS += tif_jbig.c
tiff_SRCS += tif_jpeg.c
tiff_SRCS += tif_jpeg_12.c
tiff_SRCS += tif_ojpeg.c
tiff_SRCS += tif_flush.c
tiff_SRCS += tif_luv.c
tiff_SRCS += tif_lzw.c
tiff_SRCS += tif_next.c
tiff_SRCS += tif_open.c
tiff_SRCS += tif_packbits.c
tiff_SRCS += tif_pixarlog.c
tiff_SRCS += tif_predict.c
tiff_SRCS += tif_print.c
tiff_SRCS += tif_read.c
tiff_SRCS += tif_stream.cpp
tiff_SRCS += tif_swab.c
tiff_SRCS += tif_strip.c
tiff_SRCS += tif_thunder.c
tiff_SRCS += tif_tile.c
tiff_SRCS += tif_version.c
tiff_SRCS += tif_warning.c
tiff_SRCS += tif_write.c
tiff_SRCS += tif_zip.c
tiff_SRCS += tif_win32.c
        
        
include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

