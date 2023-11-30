#include <os/lock.h>
#include <os/glucose.h>
#include <os/smp.h>
#include <pgtable.h>

static mailbox_t mailboxes[MBOX_NUM];

/*
 * Calc the length of buffer queue of mailboxes[midx].
 * NOTE: in these three inline function, midx will not
 * be checked. So ensure that midx passed to them is
 * valid (midx >= 0 && midx < MBOX_NUM).
 */
static inline unsigned int len_of_queue(int midx)
{
	if (mailboxes[midx].buf_tail >= mailboxes[midx].buf_head)
		return mailboxes[midx].buf_tail - mailboxes[midx].buf_head;
	else
		return mailboxes[midx].buf_tail + MAX_MBOX_LENGTH + 1U
			- mailboxes[midx].buf_head;
}

/*
 * Put a byte D into the buffer queue of mailboxes[midx].
 * Returns: (int)D on success, -1 if the buffer queue is full.
 */
static inline int32_t queue_enter(int midx, const uint8_t D)
{
	if (len_of_queue(midx) < MAX_MBOX_LENGTH)
	{
		mailboxes[midx].buf[mailboxes[midx].buf_tail] = D;
		if (++mailboxes[midx].buf_tail >= MAX_MBOX_LENGTH + 1U)
			mailboxes[midx].buf_tail = 0U;
		/*
		 * Actually it is buf_tail = (buf_tail + 1) % (MAX_MBOX_LENGTH + 1),
		 * but div operation is slow. So, use this instead of div. Although
		 * this may be not fast either.
		 */
		return (int32_t)D;
	}
	else	/* Queue is full */
		return -1;
}

/*
 * Remove a byte d from the buffer queue of mailboxes[midx].
 * Returns: d on success, -1 is the buffer queue is empty.
 */
static inline int32_t queue_depart(int midx)
{
	uint8_t d;	/* d must be unsigned char! */

	if (len_of_queue(midx) > 0U)
	{
		d = mailboxes[midx].buf[mailboxes[midx].buf_head];
		if (++mailboxes[midx].buf_head >= MAX_MBOX_LENGTH + 1U)
			mailboxes[midx].buf_head = 0U;
		return (int32_t)d;
	}
	else	/* Queue is empty */
		return -1;
}

void init_mbox(void)
{
	int i;

	for (i = 0; i < MBOX_NUM; ++i)
	{
		spin_lock_init(&(mailboxes[i].slock));
		mailboxes[i].opid = INVALID_PID;
	}
}

int do_mbox_open(const char *name)
{
	const unsigned int Name_max = 20U;
	unsigned int i, s = 0U, midx;

	for (i = 0U; i < Name_max && name[i] != '\0'; ++i)
		s += (uint8_t)(name[i]);
	midx = s % (unsigned int)MBOX_NUM;

	spin_lock_acquire(&mailboxes[midx].slock);
	if (mailboxes[midx].opid == INVALID_PID)
	{	/* This mailbox is NOT in use */
		mailboxes[midx].opid = cur_cpu()->pid;
		mailboxes[midx].block_queue_r = NULL;
		mailboxes[midx].block_queue_s = NULL;
		mailboxes[midx].buf_head = 0U;
		mailboxes[midx].buf_tail = 0U;
	}
	spin_lock_release(&(mailboxes[midx].slock));
	return (int)midx;
}

/*
 * Close a mailbox and wakeup all blocked processes.
 * Returns: midx on success and INT32_MIN on error.
 */
int do_mbox_close(int midx)
{
	mailbox_t *ptmbox;
	int rt;

	if (midx < 0 || midx >= MBOX_NUM)
		return INT32_MIN;
	ptmbox = mailboxes + midx;
	spin_lock_acquire(&(ptmbox->slock));
	if (ptmbox->opid == INVALID_PID)
		rt = INT32_MIN;
	else
	{
		ptmbox->opid = INVALID_PID;
		while (ptmbox->block_queue_r != NULL)
			ptmbox->block_queue_r = do_unblock(ptmbox->block_queue_r);
		while (ptmbox->block_queue_s != NULL)
			ptmbox->block_queue_s = do_unblock(ptmbox->block_queue_s);
		rt = midx;
	}
	spin_lock_release(&(ptmbox->slock));
	return rt;
}

/*
 * Send msg_length bytes from msg to mailboxes[midx].
 * Returns: time of being blocked on success and INT32_MIN on error.
 */
int do_mbox_send(int midx, const uint8_t* msg, unsigned int msg_length)
{
	mailbox_t *ptmbox;
	/* Counter of blocking */
	int block_ymr = 0;
	unsigned int i;

	if (midx < 0 || midx >= MBOX_NUM ||
		msg_length <= 0U || msg_length > MAX_MBOX_LENGTH)
		return INT32_MIN;
	ptmbox = mailboxes + midx;
	spin_lock_acquire(&(ptmbox->slock));
	if (ptmbox->opid == INVALID_PID)
		goto errs;
	while (MAX_MBOX_LENGTH - len_of_queue(midx) < msg_length)
	{
		++block_ymr;
		cur_cpu()->req_len = msg_length;
		do_block(&(ptmbox->block_queue_s), &(ptmbox->slock));
		/*
		 * If this process is woken up because the mailbox is closed,
		 * return INT32_MIN as a flag of error.
		 */
		if (ptmbox->opid == INVALID_PID)
			goto errs;
	}
	for (i = 0U; i < msg_length; ++i)
		if (queue_enter(midx, msg[i]) != msg[i])
			panic_g("Buffer queue of mailbox %d is full", midx);
	while (ptmbox->block_queue_r != NULL &&
		len_of_queue(midx) >= ptmbox->block_queue_r->req_len)
		ptmbox->block_queue_r = do_unblock(ptmbox->block_queue_r);
	/*
	 * There is another strategy to wake up processes:
	 * once some msg is put into the buffer, wake up all processes
	 * in block_queue_r.
	 */
	//while (ptmbox->block_queue_r != NULL)
	//	ptmbox->block_queue_r = do_unblock(ptmbox->block_queue_r);
	spin_lock_release(&(ptmbox->slock));
	return block_ymr;
errs:
	spin_lock_release(&(ptmbox->slock));
	return INT32_MIN;
}

/*
 * Receive msg_length bytes from mailboxes[midx] to msg.
 * Returns: time of being blocked on success and INT32_MIN on error.
 */
int do_mbox_recv(int midx, uint8_t* msg, unsigned int msg_length)
{
	mailbox_t *ptmbox;
	/* Counter of blocking */
	int block_ymr = 0;
	unsigned int i;
	int32_t d;

	if (midx < 0 || midx >= MBOX_NUM ||
		msg_length <= 0U || msg_length > MAX_MBOX_LENGTH)
		return INT32_MIN;
	if ((uintptr_t)msg >= KVA_MIN)
		/* It's a KVA! */
		return INT32_MIN;
	ptmbox = mailboxes + midx;
	spin_lock_acquire(&(ptmbox->slock));
	if (ptmbox->opid == INVALID_PID)
		goto errr;
	while (len_of_queue(midx) < msg_length)
	{
		++block_ymr;
		cur_cpu()->req_len = msg_length;
		do_block(&(ptmbox->block_queue_r), &(ptmbox->slock));
		/*
		 * If this process is woken up because the mailbox is closed,
		 * return INT32_MIN as a flag of error.
		 */
		if (ptmbox->opid == INVALID_PID)
			goto errr;
	}
	for (i = 0U; i < msg_length; ++i)
		if ((d = queue_depart(midx)) == -1)
			panic_g("Buffer queue of mailbox %d is empty", midx);
		else
			msg[i] = (uint8_t)d;
	while (ptmbox->block_queue_s != NULL)
		ptmbox->block_queue_s = do_unblock(ptmbox->block_queue_s);
	/*
	 * There is another strategy to wake up processes:
	 * when some msg is removed from the buffer, wake up processes
	 * in block_queue_r UNTIL available space is not enough for the next
	 * process (the head of block_queue_s).
	 */
	//while (ptmbox->block_queue_s != NULL &&
	//	MAX_MBOX_LENGTH - len_of_queue(midx) >= ptmbox->block_queue_s->req_len)
	//	ptmbox->block_queue_s = do_unblock(ptmbox->block_queue_s);
	spin_lock_release(&(ptmbox->slock));
	return block_ymr;
errr:
	spin_lock_release(&(ptmbox->slock));
	return INT32_MIN;
}

void mbox_release_killed(pid_t kpid)
{
	int i;

	for (i = 0; i < MBOX_NUM; ++i)
	{
		spin_lock_acquire(&(mailboxes[i].slock));
		if (mailboxes[i].opid == kpid)
		{
			mailboxes[i].opid = INVALID_PID;
			while (mailboxes[i].block_queue_r != NULL)
				mailboxes[i].block_queue_r = do_unblock(mailboxes[i].block_queue_r);
			while (mailboxes[i].block_queue_s != NULL)
				mailboxes[i].block_queue_s = do_unblock(mailboxes[i].block_queue_s);
		}
		spin_lock_release(&(mailboxes[i].slock));
	}
}