TOP = ../..
include $(TOP)/configure/CONFIG

# define these as the full paths to the executables if they are not in the PATH
ADL2UI = adl2ui
ADL2EDL = adl2edl
CSS = css -nosplash -application org.csstudio.opibuilder.adl2boy.application


# these environment definitions are suggested to suppress
# console notices when converting:
# 
# CAQTDM_DISPLAY_PATH
#    description:
#        defines paths to directories containing (possibly) 
#        included screen files
#    adl2ui notice:
#        compositeFileParse: Cannot open file: /usr/local/synApps/support/areaDetector/ADSimDetector/simDetectorApp/op/adl/ADSetup.adl
#    example definition:
#        export CAQTDM_DISPLAY_PATH=/usr/local/synApps/support/areaDetector/ADCore/ADApp/op/ui/autoconvert
#
# stylesheet.qss
#    description:
#        to include "stylesheet.qss" and suppress the notice from 
#        adl2ui, append its directory to CAQTDM_DISPLAY_PATH
#    adl2ui notice:
#        adl2ui -- file = <../../adl/simDetector.adl>
#        adl2ui -- file <stylesheet.qss> could not be loaded, is 'CAQTDM_DISPLAY_PATH' </usr/local/epics/extensions/src/caQtDM/caQtDM_Tests/> defined?
#        adl2ui -- could be a problem!
#    example definition:
#        export STYLESHEET_QSS_DIR=/usr/local/epics/synApps/support/std/stdApp/op/ui/stylesheet.qss
#        export CAQTDM_DISPLAY_PATH+=:$(STYLESHEET_QSS_DIR)


ADL_DIR =adl
UI_DIR  =ui/autoconvert/
EDL_DIR =edl/autoconvert/
OPI_DIR =opi/autoconvert/

include $(CONFIG)/RULES_OPI

