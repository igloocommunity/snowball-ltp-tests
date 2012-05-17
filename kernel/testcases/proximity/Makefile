ifeq ($(KERNELRELEASE),)
LTP_DIR = $(abspath ../../../)
LTP_FULL = $(LTP_DIR)/tempdir/ltp-full

CFLAGS+= -I$(LTP_FULL)/include -I./include
LOADLIBES+= -L$(LTP_FULL)/lib -lltp

SRCS=$(wildcard *.c)
TARGETS=$(patsubst %.c,%,$(SRCS))

all: $(TARGETS)

install:
	@for i in $(TARGETS); do if [ -f $(DESTDIR)/opt/ltp/testcases/bin/$$i ]; then rm $(DESTDIR)/opt/ltp/testcases/bin/$$i; fi ; done
	@for i in $(TARGETS); do install $$i $(DESTDIR)/opt/ltp/testcases/bin/$$i ; done
	@for i in $(SCRIPTS); do if [ -f $(DESTDIR)/opt/ltp/testcases/bin/$$i ]; then rm $(DESTDIR)/opt/ltp/testcases/bin/$$i; fi ; done
	@for i in $(SCRIPTS); do install $$i $(DESTDIR)/opt/ltp/testcases/bin/$$i ; done

clean:
	rm -f $(TARGETS)
endif
