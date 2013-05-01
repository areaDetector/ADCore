#NOTES
#     - To remove detectors from the build, delete or comment out the detector
#       in the <areaDetector>/configure/CONFIG_SITE file; not here.

# Note the only dependencies that matter in $(<detector>)_DEPEND_DIRS are
# compile-time dependencies.

TOP = ..
include $(TOP)/configure/CONFIG

DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))

define  AD_defined
  ifdef $(1)
  SUPPORT_DIRS  += $($(1))
   endif  
endef

PLUGIN_LIST = NETCDF_SRC PROSILICA_SUP TIFF_SUP HDF5_SUP NEXUS_SRC MAGICK_SUP PLUGIN_SRC
$(foreach plugin, $(PLUGIN_LIST), $(eval $(call AD_defined,$(plugin)) ))

$(NEXUS_SRC)_DEPEND_DIRS   = $(HDF5_SUP)
$(PLUGIN_SRC)_DEPEND_DIRS  = ADSrc $(NETCDF_SRC) $(PROSILICA_SUP) $(TIFF_SUP)
$(PLUGIN_SRC)_DEPEND_DIRS += $(HDF5_SUP) $(NEXUS_SRC) $(MAGIK_SUP)

DET_LIST  = SIM_DETECTOR PROSILICA PILATUS MAR_CCD MAR_345 ADSC
DET_LIST += FIREWIRE_WIN PERKIN_ELMER PVCAM BRUKER PSL ANDOR 
DET_LIST += ANDOR3 ROPER URL
$(foreach det, $(DET_LIST), $(eval $(call AD_defined,$(det)) ))

$(SIM_DETECTOR)_DEPEND_DIRS = pluginSrc
$(PROSILICA)_DEPEND_DIRS    = pluginSrc
$(PILATUS)_DEPEND_DIRS      = pluginSrc cbfSrc
$(MAR_CCD)_DEPEND_DIRS      = pluginSrc
$(MAR_345)_DEPEND_DIRS      = pluginSrc
$(ADSC)_DEPEND_DIRS         = pluginSrc
$(FIREWIRE_WIN)_DEPEND_DIRS = pluginSrc
$(PERKIN_ELMER)_DEPEND_DIRS = perkinElmerSupport pluginSrc
$(PVCAM)_DEPEND_DIRS        = pluginSrc pvcamSupport
$(BRUKER)_DEPEND_DIRS       = pluginSrc
$(PSL)_DEPEND_DIRS          = pluginSrc
$(ANDOR)_DEPEND_DIRS        = andorSupport pluginSrc
$(ANDOR3)_DEPEND_DIRS       = andor3Support pluginSrc
$(ROPER)_DEPEND_DIRS        = pluginSrc
$(SCINTX)_DEPEND_DIRS       = scintxSupport pluginSrc
$(QIMAGE)_DEPEND_DIRS       = qimageSupport pluginSrc

DIRS += ADSrc

ifdef PILATUS
DIRS += cbfSrc
endif
ifdef PVCAM
DIRS += pvcamSupport
endif
ifdef PERKIN_ELMER
DIRS += perkinElmerSupport
endif
ifdef ANDOR
DIRS += andorSupport
endif
ifdef ANDOR3
DIRS += andor3Support
endif

DIRS += $(SUPPORT_DIRS)

include $(TOP)/configure/RULES_DIRS

