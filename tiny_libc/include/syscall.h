#ifndef __SYSCALL_H__
#define __SYSCALL_H__

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

#define SYS_net_send 70
#define SYS_net_recv 71
#define SYS_net_send_array 72
#define SYS_net_recv_stream 73

#define SYS_mkfs 80
#define SYS_fsinfo 81
#define SYS_open 82
#define SYS_close 83
#define SYS_changedir 84
#define SYS_read 85
#define SYS_write 86
#define SYS_lseek 87
#define SYS_getpath 88
#define SYS_readdir 89
#define SYS_mkdir 90
#define SYS_rm 91
#define SYS_hlink 92

#define SYS_kprint_avail_table 95

#endif