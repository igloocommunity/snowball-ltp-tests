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
#include <linux/dmaengine.h>

#define MAX_NAME_LEN 128

struct buflist {
	spinlock_t lock;

	struct dma_async_tx_descriptor **desc;
	char name[MAX_NAME_LEN];
	int *list_size;
	dma_addr_t *list_dst_phy;
	dma_addr_t *list_src_phy;
	u8 **list_buf_dst;
	u8 **list_buf_dst_real;
	u8 **list_buf_src;
	struct scatterlist *sgl_dst;
	struct scatterlist *sgl_src;
	unsigned int sgl_dst_len;
	unsigned int sgl_src_len;

	unsigned long dma_engine_flags;
	bool is_dma_prepared;
	u32 list_len;

	struct dma_chan *dma_chan;
	dma_cookie_t *cookie;

	void (*callback)(struct buflist *bf);
	struct workqueue_struct *wq;
	u32 finished_jobs;
	u32 sent_jobs;

	struct delayed_work timed_out;
	int timeout_len;
	u32 end_padding;
};

/* Initiate dma test
 *
 * returns 0 on success otherwise none zero
 */
int dmatest_init(void);


/** Creates a buffer list
 * @length specifies length of buffer and size list
 *
 */
int dmatest_buflist_create(struct buflist *buflist, int length,
			   int end_padding,
			   char *name, unsigned long dma_engine_flags,
			   int timeout, bool request_phy_chan, struct dma_chan *dma_chan);

/* Destroys a buffer list, calls dmatest_buflist_free if list is != NULL */
void dmatest_buflist_destroy(struct buflist *buflist);

/* Allocates buffers according to size list and writes a pattern to the buffer,
 * dmatest_sizelist_randomize or dmatest_sizelist_set must be
 * called before this function
 */
void dmatest_buflist_alloc(struct buflist *buflist);

/* Frees the buffers allocate by dmatest_buflist_alloc */
void dmatest_buflist_free(struct buflist *buflist);

/* Initialize size list with random values between min and max */
void dmatest_sizelist_randomize(struct buflist *buflist, u32 min, u32 max, int align);

/* Initialize size list with value */
void dmatest_sizelist_set(struct buflist *buflist, u32 value, int align);

/* Start sending the buflist to DMA as many single jobs, calls callback when done */
void dmatest_buflist_start_single(struct buflist *buflist,
				  void (*callback)(struct buflist *bf));

/* Start sending the buflist to DMA as a scatter-gatter list, calls callback when done */
void dmatest_buflist_start_sg(struct buflist *buflist,
			      void (*callback)(struct buflist *bf));

/* Verify the buffer according to the pattern written by dmatest_buflist_alloc.
 * Returns 0 if ok otherwise -(index +1)
 */
int dmatest_buflist_payload_check(struct buflist *buflist);

/* prints the difference between TX and RX for buffer number buf_index */
void dmatest_buflist_payload_printdiff(struct buflist *buflist,
				       u32 buf_index);

void dmatest_buflist_reset_nbr_finished(struct buflist *buflist);

int dmatest_buflist_get_nbr_finished(struct buflist *buflist);
