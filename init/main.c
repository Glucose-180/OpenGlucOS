#include <common.h>
#include <asm.h>
#include <asm/unistd.h>
#include <os/loader.h>
#include <os/irq.h>
#include <os/sched.h>
#include <os/pcb-list-g.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <os/task.h>
#include <os/string.h>
#include <os/loader.h>
#include <os/glucose.h>
#include <os/mm.h>
#include <os/malloc-g.h>
#include <os/time.h>
#include <sys/syscall.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <type.h>
#include <csr.h>
#include <os/smp.h>

extern void ret_from_exception();

// Task info array
task_info_t taskinfo[UTASK_MAX];
unsigned int tasknum;


static void init_jmptab(void)
{
	volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

	jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
	jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
	jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
	jmptab[SD_READ]         = (long (*)())sd_read;
	jmptab[SD_WRITE]        = (long (*)())sd_write;
	jmptab[QEMU_LOGGING]    = (long (*)())qemu_logging;
	jmptab[SET_TIMER]       = (long (*)())set_timer;
	jmptab[READ_FDT]        = (long (*)())read_fdt;
	jmptab[MOVE_CURSOR]     = (long (*)())screen_move_cursor;
	jmptab[PRINT]           = (long (*)())printk;
	jmptab[YIELD]           = (long (*)())do_scheduler;
	jmptab[MUTEX_INIT]      = (long (*)())do_mutex_lock_init;
	jmptab[MUTEX_ACQ]       = (long (*)())do_mutex_lock_acquire;
	jmptab[MUTEX_RELEASE]   = (long (*)())do_mutex_lock_release;

	// TODO: [p2-task1] (S-core) initialize system call table.
	jmptab[WRITE]			= (long (*)())screen_write;
	jmptab[CLEAR]			= (long (*)())screen_clear;
	jmptab[REFLUSH]			= (long (*)())screen_reflush;

	/*jmptab[EXEC]			= (long (*)())do_exec;
	jmptab[EXIT]			= (long (*)())do_exit;*/
	jmptab[KILL]			= (long (*)())do_kill;
	/*jmptab[WAITPID]			= (long (*)())do_waitpid;*/
	jmptab[PS]				= (long (*)())do_process_show;
	/*jmptab[GETPID]			= (long (*)())do_getpid;
	jmptab[BARRIER_INIT]	= (long (*)())do_barrier_init;
	jmptab[BARRIER_WAIT]	= (long (*)())do_barrier_wait;
	jmptab[BARRIER_DESTROY]	= (long (*)())do_barrier_destroy;*/
}

static void init_taskinfo(void)
{
	// TODO: [p1-task4] Init 'tasks' array via reading app-info sector
	// NOTE: You need to get some related arguments from bootblock first
	uint32_t //imagesize = *(uint32_t *)(BOOTLOADER_ADDR + 256),	/* unit: sector */
		taskinfo_offset = *(uint32_t *)(BOOTLOADER_VADDR + 264);
	tasknum = *(uint32_t *)(BOOTLOADER_VADDR + 268);
	uint32_t taskinfo_size = tasknum * sizeof(task_info_t),
		taskinfo_start_sector, taskinfo_end_sector,
		taskinfo_sectors;
	
	/*
	 * A location to store taskinfo temporarily.
	 */
	const ptr_t Taskinfo_buffer = 0x52510000U;

	if (tasknum == 0U)
		return;
	taskinfo_start_sector = lbytes2sectors(taskinfo_offset);
	taskinfo_end_sector = lbytes2sectors(taskinfo_offset + taskinfo_size - 1U);
	taskinfo_sectors = taskinfo_end_sector - taskinfo_start_sector + 1U;
	if (tasknum > UTASK_MAX || taskinfo_sectors > TASKINFO_SECTORS_MAX)
		bios_putstr("**Warning: taskinfo is too big\n\r");

	/* Load taskinfo into the space of bootloader */
	bios_sd_read(Taskinfo_buffer, taskinfo_sectors, taskinfo_start_sector);

	memcpy((uint8_t*)taskinfo,
		(uint8_t *)pa2kva(
			(Taskinfo_buffer + (taskinfo_offset - taskinfo_start_sector * SECTOR_SIZE))
		), taskinfo_size);
}

/* NOTE: static function "init_pcb_stack" has been moved to sched.c */

static void init_pcb(void)
{
	/* TODO: [p2-task1] load needed tasks and init their corresponding PCB */
	/* TODO: [p2-task1] remember to initialize 'current_running' */
	//pcb_t *temp;

	ready_queue = NULL;
	//ready_queue = lpcb_add_node_to_tail(ready_queue, &current_running[0], &ready_queue);
	ready_queue = lpcb_insert_node(ready_queue, &pid0_pcb, NULL, &ready_queue);
	current_running[0] = &pid0_pcb;
#if NCPU == 2
	//ready_queue = lpcb_add_node_to_tail(ready_queue, &current_running[1], &ready_queue);
	ready_queue = lpcb_insert_node(ready_queue, &pid1_pcb, NULL, &ready_queue);
	current_running[1] = &pid1_pcb;
#endif
	/*
	 * NOTE: current_running->phead is ensured in definition of pid0_pcb
	 */
#if NCPU == 2
	if (ready_queue == NULL || ready_queue->next == ready_queue)
#else
	if (ready_queue == NULL)
#endif
		panic_g("init_pcb: Failed to init ready_queue");
	/*
	 * I'm not sure whether "*current_running = pid0_pcb;" will
	 * change its member "next" or not. So I use "temp" to save it.
	 *//*
	temp = current_running[0]->next;
	*current_running[0] = pid0_pcb;
	current_running[0]->next = temp;
#if NCPU == 2
	temp = current_running[1]->next;
	*current_running[1] = pid1_pcb;
	current_running[1]->next = temp;
#endif */
	if (pcb_table_add(current_running[0]) < 0 ||
#if NCPU == 2
		pcb_table_add(current_running[1]) < 0)
#else
		0)
#endif
		panic_g("init_pcb: Failed to add 0 to pcb_table");
}

static void init_syscall(void)
{
	// TODO: [p2-task3] initialize system call table.
	int i;

	for (i = 0; i < NUM_SYSCALLS; ++i)
		syscall[i] = (long (*)())invalid_syscall;

	/* Process and scheduler */ {
		syscall[SYS_exec] = (long (*)())do_exec;
		syscall[SYS_kill] = (long (*)())do_kill;
		syscall[SYS_exit] = (long (*)())do_exit;
		syscall[SYS_waitpid] = (long (*)())do_waitpid;
		syscall[SYS_getpid] = (long (*)())do_getpid;
		syscall[SYS_sleep] = (long (*)())do_sleep;
		syscall[SYS_yield] = (long (*)())do_scheduler;
		syscall[SYS_ps] = (long (*)())do_process_show;
		syscall[SYS_taskset] = (long (*)())do_taskset;
	}
	/* Screen, clock and IO */ {
		syscall[SYS_write] = (long (*)())screen_write;
		syscall[SYS_bios_getchar] = (long (*)())bios_getchar;
		syscall[SYS_move_cursor] = (long (*)())screen_move_cursor;
		syscall[SYS_reflush] = (long (*)())screen_reflush;
		syscall[SYS_clear] = (long (*)())screen_clear;
		syscall[SYS_rclear] = (long (*)())screen_rclear;
		syscall[SYS_set_cylim] = (long (*)())screen_set_cylim;
		syscall[SYS_ulog] = (long (*)())do_ulog;
		syscall[SYS_kprint_avail_table] = (long (*)())kprint_avail_table;
		syscall[SYS_get_timebase] = (long(*)())get_time_base;
		syscall[SYS_get_tick] = (long (*)())get_ticks;
	}
	/* Sync: mutex lock, semaphore and barrier */ {
		syscall[SYS_mlock_init] = (long (*)())do_mutex_lock_init;
		syscall[SYS_mlock_acquire] = (long (*)())do_mutex_lock_acquire;
		syscall[SYS_mlock_release] = (long (*)())do_mutex_lock_release;
		syscall[SYS_sema_init] = (long (*)())do_semaphore_init;
		syscall[SYS_sema_up] = (long (*)())do_semaphore_up;
		syscall[SYS_sema_down] = (long (*)())do_semaphore_down;
		syscall[SYS_sema_destroy] = (long (*)())do_semaphore_destroy;
		syscall[SYS_barr_init] = (long (*)())do_barrier_init;
		syscall[SYS_barr_wait] = (long (*)())do_barrier_wait;
		syscall[SYS_barr_destroy] = (long (*)())do_barrier_destroy;
	}
	/* Communication: mailbox */ {
		syscall[SYS_mbox_open] = (long (*)())do_mbox_open;
		syscall[SYS_mbox_close] = (long (*)())do_mbox_close;
		syscall[SYS_mbox_send] = (long (*)())do_mbox_send;
		syscall[SYS_mbox_recv] = (long (*)())do_mbox_recv;
	}
	/* Memory management */ {
		syscall[SYS_sbrk] = (long (*)())do_sbrk;
	}
}

/*
 * Revoke the temporary mapping from
 * 0x50000000 to 0x51000000.
 */
static void revoke_temp_mapping(void)
{
	uintptr_t va = 0x51000000UL;
	va &= VA_MASK;
	uint64_t vpn2 =
		va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
	((PTE*)PGDIR_VA)[vpn2] = 0UL;
	local_flush_tlb_all();
}

/************************************************************/
/*
 * a0, a1 are arguments passed from boot_kernel() and _start.
 * Now, a0 is CPU ID, and a1 is `npages_used` in boot.c.
 */
#pragma GCC diagnostic ignored "-Wmain"
int main(reg_t a0, reg_t a1)
{
	pid_t pid;
#ifndef TERMINAL_BEGIN
#define TERMINAL_BEGIN "17"
#endif
#ifndef TERMINAL_END
#define TERMINAL_END "24"
#endif

	char *argv[] = {"glush", TERMINAL_BEGIN, TERMINAL_END,
#if DEBUG_EN != 0
		"1",	/* Autolog for glush */
#else
		"0",
#endif
		NULL};

	// Init jump table provided by kernel and bios(ΦωΦ)
	init_jmptab();

	// Init task information (〃'▽'〃)
	init_taskinfo();

	// Init memory management
	init_mm();
	
	// Init Process Control Blocks |•'-'•) ✧
	init_pcb();
	printk("> [INIT] PCB initialization succeeded.\n");

	// Init screen (QAQ)
	init_screen();
	printk("> [INIT] SCREEN initialization succeeded.\n");

	// Read CPU frequency (｡•ᴗ-)_
	time_base = bios_read_fdt(TIMEBASE);
	time_max_sec = UINT64_MAX / time_base;

	// Init lock mechanism o(´^｀)o
	init_locks();
	printk("> [INIT] Lock mechanism initialization succeeded.\n");

	// Init interrupt (^_^)
	init_exception();
	printk("> [INIT] Interrupt processing initialization succeeded.\n");

	// Init system call table (0_0)
	init_syscall();
	printk("> [INIT] System call initialized successfully.\n");
#if DEBUG_EN != 0
	writelog("GlucOS, boot! I am CPU %lu.", get_current_cpu_id());
	printk("\n> [INFO] Debug mode is enabled.\n");
	printk("> [INFO] Multithreading: %d, Timer_interval_ms: %d, NCPU: %d.\n",
		MULTITHREADING, TIMER_INTERVAL_MS, NCPU);
	writelog("Multithreading: %d, Timer_interval_ms: %d.",
		MULTITHREADING, TIMER_INTERVAL_MS);
#endif

#if NCPU == 2
	smp_init();
	wakeup_other_hart();
	local_flush_tlb_all();
#else
	if (a1 != 3UL)
		panic_g("main: npages_used from boot.c is %lu", a1);
	revoke_temp_mapping();
#endif
	/*
	 * Delay 3 s to keep information on the screen
	 * and wait for another CPU to start.
	 * NOTE: CPU0 cannot start a user process before
	 * the other CPU enters kernel, because the temporary
	 * page table (the 3rd page table) will be used to
	 * start the first process.
	 */
	latency(3U);

	/*
	 * Lock kernel to protect do_exec(glush)
	 * from being affected by another CPU
	 */
	lock_kernel();
	/* Clear screen and start glush */
	screen_clear();

	if ((pid = do_exec("glush", -1, argv)) == INVALID_PID)
	{
		panic_g("main: Failed to start glush");
		/* Ignore these s**t mountain left over from history
		char **cmds, flag_success = 0;

		printk("Failed to start glush\n");
		while (flag_success == 0)
		{
			cmds = split(getcmd(), '&');
			while (*cmds != NULL)
			{
				trim(*cmds);
				if (strcmp(*cmds, "exit") == 0)
					goto loc_wfi;
				else if (strcmp(*cmds, "glush") == 0)
				{
					if (do_exec("glush", -1, argv) == INVALID_PID)
						printk("Failed to start %s\n", *cmds);
					else
						flag_success = 1;
				}
				else if (create_proc(*cmds) == INVALID_PID)
					printk("Failed to start %s\n", *cmds);
				else
					flag_success = 1;
				++cmds;
			}
		}
		screen_clear();
		*/
	}
	if (do_taskset(0, NULL, pid, ~0U) != pid)
		panic_g("main: Failed to set cpu_mask of glush %d", pid);
	unlock_kernel();

	set_preempt();
#if DEBUG_EN != 0
	writelog("CPU 0: Timer interrupt is enabled");
#endif

	while (1)
		// Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
		__asm__ volatile("wfi");
	return 0;
}

/* For secondary CPU */
int main_s(reg_t a0, reg_t a1)
{
	pcb_t *p0;

#if NCPU != 2
	panic_g("main_s: Only %d CPU but main_s() is entered", NCPU);
#endif
	if (a1 != 3UL)
		panic_g("main: npages_used from boot.c is %lu", a1);
	revoke_temp_mapping();

	init_exception_s();

	/* Print below the string CPU 0 has printed */
	p0 = pcb_search(0);
	screen_move_cursor(0, p0->cursor_y + 1);

#if DEBUG_EN != 0
	writelog("I am CPU %lu and has started!", get_current_cpu_id());
#endif
	printk("> [INIT] I am CPU %lu and has started!\n", get_current_cpu_id());

	set_preempt();
#if DEBUG_EN != 0
	writelog("CPU %lu: Timer interrupt is enabled", get_current_cpu_id());
#endif
	while (1)
		asm volatile("wfi");
	return 0;
}