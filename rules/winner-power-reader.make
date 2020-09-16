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
PACKAGES-$(PTXCONF_WINNER_POWER_READER) += winner-power-reader

#
# Paths and names
#
WINNER_POWER_READER_VERSION	:= 0.1
WINNER_POWER_READER			:= winner-power-reader
WINNER_POWER_READER_URL		:= file://$(SRCDIR)/$(WINNER_POWER_READER)
WINNER_POWER_READER_DIR		:= $(BUILDDIR)/$(WINNER_POWER_READER)
WINNER_POWER_READER_SOURCE	:= $(SRCDIR)/$(WINNER_POWER_READER)
WINNER_POWER_READER_LICENSE	:= MIT

CDUP = ..

# ----------------------------------------------------------------------------
# Get
# ----------------------------------------------------------------------------

$(STATEDIR)/winner-power-reader.get:
	@$(call targetinfo, $@)
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Extract
# ----------------------------------------------------------------------------

$(STATEDIR)/winner-power-reader.extract:
	@$(call targetinfo, $@)
	@$(call clean, $(WINNER_POWER_READER_DIR))
	cp -rd $(WINNER_POWER_READER_SOURCE) $(WINNER_POWER_READER_DIR)
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Prepare
# ----------------------------------------------------------------------------

WINNER_POWER_READER_PATH	:= PATH=$(CROSS_PATH)
WINNER_POWER_READER_ENV	:= $(CROSS_ENV)

$(STATEDIR)/winner-power-reader.prepare:
	@$(call targetinfo, $@)
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Compile
# ----------------------------------------------------------------------------

$(STATEDIR)/winner-power-reader.compile:
	@$(call targetinfo, $@)
	@cd $(WINNER_POWER_READER_DIR) && \
		$(WINNER_POWER_READER_ENV) $(WINNER_POWER_READER_PATH) DIST_DIR=$(PTXDIST_PLATFORMDIR) \
		env \
		DIST_DIR=$(PTXDIST_PLATFORMDIR) CROSS_COMPILE=$(COMPILER_PREFIX) \
		$(MAKE)			
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------------

$(STATEDIR)/winner-power-reader.install:
	@$(call targetinfo, $@)
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/winner-power-reader.targetinstall:
	@$(call targetinfo, $@)
	@$(call install_init, winner-power-reader)
	@$(call install_fixup, winner-power-reader, PRIORITY, optional)
	@$(call install_fixup, winner-power-reader, VERSION, $(WINNER_POWER_READER_VERSION))	
	@$(call install_fixup, winner-power-reader, SECTION, base)
	@$(call install_fixup, winner-power-reader, AUTHOR, "Peter Conrad <peter.conrad@uni-jena.de>")
	@$(call install_fixup, winner-power-reader, DESCRIPTION, missing)

	@$(call install_copy, winner-power-reader, 0, 0, 0755, $(WINNER_POWER_READER_DIR)/powerreader, /usr/bin/powerreader)

	@$(call install_finish, winner-power-reader)

	@$(call touch)

# ----------------------------------------------------------------------------
# Clean
# ----------------------------------------------------------------------------

winner-power-reader_clean:
	rm -rf $(STATEDIR)/winner-power-reader.*

# vim: syntax=make
