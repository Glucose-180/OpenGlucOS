#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <stdint.h>
typedef int32_t pid_t;
#define INVALID_PID (-1)
#define INVALID_TID (-1)

#ifndef OS_NAME
#define OS_NAME "GlucOS"
#endif
#ifndef USER_NAME
#define USER_NAME "glucose180"
#endif

void sys_sleep(uint32_t time);
void sys_yield(void);
void sys_write(char *buff);
int sys_bios_getchar(void);
void sys_move_cursor(int x, int y);
void sys_reflush(void);
void sys_set_cylim(int cylim_l, int cylim_h);
int sys_ulog(const char *str);

long sys_get_timebase(void);
long sys_get_tick(void);
int sys_mutex_init(int key);
int sys_mutex_acquire(int mutex_idx);
int sys_mutex_release(int mutex_idx);

long sys_thread_create(void *(*func)(), long arg);
void sys_thread_yield(void);
int sys_thread_kill(int const T);


/************************************************************/
/* TODO: [P3 task1] ps, getchar */
// void sys_ps(void);
//int  sys_getchar(void);

int sys_ps(void);
void sys_clear(void);
void sys_rclear(int ybegin, int yend);

/* TODO: [P3 task1] exec, exit, kill waitpid */
// S-core
// pid_t  sys_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2);
// A/C-core
pid_t  sys_exec(char *name, int argc, char **argv);

void sys_exit(void);
int  sys_kill(pid_t pid);
pid_t sys_waitpid(pid_t pid);
pid_t sys_getpid();
pid_t sys_taskset(int create, char *name, pid_t pid, unsigned int cpu_mask);

/* TODO: [P3 task2] barrier */ 
int sys_barrier_init(int key, int goal);
int sys_barrier_wait(int bidx);
int sys_barrier_destroy(int bidx);

/* TODO: [P3 task2] condition */ 
int sys_condition_init(int key);
void sys_condition_wait(int cond_idx, int mutex_idx);
void sys_condition_signal(int cond_idx);
void sys_condition_broadcast(int cond_idx);
void sys_condition_destroy(int cond_idx);

/* TODO: [P3 task2] mailbox */ 
int sys_mbox_open(const char * name);
int sys_mbox_close(int midx);
int sys_mbox_send(int midx, const void *msg, unsigned msg_length);
int sys_mbox_recv(int midx, void *msg, unsigned msg_length);
/************************************************************/

int sys_semaphore_init(int key, int value);
int sys_semaphore_up(int sidx);
int sys_semaphore_down(int sidx);
int sys_semaphore_destroy(int sidx);

void sys_kprint_avail_table(void);

#endif
