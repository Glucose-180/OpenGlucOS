/*
 * thread.c: To support multithreading.
 */
#include <os/sched.h>
#include <os/tcb-list-g.h>
#include <os/malloc-g.h>
#include <os/irq.h>
#include <asm/regs.h>

/*
 * Size of stack for every thread
 * 512 has been tried and caused stack overflow!
 */
static const uint32_t Tstack_size = 4 * 1024;

static void thswitch_to(switchto_context_t *prev, switchto_context_t *next);

/*
 * alloc_tid: Search proc->pcthread and allocate
 * a free TID.
 * INVALID_TID will be returned on error.
 */
static tid_t alloc_tid(pcb_t *proc)
{
	tid_t i;
	static const tid_t Tid_min = 1, Tid_max = 16;

	for (i = Tid_min; i <= Tid_max; ++i)
		if (ltcb_search_node(proc->pcthread, i) == NULL)
			return i;
	return INVALID_TID;
}

/*
 * thread_create: Create a child thread of current_running process,
 * and execute function func with argument arg.
 * Return: TID of the child thread and INVALID_TID on error.
 */
tid_t thread_create(void *(*func)(), reg_t arg)
{
	tid_t tid;
	ptr_t stack;
	tcb_t *pnew, *temp;

	if ((temp = ltcb_add_node_to_tail(current_running->pcthread, &pnew))
		== NULL)
		return INVALID_TID;
	current_running->pcthread = temp;
	if ((stack = (ptr_t)umalloc_g(Tstack_size)) == 0)
		return INVALID_TID;
	pnew->stack = stack;
	if ((tid = alloc_tid(current_running)) == INVALID_TID)
	{
		ufree_g((void *)stack);
		return INVALID_TID;
	}
	pnew->tid = tid;
	pnew->context.sepc = (reg_t)func;
	pnew->context.regs[SR_RA] = (reg_t)func;
	pnew->context.regs[SR_SP] = stack + Tstack_size - 512U;
	pnew->arg = arg;
	return tid;
}

/*
 * thread_yield: This thread yields.
 * Switch to another thread of current_running process.
 */
long thread_yield(void)
{
	switchto_context_t *prev, *next;

	if (current_running->cur_thread == NULL)
	{	/* Main thread yields */
		prev = &(current_running->context);
		current_running->cur_thread = current_running->pcthread;
		next = &(current_running->cur_thread->context);
	}
	else if (current_running->cur_thread->next == NULL)
	{	/* The last child thread yields */
		prev = &(current_running->cur_thread->context);
		/* Switch to main thread */
		current_running->cur_thread = NULL;
		next = &(current_running->context);
	}
	else
	{
		/* A child thread yields to another child thread */
		prev = &(current_running->cur_thread->context);
		current_running->cur_thread = current_running->cur_thread->next;
		next = &(current_running->cur_thread->context);
	}
	thswitch_to(prev, next);
	if (current_running->cur_thread != NULL)
		return (long)current_running->cur_thread->arg;
	/*
	 * To support the argument passed to the new born thread.
	 * The value of arg will finally be written in $a0 after RESTORE_CONTEXT.
	 */
	return 0L;
}

/*
 * thread_kill: Kill a child thread whose TID is T.
 * Only main thread can call this.
 * T will be returned on success and INVALID_TID on error.
 */
tid_t thread_kill(tid_t const T)
{
	tcb_t *pt, *pd;

	if (current_running->cur_thread != NULL)
		/* Only main thread can call this */
		return INVALID_TID;
	if ((pt = ltcb_search_node(current_running->pcthread, T)) == NULL)
		/* Not found */
		return INVALID_TID;
	current_running->pcthread = ltcb_del_node(current_running->pcthread, pt, &pd);
	if (pd == NULL)
		panic_g("thread_kill: thread %d of precess %d found but cannot be deleted",
			T, current_running->pid);
	ufree_g((void *)pd->stack);
	kfree_g((void *)pd);
	return T;
}

static void thswitch_to(switchto_context_t *prev, switchto_context_t *next)
{
	prev->sepc = current_running->trapframe->sepc;
	prev->regs[SR_RA] = current_running->trapframe->regs[OFFSET_REG_RA / sizeof(reg_t)];
	prev->regs[SR_SP] = current_running->trapframe->regs[OFFSET_REG_SP / sizeof(reg_t)];
	if (prev->regs[SR_SP] != current_running->user_sp)
		/*
		 * In SAVE_CONTEXT, user stack pointer is saved in both trapframe->regs[SP]
		 * and user_sp. They must be the same.
		 */
		panic_g("thswitch_to: tarpframe->regs[SP] is not equal to current_running->user_sp!");
	prev->regs[SR_S0] = current_running->trapframe->regs[OFFSET_REG_S0 / sizeof(reg_t)];
	prev->regs[SR_S1] = current_running->trapframe->regs[OFFSET_REG_S1 / sizeof(reg_t)];
	prev->regs[SR_S2] = current_running->trapframe->regs[OFFSET_REG_S2 / sizeof(reg_t)];
	prev->regs[SR_S3] = current_running->trapframe->regs[OFFSET_REG_S3 / sizeof(reg_t)];
	prev->regs[SR_S4] = current_running->trapframe->regs[OFFSET_REG_S4 / sizeof(reg_t)];
	prev->regs[SR_S5] = current_running->trapframe->regs[OFFSET_REG_S5 / sizeof(reg_t)];
	prev->regs[SR_S6] = current_running->trapframe->regs[OFFSET_REG_S6 / sizeof(reg_t)];
	prev->regs[SR_S7] = current_running->trapframe->regs[OFFSET_REG_S7 / sizeof(reg_t)];
	prev->regs[SR_S8] = current_running->trapframe->regs[OFFSET_REG_S8 / sizeof(reg_t)];
	prev->regs[SR_S9] = current_running->trapframe->regs[OFFSET_REG_S9 / sizeof(reg_t)];
	prev->regs[SR_S10] = current_running->trapframe->regs[OFFSET_REG_S10 / sizeof(reg_t)];
	prev->regs[SR_S11] = current_running->trapframe->regs[OFFSET_REG_S11 / sizeof(reg_t)];

	current_running->trapframe->sepc = next->sepc;
	current_running->trapframe->regs[OFFSET_REG_RA / sizeof(reg_t)] = next->regs[SR_RA];
	current_running->trapframe->regs[OFFSET_REG_SP / sizeof(reg_t)] = next->regs[SR_SP];
	current_running->user_sp = next->regs[SR_SP];
	current_running->trapframe->regs[OFFSET_REG_S0 / sizeof(reg_t)] = next->regs[SR_S0];
	current_running->trapframe->regs[OFFSET_REG_S1 / sizeof(reg_t)] = next->regs[SR_S1];
	current_running->trapframe->regs[OFFSET_REG_S2 / sizeof(reg_t)] = next->regs[SR_S2];
	current_running->trapframe->regs[OFFSET_REG_S3 / sizeof(reg_t)] = next->regs[SR_S3];
	current_running->trapframe->regs[OFFSET_REG_S4 / sizeof(reg_t)] = next->regs[SR_S4];
	current_running->trapframe->regs[OFFSET_REG_S5 / sizeof(reg_t)] = next->regs[SR_S5];
	current_running->trapframe->regs[OFFSET_REG_S6 / sizeof(reg_t)] = next->regs[SR_S6];
	current_running->trapframe->regs[OFFSET_REG_S7 / sizeof(reg_t)] = next->regs[SR_S7];
	current_running->trapframe->regs[OFFSET_REG_S8 / sizeof(reg_t)] = next->regs[SR_S8];
	current_running->trapframe->regs[OFFSET_REG_S9 / sizeof(reg_t)] = next->regs[SR_S9];
	current_running->trapframe->regs[OFFSET_REG_S10 / sizeof(reg_t)] = next->regs[SR_S10];
	current_running->trapframe->regs[OFFSET_REG_S11 / sizeof(reg_t)] = next->regs[SR_S11];
}
