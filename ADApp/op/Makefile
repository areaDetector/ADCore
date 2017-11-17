
# file: Makefile
# purpose: convert MEDM .adl files to caQtDM .ui files

# suggested environment definitions:
# export CAQTDM_SRC=/APSshare/caqtdm/caqtdm-4.0.0
# export CAQTDM_DISPLAY_PATH=./op/ui/autoconvert
# to include stylesheet.qss
# export CAQTDM_DISPLAY_PATH+=:$(CAQTDM_SRC)/caQtDM_Viewer/parser
# export EPICS_DISPLAY_PATH=./op/adl


# alias adl2ui='/APSshare/bin/adl2ui-GIT '

ui/autoconvert/%.ui: adl/%.adl Makefile
	adl2ui $<
