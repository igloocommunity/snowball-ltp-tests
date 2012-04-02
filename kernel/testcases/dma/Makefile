ifneq ($(KERNELRELEASE),)

obj-y := kernel_space/

else

SUBDIRS := user-space

all:
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i ; done

install:
	@set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i install ; done

clean:
	@set -e; for i in $(SUBDIRS) ; do $(MAKE) -C $$i clean ; done

endif
