/*
 * Copyright (C) 2010 ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 *
 * Author: 2010, Martin Persson <martin.persson@stericsson.com>,
 * Jonas Aaberg <jonas.aberg@stericsson.com>
 */

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

#include <linux/input.h>



/* Extern Global Variables */
extern int Tst_count;		/* counter for tst_xxx routines.         */
extern char *TESTDIR;		/* temporary dir created by tst_tmpdir() */

/* Global Variables */
char *TCID = "dma";

int test_cases[] = {
	22, 23, 24, 25, 1, 2, 3, 4, 6, 7, 8,
	21, 15, 16, 17, 18, 19, 20, 21, 55, 56,
	57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
	67, 68,

	/* run long tests near the end */
	26, 7, 32, 33, 5, 34, 27, 28, 29,

	/*
	 * The following fail:
	 *
	 * This due to hardware error V1, should be fixed in V2:
	 * 30, 31, 35, 51
	 *
	 * These fail due to unknown reason:
	 * 9
	 */
};

/* total number of tests in this file. */
int TST_TOTAL = sizeof(test_cases);

static int dma_test(int id)
{
	int fd = 0, size, err = 0;
	char fname[256], buff[256], answer[256];

	sprintf(fname, "/sys/kernel/debug/ste_dma40_test/test_%d", id);

	fd = open(fname, O_RDONLY);

	if (fd < 0) {
		err = -1;
		goto _exit;
	}
	memset(buff, 0, 256);
	size = read(fd, buff, 255);
	if (size <= 0) {
		err = -2;
		goto _exit;
	}

	sprintf(answer, "Finished test %d: OK\n", id);
	if (size != strlen(answer)) {
		printf("DMA testcase %d failed\n", id);
		err = -3;
		goto _exit;
	}

	if (strncmp(buff, answer, strlen(answer))) {
		printf("DMA testcase %d failed\n", id);
		err = -4;
	}

_exit:
	if (fd > 0)
		close(fd);

	return err;
}

static int n_opt;
static char *n_copt;
static int testnum;

static option_t options[] = {
	{ "n:", &n_opt, &n_copt },
	{ },
};

int main(int argc, char **argv)
{

	int lc;			/* loop counter */
	char *msg;		/* message returned from parse_opts */

	/***************************************************************
	 * parse standard options
	 ***************************************************************/
	if ((msg = parse_opts(argc, argv, options, NULL)) != (char *) NULL) {
		tst_brkm(TBROK, NULL, "OPTION PARSING ERROR - %s", msg);
		tst_exit();
	}

	if (n_opt) {
		testnum = atoi(n_copt);
		TST_TOTAL = 1;
	}

	system("modprobe stedma40_test");

	for (lc = 0; TEST_LOOPING(lc); lc++) {

		/***************************************************************
		 * only perform functional verification if flag set (-f not given)
		 ***************************************************************/
		if (STD_FUNCTIONAL_TEST) {

			for (Tst_count = 0; Tst_count < TST_TOTAL;) {
				int num;

				num = TST_TOTAL == 1 ?
				      testnum : test_cases[Tst_count];

				TEST(dma_test(num));

				if (TEST_RETURN == 0)
					tst_resm(TPASS, "Functional test %d OK\n", num);
				else
					tst_resm(TFAIL, "Return value: %d. TCID: %s (%d) File: %s Line: %d. errno=%d : %s \n",
						 TEST_RETURN, TCID, num, __FILE__, __LINE__,
						 TEST_ERRNO, strerror(TEST_ERRNO));

			}
		}
	}

	system("modprobe -r stedma40_test");

	tst_exit();
	return 0;
}
