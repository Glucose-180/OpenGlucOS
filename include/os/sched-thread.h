#ifndef __SCHED_THREAD_H__
#define __SCHED_THREAD_H__
#if MULTITHREADING != 0

#include <type.h>
#define INVALID_TID INVALID_PID
#define TID_MAX 5
typedef pcb_t tcb_t;

tcb_t *pcb_search_tcb(tid_t tid);
unsigned int pcb_search_all(pid_t pid, pcb_t **farr);


void do_thread_exit(void);
void thread_kill(tcb_t *p);

#endif
#endif