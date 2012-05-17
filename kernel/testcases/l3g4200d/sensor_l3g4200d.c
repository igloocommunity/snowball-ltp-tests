/*
 * Copyright (C) 2011 ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 *
 * Author: 2011, Chethan Krishna N <chethan.krishna@stericsson.com>
 */


/******************************************************************************/
/* Description: Test x, y and z axis values from gyroscope driver.
/*
/* Test Assertion and Strategy: Assert if all three axes data is valid.
/*
/******************************************************************************/

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
char *TCID = "l3g4200d";	/* test program identifier.             */

/* total number of tests in this file. */
int TST_TOTAL = 1;

#define INVALID_DATA_READ -1
#define VALID_DATA 0

char *gyr_filepath =          "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0068/gyrodata";
char *gyr_filepath_activate = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0068/powermode";
char *gyr_filepath_range = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0068/range";
char *gyr_filepath_rate = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0068/datarate";
char *gyr_filepath_temp = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-0068/gyrotemp";

int gyr_set_properties()
{
	FILE *f;
	unsigned long val = -1;

	f = fopen(gyr_filepath_rate, "w");
	if (f == NULL)
		return -1;

	if (fputc((int)'0', f) == EOF)
		val = EOF;
	else
		val = 0;

	fclose(f);

	f = fopen(gyr_filepath_range, "w");
	if (f == NULL)
		return -1;

	if (fputc((int)'0', f) ==EOF)
		val = EOF;
	else
		val = 0;

	fclose(f);

	return val;
}

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

int gyr_get_properties()
{
	FILE *f;
	int data;
	int ret = 4;

	/* read powermode */
	f = fopen(gyr_filepath_activate, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		goto error;
	else
	    ret--;
	fclose(f);

	/* read range */
	f = fopen(gyr_filepath_range, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		goto error;
	else
	    ret--;
	fclose(f);

	/* show datarate */
	f = fopen(gyr_filepath_rate, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		goto error;
	else
	    ret--;
	fclose(f);

	/* show gyrotemp */
	f = fopen(gyr_filepath_temp, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		goto error;
	else
	    ret--;
	fclose(f);

	error:
		fclose(f);

	return ret;
}

int set_val()
{
	FILE *f;
	unsigned long val = -1;

	/* Before acrivating device try writting to rate,
	 * range & read value to cover error cases.
	 */
	gyr_set_properties();
	/*Read output data value */
	get_val(gyr_filepath);

	f = fopen(gyr_filepath_activate, "w");
	if (f == NULL)
		return -1;

	/* Activate L3G4200D device */
	if (fputc((int)'1', f) == EOF)
		val = EOF;
	else
		val = 0;

	fclose(f);
	return val;
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
						TEST(set_val());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == EOF)
							tst_resm(TFAIL, "Functional test %d failed. Error when activating gyroscope,"
									"confirm hardware presence	TCID: %s File: %s", Tst_count, TCID, __FILE__);
						else
							tst_resm(TFAIL, "Functional test %d failed. FilePath: %s not found. TCID: %s File: %s\n"
									, Tst_count, gyr_filepath_activate, TCID, __FILE__);

						TEST(gyr_set_properties());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == EOF)
							tst_resm(TFAIL, "Functional test %d failed. Error setting gyroscope properties,"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						else
							tst_resm(TFAIL, "Functional test %d failed. TCID: %s File: %s\n", Tst_count, TCID, __FILE__);

						TEST(get_val(gyr_filepath));
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == INVALID_DATA_READ) {
							tst_resm(TFAIL, "Functional test %d failed. XYZ values not read completely"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						} else {
							tst_resm(TFAIL, "Functional test %d failed. Filepath: %s not found. TCID: %s File: %s\n"
									, Tst_count, gyr_filepath, TCID, __FILE__);
						}

						TEST(gyr_get_properties());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN > 0) {
							tst_resm(TFAIL, "Functional test %d failed. gyro property values not read completely"
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
