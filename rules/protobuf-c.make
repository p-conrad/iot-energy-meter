# -*-makefile-*-
#
# Copyright (C) 2020 by Peter Conrad <peter.conrad@uni-jena.de>
#
# See CREDITS for details about who has contributed to this project.
#
# For further information about the PTXdist project and license conditions
# see the README file.
#

#
# We provide this package
#
PACKAGES-$(PTXCONF_PROTOBUF_C) += protobuf-c

#
# Paths and names
#
PROTOBUF_C_VERSION	:= 1.3.3
PROTOBUF_C_MD5		:= dabc05a5f11c21b96d8d6db4153f5343
PROTOBUF_C			:= protobuf-c-$(PROTOBUF_C_VERSION)
PROTOBUF_C_SUFFIX	:= tar.gz
PROTOBUF_C_URL		:= https://github.com/protobuf-c/protobuf-c/releases/download/v$(PROTOBUF_C_VERSION)/$(PROTOBUF_C).$(PROTOBUF_C_SUFFIX)
PROTOBUF_C_SOURCE	:= $(SRCDIR)/$(PROTOBUF_C).$(PROTOBUF_C_SUFFIX)
PROTOBUF_C_DIR		:= $(BUILDDIR)/$(PROTOBUF_C)
PROTOBUF_C_LICENSE	:= unknown

#
# autoconf
#
PROTOBUF_C_CONF_TOOL	:= autoconf
#PROTOBUF_C_CONF_OPT	:= $(CROSS_AUTOCONF_USR)

#$(STATEDIR)/protobuf-c.prepare:
#	@$(call targetinfo)
#	@$(call clean, $(PROTOBUF_C_DIR)/config.cache)
#	cd $(PROTOBUF_C_DIR) && \
#		$(PROTOBUF_C_PATH) $(PROTOBUF_C_ENV) \
#		./configure $(PROTOBUF_C_CONF_OPT)
#	@$(call touch)

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/protobuf-c.targetinstall:
	@$(call targetinfo)

	@$(call install_init, protobuf-c)
	@$(call install_fixup, protobuf-c,PRIORITY,optional)
	@$(call install_fixup, protobuf-c,SECTION,base)
	@$(call install_fixup, protobuf-c,AUTHOR,"Peter Conrad <peter.conrad@uni-jena.de>")
	@$(call install_fixup, protobuf-c,DESCRIPTION,missing)

	@$(call install_lib, protobuf-c, 0, 0, 0644, libprotobuf-c)

	@$(call install_finish, protobuf-c)

	@$(call touch)
