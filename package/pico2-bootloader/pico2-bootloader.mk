
################################################################################
#
# PICO2_BOOTLOADER
#
################################################################################

PICO2_BOOTLOADER_VERSION = 1.0
PICO2_BOOTLOADER_SITE = $(PICO2_BOOTLOADER_PKGDIR)/bootloader
PICO2_BOOTLOADER_SITE_METHOD = local
PICO2_BOOTLOADER_INSTALL_IMAGES = YES
PICO2_BOOTLOADER_INSTALL_TARGET = NO

define PICO2_BOOTLOADER_BUILD_CMDS
	$(MAKE) CROSS_COMPILE="$(TARGET_CROSS)" -C $(@D) bootloader.bin
endef

define PICO2_BOOTLOADER_INSTALL_IMAGES_CMDS
	cp $(@D)/bootloader.bin $(BINARIES_DIR)/
endef

$(eval $(generic-package))
