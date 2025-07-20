################################################################################
#
# psram-bootloader
#
################################################################################

PSRAM_BOOTLOADER_VERSION = 1.0
PSRAM_BOOTLOADER_SITE = $(BR2_EXTERNAL_RPI_PICO2_PATH)/psram-bootloader
PSRAM_BOOTLOADER_SITE_METHOD = local
PSRAM_BOOTLOADER_DEPENDENCIES = pico-sdk2 host-picotool2 host-cmake newlib

PSRAM_BOOTLOADER_INSTALL_TARGET = NO
PSRAM_BOOTLOADER_INSTALL_IMAGES = YES

PSRAM_BOOTLOADER_MARCH = -march=rv32imac_zicsr_zifencei_zbs_zbb_zbkb -mabi=ilp32
PSRAM_BOOTLOADER_CONF_OPTS = -DPICO_SDK_PATH=$(STAGING_DIR)/usr/share/pico-sdk \
				-DCMAKE_C_FLAGS="$(PSRAM_BOOTLOADER_MARCH)" \
				-DCMAKE_ASM_FLAGS="$(PSRAM_BOOTLOADER_MARCH)" \
				-DCMAKE_CXX_FLAGS="-DPICO_CXX_DISABLE_ALLOCATION_OVERRIDES" \
				-DCMAKE_SYSROOT="$(NEWLIB_SYSROOT)" \
				-DCMAKE_EXE_LINKER_FLAGS="-nostartfiles" \
				-DCMAKE_EXECUTABLE_SUFFIX_CXX=".elf"

define PSRAM_BOOTLOADER_INSTALL_IMAGES_CMDS
	$(INSTALL) -D -m 0644 $(@D)/psram-bootloader.uf2 $(BINARIES_DIR)/psram-bootloader.uf2
endef

$(eval $(cmake-package))
