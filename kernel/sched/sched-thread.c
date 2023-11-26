
#include <os/lock.h>
#if MULTITHREADING != 0
#include <os/pcb-list-g.h>
#include <os/glucose.h>
#include <os/malloc-g.h>
#include <os/smp.h>
#include <os/string.h>
#include <asm/regs.h>

/*
 * `alloc_tid`: allocate a free TID for the current process.
 * Return the TID or `INVALID_TID` if no free TID.
 */
static tid_t alloc_tid(void)
{
	pid_t i;

	for (i = 1; i <= TID_MAX; ++i)
		if (pcb_search_tcb(i) == NULL)
			return i;
	return INVALID_TID;
}

/*
 * `do_thread_create`: create a thread for current process,
 * which starts from `entry` with argument `arg`.
 * `sp` is used to specify the stack pointer, which is UVA.
 * `exit` is the exit function of the thread, which is also UVA.
 * Return the TID or `INVALID_TID` on error.
 */
pthread_t do_thread_create(uintptr_t entry, uintptr_t arg, uintptr_t sp, uintptr_t exit)
{
	tid_t tid;
	pcb_t *qtemp, *pnew, *ccpu = cur_cpu();
	uintptr_t kernel_stack;

	if ((tid = alloc_tid()) == INVALID_TID ||
		get_proc_num() >= UPROC_MAX + NCPU)
		return INVALID_TID;
	if (!((sp > ccpu->seg_start && sp <= ccpu->seg_end) ||
		(sp > User_sp - USTACK_NPG * NORMAL_PAGE_SIZE && sp <= User_sp)))
		return INVALID_TID;
	if (entry >= KVA_MIN || arg >= KVA_MIN)
		return INVALID_TID;
	kernel_stack = (uintptr_t)kmalloc_g(Kstack_size);
	if (kernel_stack == 0UL)
		return INVALID_TID;
	qtemp = lpcb_add_node_to_tail(ready_queue, &pnew, &ready_queue);
	if (qtemp == NULL)
	{
		kfree_g((void *)kernel_stack);
		return INVALID_TID;
	}
	ready_queue = qtemp;
	pnew->status = TASK_READY;
	pnew->tid = tid;
	pnew->pid = ccpu->pid;
	pnew->pgdir_kva = ccpu->pgdir_kva;
	pnew->kernel_stack = kernel_stack;
	pnew->cursor_x = pnew->cursor_y = 0;
	pnew->cylim_h = pnew->cylim_l = -1;
	pnew->cpu_mask = ~0U;
	pnew->wait_queue = NULL;
	pnew->seg_start = ccpu->seg_start;
	pnew->seg_end = ccpu->seg_end;
	strncpy(pnew->name, ccpu->name, TASK_NAMELEN);
	pnew->name[TASK_NAMELEN] = '\0';
	init_pcb_stack(kernel_stack, entry, pnew);
	pnew->trapframe->regs[OFFSET_REG_A0 / sizeof(reg_t)] = arg;
	/*
	 * `ra` in trapframe has been set to `do_exit` in `init_pcb_stack`,
	 * now we should set it to `exit`.
	 * `user_sp` has been set to `User_sp` too, so set it again.
	 */
	pnew->user_sp = ROUNDDOWN(sp, SP_ALIGN);
	pnew->trapframe->regs[OFFSET_REG_RA / sizeof(reg_t)] = exit;
	if (pcb_table_add(pnew) < 0)
		panic_g("do_thread_create: Failed to create thread %d", tid);
#if DEBUG_EN != 0
	writelog("thread %d of process %d is created", tid, ccpu->pid);
#endif
	return tid;
}

/*
 * `do_thread_wait`: wait for a thread of current process
 * to exit.
 * Return 0 on success, 1 on invalid `tid` or 2 if no such thread.
 */
int do_thread_wait(pthread_t tid)
{
	tcb_t *p;

	if (tid == 0 || tid == cur_cpu()->tid)
	/* Don't wait 0 thread or itself */
		return 1;
	if ((p = pcb_search_tcb(tid)) == NULL)
		return 2;
	do_block(&(p->wait_queue), NULL);
	return 0;
}

/*
 * `do_thread_exit`: Child threads call this function
 * to exit.
 */
void do_thread_exit()
{
	tcb_t *ccpu = cur_cpu();

	if (ccpu->tid == 0)
		/* If the parent thread exits, terminate the process. */
		do_exit();
	//while (ccpu->wait_queue != NULL)
	//	ccpu->wait_queue = do_unblock(ccpu->wait_queue);
	/*
	 * Set status as `TASK_ZOMBIE` so that this thread will
	 * be released in `do_scheduler()`.
	 */
	ccpu->status = TASK_ZOMBIE;
	do_scheduler();
	panic_g("do_thread_exit: Thread %d of proc %d is still running after exiting",
		cur_cpu()->tid, cur_cpu()->pid);
}

/*
 * `thread_kill`: delete a thread that is NOT running
 * and release its private resource.
 */
void thread_kill(tcb_t *p)
{
	pcb_t **phead, *pdel;

	if (p->status == TASK_RUNNING)
		panic_g("thread_kill: thread %d of process %d is running",
		p->tid, p->pid);

	phead = p->phead;
	/* Checking... */
	if (lpcb_search_node_tcb(*phead, p->pid, p->tid) != p)
		panic_g("thread_kill: phead of thread %d proc %d is error",
		p->tid, p->pid);
	/* wake up threads in `wait_queue` */
	while (p->wait_queue != NULL)
		p->wait_queue = do_unblock(p->wait_queue);
	/* Free stacks of it */
	kfree_g((void *)p->kernel_stack);
	*phead = lpcb_del_node(*phead, p, &pdel);
	if (pdel == NULL)
		panic_g("thread_kill: Failed to del pcb %d, %d in queue 0x%lx",
		p->tid, p->pid, (uint64_t)*phead);
	kfree_g((void *)pdel);
	if (pcb_table_del(pdel) < 0)
		panic_g("thread_kill: Failed to remove pcb %d, %d from pcb_table",
		p->tid, p->pid);
#if DEBUG_EN != 0
	writelog("Thread %d of process %d is terminated", p->tid, p->pid);
#endif
}

#endif
