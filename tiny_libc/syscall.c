#include <syscall.h>
#include <stdint.h>
#include <kernel.h>
#include <unistd.h>

/* NOTE: size of long is 8 bytes. */

static const long Ignore = 0L;

static long invoke_syscall(long sysno, long arg0, long arg1, long arg2,
						   long arg3, long arg4)
{
	/* TODO: [p2-task3] implement invoke_syscall via inline assembly */
	//asm volatile("nop");
	long rt;

	__asm__ volatile
	(
		"mv		a7, %1\n\t"
		"mv		a0, %2\n\t"
		"mv		a1, %3\n\t"
		"mv		a2, %4\n\t"
		"mv		a3, %5\n\t"
		"mv		a4, %6\n\t"
		"ecall\n\t"
		"mv		%0, a0"
		: "=r" (rt)
		: "r"(sysno), "r"(arg0), "r"(arg1),
		"r"(arg2), "r"(arg3), "r"(arg4)
		: "a0", "a1", "a2", "a3", "a4", "a5", "a7"
	);

	return rt;
}

void sys_yield(void)
{
	/* TODO: [p2-task1] call call_jmptab to implement sys_yield */
	//call_jmptab(YIELD, Ignore, Ignore, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_yield */
	invoke_syscall(SYS_yield, Ignore, Ignore, Ignore, Ignore, Ignore);
}

void sys_move_cursor(int x, int y)
{
	/* TODO: [p2-task1] call call_jmptab to implement sys_move_cursor */
	//call_jmptab(MOVE_CURSOR, x, y, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_move_cursor */
	invoke_syscall(SYS_move_cursor, x, y, Ignore, Ignore, Ignore);
}

void sys_write(char *buff)
{
	/* TODO: [p2-task1] call call_jmptab to implement sys_write */
	//call_jmptab(WRITE, (long)buff, Ignore, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_write */
	invoke_syscall(SYS_write, (long)buff, Ignore, Ignore, Ignore, Ignore);
}

int sys_bios_getchar(void)
{
	return invoke_syscall(SYS_bios_getchar,
		Ignore, Ignore, Ignore, Ignore, Ignore);
}

void sys_reflush(void)
{
	/* TODO: [p2-task1] call call_jmptab to implement sys_reflush */
	//call_jmptab(REFLUSH, Ignore, Ignore, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_reflush */
	invoke_syscall(SYS_reflush, Ignore, Ignore, Ignore, Ignore, Ignore);
}

int sys_mutex_init(int key)
{
	/* TODO: [p2-task2] call call_jmptab to implement sys_mutex_init */
	//return call_jmptab(MUTEX_INIT, key, Ignore, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_init */
	return invoke_syscall(SYS_lock_init, key, Ignore, Ignore, Ignore, Ignore);
}

int sys_mutex_acquire(int mutex_idx)
{
	/* TODO: [p2-task2] call call_jmptab to implement sys_mutex_acquire */
	//return call_jmptab(MUTEX_ACQ, mutex_idx, Ignore, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_acquire */
	return invoke_syscall(SYS_lock_acquire, mutex_idx, Ignore, Ignore, Ignore, Ignore);
}

int sys_mutex_release(int mutex_idx)
{
	/* TODO: [p2-task2] call call_jmptab to implement sys_mutex_release */
	//return call_jmptab(MUTEX_RELEASE, mutex_idx, Ignore, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_release */
	return invoke_syscall(SYS_lock_release, mutex_idx, Ignore, Ignore, Ignore, Ignore);
}

long sys_get_timebase(void)
{
	/* TODO: [p2-task3] call invoke_syscall to implement sys_get_timebase */
	return invoke_syscall(SYS_get_timebase, Ignore, Ignore, Ignore, Ignore, Ignore);
}

long sys_get_tick(void)
{
	/* TODO: [p2-task3] call invoke_syscall to implement sys_get_tick */
	return invoke_syscall(SYS_get_tick, Ignore, Ignore, Ignore, Ignore, Ignore);
}

void sys_sleep(uint32_t time)
{
	/* TODO: [p2-task3] call invoke_syscall to implement sys_sleep */
	invoke_syscall(SYS_sleep, (long)time, Ignore, Ignore, Ignore, Ignore);
}

long sys_thread_create(void *(*func)(), long arg)
{
	return invoke_syscall(SYS_thread_create, (long)func, arg, Ignore, Ignore, Ignore);
}

void sys_thread_yield(void)
{
	invoke_syscall(SYS_thread_yield, Ignore, Ignore, Ignore, Ignore, Ignore);
}

/************************************************************/
/* Do not touch this comment. Reserved for future projects. */
/************************************************************/