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
#define GNTP_TIMEOUT 10U
#define GNTP_HEADER_START 54

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

static void reply(unsigned int type, uint32_t seq);

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
	unsigned int lfr;
	/* Has the receiving started? */
	char recv_started;
	/* How long since the last time `lfr` is moved? */
	unsigned int time;

	if ((uintptr_t)buf >= KVA_MIN || len <= 0)
		return 0;
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
				/* Some frames have just been received */
				reply(GNTP_ACK, lfr);
			if (time > GNTP_TIMEOUT)
			{
				reply(GNTP_RSD, lfr);
				time = 0U;
			}
			cur_cpu()->req_len = 1U;
			do_block(&recv_block_queue, NULL);
			if (recv_started != 0)
				++time;
		}
		else
		{	/* One frame is received */
			memcpy((void *)&header_temp, fb + GNTP_HEADER_START, sizeof(gntp_header_t));
			if (header_temp.magic != GNTP_MAGIC ||
				(header_temp.type & GNTP_DAT) == 0U)
				continue;
			recv_started = 1;
			if (header_temp.seq == lfr)
			{
				memcpy(buf + lfr, fb + GNTP_HEADER_START + sizeof(gntp_header_t),
					lfr + header_temp.length <= (unsigned int)len
					/* To avoid buffer overflow */
					? header_temp.length : (unsigned int)len - lfr);
				lfr += header_temp.length;
				if (lfr >= (unsigned int)len)
				{
					reply(GNTP_ACK, lfr);
					kfree_g(fb);
					fb = NULL;
					return lfr;
				}
				time = 0U;
			}
		}
	}
}

/*
 * reply: Reply a frame to the sender with `type` (ACK or RSD) and `seq`.
 * Note that this function would change `header_temp` and
 * the buffer pointed by `fb` as it would use the first 54 bytes.
 */
static void reply(unsigned int type, uint32_t seq)
{
	header_temp.type = type;
	header_temp.length = 0U;
	header_temp.seq = seq;
	memcpy(fb + GNTP_HEADER_START, (void *)&header_temp, sizeof(header_temp));
	while (-1 == e1000_transmit(fb,
		(unsigned int)GNTP_HEADER_START + sizeof(header_temp)))
	{
		e1000_write_reg(e1000, E1000_IMS, E1000_IMS_TXQE);
		do_block(&send_block_queue, NULL);
	}
}