#include <atomic.h>
#include <csr.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <os/irq.h>
#include <riscv.h>

void send_ipi(const unsigned long *hart_mask);

void smp_init()
{
	/* TODO: P3-TASK3 multicore*/
	reg_t sstatus;

	setup_exception();
	sstatus = r_sstatus();
	sstatus |= SR_SPIE;
	sstatus &= ~SR_SPP;
	w_sstatus(sstatus);
}

void wakeup_other_hart()
{
	/* TODO: P3-TASK3 multicore*/
	unsigned long mask = ~0L << 1;
	send_ipi(&mask);
}

void lock_kernel()
{
	/* TODO: P3-TASK3 multicore*/
}

void unlock_kernel()
{
	/* TODO: P3-TASK3 multicore*/
}

pcb_t *cur_cpu(void)
{
	if (get_current_cpu_id() != 0U)
		return current_running[1];
	else
		return current_running[0];
}