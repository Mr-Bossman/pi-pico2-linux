################################################################################
#
# pico-sdk
#
################################################################################

PICO_SDK2_VERSION = 2.1.1
PICO_SDK2_SITE = git://github.com/raspberrypi/pico-sdk.git
PICO_SDK2_SITE_METHOD = git
PICO_SDK2_GIT_SUBMODULES = YES
PICO_SDK2_LICENSE = BSD-3-Clause
PICO_SDK2_LICENSE_FILES = LICENSE.TXT
PICO_SDK2_INSTALL_STAGING = YES
# Header-only lib, as far as buildroot is concerned
PICO_SDK2_INSTALL_TARGET = NO

define PICO_SDK2_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/share/pico-sdk
	cp -r $(@D)/* $(STAGING_DIR)/usr/share/pico-sdk
endef

define HOST_PICO_SDK2_INSTALL_CMDS
	mkdir -p $(HOST_DIR)/usr/share/pico-sdk
	cp -r $(@D)/* $(HOST_DIR)/usr/share/pico-sdk
endef

$(eval $(generic-package))
$(eval $(host-generic-package))
