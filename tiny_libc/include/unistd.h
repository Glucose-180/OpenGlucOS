#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <stdint.h>

void sys_sleep(uint32_t time);
void sys_yield(void);
void sys_write(char *buff);
int sys_bios_getchar(void);
void sys_move_cursor(int x, int y);
void sys_reflush(void);
long sys_get_timebase(void);
long sys_get_tick(void);
int sys_mutex_init(int key);
int sys_mutex_acquire(int mutex_idx);
int sys_mutex_release(int mutex_idx);

long sys_thread_create(void *(*func)(), long arg);
void sys_thread_yield(void);
int sys_thread_kill(int const T);

/************************************************************/
/* Do not touch this comment. Reserved for future projects. */
/************************************************************/

#endif
