#include <atomic.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>
#include <os/kernel.h>

void smp_init()
{
	/* TODO: P3-TASK3 multicore*/
}

void wakeup_other_hart()
{
	/* TODO: P3-TASK3 multicore*/
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