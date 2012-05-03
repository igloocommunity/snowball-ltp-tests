# Standalone build file for LTP

# Edit .my_build_settings to set proper values for these variables
# ARCH 		/* arm */
# CROSS_PREFIX	/* arm-none-linux-gnueabi */
# CROSS_COMPILE	/* arm-none-linux-gnueabi- */
# KERNEL_OUTPUT	/* kernel build directory */
# DESTDIR	/* installation output directory */
-include .my_build_settings
ARCH ?= arm
KERNEL_OUTPUT ?= /lib/modules/$(shell uname -r)/build
DESTDIR ?= $(CURDIR)/build

# Default config file
LTP_CONFIG ?= snowball_defconfig
# Number of parallell make jobs
JOBS ?= 1
# ------------

# TODO: remove this export and pass command line variables to make instead.
# Setting CC is required in order to build userspace parts of kernel tests.
export CC=$(CROSS_COMPILE)gcc
# --------------------

# The following variables should have ok default values
# ----------------

# tempdir can't be changed easily because it is hardcoded in the Makefiles
TEMPDIR := $(CURDIR)/tempdir

# Local script directory
SCRIPT_DIR := $(CURDIR)/scripts
BUILD_PREFIX := i386-pc-linux-gnu
HOST_PREFIX := $(CROSS_PREFIX)
TARGET_PREFIX := $(CROSS_PREFIX)
# ----------------

# Pass variables to make
MAKEFLAGS :=
MAKEFLAGS += CROSS_COMPILE=$(CROSS_COMPILE)
MAKEFLAGS += CROSS_PREFIX=$(CROSS_PREFIX)
MAKEFLAGS += ARCH=$(ARCH)
MAKEFLAGS += KERNEL_OUTPUT=$(KERNEL_OUTPUT)
MAKEFLAGS += KERNELDIR=$(KERNEL_OUTPUT)
MAKEFLAGS += DESTDIR=$(DESTDIR)
MAKEFLAGS += TEMPDIR=$(TEMPDIR)
MAKEFLAGS += HOST_PREFIX=$(HOST_PREFIX)
MAKEFLAGS += TARGET_PREFIX=$(TARGET_PREFIX)
MAKEFLAGS += CONFIG_LTP_STE_TESTCASES=$(CONFIG_LTP_STE_TESTCASES)
MAKEFLAGS += JOBS=$(JOBS)
MAKEFLAGS += SCRIPT_DIR=$(SCRIPT_DIR)

# TODO: Replace this export and pass value via make
export LTP_TEST_CONFIG_FILE=$(CURDIR)/.config
.PHONY help:
help: FORCE
	@echo "Please read the Makefile for a list of make targets."
	@echo "Overview of build flow:"
	@echo "1. make menuconfig OR make config (set default configuration."
	@echo "   Check the defaults to make sure this is what you want"
	@echo "2. make tests (builds the out of tree tests according to the config from step #1"
	@echo "3. make install (installs the out of tree tests built in step #3"
	@echo "4. make clean (remove build content, doesn't uninstall content from step #4"

menuconfig: FORCE
	$(MAKE) -r --directory=$(CURDIR)/kconfig mconf
	$(CURDIR)/kconfig/mconf $(CURDIR)/config/Kconfig

.PHONY config:
config: FORCE
	cp config/$(LTP_CONFIG) .config

# Build the out of tree tests
.PHONY tests:
tests: FORCE
	$(MAKE) --directory=kernel -f kernel.mak $(MAKEFLAGS) build

# Install the out of tree tests
.PHONY install:
install: FORCE
	$(MAKE) --directory=kernel -f kernel.mak $(MAKEFLAGS) install
	cp -r $(CURDIR)/ltp_framework/* $(DESTDIR)/opt/ltp

.PHONY: clean
clean:
	$(MAKE) --directory=kernel -f kernel.mak $(MAKEFLAGS) clean

FORCE:
