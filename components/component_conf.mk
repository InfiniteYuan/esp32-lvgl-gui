#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

# LVGL_COMPONENT_PATH ?= $(abspath $(shell pwd)/../)

LVGL_COMPONENT_DIRS += $(LVGL_COMPONENT_PATH)/components
UGFX_COMPONENT_DIRS += $(UGFX_COMPONENT_PATH)/components/general
LVGL_COMPONENT_DIRS += $(LVGL_COMPONENT_PATH)/components/spi_devices
LVGL_COMPONENT_DIRS += $(LVGL_COMPONENT_PATH)/components/i2c_devices
LVGL_COMPONENT_DIRS += $(LVGL_COMPONENT_PATH)/components/i2c_devices/sensor
LVGL_COMPONENT_DIRS += $(LVGL_COMPONENT_PATH)/components/i2c_devices/others

EXTRA_COMPONENT_DIRS += $(LVGL_COMPONENT_DIRS)
