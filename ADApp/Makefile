TOP = ..
include $(TOP)/configure/CONFIG

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))

DIRS += ADSrc

DIRS += netCDFSrc

DIRS += tiffSupport

DIRS += xml2Src

DIRS += szipSrc

DIRS += zlibSrc

DIRS += hdf5Src
hsf5Src_DEPEND_DIRS += szipSrc zlibSrc

DIRS += nexusSrc
nexusSrc_DEPEND_DIRS += hdf5Src

DIRS += pluginSrc
pluginSrc_DEPEND_DIRS += ADSrc netCDFSrc nexusSrc tiffSupport xml2Src hdf5Src

DIRS += pluginTests
pluginTests_DEPEND_DIRS += ADSrc pluginSrc

ifeq ($(EPICS_V4), YES)
  DIRS += ntndArrayConverterSrc
  ntndArrayConverterSrc_DEPEND_DIRS += ADSrc
  DIRS += pvaDriverSrc
  pvaDriverSrc_DEPEND_DIRS += ntndArrayConverterSrc
  pluginSrc_DEPEND_DIRS += ntndArrayConverterSrc
endif

include $(TOP)/configure/RULES_DIRS

