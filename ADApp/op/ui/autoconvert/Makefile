
# file: Makefile
# purpose: convert MEDM .adl files to caQtDM .ui files

# suggested environment definitions:
# export CAQTDM_DISPLAY_PATH=./op/ui/autoconvert
# to include stylesheet.qss
# export CAQTDM_DISPLAY_PATH+=:$(STYLESHEET_QSS_DIR)
# export EPICS_DISPLAY_PATH=./op/adl
#
# also, be sure that  adl2ui  is on your path


ADLs := $(filter-out $(wildcard *_BAK.adl), $(wildcard ../../adl/*.adl))
UIs := $(subst ../../adl/,,$(ADLs:.adl=.ui))


.PHONY: all 

all :: $(UIs)


%.ui: ../../adl/%.adl Makefile
	adl2ui $<
