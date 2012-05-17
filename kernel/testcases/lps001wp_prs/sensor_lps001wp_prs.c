/*
 * Copyright (C) 2011 ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 *
 * Author: 2011, Naga Radhesh Y <naga.radheshy@stericsson.com>
 */


/******************************************************************************/
/* Description: Test pressure and temperature values from LPS001WP
*              driver.
*
* Test Assertion and Strategy: Assert if data is valid.
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
char *TCID = "lps001wp";	/* test program identifier.             */

/* total number of tests in this file. */
int TST_TOTAL = 1;

#define INVALID_DATA_READ -1
#define VALID_DATA 0

char *path_prs_enable = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/enable";
char *path_prs_pollrate = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/pollrate_ms";
char *path_prs_diffenable = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/diff_enable";
char *path_prs_ref = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/press_reference";
char *path_prs_lowpow_enable = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/lowpow_enable";
char *path_prs_pressdata = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/press_data";
char *path_prs_tempdata = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/temp_data";
char *path_prs_deltapresdata = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/deltapr_data";
char *path_prs_regval = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/reg_value";
char *path_prs_regaddr = "/sys/devices/platform/nmk-i2c.2/i2c-2/2-005c/reg_addr";

int get_val()
{
	FILE *f;
	int data, ret;

	f = fopen(path_prs_pressdata, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		ret = INVALID_DATA_READ;
	else
		ret = VALID_DATA;

	close(f);

	f = fopen(path_prs_tempdata, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		ret = INVALID_DATA_READ;
	else
		ret = VALID_DATA;

	close(f);

	f = fopen(path_prs_deltapresdata, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		ret = INVALID_DATA_READ;
	else
		ret = VALID_DATA;

	close(f);
	return ret;
}

int prs_set_properties()
{
	FILE *f;
	unsigned long val = 0;

	f = fopen(path_prs_pollrate, "w");
	if (f == NULL)
		return -1;

	/*write sampling rate value */
	if (fputc((int)'0', f) == EOF)
		val = EOF;
	if (fputs("500", f) == EOF)
		val = EOF;
	fclose(f);

	/*write Diff enable value */
	f = fopen(path_prs_diffenable, "w");
	if (f == NULL)
		return -1;

	if (fputc((int)'0', f) == EOF)
		val = EOF;
	fclose(f);

	/*write ref pressure value */
	f = fopen(path_prs_ref, "w");
	if (f == NULL)
		return -1;

	if (fputc((int)'0', f) == EOF)
		val = EOF;
	fclose(f);

	/*write ref pressure value */
	f = fopen(path_prs_lowpow_enable, "w");
	if (f == NULL)
		return -1;

	if (fputc((int)'0', f) == EOF)
		val = EOF;
	fclose(f);

	/*write ref pressure value */
	f = fopen(path_prs_regaddr, "w");
	if (f == NULL)
		return -1;

	if (fputs("0x34", f) == EOF)
		val = EOF;
	fclose(f);

	return val;
}


int prs_set_mode()
{
	FILE *f;
	unsigned long val = 0;

	/* Read all the sysfs nodes to cover
	 * code flow in error case scenarios
	 */
	prs_set_properties();
	get_val();

	f = fopen(path_prs_enable, "w");
	if (f == NULL)
		return -1;

	/* Activate LSM device */
	if (fputc((int)'1', f) == EOF)
		val = EOF;
	fclose(f);
	return val;
}

int prs_get_properties()
{
	FILE *f;
	int data;
	unsigned long val = 0;

	f = fopen(path_prs_pollrate, "r");
	if (f == NULL)
		return -1;

	/*read sampling rate value */
	if (fscanf(f, "%d", &data) != 1)
		val = EOF;
	fclose(f);

	/*read Diff enable value */
	f = fopen(path_prs_diffenable, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		val = EOF;
	fclose(f);

	/*read ref pressure value */
	f = fopen(path_prs_ref, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		val = EOF;
	fclose(f);

	/*read ref pressure value */
	f = fopen(path_prs_lowpow_enable, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		val = EOF;
	fclose(f);

	/*read ref pressure value */
	f = fopen(path_prs_regval, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%d", &data) != 1)
		val = EOF;
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
						TEST(prs_set_mode());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == EOF)
							tst_resm(TFAIL, "Functional test %d failed. Error activating pres sensor,"
									"confirm hardware presence. TCID: %s File: %s", Tst_count, TCID, __FILE__);
						else
							tst_resm(TFAIL, "Functional test %d failed. FilePath: %s not found. TCID: %s File: %s\n"
									, Tst_count, path_prs_enable, TCID, __FILE__);

						TEST(prs_set_properties());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == EOF)
							tst_resm(TFAIL, "Functional test %d failed. Error setting pres sensor properties,"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						else
							tst_resm(TFAIL, "Functional test %d failed. TCID: %s File: %s\n", Tst_count, TCID, __FILE__);

						TEST(get_val());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == INVALID_DATA_READ) {
							tst_resm(TFAIL, "Functional test %d failed. pres sensor values not read completely"
									"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						} else {
							tst_resm(TFAIL, "Functional test %d failed. Filepath: %s not found. TCID: %s File: %s\n"
									, Tst_count, path_prs_pressdata, TCID, __FILE__);
						}

						TEST(prs_get_properties());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN > 0) {
							tst_resm(TFAIL, "Functional test %d failed. pres sensor values not read completely"
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
