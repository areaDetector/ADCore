# EXAMPLE_stats_profiles.cmd
# Copy to stats_profiles.cmd and include from st.cmd AFTER commonPlugins.cmd, e.g.:
#   < $(ADCORE)/iocBoot/commonPlugins.cmd
#   < $(ADCORE)/iocBoot/stats_profiles.cmd
#
# Requires: synApps calc (acalcout). Uses PREFIX, XSIZE, YSIZE from the IOC.
# Loads NDStatsProfiles.template: pixel/scaled axis waveforms + StatsProfInit_
# to enable ComputeProfiles / related flags for Phoebus XY plots.
#
# Records are named $(P)$(R)... (default R=Stats1:) so multiple cameras that share
# the same PREFIX remain unique. Example PVs with PREFIX=MPX3-TEST: :
#   MPX3-TEST:Stats1:StatsProfInit_
#   MPX3-TEST:Stats1:Cal:xSelAxisM / Cal:ySelAxisM
# Phoebus plot axes should use $(P)$(R)Cal:xSelAxisM.AVAL (and y), with R matching
# STATS_PROF_R / ProfileStats (typically Stats1:).

epicsEnvSet("STATS_PROF_R", "Stats1:")

dbLoadRecords("$(ADCORE)/db/NDStatsProfiles.template", "P=$(PREFIX),R=$(STATS_PROF_R),XNELM=$(XSIZE),YNELM=$(YSIZE)")

# After iocInit (or via autosave of ComputeProfiles):
#   dbpf "$(PREFIX)$(STATS_PROF_R)StatsProfInit_.PROC" 1
