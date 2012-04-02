#/*
# * Copyright (C) ST-Ericsson SA 2010
# * License Terms: GNU General Public License, version 2
# */

# Runs all test for DMA
TEST_MODULE_NAME=stedma40_test
TEST_DIR=/sys/kernel/debug/ste_dma40_test
TEST_TMP_FILE=/.dma_run_test.sh

echo "Loading test module $TEST_MODULE"
modprobe $TEST_MODULE_NAME
cd $TEST_DIR

cat test_22
cat test_23
cat test_24
cat test_25

cat test_1
cat test_2
cat test_3
cat test_4
cat test_6
cat test_7
cat test_8

# Testing dma using MMC
cat test_10 > $TEST_TMP_FILE
sh $TEST_TMP_FILE
cat test_11 > $TEST_TMP_FILE
sh $TEST_TMP_FILE
cat test_12 > $TEST_TMP_FILE
sh $TEST_TMP_FILE
cat test_13 > $TEST_TMP_FILE
sh $TEST_TMP_FILE

# Endless test case, skip it here
# cat test_14

cat test_21
cat test_15
cat test_16
cat test_17
cat test_18
cat test_19
cat test_20
cat test_21
cat test_55
cat test_56
cat test_57
cat test_58
cat test_59
cat test_60
cat test_61
cat test_62
cat test_63
cat test_64

cat test_26
# run long tests near the end
cat test_7
cat test_32
cat test_33
cat test_5
cat test_34
cat test_27
cat test_28
cat test_29

# The following fails:
# This due to hardware error V1, should be fixed in V2.
# cat test_30
# cat test_31
# cat test_35
# cat test_51
# These fail due to unknown reason.
# cat test_9
#

rmmod $TEST_MODULE_NAME
