#include <os/list.h>
#include <os/pcb-list-g.h>
#include <os/tcb-list-g.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <os/malloc-g.h>
#include <os/kernel.h>
#include <os/string.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <os/loader.h>
#include <riscv.h>
#include <asm/regs.h>
#include <csr.h>
#include <os/irq.h>
#include <os/smp.h>

//pcb_t pcb[UPROC_MAX];
/*
 * Has been modified by Glucose180
 */
const ptr_t pid0_stack = INIT_KERNEL_STACK,// + PAGE_SIZE;
	pid1_stack = INIT_KERNEL_STACK_S;

/*
 * The default size for a user stack and kernel stack.
 * 16 KiB, 16 KiB.
 */
static const uint32_t Ustack_size = 16 * 1024,
	Kstask_size = 16 * 1024;

/*
 * It is used to represent main.c:main()
 */
pcb_t pid0_pcb = {
	.pid = 0,
	.kernel_sp = (ptr_t)pid0_stack,
	.user_sp = (ptr_t)pid0_stack,
	.trapframe = (void *)pid0_stack,
	/* Add more info */
	.status = TASK_RUNNING,
	.wait_queue = NULL,
	.name = "_main",
	.cursor_x = 0,
	.cursor_y = 0,
	.cylim_h = -1,
	.cylim_l = -1,
	.pcthread = NULL,
	/* to make pid0_pcb.phead point to ready_queue */
	.phead = &ready_queue
};

/* It is used for secondary CPU */
pcb_t pid1_pcb = {
	.pid = 1,
	.kernel_sp = (ptr_t)pid1_stack,
	.user_sp = (ptr_t)pid1_stack,
	.trapframe = (void *)pid1_stack,
	/* Add more info */
	.status = TASK_RUNNING,
	.wait_queue = NULL,
	.name = "_main_s",
	.cursor_x = 0,
	.cursor_y = 0,
	.cylim_h = -1,
	.cylim_l = -1,
	.pcthread = NULL,
	/* to make pid0_pcb.phead point to ready_queue */
	.phead = &ready_queue
};

//LIST_HEAD(ready_queue);
/*
 * Circular queue (linked list) of all READY and RUNNING processes
 */
pcb_t *ready_queue;
//LIST_HEAD(sleep_queue);
/*
 * Circular queue (linked list) of all processes that have called sys_sleep()
 */
pcb_t *sleep_queue;

/* current running task PCB */
pcb_t * volatile current_running[2];

/* global process id */
pid_t process_id = 1;

/*
 * alloc_pid: find a free PID.
 * INVALID_PID will be returned if no free PID is found.
 * NOTE: It can be improved as the algorithm is NOT efficient.
 */
pid_t alloc_pid(void)
{
	pid_t i;
	static const pid_t Pid_min = 1, Pid_max = 2 * UPROC_MAX;

	for (i = Pid_min; i <= Pid_max; ++i)
		if (pcb_search(i) == NULL)
			return i;
	return INVALID_PID;
}

/*
 * Use the name of a task to load it and add it in ready_queue.
 * INVALID_PID will be returned on error.
 */
pid_t create_proc(const char *taskname)
{
	ptr_t entry, user_stack, kernel_stack;
	pcb_t *pnew, *temp;//, *p0;

	//p0 = NULL;
	if (taskname == NULL || get_proc_num() >= UPROC_MAX + 1)
		return INVALID_PID;
	entry = load_task_img(taskname);
	if (entry == 0U)
		return INVALID_PID;
	user_stack = (ptr_t)umalloc_g(Ustack_size);
	kernel_stack = (ptr_t)kmalloc_g(Kstask_size);
	if (user_stack == 0 || kernel_stack == 0)
		return INVALID_PID;
	/*if (ready_queue->pid == pid0_pcb.pid)
	{
		ready_queue = lpcb_del_node(ready_queue, ready_queue, &p0);
		if (ready_queue != NULL || p0 == NULL || p0->pid != pid0_pcb.pid)
			panic_g("create_proc: Failed to remove the proc whose ID is 0");
	}*/
	if ((temp = lpcb_add_node_to_tail(ready_queue, &pnew, &ready_queue)) == NULL)
	{
		ufree_g((void *)user_stack);
		return INVALID_PID;
	}
	ready_queue = temp;
	/*if (p0 != NULL)
	{	// Has removed the 0 proc
		p0->next = ready_queue;
		if (p0 != cur_cpu())
			panic_g("create_proc: Error happened while removing the proc 0");
	}*/
	/*
	 * Set PID of new born process to INVALID_PID
	 * so that the undefined PID will not affect alloc_pid().
	 */
	pnew->pid = INVALID_PID;
	strncpy(pnew->name, taskname, TASK_NAMELEN);
	pnew->name[TASK_NAMELEN] = '\0';
	init_pcb_stack(kernel_stack, user_stack, entry, pnew);
	if (pcb_table_add(pnew) < 0)
		panic_g("create_proc: Failed to add to pcb_table");
#if DEBUG_EN != 0
	writelog("Process \"%s\" whose PID is %d is created.", pnew->name, pnew->pid);
#endif
	return pnew->pid;
}

void do_scheduler(void)
{
	pcb_t *p, *q;
	uint64_t isscpu = is_scpu();
	// TODO: [p2-task3] Check sleep queue to wake up PCBs

	/************************************************************/
	/* Do not touch this comment. Reserved for future projects. */
	/************************************************************/

	check_sleeping();
	if (cur_cpu() != NULL)
	{
		pcb_t *nextp;
		for (p = cur_cpu()->next; p != cur_cpu(); p = nextp)
		{	/* Search the linked list and find a READY process */
			nextp = p->next;	/* Store p->next in case of it is killed */
			if (p->status == TASK_READY && p->pid + cur_cpu()->pid != 1)
			{	/* The second condition is to ignore main() of the other CPU */
				if (cur_cpu()->status != TASK_EXITED)
					cur_cpu()->status = TASK_READY;
				q = cur_cpu();
				current_running[isscpu] = p;
				cur_cpu()->status = TASK_RUNNING;
				/*
				 * Define MULTITHREADING as 1 to support multithreading. When switch_to()
				 * is called, we should first determine which thread will be switched from
				 * and to. If multithreading is disabled, just keep switch_to() as normal.
				 */
#if MULTITHREADING != 0
				switch_to(q->cur_thread == NULL ? &(q->context) : &(q->cur_thread->context),
					cur_cpu()->cur_thread == NULL ? &(cur_cpu()->context)
					: &(cur_cpu()->cur_thread->context)
				);
#else
				switch_to(&(q->context), &(cur_cpu()->context));
#endif
				return;
			}
			else if (p->status == TASK_EXITED)
			{
				pid_t kpid = p->pid;
				if (kpid == cur_cpu()->pid || kpid != do_kill(kpid))
					panic_g("do_scheduler: Failed to kill proc %d", p->pid);
			}
		}
		if (cur_cpu()->pid != 0 && cur_cpu()->pid != 1)
			/*
			 * If cur_cpu()->pid is not 0, then
			 * a ready process must be found, because GlucOS keep
			 * main() of kernel as a proc with PID 0.
			 */
			panic_g("do_scheduler: Cannot find a ready process");
		return;
	}
	else
		panic_g("do_scheduler: cur_cpu() is NULL");
}

void do_sleep(uint32_t sleep_time)
{
	// TODO: [p2-task3] sleep(seconds)
	// NOTE: you can assume: 1 second = 1 `timebase` ticks
	// 1. block the cur_cpu()
	// 2. set the wake up time for the blocked task
	// 3. reschedule because the cur_cpu() is blocked.
	pcb_t *temp, *psleep;
	uint64_t isscpu = is_scpu();

	for (temp = cur_cpu()->next; temp != cur_cpu(); temp = temp->next)
		if (temp->status == TASK_READY && temp->pid + cur_cpu()->pid != 1)
			break;
	if (temp->status != TASK_READY)
		/*
		* A ready process must be found, because GlucOS keep
		* main() of kernel as a proc with PID 0.
		*/
		panic_g("do_sleep: Cannot find a READY process");

	ready_queue = lpcb_del_node(ready_queue, cur_cpu(), &psleep);
	if (psleep != cur_cpu())
		panic_g("do_sleep: Failed to remove cur_cpu()");
	if (ready_queue == NULL)
		/*
		 * Note that now we don't permit cur_cpu() == NULL,
		 * so this would cause PANIC!
		 */
		current_running[isscpu] = NULL;
	else
		current_running[isscpu] = temp;
	if ((sleep_queue = lpcb_insert_node(sleep_queue, psleep, NULL, &sleep_queue)) == NULL)
		panic_g("do_sleep: Failed to insert proc %d to sleep_queue", psleep->pid);
	psleep->status = TASK_SLEEPING;
	if ((psleep->wakeup_time = get_timer() + sleep_time) > time_max_sec)
		/* Avoid creating a sleeping task that would never be woken up */
		psleep->wakeup_time = time_max_sec;
	if (cur_cpu() == NULL)
		do_scheduler();
	else
#if MULTITHREADING != 0
		switch_to(psleep->cur_thread == NULL ? &(psleep->context)
			: &(psleep->cur_thread->context),
			cur_cpu()->cur_thread == NULL ? &(cur_cpu()->context)
			: &(cur_cpu()->cur_thread->context)
		);
#else
		switch_to(&(psleep->context), &(cur_cpu()->context));
#endif
}

/*
 * check_sleeping: Scan sleep_queue, find tasks
 * that should wake up and wake up them.
 */
void check_sleeping(void)
{
	// TODO: [p2-task3] Pick out tasks that should wake up from the sleep queue
	pcb_t *pw, *temp_next, *temp_sleep;
	uint64_t time_now_sec;

	if (sleep_queue == NULL)
		return;
	time_now_sec = get_timer();

	pw = sleep_queue;
	do {
		if (pw->status != TASK_SLEEPING)
			goto err;
		/*
		 * The wake_up function would change the sleep_queue,
		 * so two temp_x pointers are needed to keep the original
		 * condition about whether the loop should end.
		 */
		temp_next = pw->next;
		temp_sleep = sleep_queue;
		if (time_now_sec >= pw->wakeup_time)
			wake_up(pw);
		pw = temp_next;

		/*
		 * When sleep_queue is NULL, the loop must end.
		 * Otherwise, there must be something wrong while
		 * operating the linked list.
		 */
		if (sleep_queue == NULL && pw != temp_sleep)
			panic_g("check_sleeping: Linked list operation failed");
	} while (pw != temp_sleep);
	return;
err:
	panic_g("check_sleeping: proc %d in sleep_queue is not SLEEPING", pw->pid);
}

/*
 * wake_up: remove *T from sleep_queue and insert it
 * to ready_queue after *cur_cpu().
 */
void wake_up(pcb_t * const T)
{
	pcb_t *pw;

	if (T->status != TASK_SLEEPING)
		goto err;
	sleep_queue = lpcb_del_node(sleep_queue, T, &pw);
	if (pw != T)
		goto err;
	pw->status = TASK_READY;
	ready_queue = lpcb_insert_node(ready_queue, pw, cur_cpu(), &ready_queue);
	if (ready_queue == NULL)
		goto err;
	if (cur_cpu() == NULL)
		/*
		 * cur_cpu() == NULL means that *pw is joining
		 * an empty ready_queue, so run it just now by setting
		 * cur_cpu() = ready_queue. Or no task will be
		 * run and the system will loop forever.
		 */
		current_running[is_scpu()] = ready_queue;
	return;
err:
	panic_g("wake_up: Failed to wake up proc %d", T->pid);
}

/*
 * Insert *cur_cpu() to tail of *Pqueue, RELEASE spin lock slock
 * (if it is not NULL) and then reschedule.
 * The lock will be reacquired after being unblocked.
 * Returns: 0 if switch_to() is called, 1 otherwise.
 */
int do_block(pcb_t ** const Pqueue, spin_lock_t *slock)
{
	// TODO: [p2-task2] block the pcb task into the block queue
	pcb_t *p, *q, *temp;

	for (p = cur_cpu()->next; p != cur_cpu(); p = p->next)
	{
		if (p->status == TASK_READY && p->pid + cur_cpu()->pid != 1)
		{
			q = cur_cpu();
			current_running[is_scpu()] = p;
			cur_cpu()->status = TASK_RUNNING;
			ready_queue = lpcb_del_node(ready_queue, q, &p);
			if (p != q)
				panic_g("do_block: Failed to remove"
					" cur_cpu() from ready_queue");
			//ptlock->block_queue = do_block(p, &(ptlock->block_queue));
			p->status = TASK_SLEEPING;
			temp = lpcb_insert_node(*Pqueue, p, NULL, Pqueue);
			if (temp == NULL)
				panic_g("do_block: Failed to insert pcb %d to queue 0x%lx",
					p->pid, *Pqueue);
			*Pqueue = temp;
			/*
			 * Release the spin lock just after the process is inserted to *Pqueue
			 * and before switch_to().
			 */
			if (slock != NULL)
				spin_lock_release(slock);
#if MULTITHREADING != 0
			switch_to(p->cur_thread == NULL ? &(p->context) : &(p->cur_thread->context),
				cur_cpu()->cur_thread == NULL ? &(cur_cpu()->context)
				: &(cur_cpu()->cur_thread->context));
#else
			switch_to(&(p->context), &(cur_cpu()->context));
#endif
			/* Reacquire the spin lock */
			if (slock != NULL)
				spin_lock_acquire(slock);
			return 0;
		}
	}
	/*
	 * A ready process must be found, because GlucOS keep
	 * main() of kernel as a proc with PID 0.
	 */
	panic_g("do_block: Cannot find a READY process");
	return 1;
}

/*
 * Remove the head from Queue, set its status to be READY
 * and insert it into ready_queue AFTER cur_cpu().
 * Return: new head of Queue.
 */
pcb_t *do_unblock(pcb_t * const Queue)
{
	// TODO: [p2-task2] unblock the `pcb` from the block queue
	pcb_t *prec, *nq, *temp;

	nq = lpcb_del_node(Queue, Queue, &prec);
	if (prec == NULL)
		panic_g("do_unblock: Failed to remove the head of queue 0x%lx", (long)Queue);
	if (prec->status != TASK_EXITED)	/* Sleeping */
	{	/* NOTE: this situation is unlikely to happen on single core */
		if (prec->status == TASK_SLEEPING)
			prec->status = TASK_READY;
		else
			panic_g("do_unblock: proc %d has error status %d",
				prec->pid, (int)prec->status);
	}	/* Don't change status TASK_EXITED. */
	temp = lpcb_insert_node(ready_queue, prec, cur_cpu(), &ready_queue);
	if (temp == NULL)
		panic_g("do_unblock: Failed to insert process (PID=%d) to ready_queue", prec->pid);
	ready_queue = temp;
	return nq;
}

/************************************************************/
void init_pcb_stack(
	ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
	pcb_t *pcb)
{
	 /* TODO: [p2-task3] initialization of registers on kernel stack
	  * HINT: sp, ra, sepc, sstatus
	  * NOTE: To run the task in user mode, you should set corresponding bits
	  *     of sstatus(SPP, SPIE, etc.).
	  */
	pcb->kernel_stack = kernel_stack;
	pcb->kernel_sp = kernel_stack + ROUND(Kstask_size, ADDR_ALIGN)
		 - sizeof(regs_context_t);
	pcb->kernel_sp = ROUNDDOWN(pcb->kernel_sp, SP_ALIGN);
	pcb->user_stack = user_stack;
	pcb->user_sp = user_stack + ROUND(Ustack_size, ADDR_ALIGN);
	pcb->user_sp = ROUNDDOWN(pcb->user_sp, SP_ALIGN);
	/*
	 * After switch_to(), it will
	 * be switched to ret_from_exception.
	 */
	pcb->context.regs[SR_RA] = (reg_t)ret_from_exception;

	/*
	 * This stack pointer is used for calling unlock_kernel(),
	 * in ret_from_exception() just before RESTORE_CONTEXT.
	 */
	pcb->context.regs[SR_SP] = pcb->kernel_sp;

	/*
	 * As the first run of a user process is through sret,
	 * we simulate a trapframe just like it has been trapped.
	 * This trapframe will be restored just before sret is executed.
	 */
	pcb->trapframe = (void *)(pcb->kernel_sp);
	pcb->trapframe->sepc = entry_point;
	if (((pcb->trapframe->sstatus = (r_sstatus() & ~SR_SIE)) & SR_SPP) != 0UL)
		/*
		 * SIE of $sstatus must be zero to ensure that after RESTORE_CONTEXT
		 * before sret, interrupt is off. Otherwise, GlucOS will crash
		 * if timer interrupt comes at this time (when timer interval is small).
		 * And SPP must be zero to ensure that after sret,
		 * the privilige is User Mode for user process.
		 */
		panic_g("init_pcb_stack: SPP is not zero");
	pcb->trapframe->regs[OFFSET_REG_TP / sizeof(reg_t)] = (reg_t)pcb;
	pcb->trapframe->regs[OFFSET_REG_SP / sizeof(reg_t)] = (reg_t)(pcb->user_sp);
	/*
	 * scause must not be 8, which is the number of syscall.
	 * Otherwise $sepc will be added 4 in ret_from_exception.
	 */
	pcb->trapframe->scause = ~(~(0UL) >> 1);

	/*
	 * When calling sys_exit() in crt0.S is error,
	 * call do_exit() directly to cause exception (trying to access kernel).
	 */
	pcb->trapframe->regs[OFFSET_REG_RA / sizeof(reg_t)] = (reg_t)do_exit;

	pcb->status = TASK_READY;
	pcb->wait_queue = NULL;
	pcb->cursor_x = pcb->cursor_y = 0;
	pcb->cylim_h = pcb->cylim_l = -1;
	if ((pcb->pid = alloc_pid()) == INVALID_PID)
		panic_g("init_pcb_stack: No invalid PID can be used");
	
	/* No child thread at the beginning. */
	pcb->cur_thread = NULL;
	pcb->pcthread = NULL;
}

void set_preempt(void)
{
#ifndef TIMER_INTERVAL_MS
#define TIMER_INTERVAL_MS 10U /* unit: ms */
#endif
	static char flag_first = 1;
	static uint64_t timer_interval;
	/* enable preempt */
	__asm__ volatile
	(
		"not	t0, zero\n\t"
		"csrw	0x104, t0"	/* SIE := ~0 */
		:
		:
		: "t0"
	);
	if (flag_first != 0)
	{
		flag_first = 0;
		timer_interval = time_base / 1000U * TIMER_INTERVAL_MS;
	}
	bios_set_timer(get_ticks() + timer_interval);
	//bios_set_timer(get_ticks() + 1000); // Test whether it works for small interval
}

/*
 * do_process_show (ps): show all processes except PID 0.
 * number of processed will be returned and -1 on error.
 */
int do_process_show(void)
{
	int uproc_ymr = 0;
	int i;
	pcb_t *p;
	static const char * const Status[] = {
		[TASK_SLEEPING]	"Sleeping",
		[TASK_RUNNING]	"Running ",
		[TASK_READY]	"Ready   ",
		[TASK_EXITED]	"Exited  "
	};

	printk("    PID     STATUS     CMD\n");

	for (i = 0; i < UPROC_MAX + 1; ++i)
	{
		if (pcb_table[i] != NULL &&
			pcb_table[i]->pid != 0 && pcb_table[i]->pid != 1)
		{
			++uproc_ymr;
			p = pcb_table[i];
			printk("    %02d      %s   %s\n",
				p->pid, Status[p->status], p->name);
		}
	}
	if (uproc_ymr + 2 != get_proc_num())
		panic_g("do_process_show: count of proc is error");
	return uproc_ymr;
}

/*
 * do_exec: execute a user task according its name with
 * command line arguments.
 * argc: number of args; argv[]: pointer to args.
 * If argc < 0, then argc will be determined automatically.
 * If argv is NULL, then argv passed to the task has only argv[0]=NULL.
 * If argc is greater than the actual number of args, then argc
 * passed to the task is the actual number. If argc is less, then
 * argc will be kept but only argc args is passed.
 * If argc > ARGC_MAX or error occurs, INVALID_PID will be returned.
 */
pid_t do_exec(const char *name, int argc, char *argv[])
{
#define ARGC_MAX 8
#define ARG_LEN 63
	pid_t pid;
	pcb_t *pnew;
	int i = 0;
	int l;
	char **argv_base;

	if (argc > ARGC_MAX || (pid = create_proc(name)) == INVALID_PID)
		/* Failed */
		return INVALID_PID;
	if (argv != NULL)
	{
		for (; i < (argc < 0 ? ARGC_MAX : argc); ++i)
			if (argv[i] == NULL)
				break;
		/* Determine argc automatically */
		argc = i;
	}
	else
		argc = 0;
	pnew = lpcb_search_node(ready_queue, pid);
	if (pnew == NULL)
		panic_g("do_exec: Cannot find PCB of %d: %s", pid, name);
	pnew->user_sp -= (sizeof(char *)) * (unsigned int)(argc + 1);
	/* argv_base is the value of argv in user main() */
	argv_base = (char **)(pnew->user_sp);
	argv_base[argc] = NULL;

	for (i = 0; i < argc; ++i)
	{	/* Copy every arg to user stack */
		l = strlen(argv[i]);
		/* The longer part of argv[i] is ignored */
		if (l > ARG_LEN)
			l = ARG_LEN;
		argv_base[i] = (char *)(pnew->user_sp -= (unsigned int)(l + 1));
		strncpy(argv_base[i], argv[i], l);
		argv_base[i][l] = '\0';
	}
	pnew->user_sp = ROUNDDOWN(pnew->user_sp, SP_ALIGN);
	pnew->trapframe->regs[OFFSET_REG_SP / sizeof(reg_t)] = pnew->user_sp;
	pnew->trapframe->regs[OFFSET_REG_A0 / sizeof(reg_t)] = (unsigned int)argc;
	pnew->trapframe->regs[OFFSET_REG_A1 / sizeof(reg_t)] = (reg_t)argv_base;
	return pid;
#undef ARGC_MAX
#undef ARG_LEN
}

/*
 * do_kill: Terminate a process according to its PID.
 * The PID will be returned on success and INVALID_PID on error.
 * NOTE: if a process tries to kill itself, its status will be
 * set TASK_EXITED first and its PCB will be deleted while
 * calling do_scheduler() the next time.
 */
pid_t do_kill(pid_t pid)
{
	pcb_t *p, **phead, *pdel;

	if (pid == 0 || pid == 1)
		/* PID 0, 1 cannot be killed */
		return INVALID_PID;
	if (cur_cpu()->pid == pid)
	{
		cur_cpu()->status = TASK_EXITED;
		do_scheduler();
		panic_g("do_kill: proc %d is still running after killed", pid);
	}
	if ((p = pcb_search(pid)) == NULL)
		/* Not found */
		return INVALID_PID;
	
	/*
	 * Release all resources acquired by it:
	 * this work must be done before we set phead, because
	 * this may move the PCB from one queue to another queue,
	 * especially when it is blocked in a resource such as mailbox.
	 */
	ress_release_killed(pid);

	p->status = TASK_EXITED;
	phead = p->phead;

	/* Checking... */
	if (lpcb_search_node(*phead, pid) != p)
		panic_g("do_kill: phead of proc %d is error", pid);

	/* wake up proc in wait_queue */
	while (p->wait_queue != NULL)
		p->wait_queue = do_unblock(p->wait_queue);

	/* Kill all child threads */
	while (p->pcthread != NULL)
	{
		tcb_t *pd;
		p->pcthread = ltcb_del_node(p->pcthread, p->pcthread, &pd);
		if (pd == NULL)
			panic_g("do_kill: thread %d of process %d found but cannot be deleted",
				p->pcthread->tid, p->pid);
		ufree_g((void *)pd->stack);
		kfree_g((void *)pd);
	}

	/* Free stacks of it */
	kfree_g((void *)p->kernel_stack);
	ufree_g((void *)p->user_stack);

	*phead = lpcb_del_node(*phead, p, &pdel);
	if (pdel == NULL)
		panic_g("do_kill: Failed to del pcb %d in queue 0x%lx", pid, (uint64_t)*phead);
	kfree_g((void *)pdel);
	if (pcb_table_del(pdel) < 0)
		panic_g("do_kill: Failed to remove pcb %d from pcb_table", pid);
#if DEBUG_EN != 0
	writelog("Process whose pid is %d is terminated.", pid);
#endif
	return pid;
}

void do_exit(void)
{
	if (do_kill(cur_cpu()->pid) != cur_cpu()->pid)
		panic_g("do_exit: proc %d failed to exit", cur_cpu()->pid);
}

/*
 * do_waitpid: wait for a process to exit or be killed.
 * Returns: pid on success or INVALID_PID if not found
 * or trying to wait for itself ot a proc waiting for itself.
 */
pid_t do_waitpid(pid_t pid)
{
	// TODO
	pcb_t *p;

	if ((p = pcb_search(pid)) == NULL)
		return INVALID_PID;
	
	if (pid == cur_cpu()->pid)
		/* Cannot wait for itself. */
		return INVALID_PID;
	
	if (lpcb_search_node(cur_cpu()->wait_queue, pid) != NULL)
		/*
		 * Cannot wait for a proc waiting for itself,
		 * which causes "deadlock".
		 */
		return INVALID_PID;
	do_block(&(p->wait_queue), NULL);	/* Temp set slock is NULL */

	return pid;
}

/* do_getpid: get cur_cpu()->pid */
pid_t do_getpid(void)
{
	return cur_cpu()->pid;
}