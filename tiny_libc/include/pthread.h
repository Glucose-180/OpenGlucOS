#ifndef PTHREAD_H_
#define PTHREAD_H_
#include "unistd.h"

#define INVALID_TID (-1)

/* size of a thread's stack, 8 KiB */
#define TSTACK_SIZE (8U * 1024U)

/* TODO:[P4-task4] pthread_create/wait */
/*
 * `pthread_create`: create a thread starting from `start_routine`
 * with argument `arg`, and write its TID in `thread` if it's not NULL.
 * Return 0 on success or 1 on error.
 */
int pthread_create(pthread_t *thread,
                   void (*start_routine)(void*),
                   void *arg);

int pthread_join(pthread_t thread);


#endif