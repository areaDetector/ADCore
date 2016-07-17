TOP = ..
include $(TOP)/configure/CONFIG

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))

DIRS += ADSrc

DIRS += netCDFSrc

DIRS += nexusSrc

DIRS += tiffSupport

DIRS += pluginSrc
pluginSrc_DEPEND_DIRS += ADSrc netCDFSrc nexusSrc tiffSupport

DIRS += pluginTests
pluginTests_DEPEND_DIRS += ADSrc pluginSrc

ifeq ($(EPICS_V4), YES)
  DIRS += ntndArrayConverterSrc
  DIRS += pvaDriverSrc
  pvaDriverSrc_DEPEND_DIRS += pluginSrc
  pvaDriverSrc_DEPEND_DIRS += ntndArrayConverterSrc
  pluginSrc_DEPEND_DIRS += ntndArrayConverterSrc
endif

include $(TOP)/configure/RULES_DIRS

