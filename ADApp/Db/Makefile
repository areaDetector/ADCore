TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this

# General
DB += ADBase.template
DB += ADPrefixes.template

# Drivers
DB += BIS.template
DB += PerkinElmer.template
DB += URLDriver.template
DB += adsc.template
DB += andorCCD.template
DB += firewireColorCodes.template
DB += firewireDCAM.template
DB += firewireFeature.template
DB += firewireVideoModes.template
DB += firewireWhiteBalance.template
DB += mar345.template
DB += marCCD.template
DB += pilatus.template
DB += prosilica.template
DB += pvCam.template
DB += roper.template
DB += simDetector.template

# Plugins
DB += NDColorConvert.template
DB += NDFile.template
DB += NDFileJPEG.template
DB += NDFileMagick.template
DB += NDFileNetCDF.template
DB += NDFileNexus.template
DB += NDFileTIFF.template
DB += NDOverlay.template
DB += NDOverlayN.template
DB += NDPluginBase.template
DB += NDProcess.template
DB += NDROI.template
DB += NDStats.template
DB += NDStdArrays.template
DB += NDTransform.template

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_template = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

