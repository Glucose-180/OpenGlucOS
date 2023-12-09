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
#include <plic.h>
#include <e1000.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];

static void handle_irq_timer(regs_context_t *regs, uint64_t stval, uint64_t scause);
static void handle_soft(regs_context_t *regs, uint64_t stval, uint64_t scause);
static void handle_irq_ext(regs_context_t *regs, uint64_t stval, uint64_t scause);
static void handle_pagefault(regs_context_t *regs, uint64_t stval, uint64_t scause);
static void handle_other(regs_context_t *regs, uint64_t stval, uint64_t scause);

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
		panic_g("proc %d is still running after killed",
			cur_cpu()->pid);
	}
#endif
	if ((int64_t)scause < 0)
	{	/* Interrupt */
		scause &= (((uint64_t)~0UL) >> 1);
		if (scause >= IRQC_COUNT)
			panic_g("exception code of "
				"Interrupt is error: 0x%lx", scause);
		irq_table[scause](regs, stval, scause);
	}
	else
	{
		exc_table[scause](regs, stval, scause);
	}
}

void init_exception()
{
	/* TODO: [p2-task3] initialize exc_table */
	/* NOTE: handle_syscall, handle_other, etc.*/
	int i;

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
	irq_table[IRQC_S_SOFT] = handle_soft;
	irq_table[IRQC_S_EXT] = handle_irq_ext;

	/*
	 * We only set SUM, SPIE and SIE of $sstatus.
	 * Write $sstatus directly to avoid uncertainty.
	 */
	w_sstatus(SR_SUM | SR_SPIE);
	setup_exception();
}

static void handle_irq_timer(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
	// TODO: [p2-task4] clock interrupt handler.
	// Note: use bios_set_timer to reset the timer and remember to reschedule
	set_preempt();	/* Reset timer */
	do_scheduler();
}

static void handle_irq_ext(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
	// TODO: [p5-task3] external interrupt handler.
	// Note: plic_claim and plic_complete will be helpful ...
	int32_t claim = (int32_t)plic_claim();
#if DEBUG_EN != 0
	if (claim == PLIC_E1000_PYNQ_IRQ || claim == PLIC_E1000_QEMU_IRQ)
#else
	if (claim == (PYNQ ? PLIC_E1000_PYNQ_IRQ : PLIC_E1000_QEMU_IRQ))
#endif
	{
		uint32_t e1000_icr = e1000_read_reg(e1000, E1000_ICR);
		if ((e1000_icr & E1000_ICR_TXQE) != 0U)
			handle_e1000_int_txqe();
		else if ((e1000_icr & E1000_ICR_RXDMT0) != 0U)
			handle_e1000_int_rxdmt0();
#if DEBUG_EN != 0
		else
			writelog("**Warning: unknown E1000 interrupt: %u", e1000_icr);
#endif
		/* Clear interrupt mask to disenable it */
		e1000_write_reg(e1000, E1000_IMC, e1000_icr);
	}

	plic_complete(claim);
#if DEBUG_EN != 0
	writelog("PLIC %d interrupt has completed.", claim);
#endif
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

static void handle_pagefault(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
	uint64_t lpte;
	PTE* ppte;
	static unsigned int pf_ymr = 0U;
	static uint64_t last_stval = 0UL;
	static pid_t last_pid = INVALID_PID;
	static unsigned int time, last_time = 0U;
	pcb_t * ccpu = cur_cpu();

	/*
	 * Record how many times has the same `stval` of the same process appeared
	 * in a certain time period. If it is too much, panic is necessary!
	 */
	time = get_timer();
	if (stval == last_stval && ccpu->pid == last_pid &&
#if DEBUG_EN != 0
		time - last_time <= 3U)
#else
		time - last_time <= 1U)
#endif
		++pf_ymr;
	else
	{
		pf_ymr = 1U;
		last_stval = stval;
		last_pid = ccpu->pid;
		last_time = time;
	}

	if (ccpu->pid < NCPU)
		panic_g("kernel page fault: 0x%lx, $scause is 0x%lx",
			stval, scause);

	lpte = va2pte(stval, ccpu->pgdir_kva, 0, 0);
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
		panic_g("L%lu page fault of proc %d: 0x%lx",
			lpte, ccpu->pid, stval);
#if DEBUG_EN != 0
	if ((r_sstatus() & SR_SPP) != 0UL && cur_cpu()->pid >= NCPU)
		writelog("Page fault of proc %d is caused from S-mode: "
			"$stval is 0x%lx, $scause is 0x%lx", cur_cpu()->pid, stval, scause);
#endif
	if (pf_ymr >= 3U)
		panic_g("$stval 0x%lx has appeared %u times consecutively:\n"
			"PID(TID) %d(%d), $scause 0x%lx, $sstatus 0x%lx, $sepc 0x%lx, PTE 0x%lx",
			stval, pf_ymr, ccpu->pid, ccpu->tid, scause, regs->sstatus, regs->sepc, *ppte);

	if ((stval < User_sp && stval >= User_sp - USTACK_NPG * NORMAL_PAGE_SIZE) ||
		(stval >= ccpu->seg_start && stval < ccpu->seg_end))
	{
		if (*ppte == 0UL)
			/* Page hasn't been allocated */
			alloc_page_helper(stval, (uintptr_t)(ccpu->pgdir_kva), ccpu->pid);
		else
		{	/* Page is swapped to disk (V is 0) or A or D is 0 */
			/* Swap the page from disk or set A, D */
			if (get_attribute(*ppte, _PAGE_PRESENT) != 0L)
			{
#if DEBUG_EN != 0
				if (get_attribute(*ppte, _PAGE_WRITE | _PAGE_ACCESSED | _PAGE_DIRTY)
					== (_PAGE_WRITE | _PAGE_ACCESSED | _PAGE_DIRTY) &&
					scause == EXC_STORE_PAGE_FAULT)
					writelog("0x%lx caused store page fault for a W, A, D page %u, PTE 0x%lx",
						stval, get_pgidx(*ppte), *ppte);
#endif
				if (get_attribute(*ppte, _PAGE_COW) != 0L &&
					scause == EXC_STORE_PAGE_FAULT)
				{	/* Copy-on-write! */
					uintptr_t pg_kva;
					unsigned int pgidx = get_pgidx(*ppte);
#if DEBUG_EN != 0
					writelog("Proc %d(%d) 0x%lx caused page %u copy-on-write",
						ccpu->pid, ccpu->tid, stval, pgidx);
#endif
					pg_kva = alloc_page(1U, ccpu->pid, stval);
					memcpy((uint8_t*)pg_kva, (uint8_t*)pa2kva(get_pa(*ppte)),
						1U << NORMAL_PAGE_SHIFT);
					set_pfn(ppte, kva2pa(pg_kva) >> NORMAL_PAGE_SHIFT);
					set_attribute(ppte, _PAGE_WRITE);
					clear_attribute(ppte, _PAGE_COW);
					if (pg_uva[pgidx] == 0UL || pg_uva[pgidx] > UPROC_MAX)
						panic_g("page %u is read only but is 0x%lx "
							"in pg_uva[], $stval 0x%lx, $sepc 0x%lx",
							pgidx, stval, pg_uva[pgidx], regs->sepc);
					if (--pg_uva[pgidx] == 0UL)
						free_page(Pg_base + (pgidx << NORMAL_PAGE_SHIFT));
					/*
					 * Flush I-Cache in case of a process modifies its
					 * .text section, or instructions.
					 */
					local_flush_icache_all();
				}
				/* GlucOS don't use D bit at all, so just set it. */
				set_attribute(ppte, _PAGE_ACCESSED | _PAGE_DIRTY);
				/*
				 * If store page fault happens but the page is not read only,
				 * it may be caused by multithreading. One thread caused copy-on-write
				 * but another thread running on another CPU has old TLB.
				 */
				local_flush_tlb_page(stval);
			}
			else
			{
				if (swap_from_disk(ppte, stval) == 0UL)
					panic_g("Failed to swap page (PTE 0x%lx) from disk:\n"
						"$scause 0x%lx, $sstatus 0x%lx, $sepc 0x%lx",
						*ppte, scause, regs->sstatus, regs->sepc);
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

static void handle_other(regs_context_t *regs, uint64_t stval, uint64_t scause)
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
			"Unknown trap happens from S-mode:\n"
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
