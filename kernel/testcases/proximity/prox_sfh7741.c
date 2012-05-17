/*
 * Copyright (C) 2011 ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 *
 * Author: 2011, Chethan Krishna N <chethan.krishna@stericsson.com>
 */


/******************************************************************************/
/* Description: Test if proximity value from proximity driver is reasonable
/*
/* Test Assertion and Strategy: Assert if value is within range.
/******************************************************************************/

/* Harness Specific Include Files. */
#include "test.h"
#include "usctest.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <linux/input.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Extern Global Variables */
extern int Tst_count;		/* counter for tst_xxx routines.         */
extern char *TESTDIR;		/* temporary dir created by tst_tmpdir() */

/* Global Variables */
char *TCID = "prox_sfh7741";	/* test program identifier.             */

/* total number of tests in this file. */
int TST_TOTAL = 1;

#define FISHY_PROXIMITY_VALUE -10
#define PROXIMITY_NEAR_VALUE 0
#define PROXIMITY_FAR_VALUE 100000

char *filepath_input = "/dev/input/event0";
char *filepath =          "/sys/devices/platform/sensors1p.0/proximity";
char *filepath_activate = "/sys/devices/platform/sensors1p.0/proximity_activate";

int set_val()
{
	FILE *f;
	unsigned long val = -1;

	f = fopen(filepath_activate, "w");
	if (f == NULL)
		return -1;

	/* Activate Proximity driver */
	if (fputc((int)'1', f) == EOF)
		val = EOF;
	else
		val = 0;

	fclose(f);
	return val;
}

int get_val(char *fname)
{
	FILE *f;
	unsigned long val = -1;

	f = fopen(fname, "r");
	if (f == NULL)
		return -1;

	if (fscanf(f, "%ld", &val) > 0) {

		/* Reasonable values */
		if (val >= PROXIMITY_NEAR_VALUE && val <= PROXIMITY_FAR_VALUE)
			val = 0;
		else
			val = FISHY_PROXIMITY_VALUE;
	}
	fclose(f);
	return val;
}
int get_interrupt_val()
{
	int fd = -1,retval = -1;
	fd_set read_set;
	struct input_event ev;
	struct timeval tv;
	int val = -1;
	int size = sizeof(struct input_event);

        /*Initialise the structures*/
        memset(&ev, 0x00, sizeof(ev));
		memset(&tv, 0x00, sizeof(tv));
        /* open input device */
        if ((fd = open(filepath_input, O_RDONLY)) == -1)
               return -1;
        /*Intialise the read descriptor*/
        FD_ZERO(&read_set);
        FD_SET(fd,&read_set);
        /* Wait up to five seconds. */
	tv.tv_sec = 1;
	tv.tv_usec = 0;
        retval = select(fd+1, &read_set, NULL, NULL, &tv);
	if (retval > 0 ) {
            /* FD_ISSET(0, &rfds) will be true. */
            if (FD_ISSET(fd, &read_set)) {
		read(fd, &ev, size );
		if(11 == ev.code) {
			if (ev.value >= PROXIMITY_NEAR_VALUE && ev.value <= PROXIMITY_FAR_VALUE)
				val = 0;
			else
				val = FISHY_PROXIMITY_VALUE;
		}
            }
        }
	else if(retval == -1) {
		return -1;
	}
	else {
		val = 0;
	}
        if(fd > 0)
		close(fd);
	return val;
}


int main(int argc, const char **argv)
{

	int lc;			/* loop counter */
	char *msg;		/* message returned from parse_opts */
	struct utsname buf;

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
					uname(&buf);
					if(strncmp(buf.release,"3.0",3) == 0) {
						TEST(get_interrupt_val());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == FISHY_PROXIMITY_VALUE) {
							tst_resm(TFAIL, "Functional test %d failed. proximity value not valid"
								"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						} else {
							tst_resm(TFAIL, "Functional test %d failed. Filepath: %s not found. TCID: %s File: %s\n"
								, Tst_count, filepath, TCID, __FILE__);
						}
					}
					else {
						TEST(set_val());
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == EOF)
							tst_resm(TFAIL, "Functional test %d failed. Error when activating proximity. TCID: %s File: %s"
									 , Tst_count, TCID, __FILE__);
						else
							tst_resm(TFAIL, "Functional test %d failed. FilePath: %s not found. TCID: %s File: %s\n"
									 , Tst_count, filepath_activate, TCID, __FILE__);

						TEST(get_val(filepath));
						if (TEST_RETURN == 0)
							tst_resm(TPASS, "Functional test %d OK\n", Tst_count);
						else if (TEST_RETURN == FISHY_PROXIMITY_VALUE) {
							tst_resm(TFAIL, "Functional test %d failed. proximity value not valid"
								"TCID: %s File: %s", Tst_count, TCID, __FILE__);
						} else {
							tst_resm(TFAIL, "Functional test %d failed. Filepath: %s not found. TCID: %s File: %s\n"
								, Tst_count, filepath, TCID, __FILE__);
						}
					}
					break;
				}
			}
		}
		tst_exit();
		return 0;
	}
}
