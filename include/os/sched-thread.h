#ifndef __SCHED_THREAD_H__
#define __SCHED_THREAD_H__
#if MULTITHREADING != 0

#include <type.h>
#define INVALID_TID INVALID_PID
#define TID_MAX 5
typedef pcb_t tcb_t;

tcb_t *pcb_search_tcb(tid_t tid);


#endif
#endif