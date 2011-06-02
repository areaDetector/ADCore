TOP = ..
include $(TOP)/configure/CONFIG

DIRS += ADSrc
DIRS += netCDFSrc
DIRS += cbfSrc
# prosilicaSupport must be built before pluginSrc or prosilicaSrc because it installs
# header and library files needed by both
DIRS += prosilicaSupport
DIRS += tiffSupport
DIRS += hdf5Support
DIRS += nexusSrc
DIRS += pvcamSupport
DIRS += magickSupport
DIRS += pluginSrc
pluginSrc_DEPEND_DIRS = ADSrc netCDFSrc prosilicaSupport tiffSupport hdf5Support nexusSrc magickSupport
DIRS += simDetectorSrc
simDetectorSrc_DEPEND_DIRS = pluginSrc
DIRS += URLSrc
URLSrc_DEPEND_DIRS = pluginSrc
DIRS += prosilicaSrc
prosilicaSrc_DEPEND_DIRS = pluginSrc
DIRS += pilatusSrc
pilatusSrc_DEPEND_DIRS   = pluginSrc cbfSrc
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
DIRS += brukerSrc
brukerSrc_DEPEND_DIRS = pluginSrc
DIRS += PSLSrc
PSLSrc_DEPEND_DIRS = pluginSrc
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

