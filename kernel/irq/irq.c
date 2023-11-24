#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/kernel.h>
#include <printk.h>
#include <assert.h>
#include <screen.h>
#include <os/glucose.h>
#include <riscv.h>
#include <csr.h>
#include <os/mm.h>
#include <os/smp.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];

static void handle_soft(regs_context_t *regs, uint64_t stval, uint64_t scause);

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
#if NCPU == 2
	if (cur_cpu()->status == TASK_EXITING)
	{
		/*
		* ..->status being TASK_EXITING means that it is killed
		* by process on another CPU while running.
		*/
		do_exit();
		panic_g("interrupt_helper: proc %d is still running after killed",
			cur_cpu()->pid);
	}
#endif
	if ((int64_t)scause < 0)
	{	/* Interrupt */
		scause &= (((uint64_t)~0UL) >> 1);
		if (scause >= IRQC_COUNT)
			panic_g("interrupt_helper: exception code of "
				"Interrupt is error: 0x%lx", scause);
		irq_table[scause](regs, stval, scause);
	}
	else
	{
		exc_table[scause](regs, stval, scause);
	}
}

void handle_irq_timer(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
	// TODO: [p2-task4] clock interrupt handler.
	// Note: use bios_set_timer to reset the timer and remember to reschedule
	//printk("Timer interrupt comes!\n");	//Only for TEST

	set_preempt();	/* Reset timer */
	do_scheduler();
}

void init_exception()
{
	/* TODO: [p2-task3] initialize exc_table */
	/* NOTE: handle_syscall, handle_other, etc.*/
	int i;
	reg_t sstatus;

	for (i = 0; i < EXCC_COUNT; ++i)
		exc_table[i] = handle_other;
	exc_table[EXCC_SYSCALL] = handle_syscall;
	exc_table[EXCC_INST_PAGE_FAULT] = handle_pagefault;
	exc_table[EXCC_LOAD_PAGE_FAULT] = handle_pagefault;
	exc_table[EXCC_STORE_PAGE_FAULT] = handle_pagefault;
	/* TODO: [p2-task4] initialize irq_table */
	/* NOTE: handle_int, handle_other, etc.*/
	for (i = 0; i < IRQC_COUNT; ++i)
		irq_table[i] = handle_other;
	/*
	 * It seems that all timer interrupts used by us
	 * are S timer interrupts.
	 */
	//irq_table[IRQC_U_TIMER] = handle_irq_timer;
	irq_table[IRQC_S_TIMER] = handle_irq_timer;
	irq_table[IRQ_S_SOFT] = handle_soft;

	/* TODO: [p2-task3] set up the entrypoint of exceptions */
	setup_exception();

	sstatus = r_sstatus();
	sstatus |= SR_SPIE | SR_SUM;
	sstatus &= ~(SR_SPP | SR_FS);
	w_sstatus(sstatus);
}

static void handle_soft(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
#if DEBUG_EN != 0
	writelog("CPU %lu gets soft irq", get_current_cpu_id());
#else
	while (1)
		;
#endif
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
	static char* reg_name[] = {
		"zero "," ra  "," sp  "," gp  "," tp  ",
		" t0  "," t1  "," t2  ","s0/fp"," s1  ",
		" a0  "," a1  "," a2  "," a3  "," a4  ",
		" a5  "," a6  "," a7  "," s2  "," s3  ",
		" s4  "," s5  "," s6  "," s7  "," s8  ",
		" s9  "," s10 "," s11 "," t3  "," t4  ",
		" t5  "," t6  "
	};
	if ((regs->sstatus & SR_SPP) != 0UL)
	{
		for (int i = 0; i < 32; i += 3) {
			for (int j = 0; j < 3 && i + j < 32; ++j) {
				printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
			}
			printk("\n\r");
		}
		panic_g(
			"handle_other: unknown trap happens from S-mode:\n"
			"$sstatus: 0x%lx, $stval: 0x%lx, $scause: 0x%lx,\n"
			"$sepc: 0x%lx, sbadaddr: 0x%lx (should equals $stval)\n",
			regs->sstatus, stval, scause,
			regs->sepc, regs->sbadaddr
		);
	}
	else
	{	/* From U-mode */
		printk("**Exception 0x%lx happens at 0x%lx: 0x%lx\n",
			scause, regs->sepc, stval);
		do_exit();
	}
}


void handle_pagefault(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
	uint64_t lpte;
	PTE* ppte;
	pcb_t * ccpu = cur_cpu();

	if (ccpu->pid < NCPU)
		panic_g("handle_pagefault: kernel page fault: 0x%lx, $scause is 0x%lx",
			stval, scause);

	lpte = va2pte(stval, ccpu->pgdir_kva);
	ppte = (PTE*)(lpte & ~7UL);
	lpte &= 7UL;

	//if (lpte != 0UL)
		/*
		 * If `stval` is in user address space, large page found should
		 * cause panic; otherwise it is in kernel address space,
		 * kernel page fault should also cause panic.
		 * NOTE: comments above are wrong. If user process accesses a wrong
		 * UVA, `lpte != 0UL` can happen, which just means that the looking
		 * up of page table stops at L2 or L1 page table. 
		 */
	if ((r_sstatus() & SR_SPP) != 0UL && stval >= KVA_MIN)
		/* Kernel page fault at S-mode should not happen up to now */
		panic_g("handle_pagefault: L%lu page fault of proc %d: 0x%lx",
			lpte, ccpu->pid, stval);
#if DEBUG_EN != 0
	if ((r_sstatus() & SR_SPP) != 0UL && cur_cpu()->pid >= NCPU)
		writelog("Page fault of proc %d is caused from S-mode", cur_cpu()->pid);
#endif

	if (stval < User_sp && stval >= User_sp - USTACK_NPG * NORMAL_PAGE_SIZE)
	{
		unsigned pgidx =
			(stval - (User_sp - USTACK_NPG * NORMAL_PAGE_SIZE)) >> NORMAL_PAGE_SHIFT;
		if (*ppte == 0UL)
		{	/* Page hasn't been allocated */
			/* Alloc page for stack */
			if (ccpu->user_stack[pgidx] != 0UL)
				panic_g("handle_pagefault: proc %d user_stack[%u] is 0x%lx "
					"but PTE at 0x%lx is 0UL",
					ccpu->pid, pgidx, ccpu->user_stack[pgidx], (uintptr_t)ppte);
			ccpu->user_stack[pgidx] =
				alloc_page_helper(stval, (uintptr_t)(ccpu->pgdir_kva), ccpu->pid);
		}
		else
		{	/* Page is swapped to disk (V is 0) or A or D is 0 */
			/* Swap the page from disk or set A, D */
			if (get_attribute(*ppte, _PAGE_PRESENT) != 0L)
				set_attribute(ppte, _PAGE_ACCESSED | _PAGE_DIRTY);
			else
				swap_from_disk(ppte, stval);
		}
	}
	else if (stval >= ccpu->seg_start && stval < ccpu->seg_end)
	{
		if (*ppte == 0UL)
		{	/* Page hasn't been allocated */
			/* Alloc page for segment */
			alloc_page_helper(stval, (uintptr_t)(ccpu->pgdir_kva), ccpu->pid);
		}
		else
		{	/* Page is swapped to disk (V is 0) or A or D is 0 */
			/* Swap the page from disk or set A, D */
			if (get_attribute(*ppte, _PAGE_PRESENT) != 0L)
				set_attribute(ppte, _PAGE_ACCESSED | _PAGE_DIRTY);
			else
			{
				swap_from_disk(ppte, stval);
				local_flush_icache_all();
			}
		}
	}
	else
	{
		printk("**Segment fault: 0x%lx", stval);
		/*
		 * NOTE: suppose that a user process passes an invalid UVA
		 * to a syscall, and the UVA causes page fault. In this situation
		 * the current trap is taken from S-mode (SPP of $sstatus is 1).
		 * Can you ensure that no error will happen if you just call
		 * `do_exit()` and then call `do_scheduler()` to switch to another process?
		 */
		do_exit();
	}
}