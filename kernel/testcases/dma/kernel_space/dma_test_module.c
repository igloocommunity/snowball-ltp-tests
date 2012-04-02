/*
 * Copyright (C) ST-Ericsson SA 2007-2010
 * Author: Per Friden <per.friden@stericsson.com> for ST-Ericsson
 * Author: Jonas Aaberg <jonas.aberg@stericsson.com> for ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/types.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/scatterlist.h>
#include <linux/completion.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <plat/ste_dma40.h>

#include "dma_test_lib.h"

#ifndef CONFIG_DEBUG_FS
#error "DEBUG_FS must be set"
#endif


MODULE_DESCRIPTION("DMA test module: Test case for DMA");
MODULE_LICENSE("GPL");

static DEFINE_MUTEX(tc_mutex);

#define DBG_TEST(x) x
#define DBG_SPAM(x)

#define TX_ALIGN 1

#define DEFAULT_TIMEOUT 10000 /* ms */

#if defined(CONFIG_MMC) && defined(CONFIG_STE_DMA40_DEBUG)
extern int stedma40_debug_mmc_sgsize(u32 size, bool is_chain);
#endif

#ifdef CONFIG_STE_DMA40_DEBUG
extern void sted40_history_reset(void);
extern void sted40_history_disable(void);
extern void sted40_history_dump(void);
extern void sted40_history_text(char *text);
#else
#define sted40_history_reset()
#define sted40_history_disable()
#define sted40_history_dump()
#define sted40_history_text(x)
#endif

enum test_case_id {
	TEST1 = 1,
	TEST2,
	TEST3,
	TEST4,
	TEST5,
	TEST6,
	TEST7,
	TEST8,
	TEST9,
	TEST10,
	TEST11,
	TEST12,
	TEST13,
	TEST14,
	TEST15,
	TEST16,
	TEST17,
	TEST18,
	TEST19,
	TEST20,
	TEST21,
	TEST22,
	TEST23,
	TEST24,
	TEST25,
	TEST26,
	TEST27,
	TEST28,
	TEST29,
	TEST30,
	TEST31,
	TEST32,
	TEST33,
	TEST34,
	TEST35,
	/* HW stress test */
	TEST36,
	TEST37,
	TEST38,
	TEST39,
	TEST40,
	TEST41,
	TEST42,
	TEST43,
	TEST44,
	TEST45,
	TEST46,
	TEST47,
	TEST48,
	TEST49,
	TEST50,
	TEST51,
	TEST52,
	TEST53,
	/* HW stress test end */
	TEST54,
	TEST55,
	TEST56,
	TEST57,
	TEST58,
	TEST59,
	TEST60,
	TEST61,
	TEST62,
	TEST63,
	TEST64,
	TEST65,
	TEST66,
	TEST67,
	TEST68,
	TEST69,
	NBR_TESTS,
};

struct tc_struct {
	/* set by client */
	char name[MAX_NAME_LEN];
	int laps;
	int end_padding;
	int do_check_buffer;
	int list_len;

	/* err status set by test engine */
	int err;

	/* used by test engine */
	int nbr_returns_per_transfer;
	struct workqueue_struct *wq;
	spinlock_t lock;
	struct buflist buflist;
	void (*callback)(struct buflist *bf);
	struct work_struct work_start;
	struct work_struct work_close;
	struct completion done;
	int job_counter;
};

static void tc_worker_start_single(struct work_struct *work)
{
	struct tc_struct *tc =
	    container_of(work, struct tc_struct, work_start);

	dmatest_buflist_start_single(&tc->buflist, tc->callback);
}

static void tc_worker_start_sg(struct work_struct *work)
{
	struct tc_struct *tc =
	    container_of(work, struct tc_struct, work_start);

	dmatest_buflist_start_sg(&tc->buflist, tc->callback);
}

static void tc_worker(struct buflist *bf)
{
	int finished_cbs;
	struct tc_struct *tc = container_of(bf, struct tc_struct, buflist);

	finished_cbs = dmatest_buflist_get_nbr_finished(&tc->buflist);


	DBG_SPAM(printk
		 (KERN_INFO "[%s] job_counter %d\n", __func__,
		  tc->job_counter));

	tc->job_counter++;

	if (tc->job_counter == tc->nbr_returns_per_transfer) {
		if (tc->do_check_buffer)
			tc->err = dmatest_buflist_payload_check(&tc->buflist);

		tc->laps--;
		if (tc->laps == 0) {
			complete(&tc->done);
		} else {
			dmatest_buflist_reset_nbr_finished(&tc->buflist);
			tc->job_counter = 0;

			queue_work(tc->wq, &tc->work_start);
		}
	}
}

/* test wrapper functions for creating/running tests */
static int tc_test_init(struct tc_struct *tc, bool is_sg_transfer,
			int size, bool is_const_size, unsigned long dma_flags,
			u32 tx_align, int timeout)
{
	int err;

	DBG_SPAM(printk(KERN_INFO "[%s] %s\n", __func__, tc->name));

	tc->wq = create_singlethread_workqueue(tc->name);
	if (tc->wq == NULL)
		goto err;
	err = dmatest_buflist_create(&tc->buflist, tc->list_len,
				     tc->end_padding,
				     tc->name, dma_flags, timeout, false, NULL);
	if (err)
		goto err_create;


	tc->job_counter = 0;

	if (is_sg_transfer) {
		INIT_WORK(&tc->work_start, tc_worker_start_sg);
		tc->nbr_returns_per_transfer = 1;
	} else {
		tc->nbr_returns_per_transfer = tc->list_len;
		INIT_WORK(&tc->work_start, tc_worker_start_single);
	}

	init_completion(&tc->done);
	spin_lock_init(&tc->lock);
	tc->callback = tc_worker;

	if (is_const_size)
		dmatest_sizelist_set(&tc->buflist, size, tx_align);
	else
		dmatest_sizelist_randomize(&tc->buflist, 1, 60*1024, tx_align);
	dmatest_buflist_alloc(&tc->buflist);

	return 0;
 err_create:
	destroy_workqueue(tc->wq);
 err:
	return -EINVAL;
}

static void tc_test_free(struct tc_struct *tc)
{
	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	destroy_workqueue(tc->wq);
	dmatest_buflist_destroy(&tc->buflist);
}

static void tc_test_wait(struct tc_struct *tc)
{
	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	wait_for_completion_interruptible(&tc->done);
}

#ifdef CONFIG_STE_DMA40_DEBUG
static bool tc_test_is_done(struct tc_struct *tc)
{
	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	return completion_done(&tc->done);
}
#endif

static void tc_test_run(struct tc_struct *tc)
{
	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	queue_work(tc->wq, &tc->work_start);
}


/* test case 1: Send and receive 32 byte buffer
 *
 */
static int tc_1_fixed_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 1,
	};
	int length = 1;
	int end_padding = 64;

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	sted40_history_reset();

	tc.wq = create_singlethread_workqueue(__func__);

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	INIT_WORK(&tc.work_start, tc_worker_start_single);
	tc.callback = tc_worker;

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	tc.err = dmatest_buflist_create(&tc.buflist, length, end_padding,
					tc.name,
					DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
					DEFAULT_TIMEOUT, false, NULL);

	DBG_TEST(if (tc.err) printk(KERN_INFO "[%s] Error creating buflist\n", __func__));

	if (tc.err)
		goto out;

	dmatest_sizelist_set(&tc.buflist, 32, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

 out:

	return tc.err;
}


/* test case 2: Send and receive 1 byte buffer
 *
 */
static int tc_2_fixed_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 100,
	};
	int length = 30;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	tc.err = dmatest_buflist_create(&tc.buflist, length, end_padding,
					tc.name,
					DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
					DEFAULT_TIMEOUT, false, NULL);

	DBG_TEST(if (tc.err) printk(KERN_INFO "[%s] Error creating buflist\n", __func__));

	if (tc.err)
		goto out;

	dmatest_sizelist_set(&tc.buflist, 32, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);
 out:

	return tc.err;
}

/* test case 3: Send and receive 1k buffer
 *
 */
static int tc_3_fixed_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 10,
	};
	int length = 10;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	dmatest_buflist_create(&tc.buflist, length, end_padding,
			       tc.name, DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
			       DEFAULT_TIMEOUT, false, NULL);

	dmatest_sizelist_set(&tc.buflist, 1*1024, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

	return tc.err;
}

/* test case 4: Send and receive 8k buffer
 *
 */
static int tc_4_fixed_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 10,
	};
	int length = 10;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	dmatest_buflist_create(&tc.buflist, length, end_padding,
			       tc.name, DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
			       DEFAULT_TIMEOUT, false, NULL);

	dmatest_sizelist_set(&tc.buflist, 8*1024, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

	return tc.err;
}

/* test case 5: Send and receive 8k buffer
 *
 */
static int tc_5_random_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 100,
	};
	int length = 40;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	dmatest_buflist_create(&tc.buflist, length, end_padding,
			       tc.name, DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
			       DEFAULT_TIMEOUT, false, NULL);

	dmatest_sizelist_randomize(&tc.buflist, 1, 60*1024, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

	return tc.err;
}

/* test case 6: scatter-gatter buffer
 *
 */
static int tc_sg_buffer(int scatter_list_entries, int laps, int buffer_size)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
	};
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_sg);

	/* test case configuration */
	tc.laps = laps;
	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	tc.err = dmatest_buflist_create(&tc.buflist, scatter_list_entries,
					end_padding, tc.name,
					DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
					10*DEFAULT_TIMEOUT, false, NULL);

	DBG_TEST(if (tc.err) printk(KERN_INFO "[%s] Error creating buflist\n", __func__));

	if (tc.err)
		goto out;

	dmatest_sizelist_set(&tc.buflist, buffer_size, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;

	tc.nbr_returns_per_transfer = 1;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);
 out:

	return tc.err;
}

#define SCATTER_LIST_ENTRIES_MAX 25
#define SIZE_MAX (2 * PAGE_SIZE + 100)

static int tc_9_sg_buffer(void)
{
	int res = 0;
	int scatter_list_entries;
	int size;
	int transfers = 0;

	sted40_history_disable();

	for (scatter_list_entries = 1;
	    scatter_list_entries < SCATTER_LIST_ENTRIES_MAX;
	    scatter_list_entries++) {
		for(size = 2; size < SIZE_MAX; size++) {
			res = tc_sg_buffer(scatter_list_entries, 1, size);
			if (res) {
				printk(KERN_INFO "[%s] sgl with entries: %d size: %d failed\n",
				       __func__, scatter_list_entries, size);
				goto _exit;
			}
			if (transfers % 2500 == 0) {
				printk(KERN_INFO "[%s]: %d of %ld transfers done.\n",
				       __func__, transfers,
				       SCATTER_LIST_ENTRIES_MAX*SIZE_MAX);
			}

			transfers++;
		}
	}
_exit:
	return res;
}

static int tc_6_sg_buffer(void)
{
	return tc_sg_buffer(4, 1, 32);
}

#define SIZE_MAX_ETERNAL 1567

#if defined(CONFIG_MMC) && defined(CONFIG_STE_DMA40_DEBUG)
static int tc_14_sg_buffer_temporary_endless(void)
{
	int res = 0;
	int scatter_list_entries;
	int size;
	int transfers = 0;
	int errors = 0;

	printk(KERN_INFO "Warning:this testcase is endless and is only there to provoke memcpy sg errors\n");
	sted40_history_disable();
	while (1) {
		for (scatter_list_entries = 1;
		     scatter_list_entries < SCATTER_LIST_ENTRIES_MAX;
		     scatter_list_entries++) {
			for (size = 2; size < SIZE_MAX_ETERNAL; size++) {

				res = tc_sg_buffer(scatter_list_entries, 1, size);
				if (res) {
					errors++;
					printk(KERN_INFO "[%s] sgl with entries: %d size: %d failed\n"
					       "after %d transfers there are %d error(s)\n",
					       __func__, scatter_list_entries,
					       size, transfers, errors);
				}
				transfers++;
			}
		}
	}

	return 0;
}
#endif

static int tc_7_sg_buffer(void)
{
	return tc_sg_buffer(200, 3, 160);
}


/* test case 8: scatter-gatter buffer
 *
 */
static int tc_8_sg_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 2,
	};
	int length = 16;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_sg);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	tc.err = dmatest_buflist_create(&tc.buflist, length, end_padding,
					tc.name,
					DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
					DEFAULT_TIMEOUT, false, NULL);

	DBG_TEST(if (tc.err) printk(KERN_INFO "[%s] Error creating buflist\n", __func__));

	if (tc.err)
		goto out;

	dmatest_sizelist_set(&tc.buflist, 32, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;

	/* Number of lists, not entries in each list */
	tc.nbr_returns_per_transfer = 1;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);
 out:

	return tc.err;
}

#if defined(CONFIG_MMC) && defined(CONFIG_STE_DMA40_DEBUG)
static int dd_setup(struct seq_file *s, const char *func,
		    u32 elem_size, bool is_sg_chain, const char *cmd)
{
	int err;

	err = stedma40_debug_mmc_sgsize(elem_size, is_sg_chain);
	if (err)
		goto out;
	err = seq_printf(s, "# [%s()] sg_elem_size %d, sg_chain %d, run:\n"
			 "%s; sync\n",
			 func, elem_size, is_sg_chain, cmd);
 out:
	return err;
}

/* test case 10: Testing dma via MMC, require sdio_ops.patch
 *
 */
static int tc_10_sg_size_1024_one(struct seq_file *s)
{
	u32 elem_size = 1024;
	bool is_sg_chain = 0;
	const char *cmd = "dd if=/dev/zero of=/out_zero bs=4096 count=1";
	DBG_SPAM(printk(KERN_INFO "# [%s]\n", __func__));

	return dd_setup(s, __func__, elem_size, is_sg_chain, cmd);
}

/* test case 11: Testing dma via MMC, require sdio_ops.patch
 *
 */
static int tc_11_sg_size_chain_one(struct seq_file *s)
{
	u32 elem_size = 1024;
	bool is_sg_chain = 1;
	const char *cmd = "dd if=/dev/zero of=/out_zero bs=4096 count=1";

	DBG_SPAM(printk(KERN_INFO "# [%s]\n", __func__));

	return dd_setup(s, __func__, elem_size, is_sg_chain, cmd);
}

/* test case 12: Testing dma via MMC, require sdio_ops.patch
 *
 */
static int tc_12_sg_size_1024_many(struct seq_file *s)
{
	u32 elem_size = 1024;
	bool is_sg_chain = 0;
	const char *cmd = "dd if=/dev/zero of=/out_zero bs=4096 count=256";
	DBG_SPAM(printk(KERN_INFO "# [%s]\n", __func__));

	return dd_setup(s, __func__, elem_size, is_sg_chain, cmd);
}

/* test case 13: Testing dma via MMC, require sdio_ops.patch
 *
 */
static int tc_13_sg_size_1024_chain_many(struct seq_file *s)
{
	u32 elem_size = 1024;
	bool is_sg_chain = 1;
	const char *cmd = "dd if=/dev/zero of=/out_zero bs=4096 count=256";
	DBG_SPAM(printk(KERN_INFO "# [%s]\n", __func__));

	return dd_setup(s, __func__, elem_size, is_sg_chain, cmd);
}
#endif

/* test case 15: Send and receive 1-4 bytes buffer
 *
 */
static int tc_15_static_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 100,
	};
	int length = 40;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	dmatest_buflist_create(&tc.buflist, length, end_padding,
			       tc.name,
			       DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
			       DEFAULT_TIMEOUT, false, NULL);

	dmatest_sizelist_set(&tc.buflist, 1, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

	return tc.err;
}

/* test case 16: Send and receive 1-4 bytes buffer
 *
 */
static int tc_16_static_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 100,
	};
	int length = 40;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	dmatest_buflist_create(&tc.buflist, length, end_padding,
			       tc.name, DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
			       DEFAULT_TIMEOUT, false, NULL);

	dmatest_sizelist_set(&tc.buflist, 2, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

	return tc.err;
}

/* test case 17: Send and receive 1-4 bytes buffer
 *
 */
static int tc_17_static_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 100,
	};
	int length = 40;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	dmatest_buflist_create(&tc.buflist, length, end_padding,
			       tc.name, DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
			       DEFAULT_TIMEOUT, false, NULL);

	dmatest_sizelist_set(&tc.buflist, 3, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

	return tc.err;
}

/* test case 18: Send and receive 1-4 bytes buffer
 *
 */
static int tc_18_static_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 100,
	};
	int length = 40;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	dmatest_buflist_create(&tc.buflist, length, end_padding,
			       tc.name, DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
			       DEFAULT_TIMEOUT, false, NULL);

	dmatest_sizelist_set(&tc.buflist, 4, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

	return tc.err;
}

/* test case 19: Send and receive 1-4 bytes buffer
 *
 */
static int tc_19_static_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 100,
	};
	int length = 40;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	dmatest_buflist_create(&tc.buflist, length, end_padding,
			       tc.name, DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
			       DEFAULT_TIMEOUT, false, NULL);

	dmatest_sizelist_set(&tc.buflist, 5, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

	return tc.err;
}

/* test case 20: Send and receive 1-4 bytes buffer
 *
 */
static int tc_20_random_buffer(void)
{
	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 100,
	};
	int length = 40;
	int end_padding = 64;
	tc.wq = create_singlethread_workqueue(__func__);

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	init_completion(&tc.done);
	spin_lock_init(&tc.lock);

	tc.callback = tc_worker;
	INIT_WORK(&tc.work_start, tc_worker_start_single);

	snprintf(tc.name, MAX_NAME_LEN, "%s", __func__);
	dmatest_buflist_create(&tc.buflist, length, end_padding,
			       tc.name, DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
			       DEFAULT_TIMEOUT, false, NULL);

	dmatest_sizelist_randomize(&tc.buflist, 1, 4, TX_ALIGN);

	dmatest_buflist_alloc(&tc.buflist);

	tc.job_counter = 0;
	tc.nbr_returns_per_transfer = length;
	queue_work(tc.wq, &tc.work_start);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);
	destroy_workqueue(tc.wq);

	dmatest_buflist_destroy(&tc.buflist);

	return tc.err;
}

static int tc_pause_and_unpause_parallel(int max_channels, char *str,
					 unsigned long dma_flags, int tx_align)
{
	struct tc_struct *tc;
	int i;
	int err = 0;
	int bytes_left = 0;
	int bytes_left_prev = 0;
	int max = 1000;
	int pause_id = 0;

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	tc = kzalloc(sizeof(struct tc_struct) * max_channels, GFP_KERNEL);
	for (i = 0; i < max_channels; i++) {
		tc[i].do_check_buffer = 1;
		tc[i].laps = 1;
		tc[i].end_padding = 64;
		tc[i].list_len = 1;

		snprintf(tc[i].name, 32, "%s_%d", str, i);

		tc[i].err = tc_test_init(&tc[i], false, 60*1024, true,
					 dma_flags, tx_align,
					 DEFAULT_TIMEOUT);
		if (tc[i].err)
			break;
	};
	if (i == 0) {
		err = -EINVAL;
		goto out;
	}
	max_channels = i;

	DBG_SPAM(printk(KERN_INFO "[%s] max available memcpy channels %d\n", str, max_channels));

	for (i = 0; i < max_channels; i++)
		tc_test_run(&tc[i]);


	for (i = 0; i < max; i++) {
		{
			struct dma_tx_state state;

			tc[pause_id].buflist.dma_chan->device->
				device_control(tc[pause_id].buflist.dma_chan,
					       DMA_PAUSE, 0);

			(void) tc[pause_id].buflist.dma_chan->device->
				device_tx_status(tc[pause_id].buflist.dma_chan,
						 tc[pause_id].buflist.cookie[0],
						 &state);
			bytes_left = state.residue;
		}



		if (bytes_left > 0)
			break;
	}
	if (i == max) {
		DBG_SPAM(printk(KERN_INFO "[%s] i == max bytes left %d\n",
				__func__, bytes_left));
		goto wait;
	}

	DBG_SPAM(printk(KERN_INFO "[%s] bytes left %d\n",
			__func__, bytes_left));
	tc[pause_id].buflist.dma_chan->device->
		device_control(tc[pause_id].buflist.dma_chan,
			       DMA_RESUME, 0);

	do {
		mdelay(1);
		tc[pause_id].buflist.dma_chan->device->
			device_control(tc[pause_id].buflist.dma_chan,
				       DMA_PAUSE, 0);
		bytes_left_prev = bytes_left;

		{
			struct dma_tx_state state;

			(void) tc[pause_id].buflist.dma_chan->device->
				device_tx_status(tc[pause_id].buflist.dma_chan,
						 tc[pause_id].buflist.cookie[0],
						 &state);
			bytes_left = state.residue;

		}
		tc[pause_id].buflist.dma_chan->device->
			device_control(tc[pause_id].buflist.dma_chan,
				       DMA_RESUME, 0);
	} while (bytes_left != 0 || bytes_left_prev != bytes_left);

	if (bytes_left != 0 && bytes_left_prev == bytes_left) {
		DBG_SPAM(printk(KERN_INFO "[%s] bytes left = prev %d\n",
				__func__, bytes_left));
		tc[pause_id].err = -EINVAL;
		goto out;
	}


	DBG_SPAM(printk(KERN_INFO "[%s] bytes left %d\n",
			__func__, bytes_left));

 wait:
	for (i = 0; i < max_channels; i++) {
		tc_test_wait(&tc[i]);
		DBG_SPAM(printk(KERN_INFO "[%s] %d done\n", str, i));
	}

 out:
	for (i = 0; i < max_channels; i++)
		tc_test_free(&tc[i]);

	for (i = 0; i < max_channels; i++)
		err |= tc[i].err;

	kfree(tc);
	return err;
}

static int tc_21_stop_and_go(void)
{
	return tc_pause_and_unpause_parallel(1, "tc_21",
			DMA_PREP_INTERRUPT | DMA_CTRL_ACK, 4);
}

static struct dma_chan *tc_22_25_chan;
int tc_22_req(void)
{
	dma_cap_mask_t mask;

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);

	tc_22_25_chan = dma_request_channel(mask, NULL, NULL);

	if (tc_22_25_chan != NULL)
		return 0;
	else
		return -EINVAL;
}

static struct dma_chan *tc_22_25_chan;
int tc_23_no_irq(void)
{
	dma_cap_mask_t mask;
	void *buf_src;
	void *buf_dst;
	dma_addr_t addr_src;
	dma_addr_t addr_dst;
	int size = 4096;
	struct dma_async_tx_descriptor *desc;

	if (tc_22_25_chan == NULL)
		return -EINVAL;

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);

	buf_src = kzalloc(size, GFP_KERNEL);
	memset(buf_src, 0xAA, size);
	buf_dst = kzalloc(size, GFP_KERNEL);

	addr_src = dma_map_single(tc_22_25_chan->device->dev,
				  buf_src, size, DMA_BIDIRECTIONAL);
	addr_dst = dma_map_single(tc_22_25_chan->device->dev,
				  buf_dst, size, DMA_FROM_DEVICE);

	desc = tc_22_25_chan->device->
		device_prep_dma_memcpy(tc_22_25_chan,
				       addr_dst, addr_src, size,
				       DMA_CTRL_ACK);

	desc->tx_submit(desc);
	dma_async_issue_pending(tc_22_25_chan);

	msleep(1000);

	dma_unmap_single(tc_22_25_chan->device->dev,
			 addr_src, size, DMA_BIDIRECTIONAL);

	dma_unmap_single(tc_22_25_chan->device->dev,
			 addr_dst, size, DMA_FROM_DEVICE);

	if (memcmp(buf_src, buf_dst, size) == 0)
		return 0;
	else
		return -EINVAL;
}


static void tc_24_transmit_cb(void *data)
{
	struct tc_struct *tc = data;
	complete(&tc->done);
}

int tc_24_irq(void)
{
	dma_cap_mask_t mask;
	dma_addr_t addr_src;
	dma_addr_t addr_dst;
	int size = 4096;
	void *buf_src;
	void *buf_dst;

	struct tc_struct tc = {
		.do_check_buffer = 1,
		.laps = 1,
	};
	struct dma_async_tx_descriptor *desc;
	init_completion(&tc.done);

	if (tc_22_25_chan == NULL)
		return -EINVAL;

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);

	buf_src = kzalloc(size, GFP_KERNEL);
	memset(buf_src, 0xAA, size);
	buf_dst = kzalloc(size, GFP_KERNEL);

	addr_src = dma_map_single(tc_22_25_chan->device->dev,
				  buf_src, size, DMA_BIDIRECTIONAL);
	addr_dst = dma_map_single(tc_22_25_chan->device->dev,
				  buf_dst, size, DMA_FROM_DEVICE);

	desc = tc_22_25_chan->device->
		device_prep_dma_memcpy(tc_22_25_chan,
				       addr_dst, addr_src, size,
				       DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

	desc->callback = tc_24_transmit_cb;
	desc->callback_param = &tc;
	desc->tx_submit(desc);


	dma_async_issue_pending(tc_22_25_chan);

	/* block here until test case finished */
	wait_for_completion_interruptible(&tc.done);

	dma_unmap_single(tc_22_25_chan->device->dev,
			 addr_src, size, DMA_BIDIRECTIONAL);

	dma_unmap_single(tc_22_25_chan->device->dev,
			 addr_dst, size, DMA_FROM_DEVICE);

	if (memcmp(buf_src, buf_dst, size) == 0) {
		kfree(buf_src);
		kfree(buf_dst);
		return 0;
	} else {
		kfree(buf_src);
		kfree(buf_dst);
		return -EINVAL;
	}
}

int tc_25_free(void)
{
	dma_cap_mask_t mask;

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);

	if (tc_22_25_chan == NULL)
		return -EINVAL;

	dma_release_channel(tc_22_25_chan);
	tc_22_25_chan = NULL;

	return 0;
}

struct tc_parallel {
	char str[32];
	int max_channels;
	int laps;
	unsigned long dma_flags;
	int chan_start_index;
	int tx_align;
	int const_size;
	int list_len;
	int timeout;
};

static int tc_run_parallel(struct tc_parallel *tcp)
{
	struct tc_struct *tc;
	int i;
	int err = 0;
	int max_channels;
	bool use_const_size = tcp->const_size != -1;

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	tc = kzalloc(sizeof(struct tc_struct) * tcp->max_channels, GFP_KERNEL);
	for (i = 0; i < tcp->max_channels; i++) {
		tc[i].do_check_buffer = 1;
		tc[i].laps = tcp->laps;
		tc[i].end_padding = 64;
		tc[i].list_len = tcp->list_len;

		snprintf(tc[i].name, 32, "%s_%d", tcp->str, i);

		tc[i].err = tc_test_init(&tc[i], false, tcp->const_size,
					 use_const_size, tcp->dma_flags,
					 tcp->tx_align, tcp->timeout);
		if (tc[i].err)
			break;
	};
	if (i == 0) {
		err = -EINVAL;
		goto out;
	}
	max_channels = i;

	DBG_SPAM(printk(KERN_INFO "[%s] max available memcpy channels %d\n", tcp->str,
			max_channels));

	for (i = tcp->chan_start_index; i < max_channels; i++) {
		DBG_SPAM(printk(KERN_INFO "starting %d\n", i));
		tc_test_run(&tc[i]);
	}

	for (i = tcp->chan_start_index; i < max_channels; i++) {
		tc_test_wait(&tc[i]);
		DBG_SPAM(printk(KERN_INFO "[%s] %d done\n", tcp->str, i));
	}

	for (i = 0; i < max_channels; i++)
		tc_test_free(&tc[i]);

	for (i = 0; i < max_channels; i++)
		err |= tc[i].err;

 out:
	kfree(tc);
	return err;
}

static int tc_26_run_3_parallel(void)
{
	struct tc_parallel tcp = {
		.str = "tc_26",
		.max_channels = 3,
		.laps = 20,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);
}

static int tc_27_run_4_parallel(void)
{
	struct tc_parallel tcp = {
		.str = "tc_27",
		.max_channels = 4,
		.laps = 20,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);
}

static int tc_28_run_5_parallel(void)
{
	struct tc_parallel tcp = {
		.str = "tc_28",
		.max_channels = 5,
		.laps = 20,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);
}

static int tc_29_run_6_parallel(void)
{
	struct tc_parallel tcp = {
		.str = "tc_29",
		.max_channels = 6,
		.laps = 20,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);

}

static int tc_30_run_7_parallel(void)
{
	struct tc_parallel tcp = {
		.str = "tc_30",
		.max_channels = 7,
		.laps = 20,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);

}

static int tc_31_run_128_parallel(void)
{
	struct tc_parallel tcp = {
		.str = "tc_31",
		.max_channels = 128,
		.laps = 20,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);
}

static int tc_32_run_1_parallel(void)
{
	struct tc_parallel tcp = {
		.str = "tc_32",
		.max_channels = 1,
		.laps = 20,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);
}

static int tc_33_run_2_parallel(void)
{
	struct tc_parallel tcp = {
		.str = "tc_33",
		.max_channels = 2,
		.laps = 20,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);

}

static int tc_34_run_pause_and_unpause_parallel(void)
{
	return tc_pause_and_unpause_parallel(4, "tc_34",
			DMA_PREP_INTERRUPT | DMA_CTRL_ACK, 1);
}

static int tc_35_run_1_parallel_reuse(void)
{
	struct tc_parallel tcp = {
		.str = "tc_35",
		.max_channels = 1,
		.laps = 20,
		.dma_flags = DMA_PREP_INTERRUPT,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = 100*DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);

}

enum read_reg_type {
	DMA_TC_READ_PHY_CHAN_1 = 1 << 0,
	DMA_TC_READ_PHY_CHAN_2 = 1 << 1,
	DMA_TC_READ_PHY_CHAN_3 = 1 << 2,
	DMA_TC_READ_PHY_CHAN_4 = 1 << 3,
	DMA_TC_READ_PHY_CHAN_5 = 1 << 4,
	DMA_TC_READ_PHY_CHAN_6 = 1 << 5,
	DMA_TC_READ_PHY_CHAN_7 = 1 << 6,
	DMA_TC_READ_PHY_CHAN_8 = 1 << 7,
	DMA_TC_READ_GLOBAL = 1 << 8,
};


#ifdef CONFIG_STE_DMA40_DEBUG
extern void stedma40_debug_read_chan(int chan, u32 *cfg);
extern void stedma40_debug_read_global_conf(u32 *cfg);
static void tc_read_reg(unsigned long read_type)
{
	u32 cfg = 0;

	if (read_type | DMA_TC_READ_PHY_CHAN_1)
		stedma40_debug_read_chan(0, &cfg);
	if (read_type | DMA_TC_READ_PHY_CHAN_2)
		stedma40_debug_read_chan(1, &cfg);
	if (read_type | DMA_TC_READ_PHY_CHAN_3)
		stedma40_debug_read_chan(2, &cfg);
	if (read_type | DMA_TC_READ_PHY_CHAN_4)
		stedma40_debug_read_chan(3, &cfg);
	if (read_type | DMA_TC_READ_PHY_CHAN_5)
		stedma40_debug_read_chan(4, &cfg);
	if (read_type | DMA_TC_READ_PHY_CHAN_6)
		stedma40_debug_read_chan(5, &cfg);
	if (read_type | DMA_TC_READ_PHY_CHAN_7)
		stedma40_debug_read_chan(6, &cfg);
	if (read_type | DMA_TC_READ_PHY_CHAN_8)
		stedma40_debug_read_chan(7, &cfg);
	if (read_type | DMA_TC_READ_GLOBAL)
		stedma40_debug_read_global_conf(&cfg);
}


int tc_run_while_read_reg(char *str, int max_channels, int size, int list_len,
			  int laps, bool is_sg, bool is_const_size,
			  unsigned long dma_flags, unsigned long read_type)
{
	struct tc_struct *tc;
	int i;
	int err = 0;

	DBG_SPAM(printk(KERN_INFO "[%s]\n", __func__));

	sted40_history_disable();

	tc = kzalloc(sizeof(struct tc_struct) * max_channels, GFP_KERNEL);
	for (i = 0; i < max_channels; i++) {
		tc[i].do_check_buffer = 1;
		tc[i].laps = laps;
		tc[i].end_padding = 64;
		tc[i].list_len = list_len;

		snprintf(tc[i].name, 32, "%s_%d", str, i);

		tc[i].err = tc_test_init(&tc[i], is_sg, size, is_const_size,
					 dma_flags, TX_ALIGN,
					 DEFAULT_TIMEOUT);
		if (tc[i].err)
			break;
	};
	if (i == 0) {
		err = -EINVAL;
		goto out;
	}
	max_channels = i;

	DBG_SPAM(printk(KERN_INFO "[%s] max available memcpy channels %d\n", str, max_channels));

	for (i = 0; i < max_channels; i++)
		tc_test_run(&tc[i]);

	for (i = 0; i < max_channels; i++) {
		while (!tc_test_is_done(&tc[i]))
			tc_read_reg(read_type);
		DBG_SPAM(printk(KERN_INFO "[%s] %d done\n", str, i));
	}

	for (i = 0; i < max_channels; i++)
		tc_test_free(&tc[i]);

	for (i = 0; i < max_channels; i++)
		err |= tc[i].err;

out:
	kfree(tc);
	return err;
}

static int tc_36(void)
{
	return tc_run_while_read_reg("tc_36", 1, 1024, 30, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_1);
}
static int tc_37(void)
{
	return tc_run_while_read_reg("tc_37", 1, 1024, 30, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_2);
}
static int tc_38(void)
{
	return tc_run_while_read_reg("tc_38", 1, 1024, 30, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_3);
}
static int tc_39(void)
{
	return tc_run_while_read_reg("tc_39", 1, 1024, 30, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_4);
}
static int tc_40(void)
{
	return tc_run_while_read_reg("tc_40", 1, 1024, 30, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_5);
}
static int tc_41(void)
{
	return tc_run_while_read_reg("tc_41", 1, 1024, 30, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_6);
}
static int tc_42(void)
{
	return tc_run_while_read_reg("tc_42", 1, 1024, 30, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_7);
}
static int tc_43(void)
{
	return tc_run_while_read_reg("tc_43", 1, 1024, 30, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_GLOBAL);
}
static int tc_44(void)
{
	return tc_run_while_read_reg("tc_44", 1, 1024, 30, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_1 |
				     DMA_TC_READ_PHY_CHAN_2 |
				     DMA_TC_READ_PHY_CHAN_3 |
				     DMA_TC_READ_PHY_CHAN_4 |
				     DMA_TC_READ_PHY_CHAN_5 |
				     DMA_TC_READ_PHY_CHAN_6 |
				     DMA_TC_READ_PHY_CHAN_7 |
				     DMA_TC_READ_GLOBAL);
}
static int tc_45(void)
{
	return tc_run_while_read_reg("tc_45", 1, 1024, 30, 10, true, false,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_1 |
				     DMA_TC_READ_PHY_CHAN_2 |
				     DMA_TC_READ_PHY_CHAN_3 |
				     DMA_TC_READ_PHY_CHAN_4 |
				     DMA_TC_READ_PHY_CHAN_5 |
				     DMA_TC_READ_PHY_CHAN_6 |
				     DMA_TC_READ_PHY_CHAN_7 |
				     DMA_TC_READ_GLOBAL);
}
static int tc_46(void)
{
	return tc_run_while_read_reg("tc_46", 1, 128, 100, 10, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_1 |
				     DMA_TC_READ_PHY_CHAN_2 |
				     DMA_TC_READ_PHY_CHAN_3 |
				     DMA_TC_READ_PHY_CHAN_4 |
				     DMA_TC_READ_PHY_CHAN_5 |
				     DMA_TC_READ_PHY_CHAN_6 |
				     DMA_TC_READ_PHY_CHAN_7 |
				     DMA_TC_READ_GLOBAL);
}
static int tc_47(void)
{
	return tc_run_while_read_reg("tc_47", 1, 128, 100, 10, true, false,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_1 |
				     DMA_TC_READ_PHY_CHAN_2 |
				     DMA_TC_READ_PHY_CHAN_3 |
				     DMA_TC_READ_PHY_CHAN_4 |
				     DMA_TC_READ_PHY_CHAN_5 |
				     DMA_TC_READ_PHY_CHAN_6 |
				     DMA_TC_READ_PHY_CHAN_7 |
				     DMA_TC_READ_GLOBAL);
}
static int tc_48(void)
{
	return tc_run_while_read_reg("tc_48", 1, 128, 200, 1000, true, true,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_1 |
				     DMA_TC_READ_PHY_CHAN_2 |
				     DMA_TC_READ_PHY_CHAN_3 |
				     DMA_TC_READ_PHY_CHAN_4 |
				     DMA_TC_READ_PHY_CHAN_5 |
				     DMA_TC_READ_PHY_CHAN_6 |
				     DMA_TC_READ_PHY_CHAN_7 |
				     DMA_TC_READ_GLOBAL);
}
static int tc_49(void)
{
	return tc_run_while_read_reg("tc_49", 1, 128, 200, 1000, true, false,
				     DMA_PREP_INTERRUPT,
				     DMA_TC_READ_PHY_CHAN_1 |
				     DMA_TC_READ_PHY_CHAN_2 |
				     DMA_TC_READ_PHY_CHAN_3 |
				     DMA_TC_READ_PHY_CHAN_4 |
				     DMA_TC_READ_PHY_CHAN_5 |
				     DMA_TC_READ_PHY_CHAN_6 |
				     DMA_TC_READ_PHY_CHAN_7 |
				     DMA_TC_READ_GLOBAL);
}
#endif

static int tc_50(void)
{
	struct tc_parallel tcp = {
		.str = "tc_50",
		.max_channels = 5,
		.laps = 200,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 3,
		.tx_align = 4,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);
}

static int tc_51(void)
{
	return tc_pause_and_unpause_parallel(2, "tc_51",
			DMA_PREP_INTERRUPT | DMA_CTRL_ACK, 4);
}

static int tc_52(void)
{
	return tc_pause_and_unpause_parallel(4, "tc_52",
			DMA_PREP_INTERRUPT | DMA_CTRL_ACK, 4);
}

static int tc_53(void)
{
	struct tc_parallel tcp = {
		.str = "tc_53",
		.max_channels = 5,
		.laps = 200,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 3,
		.tx_align = 1,
		.const_size = -1,
		.list_len = 30,
		.timeout = DEFAULT_TIMEOUT,
	};

	return tc_run_parallel(&tcp);

}

static int tc_54_trigger(void)
{
	struct tc_parallel tcp = {
		.str = "tc_54",
		.max_channels = 5,
		.laps = 1,
		.dma_flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		.chan_start_index = 0,
		.tx_align = 1,
		.const_size = 1024,
		.list_len = 4, /* Must >2 to tigger error, when we had 4 log
				  memcpy  channsl */
		.timeout = 1000, /* 1s*/
	};

	return tc_run_parallel(&tcp);
}

struct tc_link_cfg {
	int tc;
	int jobs;
	int links;
	int buffer_size;
	bool request_phys;
	bool start_in_sequence;
	bool slow_start;
	/* randomize */
	u32 min_size;
	u32 max_size;
};

struct tc_link {
	struct buflist buflist;
	struct completion done;
	struct dma_async_tx_descriptor *desc;
};


static void tc_link_callback(void *data)
{
	struct tc_link *tc = data;
	complete(&tc->done);
}

/* used for testing job linking */
static int tc_link_core(struct tc_link_cfg *cfg)
{

	int i;
	int err = 0;
	char name[MAX_NAME_LEN];
	struct dma_chan *dma_chan = NULL;

	struct tc_link *tc;


	tc = kmalloc(cfg->jobs * sizeof(struct tc_link), GFP_KERNEL);
	if (!tc)
		goto done;

	for (i = 0 ; i < cfg->jobs; i++) {

		snprintf(name, MAX_NAME_LEN - 1, "%s_tc_%d_%.2d",
			 __func__, cfg->tc, i);
		err = dmatest_buflist_create(&tc[i].buflist, cfg->links, 0,
					     name,
					     DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
					     DEFAULT_TIMEOUT, cfg->request_phys,
					     dma_chan);
		if (err)
			/* Leaks previous allocated buflists */
			goto done;
		if (cfg->min_size && cfg->max_size)
			dmatest_sizelist_randomize(&tc->buflist, cfg->min_size,
						   cfg->max_size, TX_ALIGN);
		else
			dmatest_sizelist_set(&tc[i].buflist, cfg->buffer_size,
					     TX_ALIGN);

		dmatest_buflist_alloc(&tc[i].buflist);

		dma_chan = tc[i].buflist.dma_chan;

		tc[i].buflist.sgl_src_len =
			dma_map_sg(tc[i].buflist.dma_chan->device->dev,
				   tc[i].buflist.sgl_src,
				   tc[i].buflist.list_len,
				   DMA_BIDIRECTIONAL);
		tc[i].buflist.sgl_dst_len =
			dma_map_sg(tc[i].buflist.dma_chan->device->dev,
				   tc[i].buflist.sgl_dst,
				   tc[i].buflist.list_len,
				   /* Both direction for verifying transfered data */
				   DMA_BIDIRECTIONAL);

		tc[i].desc = dma_chan->device->device_prep_dma_sg(dma_chan,
						tc[i].buflist.sgl_dst,
						tc[i].buflist.sgl_dst_len,
						tc[i].buflist.sgl_src,
						tc[i].buflist.sgl_src_len,
						tc[i].buflist.dma_engine_flags);
		init_completion(&tc[i].done);
		tc[i].desc->callback = tc_link_callback;
		tc[i].desc->callback_param = &tc[i];
	}

	if (cfg->start_in_sequence) {
		for (i = 0; i < cfg->jobs; i++) {
			tc[i].desc->tx_submit(tc[i].desc);
			dma_async_issue_pending(dma_chan);

			if (cfg->slow_start)
				udelay(500);
		}
	} else {

		for (i = 0; i < cfg->jobs; i++)
			tc[i].desc->tx_submit(tc[i].desc);

		dma_async_issue_pending(dma_chan);
	}

	for (i = 0; i < cfg->jobs; i++) {
		wait_for_completion_interruptible(&tc[i].done);
		err |= dmatest_buflist_payload_check(&tc[i].buflist);
	}

	for (i = 0; i < cfg->jobs; i++) {
		tc[i].buflist.dma_chan = dma_chan;
		dmatest_buflist_destroy(&tc[i].buflist);
		dma_chan = NULL;
	}
done:
	kfree(tc);
	return err;
}

static int tc_55(void)
{
	/* Link 2 jobs in hw before start transfer (physical) */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 55,
		.jobs		   = 2,
		.links		   = 5, /* Just > 1 is enough */
		.buffer_size	   = SZ_4K, /* Just something big */
		.request_phys      = true,
		.start_in_sequence = false,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_56(void)
{
	/* Link 2 jobs in hw after first job has started (physical) */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 56,
		.jobs		   = 2,
		.links		   = 5, /* Just > 1 is enough */
		.buffer_size	   = SZ_4K, /* Just something big */
		.request_phys      = true,
		.start_in_sequence = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_57(void)
{
	/* Link 2 jobs in hw after first job has started (physical) */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 57,
		.jobs		   = 2,
		.links		   = 1, /* No links */
		.buffer_size	   = SZ_16K, /* Just something big */
		.request_phys      = true,
		.start_in_sequence = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_58(void)
{
	/* Link 10 jobs in hw after first job has started (physical) */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 58,
		.jobs		   = 10,
		.links		   = 1, /* No links */
		.buffer_size	   = SZ_16K, /* Just something big */
		.request_phys      = true,
		.start_in_sequence = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_59(void)
{
	/* Link 10 jobs in hw after first job has started (physical) */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 59,
		.jobs		   = 10,
		.links		   = 10,
		.buffer_size	   = SZ_4K, /* Just something big */
		.request_phys      = true,
		.start_in_sequence = true,
	};

	return tc_link_core(&tc_cfg);
}


static int tc_60(void)
{
	/* Link 2 jobs in hw before start transfer (logical) */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 60,
		.jobs		   = 2,
		.links		   = 2, /* Just > 1 is enough */
		.buffer_size	   = 4096, /* Just something big */
		.request_phys      = false,
		.start_in_sequence = false,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_61(void)
{
	/* Link 2 jobs in hw after first job has started (logical) */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 61,
		.jobs		   = 2,
		.links		   = 2, /* Just > 1 is enough */
		.buffer_size	   = 4096, /* Just something big */
		.request_phys      = false,
		.start_in_sequence = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_62(void)
{
	/*
	 * Test to transfer a logical job with >64 links (Out of lcla space
	 * then.)
	 */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 62,
		.jobs		   = 1,
		.links		   = 90,
		.buffer_size	   = 128,
		.request_phys      = false,
		.start_in_sequence = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_63(void)
{
	/*
	 * Test to transfer a logical job with >124 links (Out of lcla space
	 * then.)
	 */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 63,
		.jobs		   = 1,
		.links		   = 140,
		.buffer_size	   = 128,
		.request_phys      = false,
		.start_in_sequence = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_64(void)
{
	/* Test allocate 4 80 lli long jobs before starting */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 64,
		.jobs		   = 4,
		.links		   = 80,
		.buffer_size	   = 128,
		.request_phys      = false,
		.start_in_sequence = false,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_65(void)
{
	/* Link 10 jobs in hw after first job has started (logical) */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 59,
		.jobs		   = 10,
		.links		   = 10,
		.buffer_size	   = SZ_4K, /* Just something big */
		.request_phys      = false,
		.start_in_sequence = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_66(void)
{
	/*
	 * Link 10 jobs in hw after first job has started (logical),
	 * no links
	 */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 58,
		.jobs		   = 10,
		.links		   = 1, /* No links */
		.buffer_size	   = SZ_16K, /* Just something big */
		.request_phys      = false,
		.start_in_sequence = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_67(void)
{
	/* Link 10 jobs in hw after first job has started (logical), slowly */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 59,
		.jobs		   = 10,
		.links		   = 10,
		.buffer_size	   = SZ_4K, /* Just something big */
		.request_phys      = false,
		.start_in_sequence = true,
		.slow_start	   = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_68(void)
{
	/* Link 10 jobs in hw after first job has started (physical), slowly */
	struct tc_link_cfg tc_cfg = {
		.tc		   = 59,
		.jobs		   = 10,
		.links		   = 10,
		.buffer_size	   = SZ_4K, /* Just something big */
		.request_phys      = true,
		.start_in_sequence = true,
		.slow_start	   = true,
	};

	return tc_link_core(&tc_cfg);
}

static int tc_69(void)
{
	int err = 0;
	int i;

	/* Test large transfer than 64k */
	struct tc_link_cfg tc_cfg[] = {
		{
		.tc		   = 69,
		.jobs		   = 1,
		.links		   = 10,
		.buffer_size	   = 0x10000 - 1,
		},
		{
		.tc		   = 69,
		.jobs		   = 1,
		.links		   = 10,
		.buffer_size	   = 0x10000,
		},
		{
		.tc		   = 69,
		.jobs		   = 1,
		.links		   = 10,
		.buffer_size	   = 0x10000 + 1,
		},
		{
		.tc		   = 69,
		.jobs		   = 1,
		.links		   = 10,
		.buffer_size	   = 2*0x10000 - 1,
		},
		{
		.tc		   = 69,
		.jobs		   = 1,
		.links		   = 10,
		.buffer_size	   = 2*0x10000,
		},
		{
		.tc		   = 69,
		.jobs		   = 1,
		.links		   = 10,
		.buffer_size	   = 2*0x10000 + 1,
		},
	};
	for (i = 0; i < ARRAY_SIZE(tc_cfg); i++) {
		printk(KERN_INFO "[%s] %d out of %d\n", __func__,
		       i, ARRAY_SIZE(tc_cfg));
		err = tc_link_core(&tc_cfg[i]);
		if (err)
			goto out;
	}

 out:
	return err;
}

static int tc_nop(struct seq_file *s, int id)
{
	int err;
	err = seq_printf(s, "TEST_%d is removed\n", id);
	return err;
}

static int d40_test_run(struct seq_file *s, void *iter)
{
	int test_id = (int) s->private;
	int err = -EINVAL;
	char *str = "";

	err = mutex_lock_interruptible(&tc_mutex);

	/* the out put from these tests are actually a test script
	 * echo is needed in order to display the result
	 */
#if defined(CONFIG_MMC) && defined(CONFIG_STE_DMA40_DEBUG)
	switch (test_id) {
	case TEST10:
	case TEST11:
	case TEST12:
	case TEST13:
		str = "echo ";
		break;
	}
#endif
	if (err)
		goto out;

	switch (test_id) {
	case TEST1:
		err = tc_1_fixed_buffer();
		break;
	case TEST2:
		err = tc_2_fixed_buffer();
		break;
	case TEST3:
		err = tc_3_fixed_buffer();
		break;
	case TEST4:
		err = tc_4_fixed_buffer();
		break;
	case TEST5:
		err = tc_5_random_buffer();
		break;
	case TEST6:
		err = tc_6_sg_buffer();
		break;
	case TEST7:
		err = tc_7_sg_buffer();
		break;
	case TEST8:
		err = tc_8_sg_buffer();
		break;
	case TEST9:
		err = tc_9_sg_buffer();
		break;
#if defined(CONFIG_MMC) && defined(CONFIG_STE_DMA40_DEBUG)
	case TEST10:
		err = tc_10_sg_size_1024_one(s);
		str = "echo ";
		break;
	case TEST11:
		err = tc_11_sg_size_chain_one(s);
		str = "echo ";
		break;
	case TEST12:
		err = tc_12_sg_size_1024_many(s);
		str = "echo ";
		break;
	case TEST13:
		err = tc_13_sg_size_1024_chain_many(s);
		str = "echo ";
		break;
	case TEST14:
		err = tc_14_sg_buffer_temporary_endless();
		break;
#else
	case TEST10:
	case TEST11:
	case TEST12:
	case TEST13:
	case TEST14:
		err = tc_nop(s, test_id);
		break;
#endif
	case TEST15:
		err = tc_15_static_buffer();
		break;
	case TEST16:
		err = tc_16_static_buffer();
		break;
	case TEST17:
		err = tc_17_static_buffer();
		break;
	case TEST18:
		err = tc_18_static_buffer();
		break;
	case TEST19:
		err = tc_19_static_buffer();
		break;
	case TEST20:
		err = tc_20_random_buffer();
		break;
	case TEST21:
		err = tc_21_stop_and_go();
		break;
	case TEST22:
	case TEST23:
	case TEST24:
	case TEST25:
		err = tc_22_req();
		if (!err)
			err = tc_23_no_irq();
		if (!err)
			err = tc_24_irq();
		if (!err)
			err = tc_25_free();
		break;
	case TEST26:
		err = tc_26_run_3_parallel();
		break;
	case TEST27:
		err = tc_27_run_4_parallel();
		break;
	case TEST28:
		err = tc_28_run_5_parallel();
		break;
	case TEST29:
		err = tc_29_run_6_parallel();
		break;
	case TEST30:
		err = tc_30_run_7_parallel();
		break;
	case TEST31:
		err = tc_31_run_128_parallel();
		break;
	case TEST32:
		err = tc_32_run_1_parallel();
		break;
	case TEST33:
		err = tc_33_run_2_parallel();
		break;
	case TEST34:
		err = tc_34_run_pause_and_unpause_parallel();
		break;
	case TEST35:
		err = tc_35_run_1_parallel_reuse();
		break;

#ifdef CONFIG_STE_DMA40_DEBUG
	case TEST36:
		err = tc_36();
		break;
	case TEST37:
		err = tc_37();
		break;
	case TEST38:
		err = tc_38();
		break;
	case TEST39:
		err = tc_39();
		break;
	case TEST40:
		err = tc_40();
		break;
	case TEST41:
		err = tc_41();
		break;
	case TEST42:
		err = tc_42();
		break;
	case TEST43:
		err = tc_43();
		break;
	case TEST44:
		err = tc_44();
		break;
	case TEST45:
		err = tc_45();
		break;
	case TEST46:
		err = tc_46();
		break;
	case TEST47:
		err = tc_47();
		break;
	case TEST48:
		err = tc_48();
		break;
	case TEST49:
		err = tc_49();
		break;
#else
	case TEST36:
	case TEST37:
	case TEST38:
	case TEST39:
	case TEST40:
	case TEST41:
	case TEST42:
	case TEST43:
	case TEST44:
	case TEST45:
	case TEST46:
	case TEST47:
	case TEST48:
	case TEST49:
		err = tc_nop(s, test_id);
		break;
#endif
	case TEST50:
		err = tc_50();
		break;
	case TEST51:
		err = tc_51();
		break;
	case TEST52:
		err = tc_52();
		break;
	case TEST53:
		err = tc_53();
		break;
	case TEST54:
		err = tc_54_trigger();
		break;
	case TEST55:
		err = tc_55();
		break;
	case TEST56:
		err = tc_56();
		break;
	case TEST57:
		err = tc_57();
		break;
	case TEST58:
		err = tc_58();
		break;
	case TEST59:
		err = tc_59();
		break;
	case TEST60:
		err = tc_60();
		break;
	case TEST61:
		err = tc_61();
		break;
	case TEST62:
		err = tc_62();
		break;
	case TEST63:
		err = tc_63();
		break;
	case TEST64:
		err = tc_64();
		break;
	case TEST65:
		err = tc_65();
		break;
	case TEST66:
		err = tc_66();
		break;
	case TEST67:
		err = tc_67();
		break;
	case TEST68:
		err = tc_68();
		break;
	case TEST69:
		err = tc_69();
		break;

	default:
		err = -EINVAL;
		printk(KERN_INFO "# [%s] Invalid test id %d\n", __func__,
		       test_id);
	}

out:
	seq_printf(s, "%sFinished test %d: %s\n", str, test_id,
		   err == 0 ? "OK" : "***FAIL***");

	mutex_unlock(&tc_mutex);
	return 0;
}


static struct dentry *debugfs_dir;

static int d40_debugfs_open(struct inode *inode,
			       struct file *file)
{
	int err;

	err = single_open(file,
			  d40_test_run,
			  inode->i_private);

	return err;
}

static const struct file_operations d40_debugfs_ops = {
	.open		= d40_debugfs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init stedma40_test_init(void)
{
	char name[32];
	int i;
	int err = 0;
	void *err_ptr = NULL;

	err = mutex_lock_interruptible(&tc_mutex);
	if (err)
		goto err;

	printk(KERN_INFO "[%s] dma-test-module build: %s %s nbr tests %d\n",
	       __func__, __DATE__, __TIME__, NBR_TESTS - 1);

	debugfs_dir = debugfs_create_dir("ste_dma40_test", NULL);
	if (IS_ERR(debugfs_dir)) {
		err = PTR_ERR(debugfs_dir);
		goto out;
	}

	for (i = 1; i < NBR_TESTS; i++) {
		err = snprintf(name, 32, "test_%d", i);
		if (err < 0)
			goto out;
		err = 0;

		err_ptr = debugfs_create_file(name,
					      S_IFREG | S_IRUGO,
					      debugfs_dir, (void *)i,
					      &d40_debugfs_ops);
		if (IS_ERR(err_ptr)) {
			err = PTR_ERR(err_ptr);
			goto out;
		}
	}

 out:
	mutex_unlock(&tc_mutex);
 err:
	return err;
}
module_init(stedma40_test_init);

static void __exit stedma40_test_exit(void)
{
	DBG_TEST(printk(KERN_INFO "[%s]\n", __func__));

	sted40_history_reset();

	debugfs_remove_recursive(debugfs_dir);
}
module_exit(stedma40_test_exit);


