TOP = ..
include $(TOP)/configure/CONFIG

DIRS += ADSrc
DIRS += netCDFSrc
# Note that prosilicaSrc must be built before pluginSrc because it 
# installs include and library files needed by NDPluginColorConvert
DIRS += prosilicaSrc
DIRS += pluginSrc
DIRS += simDetectorSrc
DIRS += pilatusSrc
DIRS += marCCDSrc
# Uncomment this line if you have the header files for ADSC on Linux
#DIRS += adscSrc
DIRS += roperSrc
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

