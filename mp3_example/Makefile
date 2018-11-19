#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := lvgl_mp3_example
#If LVGL_COMPONENT_PATH is not defined, use relative path as default value
LVGL_COMPONENT_PATH ?= $(abspath $(shell pwd)/../)

ADF_VER := $(shell cd ${ADF_PATH} && git describe --always --tags --dirty)
EXTRA_COMPONENT_DIRS += $(ADF_PATH)/components/
CPPFLAGS := -D ADF_VER=\"$(ADF_VER)\"

include $(LVGL_COMPONENT_PATH)/components/component_conf.mk
include $(IDF_PATH)/make/project.mk