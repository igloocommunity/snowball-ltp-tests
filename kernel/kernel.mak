# Compile this package with small thumb code -
# not all packages work with this, then you have to comment
# this out or patch the package.
ifdef USE_THUMB
	CFLAGS := -mthumb -mthumb-interwork $(CFLAGS)
endif

BUILD_ROOT := testcases
KERNELDIR ?= ../../kernel

.PHONY: build
build: build-modules build-userspace

.PHONY: build-modules
build-modules: kernel.mak
	echo "$@"
	$(MAKE) --directory=$(KERNEL_OUTPUT) M=$(shell pwd) -j$(JOBS) modules

.PHONY: build-userspace
build-userspace: kernel.mak
	echo "$@"
	$(MAKE) --directory=$(BUILD_ROOT) -j$(JOBS) all


.PHONY: install
install: install-modules install-userspace

.PHONY: install-modules
install-modules: build-modules
	echo "$@"
ifneq ($(CROSS_COMPILE),)
	$(MAKE) --directory=$(KERNEL_OUTPUT) M=$(shell pwd) -j$(JOBS) modules_install
else
	$(MAKE) --directory=$(KERNEL_OUTPUT) M=$(shell pwd) INSTALL_MOD_PATH=$(DESTDIR) -j$(JOBS) modules_install
endif
	depmod

.PHONY: install-userspace
install-userspace: build-userspace
	echo "$@"
	cd $(BUILD_ROOT) && $(MAKE) install DESTDIR=$(DESTDIR)
	$(shell install -d $(DESTDIR)/opt/ltp/runtest)
	cp -f runtest/* $(DESTDIR)/opt/ltp/runtest

.PHONY: clean
clean:
	$(MAKE) --directory=$(BUILD_ROOT) clean
	$(MAKE) --directory=$(KERNELDIR) M=$(shell pwd) clean
	find . -name 'modules.order' -delete
