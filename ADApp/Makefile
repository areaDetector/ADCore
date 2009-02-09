TOP = ..
include $(TOP)/configure/CONFIG

DIRS += ADSrc
DIRS += netCDFSrc
# prosilicaSupport must be built before pluginSrc or prosilicaSrc because it installs
# header and library files needed by both
DIRS += prosilicaSupport
DIRS += pluginSrc
pluginSrc_DEPEND_DIRS = ADSrc netCDFSrc
DIRS += simDetectorSrc
simDetectorSrc_DEPEND_DIRS = pluginSrc
DIRS += prosilicaSrc
prosilicaSrc_DEPEND_DIRS = pluginSrc
DIRS += pilatusSrc
pilatusSrc_DEPEND_DIRS   = pluginSrc
DIRS += marCCDSrc
marCCDSrc_DEPEND_DIRS = pluginSrc
DIRS += adscSrc
adscSrc_DEPEND_DIRS = pluginSrc
ifeq ($(EPICS_HOST_ARCH), cygwin-x86)
ifeq ($(CYGWIN_HAVE_AFXWIN_H), YES)
DIRS += roperSrc
roperSrc_DEPEND_DIRS = pluginSrc
endif
else
DIRS += roperSrc
roperSrc_DEPEND_DIRS = pluginSrc
endif

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

