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
#include <os/smp.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];

static void handle_dasics(regs_context_t *regs, uint64_t stval, uint64_t scause);
static void handle_soft(regs_context_t *regs, uint64_t stval, uint64_t scause);

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
	// TODO: [p2-task3] & [p2-task4] interrupt handler.
	// call corresponding handler by the value of `scause`
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
		if (scause >= EXCC_COUNT)
		{
			if (scause >= EXCC_DASICS)
				handle_dasics(regs, stval, scause);
			else
				handle_other(regs, stval, scause);
		}
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
	char* reg_name[] = {
		"zero "," ra  "," sp  "," gp  "," tp  ",
		" t0  "," t1  "," t2  ","s0/fp"," s1  ",
		" a0  "," a1  "," a2  "," a3  "," a4  ",
		" a5  "," a6  "," a7  "," s2  "," s3  ",
		" s4  "," s5  "," s6  "," s7  "," s8  ",
		" s9  "," s10 "," s11 "," t3  "," t4  ",
		" t5  "," t6  "
	};
	for (int i = 0; i < 32; i += 3) {
		for (int j = 0; j < 3 && i + j < 32; ++j) {
			printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
		}
		printk("\n\r");
	}
	/*printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lu\n\r",
		   regs->sstatus, regs->sbadaddr, regs->scause);
	printk("sepc: 0x%lx\n\r", regs->sepc);
	printk("tval: 0x%lx cause: 0x%lx\n", stval, scause);*/
	//assert(0);
	panic_g(
		"handle_other: unknown trap happens:\n"
		"$sstatus: 0x%lx, $stval: 0x%lx, $scause: 0x%lx,\n"
		"$sepc: 0x%lx, sbadaddr: 0x%lx (should equals $stval)\n",
		regs->sstatus, stval, scause,
		regs->sepc, regs->sbadaddr
	);
}

static void handle_dasics(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
	printk("**Segmentation fault: 0x%lx", stval);
	do_exit();
}