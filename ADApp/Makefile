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

include $(TOP)/configure/RULES_DIRS

