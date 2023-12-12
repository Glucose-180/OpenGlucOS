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
static void copy_header_to_buffer(uint8_t *fb);
static void copy_header_from_buffer(const uint8_t *fb);

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
			copy_header_from_buffer(fb + GNTP_HEADER_START);
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