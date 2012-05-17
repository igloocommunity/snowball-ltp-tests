/*
 * Copyright (C) 2011 ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 *
 * Author: 2011, Chethan Krishna N <chethan.krishna@stericsson.com>
 */


/******************************************************************************/
/* Description: Test x, y and z axis values from acceleromter and magnetometer
*              drivers.
*
* Test Assertion and Strategy: Assert if all three axes data is valid.
*
******************************************************************************/

/* Harness Specific Include Files. */
#include "test.h"
#include "usctest.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Extern Global Variables */
extern int Tst_count;		/* counter for tst_xxx routines.         */
extern char *TESTDIR;		/* temporary dir created by tst_tmpdir() */

/* Global Variables */
char *TCID = "lsm303dlh";	/* test program identifier.             */

/* total number of tests in this file. */
int TST_TOTAL = 1;
int id;

#define INVALID_DATA_READ -1
#define VALID_DATA 0
#define MAX_LENGTH 100

char acc_filepath[MAX_LENGTH] =          "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0018/data";
char acc_filepath_activate[MAX_LENGTH] = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0018/mode";
char acc_filepath_range[MAX_LENGTH] = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0018/range";
char acc_filepath_sleepwake[MAX_LENGTH] = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0018/sleep_wake";
char acc_filepath_id[MAX_LENGTH] = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0018/id";
char acc_filepath_rate[MAX_LENGTH] = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0018/rate";

char *mag_filepath_range = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-001e/range";
char *mag_filepath =          "/sys/devices/platform/nmk-i2c.2/i2c-2/2-001e/data";
char *mag_filepath_activate = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-001e/mode";
char *mag_filepath_rate = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-001e/rate";
char *mag_filepath_gain = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-001e/gain";

int get_val(char *fname)
{
	FILE *f;
	int data[3], ret;

	f = fopen(fname, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%8x:%8x:%8x", &data[0], &data[1], &data[2]) != 3)
		ret = INVALID_DATA_READ;
	else
		ret = VALID_DATA;

	fclose(f);
	return ret;
}

int acc_set_val()
{
	FILE *f;
	unsigned long val = -1;

	/* Before acrivating device try writting to rate,
	 * range and sleepwake & read out data to cover error cases.
	 */
	if(id == 50) {
		/*write sampling rate value */
		f = fopen(acc_filepath_rate, "w");
		if (f == NULL)
			return -1;
		fputc((int)'0', f);
		fclose(f);

		/*write sampling sleepwake value */
		f = fopen(acc_filepath_sleepwake, "w");
		if (f == NULL)
			return -1;
		fputc((int)'0', f);
		fclose(f);
	}

	/*write range value */
	f = fopen(acc_filepath_range, "w");
	if (f == NULL)
		return -1;
	fputc((int)'1', f);
	fclose(f);

	/*read the output data*/
	get_val(acc_filepath);

	f = fopen(acc_filepath_activate, "w");
	if (f == NULL)
		return -1;

	/* Activate LSM device */
	if (fputc((int)'1', f) == EOF)
		val = EOF;
	else
		val = 0;

	fclose(f);
	return val;
}

int acc_set_properties()
{
	FILE *f;
	unsigned long val = -1;

	if (id == 50) {
		f = fopen(acc_filepath_rate, "w");
		if (f == NULL)
			return -1;

		/*write sampling rate value */
		if (fputc((int)'0', f) == EOF)
			val = EOF;
		else
			val = 0;

		fclose(f);

		f = fopen(acc_filepath_sleepwake, "w");
		if (f == NULL)
			return -1;

		/*write sampling sleepwake value */
		if (fputc((int)'0', f) == EOF)
			val = EOF;
		else
			val = 0;

		fclose(f);
	}

	f = fopen(acc_filepath_range, "w");
	if (f == NULL)
		return -1;

	/*write all the range value specified in datasheet*/
	if (fputc((int)'1', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);

	if (fputc((int)'2', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);

	if (fputc((int)'3', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);

	if (fputc((int)'0', f) == EOF)
		val = EOF;
	else
		val = 0;

	fclose(f);

	return val;
}

int mag_set_properties()
{
	FILE *f;
	unsigned long val = -1;

	f = fopen(mag_filepath_rate, "w");
	if (f == NULL)
		return -1;

	/*write sampling rate value */
	if (fputc((int)'4', f) == EOF)
		val = EOF;
	else
		val = 0;

	fclose(f);

	f = fopen(mag_filepath_range, "w");
	if (f == NULL)
		return -1;

	/*write range value */
	if (fputc((int)'1', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);

	if (fputc((int)'2', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);

	if (fputc((int)'3', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);

	if (fputc((int)'5', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);

	if (fputc((int)'6', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);

	if (fputc((int)'7', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);
	if (fputc((int)'8', f) == EOF)
		val = EOF;
	else
		val = 0;
	fflush(f);
	if (fputc((int)'4', f) == EOF)
		val = EOF;
	else
		val = 0;
	fclose(f);

	return val;
}

int mag_set_val()
{
	FILE *f;
	unsigned long val = -1;

	/* Before acrivating device try writting to rate,
	 * range & read out value to cover error cases.
	 */
	/*write sampling rate value */
	f = fopen(mag_filepath_rate, "w");
	if (f == NULL)
		return -1;
	fputc((int)'4', f);
	fclose(f);

	/*write range value */
	f = fopen(mag_filepath_range, "w");
	if (f == NULL)
		return -1;

	fputc((int)'1', f);
	fclose(f);

	/*read the output data*/
	get_val(mag_filepath);

	f = fopen(mag_filepath_activate, "w");
	if (f == NULL)
		return -1;

	/* Activate LSM device */
	if (fputc((int)'0', f) == EOF)
		val = EOF;
	else
		val = 0;

	fclose(f);
	return val;
}

int acc_get_properties()
{
	FILE *f;
	int data;
	int ret = 4;

	/* read power mode */
	f = fopen(acc_filepath_activate, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		goto error;
	else
	    ret--;
	fclose(f);

	/* read range */
	f = fopen(acc_filepath_range, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		goto error;
	else
	    ret--;
	if(id == 50) {
		fclose(f);
		/* read data rate */
		f = fopen(acc_filepath_rate, "r");
		if (f == NULL)
			return -1;

		if (fscanf(f, "%d", &data) != 1)
			goto error;
		else
		    ret--;
		fclose(f);

		/* read sleep-to-wake */
		f = fopen(acc_filepath_sleepwake, "r");
		if (f == NULL)
			return -1;

		if (fscanf(f, "%d", &data) != 1)
			goto error;
		else
		    ret--;
	}
	else
		ret -=2;

	error:
		fclose(f);

	return ret;
}

int mag_get_properties()
{
	FILE *f;
	int data[3];
	int ret = 4;

	/* read power mode */
	f = fopen(mag_filepath_activate, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data[0]) != 1)
		goto error;
	else
	    ret--;
	fclose(f);

	/* read range */
	f = fopen(mag_filepath_range, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data[0]) != 1)
		goto error;
	else
	    ret--;
	fclose(f);

	/* read data rate */
	f = fopen(mag_filepath_rate, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data[0]) != 1)
		goto error;
	else
	    ret--;
	fclose(f);

	/* read gain */
	f = fopen(mag_filepath_gain, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%8x:%8x:%8x", &data[0], &data[1], &data[2]) != 3)
		goto error;
	else
	    ret--;

	error:
		fclose(f);

	return ret;
}

int check_acc_devid()
{
	FILE *f = NULL;
	int ret = 0;
	f = fopen(acc_filepath, "r");
	if (f == NULL) {
		f = fopen("/sys/devices/platform/nmk-i2c.2/i2c-2/2-0019/data", "r");
		if (f == NULL)
			return -1;
		strncpy(acc_filepath,"/sys/devices/platform/nmk-i2c.2/i2c-2/2-0019/data",(MAX_LENGTH -1));
		strncpy(acc_filepath_activate,"/sys/devices/platform/nmk-i2c.2/i2c-2/2-0019/mode",(MAX_LENGTH -1));
		strncpy(acc_filepath_range,"/sys/devices/platform/nmk-i2c.2/i2c-2/2-0019/range",(MAX_LENGTH -1));
		strncpy(acc_filepath_rate,"/sys/devices/platform/nmk-i2c.2/i2c-2/2-0019/rate",(MAX_LENGTH -1));
		strncpy(acc_filepath_sleepwake,"/sys/devices/platform/nmk-i2c.2/i2c-2/2-0019/sleep_wake",(MAX_LENGTH -1));
		strncpy(acc_filepath_id,"/sys/devices/platform/nmk-i2c.2/i2c-2/2-0019/id",(MAX_LENGTH -1));
	}
	fclose(f);
	/* read id */
	f = fopen(acc_filepath_id, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &id) != 1)
		ret = -1;
	fclose(f);
	return ret;
}

int main(int argc, const char **argv)
{

	int lc;			/* loop counter */
	char *msg;		/* message returned from parse_opts */

	/***************************************************************
	 * parse standard options
	 ***************************************************************/
	if ((msg = parse_opts(argc, argv, (option_t *) NULL, NULL)) != (char *) NULL) {
		tst_brkm(TBROK, NULL, "OPTION PARSING ERROR - %s", msg);
		tst_exit();
	}

	/***************************************************************
	 * check looping state if -c option given
	 ***************************************************************/
	for (lc = 0; TEST_LOOPING(lc); lc++) {

		/***************************************************************
		 * only perform functional verification if flag set (-f not given)
		 ***************************************************************/
		if (STD_FUNCTIONAL_TEST) {

			for (Tst_count = 0; Tst_count < TST_TOTAL;) {

				switch (Tst_count) {
					case 0:
						TEST(check_acc_devid());
						TEST(acc_set_val());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == EOF)
							tst_resm(TFAIL, "Functional test %d failed. Error activating accelerometer,"
									"confirm hardware presence. TCID: %s File: %s", Tst_count, TCID, __FILE__);
						else
							tst_resm(TFAIL, "Functional test %d failed. FilePath: %s not found. TCID: %s File: %s\n"
									, Tst_count, acc_filepath_activate, TCID, __FILE__);

						TEST(acc_set_properties());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == EOF)
							tst_resm(TFAIL, "Functional test %d failed. Error setting accelerometer properties,"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						else
							tst_resm(TFAIL, "Functional test %d failed. TCID: %s File: %s\n", Tst_count, TCID, __FILE__);

						TEST(get_val(acc_filepath));
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == INVALID_DATA_READ) {
							tst_resm(TFAIL, "Functional test %d failed. Accelerometer XYZ values not read completely"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						} else {
							tst_resm(TFAIL, "Functional test %d failed. Filepath: %s not found. TCID: %s File: %s\n"
									, Tst_count, acc_filepath, TCID, __FILE__);
						}

						TEST(mag_set_val());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == EOF)
							tst_resm(TFAIL, "Functional test %d failed. Error when activating magnetometer,"
									"confirm hardware presence. TCID: %s File: %s" , Tst_count, TCID, __FILE__);
						else
							tst_resm(TFAIL, "Functional test %d failed. FilePath: %s not found. TCID: %s File: %s\n"
									, Tst_count, acc_filepath_activate, TCID, __FILE__);

						TEST(mag_set_properties());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == EOF)
							tst_resm(TFAIL, "Functional test %d failed. Error setting magnetometer properties,"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						else
							tst_resm(TFAIL, "Functional test %d failed. TCID: %s File: %s\n", Tst_count, TCID, __FILE__);

						TEST(get_val(mag_filepath));
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == INVALID_DATA_READ) {
							tst_resm(TFAIL, "Functional test %d failed. Magnetometer XYZ values not read completely"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						} else {
							tst_resm(TFAIL, "Functional test %d failed. Filepath: %s not found. TCID: %s File: %s\n"
									, Tst_count, mag_filepath, TCID, __FILE__);
						}

						TEST(acc_get_properties());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN > 0) {
							tst_resm(TFAIL, "Functional test %d failed. accelerometer property values not read completely"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						} else {
							tst_resm(TFAIL, "Functional test %d failed. TCID: %s File: %s\n", Tst_count, TCID, __FILE__);
						}

						TEST(mag_get_properties());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN > 0) {
							tst_resm(TFAIL, "Functional test %d failed. magnetometer property values not read completely"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						} else {
							tst_resm(TFAIL, "Functional test %d failed. TCID: %s File: %s\n", Tst_count, TCID, __FILE__);
						}
						break;
				}
			}
		}
		tst_exit();
		return 0;
	}
}
