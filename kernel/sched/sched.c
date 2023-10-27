#include <os/list.h>
#include <os/pcb-list-g.h>
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

//pcb_t pcb[NUM_MAX_TASK];
/*
 * Has been modified by Glucose180
 */
const ptr_t pid0_stack = INIT_KERNEL_STACK;// + PAGE_SIZE;

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
	.cursor_x = 0,
	.cursor_y = 0
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
pcb_t * volatile current_running;

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
	static const pid_t Pid_min = 1, Pid_max = 2 * NUM_MAX_TASK;

	for (i = Pid_min; i <= Pid_max; ++i)
		if (lpcb_search_node(ready_queue, i) == NULL)
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
	if (taskname == NULL)
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
	if ((temp = lpcb_add_node_to_tail(ready_queue, &pnew)) == NULL)
	{
		ufree_g((void *)user_stack);
		return INVALID_PID;
	}
	ready_queue = temp;
	/*if (p0 != NULL)
	{	// Has removed the 0 proc
		p0->next = ready_queue;
		if (p0 != current_running)
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
	return pnew->pid;
}

void do_scheduler(void)
{
	pcb_t *p, *q;
	// TODO: [p2-task3] Check sleep queue to wake up PCBs

	/************************************************************/
	/* Do not touch this comment. Reserved for future projects. */
	/************************************************************/

	check_sleeping();
	if (current_running != NULL)
	{
		for (p = current_running->next; p != current_running; p = p->next)
		{	/* Search the linked list and find a READY process */
			if (p->status == TASK_READY)
			{
				current_running->status = TASK_READY;
				q = current_running;
				current_running = p;
				current_running->status = TASK_RUNNING;
				/*
				 * Define MULTITHREADING as 1 to support multithreading. When switch_to()
				 * is called, we should first determine which thread will be switched from
				 * and to. If multithreading is disabled, just keep switch_to() as normal.
				 */
#if MULTITHREADING != 0
				switch_to(q->cur_thread == NULL ? &(q->context) : &(q->cur_thread->context),
					current_running->cur_thread == NULL ? &(current_running->context)
					: &(current_running->cur_thread->context)
				);
#else
				switch_to(&(q->context), &(current_running->context));
#endif
				return;
			}
		}
		return;
	}
	else
		panic_g("do_scheduler: current_running is NULL");
}

void do_sleep(uint32_t sleep_time)
{
	// TODO: [p2-task3] sleep(seconds)
	// NOTE: you can assume: 1 second = 1 `timebase` ticks
	// 1. block the current_running
	// 2. set the wake up time for the blocked task
	// 3. reschedule because the current_running is blocked.
	pcb_t *temp, *psleep;

	temp = current_running->next;
	ready_queue = lpcb_del_node(ready_queue, current_running, &psleep);
	if (psleep != current_running)
		panic_g("do_sleep: Failed to remove current_running");
	if (ready_queue == NULL)
		/*
		 * Note that now we don't permit current_running == NULL,
		 * so this would cause PANIC!
		 */
		current_running = NULL;
	else
		current_running = temp;
	if ((sleep_queue = lpcb_insert_node(sleep_queue, psleep, NULL)) == NULL)
		panic_g("do_sleep: Failed to insert proc %d to sleep_queue", psleep->pid);
	psleep->status = TASK_SLEEPING;
	if ((psleep->wakeup_time = get_timer() + sleep_time) > time_max_sec)
		/* Avoid creating a sleeping task that would never be woken up */
		psleep->wakeup_time = time_max_sec;
	if (current_running == NULL)
		do_scheduler();
	else
#if MULTITHREADING != 0
		switch_to(psleep->cur_thread == NULL ? &(psleep->context)
			: &(psleep->cur_thread->context),
			current_running->cur_thread == NULL ? &(current_running->context)
			: &(current_running->cur_thread->context)
		);
#else
		switch_to(&(psleep->context), &(current_running->context));
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
 * to ready_queue after *current_running.
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
	ready_queue = lpcb_insert_node(ready_queue, pw, current_running);
	if (ready_queue == NULL)
		goto err;
	if (current_running == NULL)
		/*
		 * current_running == NULL means that *pw is joining
		 * an empty ready_queue, so run it just now by setting
		 * current_running = ready_queue. Or no task will be
		 * run and the system will loop forever.
		 */
		current_running = ready_queue;
	return;
err:
	panic_g("wake_up: Failed to wake up proc %d", T->pid);
}

//void do_block(list_node_t *pcb_node, list_head *queue)
/*
 * Insert *Pt to tail of Queue and return the new head of Queue.
 * NULL will be returned on error (It is unlike to happen).
 */
pcb_t *do_block(pcb_t * const Pt, pcb_t * const Queue)
{
	// TODO: [p2-task2] block the pcb task into the block queue
	Pt->status = TASK_SLEEPING;
	return lpcb_insert_node(Queue, Pt, NULL);
}

//void do_unblock(list_node_t *pcb_node)
/*
 * Remove the head from Queue, set its status to be READY
 * and insert it into ready_queue AFTER current_running.
 * Return: new head of Queue.
 */
pcb_t *do_unblock(pcb_t * const Queue)
{
	// TODO: [p2-task2] unblock the `pcb` from the block queue
	pcb_t *prec, *nq, *temp;

	nq = lpcb_del_node(Queue, Queue, &prec);
	if (prec == NULL)
		panic_g("do_unblock: Failed to remove the head of queue 0x%lx", (long)Queue);
	prec->status = TASK_READY;
	temp = lpcb_insert_node(ready_queue, prec, current_running);
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
	 * As the first run of a user process is through sret,
	 * we simulate a trapframe just like it has been trapped.
	 * This trapframe will be restored just before sret is executed.
	 */
	pcb->trapframe = (void *)(pcb->kernel_sp);
	pcb->trapframe->sepc = entry_point;
	if (((pcb->trapframe->sstatus = r_sstatus()) & SR_SPP) != 0UL)
		/*
		* SPP of $sstatus must be zero to ensure that after sret,
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

	pcb->status = TASK_READY;
	pcb->cursor_x = pcb->cursor_y = 0;
	if ((pcb->pid = alloc_pid()) == INVALID_PID)
		panic_g("init_pcb_stack: No invalid PID can be used");
	
	/* No child thread at the beginning. */
	pcb->cur_thread = NULL;
	pcb->pcthread = NULL;
}

void set_preempt(void)
{
#ifndef TIMER_INTERVAL_MS
#define TIMER_INTERVAL_MS 40U /* unit: ms */
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
}

/*
 * do_process_show (ps): show all processes except PID 0.
 * number of processed will be returned and -1 on error.
 */
int do_process_show(void)
{
	int proc_ymr = 0;
	pcb_t *p;
	static const char * const Status[] = {
		[TASK_SLEEPING]	"Sleeping",
		[TASK_RUNNING]	"Running ",
		[TASK_READY]	"Ready   ",
		[TASK_EXITED]	"Exited  "
	};

	printk("\tPID\t\t STATUS \t\tCMD\n");

	if (ready_queue != NULL)
	{
		p = ready_queue;
		do {
			if (p->pid != 0)
			{	/* Skip PID 0 */
				++proc_ymr;
				printk("\t %d\t\t%s\t\t%s\n",
					p->pid, Status[p->status], p->name);
			}
		} while ((p = p->next) != ready_queue);
	}
	if (sleep_queue != NULL)
	{
		p = sleep_queue;
		do {
			++proc_ymr;
			printk("\t %d\t\t%s\t\t%s\n",
				p->pid, Status[p->status], p->name);
		} while ((p = p->next) != sleep_queue);
	}
	/*
	 * NOTE: after sys_waitpid() is implemented,
	 * processes waiting for another process maybe also
	 * shoule be considered.
	 */
	return proc_ymr;
}

pid_t do_exec(const char *name, char *argv[])
{
#define ARGC_MAX 8
#define ARG_LEN 63
	pid_t pid;
	pcb_t *pnew;
	int i, argc;
	int l;
	char **argv_base;

	if ((pid = create_proc(name)) == INVALID_PID)
		/* Failed */
		return INVALID_PID;
	for (i = 0; i < ARGC_MAX; ++i)
		if (argv[i] == NULL)
			break;
	/* Ignore args more than ARGC_MAX */
	argv[argc = i] = NULL;
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