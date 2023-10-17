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

extern void ret_from_exception();

// Task info array
task_info_t taskinfo[TASK_MAXNUM];
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
	jmptab[REFLUSH]			= (long (*)())screen_reflush;
}

static void init_taskinfo(void)
{
	// TODO: [p1-task4] Init 'tasks' array via reading app-info sector
	// NOTE: You need to get some related arguments from bootblock first
	uint32_t //imagesize = *(uint32_t *)(BOOTLOADER_ADDR + 256),	/* unit: sector */
		taskinfo_offset = *(uint32_t *)(BOOTLOADER_ADDR + 264);
	tasknum = *(uint32_t *)(BOOTLOADER_ADDR + 268);
	uint32_t taskinfo_size = tasknum * sizeof(task_info_t),
		taskinfo_start_sector, taskinfo_end_sector,
		taskinfo_sectors;

	if (tasknum == 0U)
		return;
	taskinfo_start_sector = lbytes2sectors(taskinfo_offset);
	taskinfo_end_sector = lbytes2sectors(taskinfo_offset + taskinfo_size - 1U);
	taskinfo_sectors = taskinfo_end_sector - taskinfo_start_sector + 1U;
	if (tasknum > TASK_MAXNUM || taskinfo_sectors > TASKINFO_SECTORS_MAX)
		bios_putstr("**Warning: taskinfo is too big\n\r");

	/* Load taskinfo into the space of bootloader */
	bios_sd_read(BOOTLOADER_ADDR, taskinfo_sectors, taskinfo_start_sector);

	memcpy((uint8_t *)taskinfo,	(uint8_t *)(uint64_t)
		(BOOTLOADER_ADDR + (taskinfo_offset - taskinfo_start_sector * SECTOR_SIZE)),
		taskinfo_size);
}

/* NOTE: static function "init_pcb_stack" has been moved to sched.c */

static void init_pcb(void)
{
	/* TODO: [p2-task1] load needed tasks and init their corresponding PCB */
	/* TODO: [p2-task1] remember to initialize 'current_running' */
	pcb_t *temp;

	ready_queue = NULL;
	ready_queue = add_node_to_tail(ready_queue, &current_running);
	if (ready_queue == NULL)
		panic_g("init_pcb: Failed to init ready_queue");
	/*
	 * I'm not sure whether "*current_running = pid0_pcb;" will
	 * change its member "next" or not. So I use "temp" to save it.
	 */
	temp = current_running->next;
	*current_running = pid0_pcb;
	current_running->next = temp;
}

static void init_syscall(void)
{
	// TODO: [p2-task3] initialize system call table.
	int i;

	for (i = 0; i < NUM_SYSCALLS; ++i)
		syscall[i] = (long (*)())invalid_syscall;
	syscall[SYSCALL_SLEEP] = (long (*)())do_sleep;
	syscall[SYSCALL_YIELD] = (long (*)())do_scheduler;
	syscall[SYSCALL_WRITE] = (long (*)())screen_write;
	syscall[SYSCALL_BIOS_GETCHAR] = (long (*)())bios_getchar;
	syscall[SYSCALL_CURSOR] = (long (*)())screen_move_cursor;
	syscall[SYSCALL_REFLUSH] = (long (*)())screen_reflush;
	syscall[SYSCALL_GET_TIMEBASE] = (long(*)())get_time_base;
	syscall[SYSCALL_GET_TICK] = (long (*)())get_ticks;
	syscall[SYSCALL_LOCK_INIT] = (long (*)())do_mutex_lock_init;
	syscall[SYSCALL_LOCK_ACQ] = (long (*)())do_mutex_lock_acquire;
	syscall[SYSCALL_LOCK_RELEASE] = (long (*)())do_mutex_lock_release;
}
/************************************************************/

int main(void)
{
	// Init jump table provided by kernel and bios(ΦωΦ)
	init_jmptab();

	// Init task information (〃'▽'〃)
	init_taskinfo();

	// Init memory alloc
	malloc_init();
	
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


	// TODO: [p2-task4] Setup timer interrupt and enable all interrupt globally
	// NOTE: The function of sstatus.sie is different from sie's
	



	// Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
	while (1)
	{
		// If you do non-preemptive scheduling, it's used to surrender control
		//do_scheduler();

		char **cmds, flag_success = 0;
		while (flag_success == 0)
		{
			cmds = split(getcmd(), '&');
			while (*cmds != NULL)
			{
				trim(*cmds);
				if (strcmp(*cmds, "exit") == 0)
					goto loc_wfi;
				else if (create_proc(*cmds) == INVALID_PID)
					printk("Failed to start %s\n", *cmds);
				else
					flag_success = 1;
				++cmds;
			}
		}
		screen_clear();
		while (1)
			do_scheduler();

		// If you do preemptive scheduling, they're used to enable CSR_SIE and wfi
		// enable_preempt();
		// asm volatile("wfi");
	}


	// Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
loc_wfi:
	printk("logout\n");
	set_preempt();	// ONLY for TEST!!!
	while (1)
	{
		asm volatile("wfi");
	}

	return 0;
}
