#include <e1000.h>
#include <type.h>
#include <os/string.h>
#include <os/time.h>
#include <os/sched.h>
#include <assert.h>
#include <pgtable.h>

// E1000 Registers Base Pointer
volatile uint8_t *e1000;  // use virtual memory address

// E1000 Tx & Rx Descriptors: circular queues
static volatile struct e1000_tx_desc
	tx_desc_cq[TXDESCS] __attribute__((aligned(16)));
static volatile struct e1000_rx_desc
	rx_desc_cq[RXDESCS] __attribute__((aligned(16)));

static volatile unsigned int tcq_head, tcq_tail; /* Tx desc queue pointers */
static volatile unsigned int rcq_tail; /* Rx desc queue pointers */

// E1000 Tx & Rx frame buffer
static char tx_frm_buffer[TXDESCS][TX_FRM_SIZE];
static char rx_frm_buffer[RXDESCS][RX_FRM_SIZE];

// Fixed Ethernet MAC Address of E1000
static const uint8_t Enetaddr[] = {0x00, 0x0a, 0x35, 0x00, 0x1e, 0x53};

static inline unsigned tcq_len(void)
{
	if (tcq_tail >= tcq_head)
		return tcq_tail - tcq_head;
	else
		return tcq_tail + TXDESCS - tcq_head;
}

/**
 * e1000_reset - Reset Tx and Rx Units; mask and clear all interrupts.
 **/
static void e1000_reset(void)
{
	/* Turn off the ethernet interface */
	e1000_write_reg(e1000, E1000_RCTL, 0);
	e1000_write_reg(e1000, E1000_TCTL, 0);

	/* Clear the transmit ring */
	e1000_write_reg(e1000, E1000_TDH, 0);
	e1000_write_reg(e1000, E1000_TDT, 0);

	/* Clear the receive ring */
	e1000_write_reg(e1000, E1000_RDH, 0);
	e1000_write_reg(e1000, E1000_RDT, 0);

	/**
	 * Delay to allow any outstanding PCI transactions to complete before
	 * resetting the device
	 */
	latency(1);

	/* Clear interrupt mask to stop board from generating interrupts */
	e1000_write_reg(e1000, E1000_IMC, 0xffffffff);

	/* Clear any pending interrupt events. */
	while (0 != e1000_read_reg(e1000, E1000_ICR)) ;
}

/**
 * e1000_configure_tx - Configure 8254x Transmit Unit after Reset
 **/
static void e1000_configure_tx(void)
{
	uint32_t e1000_tctl;
	/* TODO: [p5-task1] Initialize tx descriptors */
	tcq_head = tcq_tail = 0U;
	/* TODO: [p5-task1] Set up the Tx descriptor base address and length */
	e1000_write_reg(e1000, E1000_TDBAL, (uint32_t)kva2pa((uintptr_t)tx_desc_cq));
	e1000_write_reg(e1000, E1000_TDBAH, (uint32_t)(kva2pa((uintptr_t)tx_desc_cq) >> 32));
	/* TODO: [p5-task1] Set up the HW Tx Head and Tail descriptor pointers */
	e1000_write_reg(e1000, E1000_TDH, tcq_head);
	e1000_write_reg(e1000, E1000_TDT, tcq_tail);
	e1000_write_reg(e1000, E1000_TDLEN, sizeof(tx_desc_cq));
	/* TODO: [p5-task1] Program the Transmit Control Register */
	e1000_tctl = e1000_read_reg(e1000, E1000_TCTL);
	e1000_tctl |= E1000_TCTL_EN | E1000_TCTL_PSP;
	e1000_tctl &= ~(E1000_TCTL_CT | E1000_TCTL_COLD);
	e1000_tctl |= 0x10UL << 4;  /* TCTL::CT */
	e1000_tctl |= 0x40UL << 12; /* TCTL::COLD */
	e1000_write_reg(e1000, E1000_TCTL, e1000_tctl);
}

/**
 * e1000_configure_rx - Configure 8254x Receive Unit after Reset
 **/
static void e1000_configure_rx(void)
{
	unsigned int i;
	/* TODO: [p5-task2] Set e1000 MAC Address to RAR[0] */
	e1000_write_reg_array(e1000, E1000_RA, 0U,		/* $RAL0 */
		*(uint32_t*)Enetaddr);
	e1000_write_reg_array(e1000, E1000_RA, 1U,		/* $RAH0 */
		(uint32_t)*(uint16_t*)(Enetaddr + 4) | E1000_RAH_AV);
	/* TODO: [p5-task2] Initialize rx descriptors */
	for (i = 0U; i < RXDESCS; ++i)
		rx_desc_cq[i].addr = kva2pa((uintptr_t)rx_frm_buffer[i]);
	rcq_tail = RXDESCS - 1U;
	/* TODO: [p5-task2] Set up the Rx descriptor base address and length */
	e1000_write_reg(e1000, E1000_RDBAL, (uint32_t)kva2pa((uintptr_t)rx_desc_cq));
	e1000_write_reg(e1000, E1000_RDBAH, (uint32_t)(kva2pa((uintptr_t)rx_desc_cq) >> 32));
	e1000_write_reg(e1000, E1000_RDLEN, sizeof(rx_desc_cq));
	/* TODO: [p5-task2] Set up the HW Rx Head and Tail descriptor pointers */
	e1000_write_reg(e1000, E1000_RDH, 0U);
	e1000_write_reg(e1000, E1000_RDT, rcq_tail);
	/* TODO: [p5-task2] Program the Receive Control Register */
	e1000_write_reg(e1000, E1000_RCTL, (E1000_RCTL_RDMTS_HALF |
		E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SZ_2048) & ~E1000_RCTL_BSEX);
	/* TODO: [p5-task3] Enable RXDMT0 Interrupt */

}

/**
 * e1000_init - Initialize e1000 device and descriptors
 **/
void e1000_init(void)
{
	/* Reset E1000 Tx & Rx Units; mask & clear all interrupts */
	e1000_reset();

	/* Configure E1000 Tx Unit */
	e1000_configure_tx();

	/* Configure E1000 Rx Unit */
	e1000_configure_rx();
}

/*
 * e1000_transmit: Transmit data frame through e1000 net device
 * `txpacket`: The buffer address of frame to be transmitted
 * `length`: Length of this frame
 * Return: Number of bytes that are transmitted successfully,
 * or -1 if the queue is full, 0 on error.
 */
int e1000_transmit(const void *txframe, int length)
{
	/* TODO: [p5-task1] Transmit one frame from txpacket */
	if (length > TX_FRM_SIZE || length <= 0)
		return 0;
	tcq_head = e1000_read_reg(e1000, E1000_TDH);
	if (tcq_len() >= TXDESCS - 1U)
		return -1;
	memcpy((uint8_t *)tx_frm_buffer[tcq_tail], txframe, (unsigned int)length);
	tx_desc_cq[tcq_tail].addr = kva2pa((uintptr_t)tx_frm_buffer[tcq_tail]);
	tx_desc_cq[tcq_tail].length = (uint16_t)length;
	tx_desc_cq[tcq_tail].cmd = (E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP) &
		~E1000_TXD_CMD_DEXT;
	if (++tcq_tail >= TXDESCS)
		tcq_tail = 0U;
	local_flush_dcache();
	e1000_write_reg(e1000, E1000_TDT, tcq_tail);
	return length;
}

/*
 * e1000_poll: Receive data frame through e1000 net device
 * `rxbuffer`: The address of buffer to store received frame
 * Return: Length of received frame or -1 if no frame is done
 */
int e1000_poll(void *rxbuffer)
{
	int len;
	unsigned int tail_next = (rcq_tail + 1U) % RXDESCS;

	local_flush_dcache();
	/* TODO: [p5-task2] Receive one frame and put it into rxbuffer */
	if ((rx_desc_cq[tail_next].status & E1000_RXD_STAT_DD) == 0U)
		return -1;
	len = (int)(uint32_t)rx_desc_cq[tail_next].length;
	memcpy((uint8_t*)rxbuffer, (uint8_t *)rx_frm_buffer[tail_next], len);

	rx_desc_cq[tail_next].addr = kva2pa((uintptr_t)rx_frm_buffer[tail_next]);
	*(((uint64_t *)(rx_desc_cq + tail_next)) + 1) = 0UL;	/* Clear other bits */

	rcq_tail = tail_next;
	e1000_write_reg(e1000, E1000_RDT, rcq_tail);
	local_flush_dcache();
	return len;
}

/* Transmission queue empty interrupt */
void handle_e1000_int_txqe(void)
{
	while (send_block_queue != NULL)
		send_block_queue = do_unblock(send_block_queue);
}

/* Receive descriptor minimum threshold interrupt */
void handle_e1000_int_rxdmt0(void)
{
	while (recv_block_queue != NULL)
		recv_block_queue = do_unblock(recv_block_queue);
}