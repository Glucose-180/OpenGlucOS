#ifndef __ASM_UNISTD_H__
#define __ASM_UNISTD_H__

#define SYS_exec 0
#define SYS_exit 1
#define SYS_sleep 2
#define SYS_kill 3
#define SYS_waitpid 4
#define SYS_ps 5
#define SYS_getpid 6
#define SYS_yield 7
#define SYS_taskset 8
#define SYS_fork 9

#define SYS_thread_create 10
#define SYS_thread_wait 11
#define SYS_thread_exit 12

#define SYS_screen_write 20
#define SYS_bios_getchar 21
#define SYS_move_cursor 22
#define SYS_reflush 23
#define SYS_clear 24
#define SYS_rclear 25
#define SYS_set_cylim 26
#define SYS_ulog 27

#define SYS_get_timebase 30
#define SYS_get_tick 31

#define SYS_mlock_init 40
#define SYS_mlock_acquire 41
#define SYS_mlock_release 42
#define SYS_show_task 43
#define SYS_barr_init 44
#define SYS_barr_wait 45
#define SYS_barr_destroy 46
#define SYS_cond_init 47
#define SYS_cond wait 48
#define SYS_cond_signal 49
#define SYS_cond_broadcast 50
#define SYS_cond_destroy 51
#define SYS_mbox_open 52
#define SYS_mbox_close 53
#define SYS_mbox_send 54
#define SYS_mbox_recv 55

#define SYS_sema_init 56
#define SYS_sema_up 57
#define SYS_sema_down 58
#define SYS_sema_destroy 59

#define SYS_sbrk 60
#define SYS_shm_get 61
#define SYS_shm_dt 62

#define SYS_kprint_avail_table 90
/*
#define SYSCALL_EXEC 0
#define SYSCALL_EXIT 1
#define SYSCALL_SLEEP 2
#define SYSCALL_KILL 3
#define SYSCALL_WAITPID 4
#define SYSCALL_PS 5
#define SYSCALL_GETPID 6
#define SYSCALL_YIELD 7
#define SYSCALL_WRITE 20
#define SYSCALL_READCH 21
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23
#define SYSCALL_CLEAR 24
#define SYSCALL_GET_TIMEBASE 30
#define SYSCALL_GET_TICK 31
#define SYSCALL_LOCK_INIT 40
#define SYSCALL_LOCK_ACQ 41
#define SYSCALL_LOCK_RELEASE 42
#define SYSCALL_SHOW_TASK 43
#define SYSCALL_BARR_INIT 44
#define SYSCALL_BARR_WAIT 45
#define SYSCALL_BARR_DESTROY 46
#define SYSCALL_COND_INIT 47
#define SYSCALL_COND_WAIT 48
#define SYSCALL_COND_SIGNAL 49
#define SYSCALL_COND_BROADCAST 50
#define SYSCALL_COND_DESTROY 51
#define SYSCALL_MBOX_OPEN 52
#define SYSCALL_MBOX_CLOSE 53
#define SYSCALL_MBOX_SEND 54
#define SYSCALL_MBOX_RECV 55
#define SYSCALL_SHM_GET 56
#define SYSCALL_SHM_DT 57
*/

#endif