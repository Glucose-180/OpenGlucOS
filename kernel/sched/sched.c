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
#include <os/smp.h>

/*
 * Has been modified by Glucose180
 */
const ptr_t pid0_stack = 0xffffffc051000000,
	pid1_stack = 0xffffffc050f00000;

const uintptr_t User_sp = USER_STACK_ADDR;

/*
 * The default kernel size for a process:
 * 32 KiB.
 */
const uint32_t Kstack_size = 32U * 1024U;

pid_t pid_glush = INVALID_PID;

/*
 * It is used to represent main.c:main()
 */
pcb_t pid0_pcb = {
	.pid = 0,
	.kernel_sp = (ptr_t)pid0_stack,
	.user_sp = (ptr_t)pid0_stack,
	/*
	 * In face, `user_sp` and `trapframe` are
	 * meaningless for `pid0_pcb` and `pid1_pcb`.
	 */
	.trapframe = (void *)pid0_stack,
	/* Add more info */
	.status = TASK_RUNNING,
	.wait_queue = NULL,
	.name = "_main",
	.cursor_x = 0,
	.cursor_y = 0,
	.cylim_h = -1,
	.cylim_l = -1,
	/* to make pid0_pcb.phead point to ready_queue */
	.phead = &ready_queue,
//#if MULTITHREADING != 0
	.tid = 0,
//#endif
	.cpu_mask = 1U << 0,
	.pgdir_kva = (PTE*)PGDIR_VA,
	.cpath = "/",
	.cur_ino = 0U
};

#if NCPU == 2
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
	/* to make pid0_pcb.phead point to ready_queue */
	.phead = &ready_queue,
//#if MULTITHREADING != 0
	.tid = 0,
//#endif
	.cpu_mask = 1U << 1,
	.pgdir_kva = (PTE*)PGDIR_VA,
	.cpath = "/",
	.cur_ino = 0U
};
#endif

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
pcb_t * volatile current_running[NCPU];

/*
 * alloc_pid: find a free PID.
 * INVALID_PID will be returned if no free PID is found.
 * NOTE: It can be improved as the algorithm is NOT efficient.
 */
pid_t alloc_pid(void)
{
	pid_t i;
	static const pid_t Pid_min = NCPU, Pid_max = 2 * UPROC_MAX;

	for (i = Pid_min; i <= Pid_max; ++i)
		if (pcb_search(i) == NULL)
			return i;
	return INVALID_PID;
}

/*
 * Use the name of a task to load it and add it in ready_queue.
 * INVALID_PID will be returned on error.
 */
pid_t create_proc(const char *taskname, unsigned int cpu_mask)
{
	uintptr_t entry, kernel_stack, seg_start, seg_end;
	pcb_t *pnew, *qtemp;
	PTE* pgdir_kva;
	pid_t pid;
	pcb_t *pdel;

	if (taskname == NULL || get_proc_num() >= UPROC_MAX + NCPU)
		return INVALID_PID;

	if ((pid = alloc_pid()) == INVALID_PID)
		panic_g("No invalid PID");
	
	pgdir_kva = (PTE*)alloc_pagetable(pid);
	share_pgtable(pgdir_kva, (PTE*)PGDIR_VA);

	if ((qtemp = lpcb_add_node_to_tail(ready_queue, &pnew, &ready_queue)) == NULL)
	{
		free_pages_of_proc((PTE*)pgdir_kva, pid);
		return INVALID_PID;
	}
	if (pcb_table_add(pnew) < 0)
		/*
		 * The number of processes has been checked at the beginning,
		 * so this opreation shouldn't fail.
		 */
		panic_g("Failed to add to pcb_table");
	ready_queue = qtemp;
	pnew->pid = pid;
	pnew->pgdir_kva = pgdir_kva;

	/*
	 * Allocate PCB and add it to `ready_queue` and `pcb_table[]` before
	 * allocate page frames for the new born process, in case of page swap
	 * happens while allocating page frames, which may cause panic in
	 * `swap_to_disk()` because the PCB may not be found.
	 */
	entry = load_task_img(taskname, pgdir_kva, pid, &seg_start, &seg_end);
	if (entry == 0U)
		goto del_pcb_and_pg_on_error;

	/*
	 * Just allocate one page for command line arguments.
	 */
	alloc_page_helper(User_sp - NORMAL_PAGE_SIZE, (uintptr_t)pgdir_kva, pid);

	kernel_stack = (uintptr_t)kmalloc_g(Kstack_size);

	if (kernel_stack == 0)
		goto del_pcb_and_pg_on_error;
	pnew->seg_start = seg_start;
	pnew->seg_end = seg_end;
	strncpy(pnew->name, taskname, TASK_NAMELEN);
	pnew->name[TASK_NAMELEN] = '\0';
	pnew->status = TASK_READY;
	pnew->wait_queue = NULL;
	pnew->cursor_x = pnew->cursor_y = 0;
	pnew->cylim_h = pnew->cylim_l = -1;
	pnew->cpu_mask = cpu_mask;
	pnew->cpath[0] = '/';
	pnew->cpath[1] = '\0';
	pnew->cur_ino = 0U;
	fd_init(pnew, NULL);
#if MULTITHREADING != 0
	/* 0 TID is the main thread */
	pnew->tid = 0;
#endif
	init_pcb_stack(kernel_stack, entry, pnew);
#if DEBUG_EN != 0
	writelog("Process \"%s\" (PID: %d) is created", pnew->name, pnew->pid);
#endif
	return pnew->pid;
del_pcb_and_pg_on_error:
	free_pages_of_proc((PTE*)pgdir_kva, pid);
	ready_queue = lpcb_del_node(ready_queue, pnew, &pdel);
	if (pnew != pdel)
		panic_g("Failed to remove PCB %d from ready_queue", pid);
	kfree_g((void *)pdel);
	if (pcb_table_del(pdel) < 0)
		panic_g("Failed to remove PCB %d from pcb_table[]", pid);
	return INVALID_PID;
}

void do_scheduler(void)
{
	pcb_t *p, *q;
	pcb_t *ccpu = cur_cpu();
	uint64_t isscpu = is_scpu();

	check_sleeping();

	void check_sleeping_on_nic(void);
	check_sleeping_on_nic();

	if (ccpu != NULL)
	{
		pcb_t *nextp;
		for (p = ccpu->next; p != ccpu; p = nextp)
		{	/* Search the linked list and find a READY process */
			nextp = p->next;	/* Store p->next in case of it is killed */
			if (p->status == TASK_READY &&
				(p->cpu_mask & (1U << get_current_cpu_id())) != 0U)
			{
				//if (cur_cpu()->status != TASK_EXITED)
				if (ccpu->status == TASK_RUNNING)
					/*
					 * NOTE: this is a dangerous operation! Many bugs have been
					 * detected at here. For example, when a thread is set `TASK_ZOMBIE`
					 * and then calls `do_scheduler()`, its status is set to `TASK_READY`
					 * again. That causes it to be scheduled again!
					 */
					ccpu->status = TASK_READY;
				q = ccpu;
				current_running[isscpu] = p;
				ccpu = cur_cpu();	/* Update `ccpu` because `cur_cpu()` is changed */
				ccpu->status = TASK_RUNNING;
				switch_to(&(q->context), &(ccpu->context));
				return;
			}
			else if (p->status == TASK_EXITED)
			{
				pid_t kpid = p->pid;
				if (kpid == ccpu->pid || kpid != do_kill(kpid))
					panic_g("Failed to kill proc %d", p->pid);
			}
#if MULTITHREADING != 0
			else if (p->status == TASK_ZOMBIE)
			{
				thread_kill(p);
			}
#endif
		}
		if (cur_cpu()->pid >= NCPU)
			/*
			 * If cur_cpu()->pid is not 0, then
			 * a ready process must be found, because GlucOS keep
			 * main() of kernel as a proc with PID 0.
			 */
			panic_g("Cannot find a ready process");
		return;
	}
	else
		panic_g("cur_cpu() is NULL");
}

void do_sleep(uint32_t sleep_time)
{
	// TODO: [p2-task3] sleep(seconds)
	// NOTE: you can assume: 1 second = 1 `timebase` ticks
	// 1. block the cur_cpu()
	// 2. set the wake up time for the blocked task
	// 3. reschedule because the cur_cpu() is blocked.
	pcb_t *psleep = cur_cpu();

	psleep->status = TASK_SLEEPING;
	if ((psleep->wakeup_time = get_timer() + sleep_time) > time_max_sec)
		/* Avoid creating a sleeping task that would never be woken up */
		psleep->wakeup_time = time_max_sec;
	do_block(&sleep_queue, NULL);
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
			panic_g("Linked list operation failed");
	} while (pw != temp_sleep);
	return;
err:
	panic_g("proc %d in sleep_queue is not SLEEPING", pw->pid);
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
	panic_g("Failed to wake up proc %d", T->pid);
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
	pcb_t *p, *q, *temp, *ccpu = cur_cpu();
	int isscpu = is_scpu();

	for (p = ccpu->next; p != ccpu; p = p->next)
	{
		if (p->status == TASK_READY &&
			(p->cpu_mask & (1U << get_current_cpu_id())) != 0U)
		{
			q = ccpu;
			ccpu = current_running[isscpu] = p;
			ccpu->status = TASK_RUNNING;
			ready_queue = lpcb_del_node(ready_queue, q, &p);
			if (p != q)
				panic_g("Failed to remove"
					" cur_cpu() from ready_queue");
			p->status = TASK_SLEEPING;
			temp = lpcb_insert_node(*Pqueue, p, NULL, Pqueue);
			if (temp == NULL)
				panic_g("Failed to insert pcb %d to queue 0x%lx",
					p->pid, *Pqueue);
			*Pqueue = temp;
			/*
			 * Release the spin lock just after the process is inserted to *Pqueue
			 * and before switch_to().
			 */
			if (slock != NULL)
				spin_lock_release(slock);
			switch_to(&(p->context), &(ccpu->context));

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
	panic_g("Cannot find a READY process");
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
		panic_g("Failed to remove the head of queue 0x%lx", (long)Queue);
	if (prec->status == TASK_SLEEPING)
		prec->status = TASK_READY;
	else
		panic_g("proc %d in queue 0x%lx has error status %d",
			prec->pid, (uintptr_t)Queue, (int)prec->status);
	temp = lpcb_insert_node(ready_queue, prec, cur_cpu(), &ready_queue);
	if (temp == NULL)
		panic_g("Failed to insert process %d to ready_queue", prec->pid);
	ready_queue = temp;
	return nq;
}

/************************************************************/
void init_pcb_stack(
	uintptr_t kernel_stack, uintptr_t entry_point, pcb_t *pcb)
{
	 /* TODO: [p2-task3] initialization of registers on kernel stack
	  * HINT: sp, ra, sepc, sstatus
	  * NOTE: To run the task in user mode, you should set corresponding bits
	  *     of sstatus(SPP, SPIE, etc.).
	  */
	pcb->kernel_stack = kernel_stack;
	pcb->kernel_sp = kernel_stack + ROUND(Kstack_size, ADDR_ALIGN)
		 - sizeof(regs_context_t);
	pcb->kernel_sp = ROUNDDOWN(pcb->kernel_sp, SP_ALIGN);
	/*
	 * pcb->user_sp saves the user virtual address (UVA)
	 * rather than KVA.
	 */
	pcb->user_sp = User_sp;
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
	//if (((pcb->trapframe->sstatus = (r_sstatus() & ~SR_SIE) | SR_SPIE) & SR_SPP) != 0UL)
		/*
		 * SIE of $sstatus must be zero to ensure that after RESTORE_CONTEXT
		 * before sret, interrupt is off. Otherwise, GlucOS will crash
		 * if timer interrupt comes at this time (when timer interval is small).
		 * SPIE must be 1 to enable interrupt after sret.
		 * And SPP must be 0 to ensure that after sret,
		 * the privilige is User Mode for user process.
		 */
	pcb->trapframe->sstatus = (r_sstatus() & ~SR_SIE & ~SR_SPP) | SR_SPIE;// | SR_SUM;
	//panic_g("SPP is not zero");
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
}

void set_preempt(void)
{
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
	int i, cpuid;
	pcb_t *p;
	static const char * const Status[] = {
		[TASK_SLEEPING]	"Sleeping",
		[TASK_RUNNING]	"Running",
		[TASK_READY]	"Ready   ",
		[TASK_EXITED]	"Exited  ",
		[TASK_EXITING]	"Exiting ",
		[TASK_ZOMBIE]	"Zombie  "
	};

#if MULTITHREADING != 0
	printk(" PID(TID)    STATUS    CPUmask    CMD\n");
#else
	printk("    PID     STATUS    CPUmask    CMD\n");
#endif

	for (i = 0; i < UPROC_MAX + NCPU; ++i)
	{
		if (pcb_table[i] != NULL &&
			//pcb_table[i]->pid != 0 && pcb_table[i]->pid != 1)
			pcb_table[i]->pid >= NCPU)
		{
			++uproc_ymr;
			p = pcb_table[i];

			/* For RUNNING processes */
			if (current_running[0] == p)
				cpuid = 0;
			else
				cpuid = 1;
#if MULTITHREADING != 0
			if (p->status != TASK_RUNNING)
				printk("  %02d(%02d)     %s    0x%x    %s\n",
					p->pid, p->tid, Status[p->status], p->cpu_mask & 0xffU, p->name);
			else
				printk("  %02d(%02d)     %s%d    0x%x    %s\n",
					p->pid, p->tid, Status[p->status], cpuid, p->cpu_mask & 0xffU, p->name);
#else
			if (p->status != TASK_RUNNING)
				printk("    %02d      %s    0x%x    %s\n",
					p->pid, Status[p->status], p->cpu_mask & 0xffU, p->name);
			else
				printk("    %02d      %s%d    0x%x    %s\n",
					p->pid, Status[p->status], cpuid, p->cpu_mask & 0xffU, p->name);
#endif
		}
	}
	if (uproc_ymr + NCPU != get_proc_num())
		panic_g("count of proc is error");
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
	char **argv_base, **argv_base_kva;

	if (argc > ARGC_MAX ||
		/*
		 * By default, a process has the same cpu_mask as the process
		 * that creates it.
		 */
		(pid = create_proc(name, cur_cpu()->cpu_mask)) == INVALID_PID)
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
		panic_g("Cannot find PCB of %d: %s", pid, name);
	/*
	 * NOTE: command arguments should NOT be too large to
	 * ensure that writing them wouldnot exceed a page!
	 */
	pnew->user_sp -= (sizeof(char *)) * (unsigned int)(argc + 1);
	/*
	 * `argv_base` is the value of argv in user main().
	 * `argv_base_kva` is the coresponding KVA.
	 */
	argv_base = (char **)(pnew->user_sp);
	argv_base_kva = (char **)va2kva((uintptr_t)argv_base, pnew->pgdir_kva);
	if (argv_base_kva == NULL)
		panic_g("Failed to translate virtual address 0x%lx in page dir 0x%lx",
			pnew->user_sp, (uint64_t)pnew->pgdir_kva);
	argv_base_kva[argc] = NULL;

	for (i = 0; i < argc; ++i)
	{	/* Copy every arg to user stack */
		char *arg_kva;
		l = strlen(argv[i]);
		/* The longer part of argv[i] is ignored */
		if (l > ARG_LEN)
			l = ARG_LEN;
		argv_base_kva[i] = (char *)(pnew->user_sp -= (unsigned int)(l + 1));
		arg_kva = (char *)va2kva((uintptr_t)argv_base_kva[i], pnew->pgdir_kva);
		if (arg_kva == NULL)
			panic_g("Failed to translate virtual address 0x%lx in page dir 0x%lx",
				argv_base_kva[i], (uint64_t)pnew->pgdir_kva);
		strncpy(arg_kva, argv[i], l);
		arg_kva[l] = '\0';
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
#if DEBUG_EN == 0
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
pid_t do_kill(pid_t pid)
{
	unsigned int pgfreed_ymr;
	pid_t start_glush(void);

	if (pid < NCPU)
		return INVALID_PID;
#if MULTITHREADING != 0
	/* For multithreading */
	pcb_t *farr[TID_MAX + 1];
	PTE *pgdir = NULL;
	unsigned int i, nth = pcb_search_all(pid, farr);
	if (nth == 0U)
		/* Not found */
		return INVALID_PID;
	for (i = 0U; i < nth; ++i)
	{
		if (pgdir == NULL)
			pgdir = farr[i]->pgdir_kva;
		else if (farr[i]->pgdir_kva != pgdir)
			panic_g("Thread %d of proc %d has different pgdir_kva",
				farr[i]->tid, pid);
		if (farr[i]->status != TASK_RUNNING &&
			farr[i]->status != TASK_EXITING)
			thread_kill(farr[i]);
		else
		{
			farr[i]->status = TASK_EXITING;
#if DEBUG_EN != 0
			writelog("Thread %d of proc %d is to be killed (EXITING)",
			farr[i]->tid, farr[i]->pid);
#endif
		}
	}
	if (cur_cpu()->pid == pid)
	{
		cur_cpu()->status = TASK_EXITED;
		do_scheduler();
		panic_g("thread %d of proc %d is still running after killed",
			cur_cpu()->tid, pid);
	}
	if ((nth = pcb_search_all(pid, farr)) == 0U)
	{
		ress_release_killed(pid);
		pgfreed_ymr = free_pages_of_proc(pgdir, pid);
#if DEBUG_EN != 0
		writelog("Process %d is terminated and %u page frames are freed",
			pid, pgfreed_ymr);
#endif
		if (pid == pid_glush)
			/* If glush is killed, restart it. */
			pid_glush = start_glush();
	}
	return pid;
#else
	pcb_t *p, **phead, *pdel;
	if (cur_cpu()->pid == pid)
	{
		cur_cpu()->status = TASK_EXITED;
		do_scheduler();
		panic_g("proc %d is still running after killed", pid);
	}
	if ((p = pcb_search(pid)) == NULL)
		/* Not found */
		return INVALID_PID;
#if NCPU == 2
	if (p->status == TASK_RUNNING)
	{
		p->status = TASK_EXITING;
#if DEBUG_EN != 0
		writelog("Process %d is to be killed (EXITING)", pid);
#endif
		return pid;
	}
#endif
	
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
		panic_g("phead of proc %d is error", pid);

	/* wake up proc in wait_queue */
	while (p->wait_queue != NULL)
		p->wait_queue = do_unblock(p->wait_queue);

	/* Free stacks of it */
	kfree_g((void *)p->kernel_stack);
	pgfreed_ymr = free_pages_of_proc(p->pgdir_kva, pid);

	flist_dec_fnode(p->cur_ino, 0);

	*phead = lpcb_del_node(*phead, p, &pdel);
	if (pdel == NULL)
		panic_g("Failed to del pcb %d in queue 0x%lx", pid, (uint64_t)*phead);
	kfree_g((void *)pdel);
	if (pcb_table_del(pdel) < 0)
		panic_g("Failed to remove pcb %d from pcb_table", pid);
#if DEBUG_EN != 0
	writelog("Process %d is terminated and %u page frames are freed",
		pid, pgfreed_ymr);
#endif
	if (pid == pid_glush)
		/* If glush is killed, restart it. */
		pid_glush = start_glush();
	return pid;
#endif
}

void do_exit(void)
{
	if (do_kill(cur_cpu()->pid) != cur_cpu()->pid)
		panic_g("proc %d failed to exit", cur_cpu()->pid);
}

/*
 * do_waitpid: wait for a process to exit or be killed.
 * Returns: pid on success or INVALID_PID if not found
 * or trying to wait for itself ot a proc waiting for itself.
 */
pid_t do_waitpid(pid_t pid)
{
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
	do_block(&(p->wait_queue), NULL);

	return pid;
}

/* do_getpid: get `cur_cpu()->pid` */
pid_t do_getpid(void)
{
	return cur_cpu()->pid;
}

/*
 * do_taskset: Set CPU affinity of a process.
 * If `create` is zero, it will set an existing process according to PID
 * and `name` will be ignored. Otherwise, it will create a new process
 * according to its `name` and `pid` will be ignored.
 * Returns: PID of set process on success and INVALID_PID on error.
 */
pid_t do_taskset(int create, char *name, pid_t pid, unsigned int cpu_mask)
{
	pid_t npid;
	pcb_t *p;

	if (create != 0)
	{
		npid = do_exec(name, 1, &name);
		if (npid == INVALID_PID)
			/* Failed */
			return npid;
		p = pcb_search(npid);
		if (p == NULL)
			panic_g("Cannot find process \"%s\" with PID %d",
				name, npid);
		p->cpu_mask = cpu_mask;
		return npid;
	}
	else
	{
		if (pid < NCPU)
			/* Cannot set 0, 1 */
			return INVALID_PID;
#if MULTITHREADING != 0
		tcb_t *farr[TID_MAX + 1];
		unsigned int i, nth = pcb_search_all(pid, farr);
		if (nth == 0U)
			return INVALID_PID;
		else
			for (i = 0U; i < nth; ++i)
				farr[i]->cpu_mask = cpu_mask;

#else
		p = pcb_search(pid);
		if (p == NULL)
			return INVALID_PID;
		p->cpu_mask = cpu_mask;
#endif
		return pid;
	}
}