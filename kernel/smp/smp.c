#include <atomic.h>
#include <csr.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <os/irq.h>
#include <riscv.h>
#include <os/glucose.h>

static spin_lock_t global_kernel_lock;

void send_ipi(const unsigned long *hart_mask);

void smp_init()
{
	/* TODO: P3-TASK3 multicore*/
	global_kernel_lock.status = UNLOCKED;
}

void init_exception_s(void)
{
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
	while (__sync_lock_test_and_set(&(global_kernel_lock.status), LOCKED) == LOCKED)
		;
	__sync_synchronize();
}

void unlock_kernel()
{
	/* TODO: P3-TASK3 multicore*/
	if (global_kernel_lock.status == UNLOCKED)
		panic_g("unlock_kernel: CPU %lu: Global kernel lock is unlocked",
			get_current_cpu_id());
	__sync_synchronize();
	__sync_lock_release(&(global_kernel_lock.status));
}

pcb_t *cur_cpu(void)
{
#if NCPU == 2
	if (get_current_cpu_id() != 0U)
		return current_running[1];
	else
#endif
		return current_running[0];
}