TOP = ..
include $(TOP)/configure/CONFIG

DIRS += ADSrc
DIRS += netCDFSrc
# prosilicaSupport must be built before pluginSrc or prosilicaSrc because it installs
# header and library files needed by both
DIRS += prosilicaSupport
DIRS += tiffSupport
DIRS += nexusSupport
DIRS += pvcamSupport
DIRS += pluginSrc
pluginSrc_DEPEND_DIRS = ADSrc netCDFSrc prosilicaSupport
DIRS += simDetectorSrc
simDetectorSrc_DEPEND_DIRS = pluginSrc
DIRS += prosilicaSrc
prosilicaSrc_DEPEND_DIRS = pluginSrc
DIRS += pilatusSrc
pilatusSrc_DEPEND_DIRS   = pluginSrc
DIRS += marCCDSrc
marCCDSrc_DEPEND_DIRS = pluginSrc
DIRS += mar345Src
mar345Src_DEPEND_DIRS = pluginSrc
DIRS += adscSrc
adscSrc_DEPEND_DIRS = pluginSrc
DIRS += roperSrc
roperSrc_DEPEND_DIRS = pluginSrc
DIRS += firewireWinSrc
firewireWinSrc_DEPEND_DIRS = pluginSrc
DIRS += perkinElmerSrc
perkinElmerSrc_DEPEND_DIRS = pluginSrc
DIRS += pvcamSrc
pvcamSrc_DEPEND_DIRS = pluginSrc pvcamSupport
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

