-include $(abspath $(TOPLEVEL)/../../ltp/.config)
ifeq ($(realpath $(TOPLEVEL)/../../ltp/.config),)
include $(LTP_TEST_CONFIG_FILE)
endif

obj-$(CONFIG_LTP_STE_TESTCASES) += testcases/
