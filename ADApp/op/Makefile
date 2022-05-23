TOP = ../..
include $(TOP)/configure/CONFIG

# Generate UIs for all pvi yaml files
PVI_ADLS = adl/pvi
PVI_EDLS = edl/pvi
PVI_BOBS = bob/pvi
PVI_YAML_PATHS = $(TOP)/pvi
-include $(PVI)/configure/RULES_PVI

ADL_DIR = adl
UI_DIR  = ui/autoconvert
EDL_DIR = edl/autoconvert
OPI_DIR = opi/autoconvert
BOB_DIR = bob/autoconvert

include $(CONFIG)/RULES_DIRS

-include $(SUPPORT)/configure/RULES_OPI
