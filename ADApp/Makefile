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
# On some platforms hdf5Support must be built before nexus because of library dependencies
nexusSrc_DEPEND_DIRS = hdf5Support

DIRS += pvcamSupport

DIRS += perkinElmerSupport

DIRS += magickSupport

DIRS += andorSupport

DIRS += andor3Support

DIRS += pluginSrc
pluginSrc_DEPEND_DIRS = ADSrc netCDFSrc prosilicaSupport tiffSupport hdf5Support nexusSrc magickSupport

DIRS += simDetectorSrc
simDetectorSrc_DEPEND_DIRS = pluginSrc

ifeq ($(USE_GRAPHICSMAGICK), YES)
DIRS += URLSrc
URLSrc_DEPEND_DIRS = pluginSrc
endif

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

DIRS += firewireWinSrc
firewireWinSrc_DEPEND_DIRS = pluginSrc

DIRS += perkinElmerSrc
perkinElmerSrc_DEPEND_DIRS = perkinElmerSupport pluginSrc

DIRS += pvcamSrc
pvcamSrc_DEPEND_DIRS = pluginSrc pvcamSupport

DIRS += brukerSrc
brukerSrc_DEPEND_DIRS = pluginSrc

DIRS += PSLSrc
PSLSrc_DEPEND_DIRS = pluginSrc

DIRS += andorSrc
andorSrc_DEPEND_DIRS = andorSupport pluginSrc

DIRS += andor3Src
andor3Src_DEPEND_DIRS = andor3Support pluginSrc

DIRS += roperSrc
roperSrc_DEPEND_DIRS = pluginSrc

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

