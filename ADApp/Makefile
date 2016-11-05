TOP = ..
include $(TOP)/configure/CONFIG

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))

DIRS += ADSrc

DIRS += pluginSrc
pluginSrc_DEPEND_DIRS += ADSrc

DIRS += pluginTests
pluginTests_DEPEND_DIRS += pluginSrc

ifeq ($(WITH_EPICS_V4), YES)
  DIRS += ntndArrayConverterSrc
  ntndArrayConverterSrc_DEPEND_DIRS += ADSrc
  DIRS += pvaDriverSrc
  pvaDriverSrc_DEPEND_DIRS += ntndArrayConverterSrc
  pluginSrc_DEPEND_DIRS += ntndArrayConverterSrc
endif

include $(TOP)/configure/RULES_DIRS

