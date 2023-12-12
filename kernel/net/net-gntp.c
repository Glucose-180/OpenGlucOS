/*
 * Use "GlucOS Network Transmission Protocol" to
 * implement reliable data transmission.
 */
#include <e1000.h>
#include <type.h>
#include <os/lock.h>
#include <os/string.h>
#include <os/list.h>
#include <os/smp.h>
#include <os/glucose.h>
#include <os/malloc-g.h>

/* Constants */
#define GNTP_MAGIC 0x45U
#define GNTP_TIMEOUT 100U
#define GNTP_HEADER_START 54
#define GNTP_WINSIZE 10000U
#define LOCK_IDX 15	/* Mutex lock for `do_net_recv_stream()` */

/* Mask of the `type` byte in header */
#define GNTP_ACK (1U << 2)
#define GNTP_RSD (1U << 1)
#define GNTP_DAT (1U << 0)

typedef struct {
	uint8_t magic;
	uint8_t type;
	uint16_t length;
	uint32_t seq;
} gntp_header_t;

/* A temp header to avoid unaligned access */
static gntp_header_t header_temp;
/* Frame buffer */
static uint8_t *fb = NULL;

typedef struct dlist_node_t {
	struct dlist_node_t *next;
	uint32_t start, end;	/* Data in [.start, .end) have been received */
} dlist_node_t;

static dlist_node_t dlh;
static dlist_node_t *dlist_head = &dlh;

static void reply(unsigned int type, uint32_t seq);
static void copy_header_to_buffer(uint8_t *fb);
static void copy_header_from_buffer(const uint8_t *fb);

static void dlist_clear(void);
static int dlist_insert(uint32_t start, uint32_t end);

/*
 * do_net_recv_stream: Receive `len` bytes through NIC using
 * GNTP to implement reliable data transmission.
 * Return the number of bytes received or 0 on error.
 */
unsigned int do_net_recv_stream(uint8_t *buf, int len)
{
	/* Length of a frame */
	int flen;
	/* Bytes lower than `lfr` are received successfully. */
#define lfr (dlh.end)
	//unsigned int lfr;
	/* The max seq of all packets received */
	unsigned int seq_max = 0U;
	/* Has the receiving started? */
	char recv_started;
	/* How long since the last time `lfr` is moved? */
	unsigned int time;
	
	int rt;
	/* Save the data about TCP before GNTP header */
	uint8_t tcp_header[GNTP_HEADER_START];

	if ((uintptr_t)buf >= KVA_MIN || len <= 0)
		return 0;

	/* This function can only be used by one process */
	do_mutex_lock_acquire(LOCK_IDX);
	/*
	 * Initialization is necessary as the last process
	 * using it may be killed.
	 */
	dlist_clear();
	if (fb == NULL)
		fb = kmalloc_g(RX_FRM_SIZE + 256U);
	if (fb == NULL)
		return 0;

	lfr = time = 0U;
	recv_started = 0;
	while (1)
	{
		flen = e1000_poll(fb);
		if (flen == -1)
		{	/* This transmission has finished. */
			if (lfr > 0U && time == 0U)
				/* Some packets have just been received */
				reply(GNTP_ACK, lfr);
			if (time > GNTP_TIMEOUT)
			{
				if (seq_max > lfr || time > GNTP_TIMEOUT * 2U)
				{
					/*
					 * If first timeout, only when a packet with greater `seq` has received
					 * can we reply RSD. Otherwise many invalid RSD might be send
					 * (an RSD with a `seq` that has never be sent).
					 * But if the timeout is big, reply RSD to deal with loss.
					 */
					reply(GNTP_RSD, lfr);
					time = 0U;
				}
			}
			cur_cpu()->req_len = 1U;
			do_block(&recv_block_queue, NULL);
			if (recv_started != 0)
				++time;
		}
		else
		{	/* One packet is received */
			copy_header_from_buffer(fb + GNTP_HEADER_START);
			if (header_temp.magic != GNTP_MAGIC ||
				(header_temp.type & GNTP_DAT) == 0U)
			{	/*
				 * Restore the data about TCP as
				 * `fb` would be used to reply.
				 */
				if (recv_started != 0)
					/* If `tcp_header[]` has been saved... */
					memcpy(fb, tcp_header, GNTP_HEADER_START);
				continue;
			}
			if (recv_started == 0)
			{
				memcpy(tcp_header, fb, GNTP_HEADER_START);
				recv_started = 1;
			}

			rt = dlist_insert(header_temp.seq, header_temp.seq + header_temp.length);
			if (rt == 0)
			{
				unsigned int i, j;
				for (i = header_temp.seq, j = GNTP_HEADER_START + sizeof(gntp_header_t);
					i < (unsigned int)len && i < header_temp.seq + header_temp.length;
					++i, ++j)
					buf[i] = fb[j];
				if (lfr >= (unsigned int)len)
				{
					uint32_t temp = lfr;
					reply(GNTP_ACK, lfr);
					dlist_clear();
					kfree_g(fb);
					fb = NULL;
					do_mutex_lock_release(LOCK_IDX);
					return temp;
				}
				time = 0U;
			}
			else if (rt == -1)
			{
				dlist_clear();
				kfree_g(fb);
				fb = NULL;
				do_mutex_lock_release(LOCK_IDX);
				return 0U;	/* Failed */
			}

			if (header_temp.seq > seq_max)
				seq_max = header_temp.seq;
		}
	}
#undef lfr
}

/*
 * reply: Reply a frame to the sender with `type` (ACK or RSD) and `seq`.
 * Note that this function would change `header_temp` and
 * the buffer pointed by `fb` as it would use the first 54 bytes.
 */
static void reply(unsigned int type, uint32_t seq)
{
	header_temp.magic = GNTP_MAGIC;
	header_temp.type = type;
	header_temp.length = 0U;
	header_temp.seq = seq;
	copy_header_to_buffer(fb + GNTP_HEADER_START);
	while (-1 == e1000_transmit(fb,
		(unsigned int)GNTP_HEADER_START + sizeof(header_temp)))
	{
		e1000_write_reg(e1000, E1000_IMS, E1000_IMS_TXQE);
		do_block(&send_block_queue, NULL);
	}
}

/*
 * copy_header_to_buffer & copy_header_from_header:
 * The frame header received is Big Endian, but GlucOS is Little Endian.
 * So use these two functions to copy the header to or from
 * frame buffer from or to `header_temp`.
 */
static void copy_header_to_buffer(uint8_t *fb)
{
	fb[0] = header_temp.magic;
	fb[1] = header_temp.type;
	*(uint16_t *)(fb + 2) = (uint16_t)
		(((uint32_t)header_temp.length >> 8) | (header_temp.length << 8));
	*(uint32_t *)(fb + 4) = (uint32_t)(
		((header_temp.seq >> 24) & 0xffU) |
		((header_temp.seq >> 8) & (0xffU << 8)) |
		((header_temp.seq << 8) & (0xffU << 16)) |
		((header_temp.seq << 24) & (0xffU << 24))
	);
}
static void copy_header_from_buffer(const uint8_t *fb)
{
	header_temp.magic = fb[0];
	header_temp.type = fb[1];
	header_temp.length = ((uint32_t)fb[2] << 8) | (uint32_t)fb[3];
	header_temp.seq = (
		((uint32_t)fb[4] << 24) |
		((uint32_t)fb[5] << 16) |
		((uint32_t)fb[6] << 8) |
		(uint32_t)fb[7]
	);
}

/*
 * dlist_clear: free all nodes except the head,
 * and initialize the list.
 */
static void dlist_clear()
{
	dlist_node_t *p, *q;

	dlist_head = &dlh;
	for (p = dlist_head->next; p != NULL; p = q)
	{
		q = p->next;
		kfree_g(p);
	}
	dlist_head->start = dlist_head->end = 0U;
	dlist_head->next = NULL;
}

/*
 * dlist_insert: insert a node with `start` and `end`.
 * Return 0 if the data packet is valid, 1 if invalid
 * or -1 on error.
 */
static int dlist_insert(uint32_t start, uint32_t end)
{
	dlist_node_t *p, *q;

	if (start >= end || end >= dlh.end + GNTP_WINSIZE)
		return 1;
	for (p = dlist_head; p->next != NULL && p->next->start <= start;
		p = p->next)
		;
	/* Now we have p->start <= start &&
		(p->next == NULL || p->next->start > start) */
	if (!(p->start <= start &&
		(p->next == NULL || p->next->start > start)))
		panic_g("Data list operation failed!");
	if (start < p->end || (p->next != NULL && end > p->next->start))
		return 1;	/* Overlap is not allowed */
	if (start == p->end)
	{
		p->end = end;
		if (p->next != NULL && end == p->next->start)
		{
			p->end = p->next->end;
			q = p->next->next;
			kfree_g(p->next);
			p->next = q;
		}
	}
	else
	{
		if (p->next != NULL && end == p->next->start)
			p->next->start = start;
		else
		{
			if ((q = kmalloc_g(sizeof(dlist_node_t))) == NULL)
				return -1;	/* Error! */
			q->next = p->next;
			q->start = start;
			q->end = end;
			p->next = q;
		}
	}
	return 0;
}
