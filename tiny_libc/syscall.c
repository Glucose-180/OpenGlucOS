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

unsigned int sys_screen_write(char *buff, unsigned int len)
{
	return invoke_syscall(SYS_screen_write, (long)buff, (long)len,
		Ignore, Ignore, Ignore);
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

void sys_set_cylim(int cylim_l, int cylim_h)
{
	invoke_syscall(SYS_set_cylim, (long)cylim_l, (long)cylim_h,
		Ignore, Ignore, Ignore);
}

int sys_ulog(const char *str)
{
	return invoke_syscall(SYS_ulog, (long)str, Ignore, Ignore, Ignore, Ignore);
}

int sys_mutex_init(int key)
{
	/* TODO: [p2-task2] call call_jmptab to implement sys_mutex_init */
	//return call_jmptab(MUTEX_INIT, key, Ignore, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_init */
	return invoke_syscall(SYS_mlock_init, key, Ignore, Ignore, Ignore, Ignore);
}

int sys_mutex_acquire(int mutex_idx)
{
	/* TODO: [p2-task2] call call_jmptab to implement sys_mutex_acquire */
	//return call_jmptab(MUTEX_ACQ, mutex_idx, Ignore, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_acquire */
	return invoke_syscall(SYS_mlock_acquire, mutex_idx, Ignore, Ignore, Ignore, Ignore);
}

int sys_mutex_release(int mutex_idx)
{
	/* TODO: [p2-task2] call call_jmptab to implement sys_mutex_release */
	//return call_jmptab(MUTEX_RELEASE, mutex_idx, Ignore, Ignore, Ignore, Ignore);
	/* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_release */
	return invoke_syscall(SYS_mlock_release, mutex_idx, Ignore, Ignore, Ignore, Ignore);
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

pthread_t sys_thread_create(void (*entry)(void *), void *arg, ptr_t sp, void (*exit)(void))
{
	return invoke_syscall(SYS_thread_create, (long)entry, (long)arg, (long)sp,
		(long)exit, Ignore);
}

int sys_thread_wait(pthread_t tid)
{
	return invoke_syscall(SYS_thread_wait, (long)tid, Ignore, Ignore, Ignore, Ignore);
}

void sys_thread_exit(void)
{
	invoke_syscall(SYS_thread_exit, Ignore, Ignore, Ignore, Ignore, Ignore);
}

/************************************************************/
#ifdef S_CORE
pid_t  sys_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
	/* TODO: [p3-task1] call invoke_syscall to implement sys_exec for S_CORE */
}    
#else
pid_t  sys_exec(char *name, int argc, char **argv)
{
	/* TODO: [p3-task1] call invoke_syscall to implement sys_exec */
	return invoke_syscall(SYS_exec, (long)name, (long)argc, (long)argv,
		Ignore, Ignore);
}
#endif

void sys_exit(void)
{
	/* TODO: [p3-task1] call invoke_syscall to implement sys_exit */
	invoke_syscall(SYS_exit, Ignore, Ignore, Ignore, Ignore, Ignore);
}

int  sys_kill(pid_t pid)
{
	/* TODO: [p3-task1] call invoke_syscall to implement sys_kill */
	return invoke_syscall(SYS_kill, (long)pid, Ignore, Ignore, Ignore, Ignore);
}

pid_t sys_waitpid(pid_t pid)
{
	/* TODO: [p3-task1] call invoke_syscall to implement sys_waitpid */
	return invoke_syscall(SYS_waitpid, (long)pid, Ignore, Ignore,
		Ignore, Ignore);
}


int sys_ps(void)
{
	/* TODO: [p3-task1] call invoke_syscall to implement sys_ps */
	return invoke_syscall(SYS_ps, Ignore, Ignore, Ignore, Ignore, Ignore);
}

void sys_clear(void)
{
	invoke_syscall(SYS_clear, Ignore, Ignore, Ignore, Ignore, Ignore);
}

void sys_rclear(int ybegin, int yend)
{
	invoke_syscall(SYS_rclear, ybegin, yend, Ignore, Ignore, Ignore);
}

pid_t sys_getpid()
{
	/* TODO: [p3-task1] call invoke_syscall to implement sys_getpid */
	return invoke_syscall(SYS_getpid, Ignore, Ignore, Ignore ,Ignore, Ignore);
}

/*int  sys_getchar(void)
{
	// TODO: [p3-task1] call invoke_syscall to implement sys_getchar
}*/
pid_t sys_taskset(int create, char *name, pid_t pid, unsigned int cpu_mask)
{
	return invoke_syscall(SYS_taskset, create, (long)name, (long)pid, (long)cpu_mask, Ignore);
}

pid_t sys_fork(void)
{
	return invoke_syscall(SYS_fork, Ignore, Ignore, Ignore, Ignore, Ignore);
}

int sys_barrier_init(int key, int goal)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_barrier_init */
	return invoke_syscall(SYS_barr_init, key, goal, Ignore, Ignore, Ignore);
}

int sys_barrier_wait(int bidx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_barrie_wait */
	return invoke_syscall(SYS_barr_wait, bidx, Ignore, Ignore, Ignore, Ignore);
}

int sys_barrier_destroy(int bidx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_barrie_destory */
	return invoke_syscall(SYS_barr_destroy, bidx, Ignore, Ignore, Ignore, Ignore);
}

int sys_condition_init(int key)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_condition_init */
	return 0;
}

void sys_condition_wait(int cond_idx, int mutex_idx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_condition_wait */
}

void sys_condition_signal(int cond_idx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_condition_signal */
}

void sys_condition_broadcast(int cond_idx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_condition_broadcast */
}

void sys_condition_destroy(int cond_idx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_condition_destroy */
}

int sys_semaphore_init(int key, int value)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_semaphore_init */
	return invoke_syscall(SYS_sema_init, key, value, Ignore, Ignore, Ignore);
}

int sys_semaphore_up(int sidx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_semaphore_up */
	return invoke_syscall(SYS_sema_up, sidx, Ignore, Ignore, Ignore, Ignore);
}

int sys_semaphore_down(int sidx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_semaphore_down */
	return invoke_syscall(SYS_sema_down, sidx, Ignore, Ignore, Ignore, Ignore);
}

int sys_semaphore_destroy(int sidx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_semaphore_destroy */
	return invoke_syscall(SYS_sema_destroy, sidx, Ignore, Ignore, Ignore, Ignore);
}

int sys_mbox_open(const char * name)
{
	return invoke_syscall(SYS_mbox_open, (long)name, Ignore, Ignore, Ignore, Ignore);
}

int sys_mbox_close(int midx)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_close */
	return invoke_syscall(SYS_mbox_close, midx, Ignore, Ignore, Ignore, Ignore);
}

int sys_mbox_send(int midx, const void *msg, unsigned msg_length)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_send */
	return invoke_syscall(SYS_mbox_send, midx, (long)msg, msg_length, Ignore, Ignore);
}

int sys_mbox_recv(int midx, void *msg, unsigned msg_length)
{
	/* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_recv */
	return invoke_syscall(SYS_mbox_recv, midx, (long)msg, msg_length, Ignore, Ignore);
}
/************************************************************/

void sys_kprint_avail_table(void)
{
	invoke_syscall(SYS_kprint_avail_table, Ignore, Ignore,
		Ignore, Ignore, Ignore);
}

void *sys_sbrk(uint64_t size)
{
	return (void *)invoke_syscall(SYS_sbrk, (long)size, Ignore, Ignore, Ignore, Ignore);
}

void* sys_shmpageget(int key)
{
    /* TODO: [p4-task4] call invoke_syscall to implement sys_shmpageget */
	return (void*)invoke_syscall(SYS_shm_get, key, Ignore, Ignore, Ignore, Ignore);
}

int sys_shmpagedt(void *addr)
{
    /* TODO: [p4-task4] call invoke_syscall to implement sys_shmpagedt */
	return invoke_syscall(SYS_shm_dt, (long)addr, Ignore, Ignore, Ignore, Ignore);
}

int sys_net_send(const void *txpacket, int length)
{
    /* TODO: [p5-task1] call invoke_syscall to implement sys_net_send */
    return invoke_syscall(SYS_net_send, (long)txpacket, (long)length,
		Ignore, Ignore, Ignore);
}

int sys_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens)
{
    /* TODO: [p5-task2] call invoke_syscall to implement sys_net_recv */
	return invoke_syscall(SYS_net_recv, (long)rxbuffer, (long)pkt_num,
		(long)pkt_lens, Ignore, Ignore);
}

int sys_net_send_array(const void **vpkt, int *vlen, int npkt)
{
	return invoke_syscall(SYS_net_send_array, (long)vpkt, (long)vlen,
		npkt, Ignore, Ignore);
}

unsigned int sys_net_recv_stream(uint8_t *buf, int len)
{
	return invoke_syscall(SYS_net_recv_stream, (long)buf, len,
		Ignore, Ignore, Ignore);
}

int sys_mkfs(int force)
{
	return invoke_syscall(SYS_mkfs, force, Ignore,
		Ignore, Ignore, Ignore);
}

int sys_fsinfo(void)
{
	return invoke_syscall(SYS_fsinfo, Ignore, Ignore, Ignore,
		Ignore, Ignore);
}

long sys_open(const char* name, long flags)
{
	return invoke_syscall(SYS_open, (long)name, flags, Ignore,
		Ignore, Ignore);
}

long sys_close(long fd)
{
	return invoke_syscall(SYS_close, fd, Ignore, Ignore,
		Ignore, Ignore);
}

long sys_read(long fd, char *buf, long n)
{
	return invoke_syscall(SYS_read, fd, (long)buf, n,
		Ignore, Ignore);
}

long sys_write(long fd, const char *buf, long n)
{
	return invoke_syscall(SYS_write, fd, (long)buf, n,
		Ignore, Ignore);
}
/************************************************************/
