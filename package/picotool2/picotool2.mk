################################################################################
#
# picotool
#
################################################################################

PICOTOOL2_VERSION = 2.1.1
PICOTOOL2_SITE = $(call github,raspberrypi,picotool,$(PICOTOOL2_VERSION))
HOST_PICOTOOL2_CONF_OPTS = -DPICO_SDK_PATH=$(HOST_DIR)/usr/share/pico-sdk
HOST_PICOTOOL2_DEPENDENCIES = host-libusb host-pico-sdk2 host-cmake
PICOTOOL2_CONF_OPTS = -DPICO_SDK_PATH=$(STAGING_DIR)/usr/share/pico-sdk
PICOTOOL2_DEPENDENCIES = libusb pico-sdk2 host-cmake
PICOTOOL2_LICENSE = BSD-3-Clause
PICOTOOL2_LICENSE_FILES = LICENSE.TXT

$(eval $(cmake-package))
$(eval $(host-cmake-package))
