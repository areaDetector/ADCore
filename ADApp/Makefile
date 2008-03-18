TOP = ..
include $(TOP)/configure/CONFIG

DIRS += ADSrc
DIRS += simDetectorSrc
DIRS += prosilicaSrc
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

