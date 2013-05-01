#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *app))
ifeq ($(BUILD_APPS), YES)
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocBoot))
endif
include $(TOP)/configure/RULES_TOP
