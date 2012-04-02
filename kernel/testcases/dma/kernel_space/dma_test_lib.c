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
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <plat/ste_dma40.h>

#include "dma_test_lib.h"

#ifdef CONFIG_STE_DMA40_DEBUG
extern void sted40_history_dump(void);
#else
static void sted40_history_dump(void)
{
}

#endif

MODULE_DESCRIPTION("DMA test lib: Support functions for DMA testing");
MODULE_LICENSE("GPL");

/* To get various debug prints put an x after this macro */
#define DBG_PRINT(x)
#define DBG_FAULT_PRINT(x) x


static void transfer_timedout(struct work_struct *work)
{
	int i;
	struct buflist *buflist;

	buflist = container_of(work, struct buflist, timed_out.work);

	DBG_FAULT_PRINT(printk(KERN_ERR "dma_test_lib: ERROR - DMA transfer timed out!\n"));
	DBG_FAULT_PRINT(printk(KERN_ERR "Testcase: %s failed\n",
			       buflist->name));


	for (i = 0; buflist->list_size[i] != -1; i++) {
		printk(KERN_INFO "%p Transfer %d bytes from phy %x to %x\n",
		       buflist,
		       buflist->list_size[i],
		       buflist->list_src_phy[i],
		       buflist->list_dst_phy[i]);
	}
	printk(KERN_INFO "%p %d jobs\n", buflist, i);

	sted40_history_dump();

}

static void transmit_cb(void *data)
{
	struct buflist *buflist = data;
	unsigned long flags;

	DBG_PRINT(printk
		  (KERN_INFO "[%s] finished_jobs %d\n", __func__,
		   buflist->finished_jobs));

	cancel_delayed_work(&buflist->timed_out);

	spin_lock_irqsave(&buflist->lock, flags);
	buflist->finished_jobs++;
	spin_unlock_irqrestore(&buflist->lock, flags);

	if (buflist->callback)
		buflist->callback(buflist);
}

int dmatest_buflist_create(struct buflist *buflist, int length,
			   int end_padding,
			   char *name, unsigned long dma_engine_flags,
			   int timeout, bool request_phy_chan,
			   struct dma_chan *dma_chan)
{
	dma_cap_mask_t mask;

	DBG_PRINT(printk(KERN_INFO "[%s]\n", __func__));
	buflist->is_dma_prepared = false;
	buflist->dma_engine_flags = dma_engine_flags;

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);
	if (request_phy_chan)
		dma_cap_set(DMA_SLAVE, mask);

	if (dma_chan)
		buflist->dma_chan = dma_chan;
	else
		buflist->dma_chan = dma_request_channel(mask, NULL, NULL);

	if (buflist->dma_chan == NULL) {
		DBG_PRINT(printk(KERN_ERR "[%s] dma_request_channel failed\n",
				 __func__));
		return -EINVAL;
	}

	spin_lock_init(&buflist->lock);

	buflist->list_len = length;
	buflist->finished_jobs = 0;
	strncpy(buflist->name, name, MAX_NAME_LEN-1);

	buflist->sent_jobs = 0;

	buflist->list_size =
	    kmalloc(sizeof(buflist->list_size) * length + 1, GFP_KERNEL);
	buflist->list_buf_dst =
		kmalloc(sizeof(buflist->list_buf_dst) * length, GFP_KERNEL);
	buflist->list_buf_dst_real =
		kmalloc(sizeof(buflist->list_buf_dst_real) * length,
			GFP_KERNEL);
	buflist->list_buf_src =
	    kmalloc(sizeof(buflist->list_buf_src) * length, GFP_KERNEL);
	buflist->desc =
	    kmalloc(sizeof(struct dma_async_tx_descriptor *) * length,
		    GFP_KERNEL);
	buflist->cookie =
	    kmalloc(sizeof(dma_cookie_t) * length,
		    GFP_KERNEL);
	buflist->list_dst_phy =	kmalloc(sizeof(dma_addr_t) * length,
			GFP_KERNEL);
	buflist->list_src_phy =	kmalloc(sizeof(dma_addr_t) * length,
			GFP_KERNEL);
	buflist->sgl_src =
	    kmalloc(sizeof(struct scatterlist) * length, GFP_KERNEL);
	buflist->sgl_dst =
	    kmalloc(sizeof(struct scatterlist) * length, GFP_KERNEL);

	sg_init_table(buflist->sgl_src, length);
	sg_init_table(buflist->sgl_dst, length);

	buflist->end_padding = end_padding;
	buflist->timeout_len = timeout;
	buflist->callback = NULL;

	INIT_DELAYED_WORK_DEFERRABLE(&buflist->timed_out, transfer_timedout);

	return 0;
}

void dmatest_buflist_destroy(struct buflist *buflist)
{
	DBG_PRINT(printk(KERN_INFO "[%s]\n", __func__));

	if (buflist->dma_chan)
		dma_release_channel(buflist->dma_chan);

	dmatest_buflist_free(buflist);

	kfree(buflist->list_size);
	kfree(buflist->list_buf_dst);
	kfree(buflist->list_buf_dst_real);
	kfree(buflist->list_buf_src);
	kfree(buflist->desc);
	kfree(buflist->cookie);
	kfree(buflist->list_dst_phy);
	kfree(buflist->list_src_phy);
	kfree(buflist->sgl_src);
	kfree(buflist->sgl_dst);

	buflist->list_size = NULL;
	buflist->list_buf_dst = NULL;
	buflist->list_buf_dst_real = NULL;
	buflist->list_buf_src = NULL;
	buflist->desc = NULL;
	buflist->cookie = NULL;
	buflist->list_dst_phy = NULL;
	buflist->list_src_phy = NULL;
	buflist->sgl_src = NULL;
	buflist->sgl_dst = NULL;
}

void dmatest_sizelist_randomize(struct buflist *buflist, u32 min, u32 max, int align)
{
	int i;
	struct timespec ts = current_kernel_time();

	DBG_PRINT(printk(KERN_INFO "[%s]\n", __func__));

	srandom32(ts.tv_nsec);

	for (i = 0; i < buflist->list_len; i++) {
		buflist->list_size[i] =
			ALIGN(random32() / (0xFFFFFFFF / (max - min)) + min, align);
/*		DBG_PRINT(printk(KERN_INFO "rand value %d , min %d max %d\n", */
/*				 buflist->list_size[i], min, max)); */

		/* TODO replace BUG_ON:s with error report, return -1 */
		BUG_ON(buflist->list_size[i] > ALIGN(max, align));
	}

	buflist->list_size[i] = -1;
}

void dmatest_sizelist_set(struct buflist *buflist, u32 value, int align)
{
	int i;

	DBG_PRINT(printk(KERN_INFO "[%s]\n", __func__));

	for (i = 0; i < buflist->list_len; i++) {
		buflist->list_size[i] = ALIGN(value, align);
	}

	buflist->list_size[i] = -1;
}

void dmatest_buflist_alloc(struct buflist *buflist)
{
	int i;
	int buf_i;
	int size;

	DBG_PRINT(printk(KERN_INFO "[%s]\n", __func__));

	for (buf_i = 0; buflist->list_size[buf_i] != -1; buf_i++) {
		size = buflist->list_size[buf_i];
		buflist->list_buf_dst_real[buf_i] =
		    kmalloc(size + buflist->end_padding + 2 * dma_get_cache_alignment(),
			    GFP_KERNEL);
		buflist->list_buf_dst[buf_i] = PTR_ALIGN(buflist->list_buf_dst_real[buf_i],
							 dma_get_cache_alignment());
		buflist->list_buf_src[buf_i] =
		    kmalloc(size + buflist->end_padding, GFP_KERNEL);

		for (i = 0; i < size; i++) {
			buflist->list_buf_src[buf_i][i] = (u8) (i + buf_i);
			buflist->list_buf_dst[buf_i][i] = 0xAA;
		}
		for (i = size; i < size + buflist->end_padding; i++) {
			buflist->list_buf_src[buf_i][i] = 0xBE;
			buflist->list_buf_dst[buf_i][i] = 0xBE;
		}
		if (buflist->end_padding) {
			dma_map_single(buflist->dma_chan->device->dev,
				       &buflist->list_buf_dst[buf_i][size],
				       buflist->end_padding,
				       DMA_BIDIRECTIONAL);
			dma_map_single(buflist->dma_chan->device->dev,
				       &buflist->list_buf_src[buf_i][size],
				       buflist->end_padding,
				       DMA_BIDIRECTIONAL);
		}
	}

	for (i = 0; buflist->list_size[i] != -1; i++) {
		sg_set_buf(&buflist->sgl_src[i],
			   buflist->list_buf_src[i],
			   buflist->list_size[i]);
		sg_set_buf(&buflist->sgl_dst[i],
			   buflist->list_buf_dst[i],
			   buflist->list_size[i]);
	}

}

void dmatest_buflist_free(struct buflist *buflist)
{
	int i;

	DBG_PRINT(printk(KERN_INFO "[%s]\n", __func__));

	for (i = 0; buflist->list_size[i] != -1; i++) {
		kfree(buflist->list_buf_dst_real[i]);
		kfree(buflist->list_buf_src[i]);

		buflist->list_buf_dst[i] = 0;
		buflist->list_buf_src[i] = 0;
	}

}

int dmatest_buflist_payload_check(struct buflist *buflist)
{
	int err = 0;
	int i;

	DBG_PRINT(printk(KERN_INFO "[%s]\n", __func__));

	if (buflist->sgl_src_len)
		dma_unmap_sg(buflist->dma_chan->device->dev,
			     buflist->sgl_src, buflist->sgl_src_len,
			     DMA_BIDIRECTIONAL);

	if (buflist->sgl_dst_len)
		dma_unmap_sg(buflist->dma_chan->device->dev,
			     buflist->sgl_dst, buflist->sgl_dst_len,
			     DMA_BIDIRECTIONAL);

	for (i = 0; buflist->list_size[i] != -1; i++) {
		if (!buflist->sgl_src_len)
			dma_unmap_single(buflist->dma_chan->device->dev,
					 buflist->list_src_phy[i],
					 buflist->list_size[i], DMA_BIDIRECTIONAL);

		if (!buflist->sgl_dst_len)
			dma_unmap_single(buflist->dma_chan->device->dev,
					 buflist->list_dst_phy[i],
					 buflist->list_size[i], DMA_BIDIRECTIONAL);

		if (memcmp
		    (buflist->list_buf_dst[i], buflist->list_buf_src[i],
		     buflist->list_size[i]) != 0) {

			DBG_FAULT_PRINT(printk
					(KERN_INFO "[%s] fault index %d\n",
					 __func__, i));

			dmatest_buflist_payload_printdiff(buflist, i);
			err = -1;
		}
	}

	if (err) {
		DBG_FAULT_PRINT(printk(KERN_INFO "Testcase: %s failed\n", buflist->name));
		sted40_history_dump();
	}

	return err;
}

#define MAX_ERROR_PRINTS 16
void dmatest_buflist_payload_printdiff(struct buflist *buflist,
				       u32 buf_index)
{
	int i, el = 0;
	int size = buflist->list_size[buf_index];
	u8 *buf_dst = buflist->list_buf_dst[buf_index];
	u8 *buf_src = buflist->list_buf_src[buf_index];

	DBG_FAULT_PRINT(printk
			(KERN_INFO "[%s] fault buffer index %d dst at phy: 0x%lx\n",
			 __func__, buf_index, virt_to_phys(buf_dst)));

	for (i = 0; i < size; i++) {
		if (buf_dst[i] != buf_src[i]) {
			DBG_FAULT_PRINT(printk
					(KERN_INFO
					 "Buffer fault index %d: "
					 "DEST data 0x%x Virt addr %p, phy 0x%x, SRC data: 0x%x\n",
					 i, buf_dst[i],
					 buflist->list_buf_dst[buf_index] + i,
					 buflist->list_dst_phy[buf_index] + i,
					 buf_src[i]));
			el++;
			if (el == MAX_ERROR_PRINTS)
				break;
		}
	}
}

void dmatest_buflist_start_single(struct buflist *buflist,
				  void (*callback)(struct buflist *bf))
{
	int i;

	DBG_PRINT(printk(KERN_INFO "[%s]\n", __func__));

	if ((buflist->dma_engine_flags & DMA_CTRL_ACK) ||
	    !buflist->is_dma_prepared) {
		for (i = 0; buflist->list_size[i] != -1; i++) {

			buflist->list_src_phy[i] =
				dma_map_single(buflist->dma_chan->device->dev,
					       buflist->list_buf_src[i],
					       buflist->list_size[i],
					       DMA_BIDIRECTIONAL);

			buflist->list_dst_phy[i] =
				dma_map_single(buflist->dma_chan->device->dev,
					       buflist->list_buf_dst[i],
					       buflist->list_size[i],
					       DMA_BIDIRECTIONAL);

			buflist->desc[i] = buflist->dma_chan->device->
				device_prep_dma_memcpy(buflist->dma_chan,
						       buflist->list_dst_phy[i],
						       buflist->list_src_phy[i],
						       buflist->list_size[i],
						       buflist->dma_engine_flags);

			buflist->desc[i]->callback = transmit_cb;
			buflist->desc[i]->callback_param = buflist;
		}
		buflist->is_dma_prepared = true;
	}
	buflist->callback = callback;

	buflist->sgl_src_len = 0;
	buflist->sgl_dst_len = 0;

	for (i = 0; buflist->list_size[i] != -1; i++) {
		buflist->cookie[i] = buflist->desc[i]->tx_submit(buflist->desc[i]);
	}
	schedule_delayed_work(&buflist->timed_out,
			      msecs_to_jiffies(buflist->timeout_len));

	dma_async_issue_pending(buflist->dma_chan);
}

void dmatest_buflist_start_sg(struct buflist *buflist,
			      void (*callback)(struct buflist *bf))
{
	DBG_PRINT(printk(KERN_INFO "[%s]\n", __func__));

	buflist->sgl_src_len = dma_map_sg(buflist->dma_chan->device->dev,
					  buflist->sgl_src, buflist->list_len,
					  DMA_BIDIRECTIONAL);
	buflist->sgl_dst_len = dma_map_sg(buflist->dma_chan->device->dev,
					  buflist->sgl_dst, buflist->list_len,
					  /* Both direction for verifying transfered data */
					  DMA_BIDIRECTIONAL);

	if ((buflist->dma_engine_flags & DMA_CTRL_ACK) ||
	    !buflist->is_dma_prepared) {
		buflist->desc[0] = buflist->dma_chan->device->device_prep_dma_sg(buflist->dma_chan,
						      buflist->sgl_dst,
						      buflist->sgl_dst_len,
						      buflist->sgl_src,
						      buflist->sgl_src_len,
						      buflist->dma_engine_flags);

		buflist->desc[0]->callback = transmit_cb;
		buflist->desc[0]->callback_param = buflist;
		buflist->is_dma_prepared = true;
	}

	buflist->callback = callback;

	buflist->cookie[0] = buflist->desc[0]->tx_submit(buflist->desc[0]);
	schedule_delayed_work(&buflist->timed_out,
			      msecs_to_jiffies(buflist->timeout_len));

	dma_async_issue_pending(buflist->dma_chan);

}

void dmatest_buflist_reset_nbr_finished(struct buflist *buflist)
{
	unsigned long flags;

	spin_lock_irqsave(&buflist->lock, flags);

	buflist->finished_jobs = 0;
	buflist->sent_jobs = 0;
	spin_unlock_irqrestore(&buflist->lock, flags);
}

int dmatest_buflist_get_nbr_finished(struct buflist *buflist)
{
	unsigned long flags;
	u32 ret;

	spin_lock_irqsave(&buflist->lock, flags);
	ret = buflist->finished_jobs;
	spin_unlock_irqrestore(&buflist->lock, flags);

	return ret;
}

