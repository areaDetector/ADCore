TOP = ..
include $(TOP)/configure/CONFIG

DIRS += ADSrc
DIRS += netCDFSrc
DIRS += pluginSrc
DIRS += simDetectorSrc
DIRS += prosilicaSrc
DIRS += pilatusSrc
DIRS += marCCDSrc
# Uncomment this line if you have the header files for ADSC on Linux
#DIRS += adscSrc
DIRS += roperSrc
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

