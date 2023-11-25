#include <pthread.h>

/* TODO:[P4-task4] pthread_create/wait */
/*
 * `pthread_create`: create a thread starting from `start_routine`
 * with argument `arg`, and write its TID in `thread` if it's not NULL.
 * Return 0 on success or 1 on error.
 */
int pthread_create(pthread_t *thread,
				   void (*start_routine)(void*),
				   void *arg)
{
	/* TODO: [p4-task4] implement pthread_create */
	uintptr_t sp = (uintptr_t)sys_sbrk(TSTACK_SIZE);
	pthread_t tid;
	if (sp == 0UL)
		return 1;
	sp += TSTACK_SIZE;
	tid = sys_thread_create(start_routine, arg, sp, sys_thread_exit);
	if (tid == INVALID_TID)
		return 1;
	if (thread != (void*)0)
		*thread = tid;
	return 0;
}

/*
 * `pthread_join`: wait for a thread whose TID is `thread`
 * to exit.
 * Return 0 on success, 1 on invalid `tid` or 2 if no such thread.
 */
int pthread_join(pthread_t thread)
{
	/* TODO: [p4-task4] implement pthread_join */
	return sys_thread_wait(thread);
}
