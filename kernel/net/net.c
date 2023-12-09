#include <e1000.h>
#include <type.h>
#include <os/lock.h>
#include <os/string.h>
#include <os/list.h>
#include <os/smp.h>
#include <os/glucose.h>

pcb_t *send_block_queue, *recv_block_queue;

/*
 * do_net_send: Send a packet of `length` through NIC.
 * Return the number of bytes transmitted successfully
 * or 0 on error.
 */
#if DEBUG_EN == 0
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
int do_net_send(const void *txpacket, int length)
{
	// TODO: [p5-task1] Transmit one network packet via e1000 device
	// TODO: [p5-task3] Call do_block when e1000 transmit queue is full
	// TODO: [p5-task3] Enable TXQE interrupt if transmit queue is full
	int rt;
	unsigned int block_ymr = 0U;

	if ((uintptr_t)txpacket >= KVA_MIN || length <= 0)
		return 0;
	while ((rt = e1000_transmit(txpacket, length)) == -1)
	{
		/* Sending queue is full */
		e1000_write_reg(e1000, E1000_IMS, E1000_IMS_TXQE);
		do_block(&send_block_queue, NULL);
		++block_ymr;
	}
#if DEBUG_EN != 0
	if (block_ymr > 0U)
		writelog("Proc %d has been blocked %u times in send_block_queue",
			cur_cpu()->pid, block_ymr);
#endif
	return rt;
}

/*
 * do_net_recv: Receive `pkt_num` packets from NIC, write them
 * to `rx_buffer` and their lengths to `pkt_lens`.
 * Return the number of bytes received successfully
 * or 0 on error.
 */
int do_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens)
{
	// TODO: [p5-task2] Receive one network packet via e1000 device
	// TODO: [p5-task3] Call do_block when there is no packet on the way
	int i, r_ymr = 0, rt;

	if ((uintptr_t)rxbuffer >= KVA_MIN || pkt_num <= 0 ||
		(uintptr_t)pkt_lens >= KVA_MIN)
		return 0;
	for (i = 0; i < pkt_num; ++i)
	{
		while ((rt = e1000_poll((uint8_t*)rxbuffer + r_ymr)) == -1)
		{
			/* Store the number of packets to be received */
			cur_cpu()->req_len = pkt_num - i;
			e1000_write_reg(e1000, E1000_IMS, E1000_IMS_RXDMT0);
			do_block(&recv_block_queue, NULL);
		}
		r_ymr += rt;
		pkt_lens[i] = rt;
	}
	return r_ymr;  // Bytes it has received
}

/*
 * check_sleeping_on_nic: Wake up processes that are waiting for
 * packets less than or equal to half of the rx descriptors when
 * the timer interrupt comes. That is because RXDMT interrupt
 * comes only when more than half of the queue is filled with
 * received data.
 */
void check_sleeping_on_nic(void)
{
	while (recv_block_queue != NULL &&
		recv_block_queue->req_len <= (RXDESCS >> 1))
		recv_block_queue = do_unblock(recv_block_queue);
}