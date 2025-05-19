TOP = ..
include $(TOP)/configure/CONFIG

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))

DIRS += ADSrc

DIRS += pluginSrc
pluginSrc_DEPEND_DIRS += ADSrc

DIRS += pluginTests
pluginTests_DEPEND_DIRS += pluginSrc

ifeq ($(WITH_PVA), YES)
  DIRS += ntndArrayConverterSrc
  ntndArrayConverterSrc_DEPEND_DIRS += ADSrc
  pluginSrc_DEPEND_DIRS += ntndArrayConverterSrc
else ifeq ($(WITH_PVXS), YES)
  DIRS += ntndArrayConverterSrc
  ntndArrayConverterSrc_DEPEND_DIRS += ADSrc
  pluginSrc_DEPEND_DIRS += ntndArrayConverterSrc
endif

DIRS += op

include $(TOP)/configure/RULES_DIRS

