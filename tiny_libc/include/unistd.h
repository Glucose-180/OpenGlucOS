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

#ifndef MULTITHREADING
/*
 * Support multithreading or not.
 * It appears in many header files!
 */
#define MULTITHREADING 1
#endif

typedef pid_t pthread_t;

void sys_sleep(uint32_t time);
void sys_yield(void);
unsigned int sys_screen_write(char *buff, unsigned int len);
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

pthread_t sys_thread_create(void (*entry)(void *), void *arg, ptr_t sp, void (*exit)(void));
int sys_thread_wait(pthread_t tid);
void sys_thread_exit(void);


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
pid_t sys_fork(void);

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

void *sys_sbrk(uint64_t size);
/* TODO: [P4-task5] shmpageget/dt */
/* shmpageget/dt */
void* sys_shmpageget(int key);
int sys_shmpagedt(void *addr);

/* net send and recv */
int sys_net_send(const void *txpacket, int length);
int sys_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens);
int sys_net_send_array(const void **vpkt, int *vlen, int npkt);
unsigned int sys_net_recv_stream(uint8_t *buf, int len);
/************************************************************/

int sys_semaphore_init(int key, int value);
int sys_semaphore_up(int sidx);
int sys_semaphore_down(int sidx);
int sys_semaphore_destroy(int sidx);

/* Open file: Read and write */
#define O_RDWR (1L << 0)

int sys_mkfs(int force);
int sys_fsinfo(void);
long sys_open(const char* name, long flags);
long sys_close(long fd);
long sys_changedir(const char *path);
long sys_read(long fd, char *buf, long n);
long sys_write(long fd, const char *buf, long n);
long sys_lseek(long fd, long offset, long whence);
long sys_getpath(char *path);
unsigned int sys_readdir(const char *path, int det);
long sys_mkdir(const char *path);
long sys_rm(const char *stpath);
// TODO: sys_rm, ...

void sys_kprint_avail_table(void);

#endif
