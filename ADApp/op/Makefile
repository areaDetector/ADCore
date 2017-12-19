TOP = ../..
include $(TOP)/configure/CONFIG
-include $(SUPPORT)/configure/CONFIG_SITE
-include $(SUPPORT)/configure/CONFIG_SITE.$(EPICS_HOST_ARCH)

# Note: parallel make cannot be used here because edl2adl uses a file called adltemp and parallel make
# will have multiple jobs modifying the file
ADL_DIR =adl
UI_DIR  =ui/autoconvert/
EDL_DIR =edl/autoconvert/
OPI_DIR =opi/autoconvert/

-include $(SUPPORT)/configure/RULES_OPI
