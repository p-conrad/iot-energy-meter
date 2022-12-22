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
PACKAGES-$(PTXCONF_IOT_ENERGY_METER) += iot-energy-meter

#
# Paths and names
#
IOT_ENERGY_METER_VERSION	:= 0.5
IOT_ENERGY_METER			:= iot-energy-meter
IOT_ENERGY_METER_URL		:= file://$(SRCDIR)/$(IOT_ENERGY_METER)
IOT_ENERGY_METER_DIR		:= $(BUILDDIR)/$(IOT_ENERGY_METER)
IOT_ENERGY_METER_SOURCE	:= $(SRCDIR)/$(IOT_ENERGY_METER)
IOT_ENERGY_METER_LICENSE	:= MIT

CDUP = ..

# ----------------------------------------------------------------------------
# Get
# ----------------------------------------------------------------------------

$(STATEDIR)/iot-energy-meter.get:
	@$(call targetinfo, $@)
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Extract
# ----------------------------------------------------------------------------

$(STATEDIR)/iot-energy-meter.extract:
	@$(call targetinfo, $@)
	@$(call clean, $(IOT_ENERGY_METER_DIR))
	cp -rd $(IOT_ENERGY_METER_SOURCE) $(IOT_ENERGY_METER_DIR)
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Prepare
# ----------------------------------------------------------------------------

IOT_ENERGY_METER_PATH	:= PATH=$(CROSS_PATH)
IOT_ENERGY_METER_ENV	:= $(CROSS_ENV)

$(STATEDIR)/iot-energy-meter.prepare:
	@$(call targetinfo, $@)
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Compile
# ----------------------------------------------------------------------------

$(STATEDIR)/iot-energy-meter.compile:
	@$(call targetinfo, $@)
	@cd $(IOT_ENERGY_METER_DIR) && \
		$(IOT_ENERGY_METER_ENV) $(IOT_ENERGY_METER_PATH) DIST_DIR=$(PTXDIST_PLATFORMDIR) \
		env \
		DIST_DIR=$(PTXDIST_PLATFORMDIR) CROSS_COMPILE=$(COMPILER_PREFIX) \
		$(MAKE)
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------------

$(STATEDIR)/iot-energy-meter.install:
	@$(call targetinfo, $@)
	@$(call touch, $@)

# ----------------------------------------------------------------------------
# Target-Install
# ----------------------------------------------------------------------------

$(STATEDIR)/iot-energy-meter.targetinstall:
	@$(call targetinfo, $@)
	@$(call install_init, iot-energy-meter)
	@$(call install_fixup, iot-energy-meter, PRIORITY, optional)
	@$(call install_fixup, iot-energy-meter, VERSION, $(IOT_ENERGY_METER_VERSION))
	@$(call install_fixup, iot-energy-meter, SECTION, base)
	@$(call install_fixup, iot-energy-meter, AUTHOR, "Peter Conrad <peter.conrad@uni-jena.de>")
	@$(call install_fixup, iot-energy-meter, DESCRIPTION, missing)

	@$(call install_copy, iot-energy-meter, 0, 0, 0755, $(IOT_ENERGY_METER_DIR)/energymeter, /usr/bin/energymeter)

	@$(call install_finish, iot-energy-meter)

	@$(call touch)

# ----------------------------------------------------------------------------
# Clean
# ----------------------------------------------------------------------------

iot-energy-meter_clean:
	rm -rf $(STATEDIR)/iot-energy-meter.*

# vim: syntax=make
