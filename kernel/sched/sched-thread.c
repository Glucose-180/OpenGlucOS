#include <os/sched.h>
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
tid_t alloc_tid(void)
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
 * Return the TID or `INVALID_TID` on error.
 */
pthread_t do_thread_create(uintptr_t entry, uintptr_t arg, uintptr_t sp)
{
	tid_t tid;
	pcb_t *qtemp, *pnew, *ccpu = cur_cpu();
	uintptr_t kernel_stack;

	if ((tid = alloc_tid()) == INVALID_TID ||
		get_proc_num() >= UPROC_MAX + NCPU)
		return INVALID_TID;
	if (!((sp >= ccpu->seg_start && sp < ccpu->seg_end) ||
		(sp >= User_sp - USTACK_NPG * NORMAL_PAGE_SIZE && sp < User_sp)))
		return INVALID_TID;
	if (entry >= KVA_MIN || arg >= KVA_MIN)
		return INVALID_TID;
	kernel_stack = (uintptr_t)kmalloc_g(Kstack_size);
	if (kernel_stack == 0UL)
		return INVALID_TID;
	qtemp = lpcb_add_node_to_tail(ready_queue, &pnew, &ready_queue);
	if (qtemp == NULL)
		return INVALID_TID;
	ready_queue = qtemp;
	pnew->tid = tid;
	pnew->pid = ccpu->pid;
	pnew->pgdir_kva = ccpu->pgdir_kva;
	pnew->user_sp = sp;
	pnew->kernel_stack = kernel_stack;
	strncpy(pnew->name, ccpu->name, TASK_NAMELEN);
	pnew->name[TASK_NAMELEN] = '\0';
	init_pcb_stack(kernel_stack, entry, pnew);
	pnew->trapframe->regs[OFFSET_REG_A0 / sizeof(reg_t)] = arg;
	if (pcb_table_add(pnew) < 0)
		panic_g("do_thread_create: Failed to create thread %d", tid);
	return tid;
}

/*
 * `do_thread_wait`: wait for a thread of current process
 * to exit.
 */
int do_thread_wait(pthread_t tid)
{
	return 0;
}
