#ifndef __SCHED_THREAD_H__
#define __SCHED_THREAD_H__
#if MULTITHREADING != 0

#include <type.h>
#define INVALID_TID INVALID_PID
#define TID_MAX 5
typedef pcb_t tcb_t;

/*
 * NOTE: these three functions are not defined in `sched-thread.c`,
 * but is supplemented in other source files to support multithreading.
 * Move there declarations here to manage them by `MULTITHREADING`.
 */
tcb_t *pcb_search_tcb(tid_t tid);
pcb_t *lpcb_search_node_tcb(pcb_t * const Head, pid_t const Pid, tid_t const Tid);
unsigned int pcb_search_all(pid_t pid, pcb_t **farr);

pthread_t do_thread_create(uintptr_t entry, uintptr_t arg, uintptr_t sp, uintptr_t exit);
int do_thread_wait(pthread_t tid);
void do_thread_exit(void);
void thread_kill(tcb_t *p);

#endif
#endif