################################################################################
#
# newlib
#
################################################################################

NEWLIB_VERSION = 4.5.0.20241231
NEWLIB_SITE = https://sourceware.org/ftp/newlib
NEWLIB_SOURCE = newlib-$(NEWLIB_VERSION).tar.gz
NEWLIB_ADD_TOOLCHAIN_DEPENDENCY = NO
NEWLIB_LICENSE = GPL-2.0, GPL-3.0, LGPL-2.1, LGPL-3.0
NEWLIB_LICENSE_FILES = \
	COPYING \
	COPYING.LIB \
	COPYING.LIBGLOSS \
	COPYING.NEWLIB
NEWLIB_CPE_ID_VENDOR = newlib_project
NEWLIB_CPE_ID_PRODUCT = newlib

NEWLIB_INSTALL_STAGING = YES
NEWLIB_INSTALL_TARGET = NO
NEWLIB_MAKE_OPTS = MAKEINFO=true

NEWLIB_ARCH_TUPLE = $(notdir $(patsubst %-,%,$(TARGET_CROSS)))
NEWLIB_SYSROOT = $(HOST_DIR)/$(NEWLIB_ARCH_TUPLE)-newlib/sysroot
NEWLIB_CONF_ENV = CC="$(TARGET_CC)"

NEWLIB_CONF_OPTS = \
	--build=$(NEWLIB_ARCH_TUPLE) \
	--target=$(NEWLIB_ARCH_TUPLE) \
	--prefix=/usr \
	--exec-prefix=/usr \
	--sysconfdir=/etc \
	--localstatedir=/var \
	--program-prefix="" \
	$(if $$($$(PKG)_OVERRIDE_SRCDIR),,--disable-dependency-tracking) \
	$(QUIET) \
	--enable-newlib-io-c99-formats \
	--enable-newlib-io-long-long \
	--enable-newlib-io-float \
	--enable-newlib-io-long-double \
	--disable-multilib \
	--with-tooldir=/usr

define NEWLIB_CONFIGURE_CMDS
	mkdir -p $(@D)/build
	cd $(@D)/build && \
	PATH=$(BR_PATH) \
	CONFIG_SITE=/dev/null \
	$(NEWLIB_CONF_ENV) \
	$(@D)/newlib/configure \
	$(NEWLIB_CONF_OPTS)
endef

define NEWLIB_BUILD_CMDS
	PATH=$(BR_PATH) $(MAKE) $(NEWLIB_MAKE_OPTS) -C $(@D)/build
endef

define NEWLIB_INSTALL_STAGING_CMDS
	PATH=$(BR_PATH) $(MAKE1) $(NEWLIB_MAKE_OPTS) -C $(@D)/build \
		DESTDIR=$(NEWLIB_SYSROOT) install
	touch $(NEWLIB_SYSROOT)/usr/lib/nosys.specs
endef

$(eval $(autotools-package))
