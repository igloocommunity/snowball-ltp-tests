#    Disabled test   |                        Reason
# -------------------|--------------------------------------------------------
#    pwrmgmt         |        Specific hw/sw requirements


-include $(abspath $(TOPLEVEL)/../../ltp/.config)
# If TOPLEVEL path doesn't exist test local path
ifeq ($(realpath $(TOPLEVEL)/../../ltp/.config),)
include $(LTP_TEST_CONFIG_FILE)
endif

#obj-$(CONFIG_LTP_ALSA_I2C) += alsa_i2c/
#obj-$(CONFIG_LTP_SENSORS_AB8500) += sensors_ab8500/
#obj-$(CONFIG_LTP_METATEST) += metatest/
#obj-$(CONFIG_LTP_EXAMPLE) += example/
obj-$(CONFIG_LTP_DMA) += dma/
#obj-$(CONFIG_LTP_RTC) += rtc/
#obj-$(CONFIG_LTP_BLUETOOTH) += ste_conn/
#obj-$(CONFIG_LTP_GPIO) += gpio/
#obj-$(CONFIG_LTP_MCDE) += mcde/
#obj-$(CONFIG_LTP_B2R2) += b2r2/
#obj-$(CONFIG_LTP_UART) += uart/
#obj-$(CONFIG_LTP_SSP) += ssp/
#obj-$(CONFIG_LTP_SHRM) += shrm/
#obj-$(CONFIG_LTP_I2C) += i2c/
#obj-$(CONFIG_LTP_DLP) += dlp/
#obj-$(CONFIG_LTP_MMC) += mmc/
#obj-$(CONFIG_LTP_EMMC_FILESYSTEM_MMC_VALIDATION_LTP) += emmc_filesystem_mmc_Validation_LTP/
#obj-$(CONFIG_LTP_MSP) += msp/
#obj-$(CONFIG_LTP_ALS) += als/
#obj-$(CONFIG_LTP_LSM303DLH) += lsm303dlh/
#obj-$(CONFIG_LTP_LPS001WP) += lps001wp_prs/
#obj-$(CONFIG_LTP_L3G4200D) += l3g4200d/
#obj-$(CONFIG_LTP_PROXIMITY) += proximity/
#obj-$(CONFIG_LTP_PWRMGMT) += pwrmgmt/
#obj-$(CONFIG_LTP_TEE) += tee/
#obj-$(CONFIG_LTP_HIGHRES_TIMER) += highres_timer/
#obj-$(CONFIG_LTP_TIMER) += timer/
#obj-$(CONFIG_LTP_HSI) += hsi_test/
#obj-$(CONFIG_LTP_LED) += led/
#obj-$(CONFIG_LTP_VIBRATOR) += vibrator/
#obj-$(CONFIG_LTP_L2CACHE) += l2cache/

ifneq ($(KERNELRELEASE),)

# kernel modules should be built by descending here like this:
# obj-y := dir1/ dir2/

else

all:
	@set -e; for i in $(obj-y); do $(MAKE) -C $$i ; done

install:
	@set -e; for i in $(obj-y); do $(MAKE) -C $$i install ; done

clean:
	@set -e; for i in $(obj-y) ; do $(MAKE) -C $$i clean ; done

endif
