TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================



LIBRARY_IOC_WIN32 = libxml2
LIBRARY_IOC_vxWorks = libxml2

ifeq ($(SHARED_LIBRARIES),NO)
#  USR_CFLAGS_WIN32 += -DLIBXML_STATIC
endif

# OS-specific files in os/ARCH
INC_WIN32 += win32config.h
INC_WIN32 += wsockcompat.h
INC_WIN32 += libxml/DOCBparser.h
INC_WIN32 += libxml/globals.h 
INC_WIN32 += libxml/tree.h
INC_WIN32 += libxml/xmlregexp.h
INC_WIN32 += libxml/HTMLparser.h
INC_WIN32 += libxml/HTMLtree.h
INC_WIN32 += libxml/SAX.h
INC_WIN32 += libxml/SAX2.h
INC_WIN32 += libxml/c14n.h
INC_WIN32 += libxml/catalog.h
INC_WIN32 += libxml/chvalid.h
INC_WIN32 += libxml/debugXML.h
INC_WIN32 += libxml/dict.h
INC_WIN32 += libxml/encoding.h
INC_WIN32 += libxml/entities.h
INC_WIN32 += libxml/hash.h
INC_WIN32 += libxml/list.h
INC_WIN32 += libxml/nanoftp.h
INC_WIN32 += libxml/nanohttp.h
INC_WIN32 += libxml/parser.h
INC_WIN32 += libxml/parserInternals.h
INC_WIN32 += libxml/pattern.h
INC_WIN32 += libxml/relaxng.h
INC_WIN32 += libxml/schemasInternals.h
INC_WIN32 += libxml/schematron.h
INC_WIN32 += libxml/threads.h
INC_WIN32 += libxml/uri.h
INC_WIN32 += libxml/valid.h
INC_WIN32 += libxml/xinclude.h
INC_WIN32 += libxml/xlink.h
INC_WIN32 += libxml/xmlIO.h
INC_WIN32 += libxml/xmlautomata.h
INC_WIN32 += libxml/xmlerror.h
INC_WIN32 += libxml/xmlexports.h
INC_WIN32 += libxml/xmlmemory.h
INC_WIN32 += libxml/xmlreader.h
INC_WIN32 += libxml/xmlsave.h
INC_WIN32 += libxml/xmlschemas.h
INC_WIN32 += libxml/xmlschemastypes.h
INC_WIN32 += libxml/xpointer.h
INC_WIN32 += libxml/xmlstring.h
INC_WIN32 += libxml/xmlunicode.h
INC_WIN32 += libxml/xmlversion.h
INC_WIN32 += libxml/xmlwriter.h
INC_WIN32 += libxml/xpath.h
INC_WIN32 += libxml/xpathInternals.h
INC_WIN32 += libxml/xmlmodule.h

INC_vxWorks += libxml/DOCBparser.h
INC_vxWorks += libxml/globals.h 
INC_vxWorks += libxml/tree.h
INC_vxWorks += libxml/xmlregexp.h
INC_vxWorks += libxml/HTMLparser.h
INC_vxWorks += libxml/HTMLtree.h
INC_vxWorks += libxml/SAX.h
INC_vxWorks += libxml/SAX2.h
INC_vxWorks += libxml/c14n.h
INC_vxWorks += libxml/catalog.h
INC_vxWorks += libxml/chvalid.h
INC_vxWorks += libxml/debugXML.h
INC_vxWorks += libxml/dict.h
INC_vxWorks += libxml/encoding.h
INC_vxWorks += libxml/entities.h
INC_vxWorks += libxml/hash.h
INC_vxWorks += libxml/list.h
INC_vxWorks += libxml/nanoftp.h
INC_vxWorks += libxml/nanohttp.h
INC_vxWorks += libxml/parser.h
INC_vxWorks += libxml/parserInternals.h
INC_vxWorks += libxml/pattern.h
INC_vxWorks += libxml/relaxng.h
INC_vxWorks += libxml/schemasInternals.h
INC_vxWorks += libxml/schematron.h
INC_vxWorks += libxml/threads.h
INC_vxWorks += libxml/uri.h
INC_vxWorks += libxml/valid.h
INC_vxWorks += libxml/xinclude.h
INC_vxWorks += libxml/xlink.h
INC_vxWorks += libxml/xmlIO.h
INC_vxWorks += libxml/xmlautomata.h
INC_vxWorks += libxml/xmlerror.h
INC_vxWorks += libxml/xmlexports.h
INC_vxWorks += libxml/xmlmemory.h
INC_vxWorks += libxml/xmlreader.h
INC_vxWorks += libxml/xmlsave.h
INC_vxWorks += libxml/xmlschemas.h
INC_vxWorks += libxml/xmlschemastypes.h
INC_vxWorks += libxml/xpointer.h
INC_vxWorks += libxml/xmlstring.h
INC_vxWorks += libxml/xmlunicode.h
INC_vxWorks += libxml/xmlversion.h
INC_vxWorks += libxml/xmlwriter.h
INC_vxWorks += libxml/xpath.h
INC_vxWorks += libxml/xpathInternals.h
INC_vxWorks += libxml/xmlmodule.h

LIB_SRCS += buf.c
LIB_SRCS += c14n.c
LIB_SRCS += catalog.c
LIB_SRCS += chvalid.c
LIB_SRCS += debugXML.c
LIB_SRCS += dict.c
LIB_SRCS += DOCBparser.c
LIB_SRCS += encoding.c
LIB_SRCS += entities.c
LIB_SRCS += error.c
LIB_SRCS += globals.c
LIB_SRCS += hash.c
LIB_SRCS += HTMLparser.c
LIB_SRCS += HTMLtree.c
LIB_SRCS += legacy.c
LIB_SRCS += list.c
LIB_SRCS += nanoftp.c
LIB_SRCS += nanohttp.c
LIB_SRCS += parser.c
LIB_SRCS += parserInternals.c
LIB_SRCS += pattern.c
LIB_SRCS += relaxng.c
LIB_SRCS += SAX2.c
LIB_SRCS += SAX.c
LIB_SRCS += schematron.c
LIB_SRCS += threads.c
LIB_SRCS += tree.c
LIB_SRCS += uri.c
LIB_SRCS += valid.c
LIB_SRCS += xinclude.c
LIB_SRCS += xlink.c
LIB_SRCS += xmlIO.c
LIB_SRCS += xmlmemory.c
LIB_SRCS += xmlreader.c
LIB_SRCS += xmlregexp.c
LIB_SRCS += xmlmodule.c
LIB_SRCS += xmlsave.c
LIB_SRCS += xmlschemas.c
LIB_SRCS += xmlschemastypes.c
LIB_SRCS += xmlunicode.c
LIB_SRCS += xmlwriter.c
LIB_SRCS += xpath.c
LIB_SRCS += xpointer.c
LIB_SRCS += xmlstring.c


include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

