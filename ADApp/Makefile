TOP = ..
include $(TOP)/configure/CONFIG

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))

DIRS += simDetectorSrc
simDetectorSrc_DEPEND_DIRS = pluginSrc

ifeq ($(EPICS_V4_PLUGIN), YES)
  DIRS += pvaDriverSrc
endif

DIRS += ADSrc

DIRS += pluginSrc
pluginSrc_DEPEND_DIRS += ADSrc netCDFSrc nexusSrc tiffSupport

DIRS += netCDFSrc

DIRS += nexusSrc
#nexusSrc_DEPEND_DIRS += ADBinaries

DIRS += tiffSupport

include $(TOP)/configure/RULES_DIRS

