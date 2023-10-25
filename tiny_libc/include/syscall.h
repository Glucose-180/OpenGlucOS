#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define SYS_sleep 2
#define SYS_yield 7
#define SYS_write 20
#define SYS_bios_getchar 21
#define SYS_move_cursor 22
#define SYS_reflush 23
#define SYS_get_timebase 30
#define SYS_get_tick 31
#define SYS_lock_init 40
#define SYS_lock_acquire 41
#define SYS_lock_release 42

#define SYS_thread_create 50
#define SYS_thread_yield 51

#endif