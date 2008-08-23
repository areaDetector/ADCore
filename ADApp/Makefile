TOP = ..
include $(TOP)/configure/CONFIG

DIRS += ADSrc
DIRS += netCDFSrc
DIRS += pluginSrc
DIRS += simDetectorSrc
DIRS += prosilicaSrc
DIRS += pilatusSrc
#DIRS += adscSrc
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

