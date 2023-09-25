#include <common.h>
#include <asm.h>
#include <os/kernel.h>
#include <os/task.h>
#include <os/string.h>
#include <os/loader.h>
#include <os/glucose.h>
#include <type.h>

#define VERSION_BUF 50

int version = 2; // version must between 0 and 9
char buf[VERSION_BUF];

// Task info array
task_info_t taskinfo[TASK_MAXNUM];
unsigned int tasknum;

static int bss_check(void)
{
	for (int i = 0; i < VERSION_BUF; ++i)
	{
		if (buf[i] != 0)
		{
			return 0;
		}
	}
	return 1;
}

static void init_jmptab(void)
{
	volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

	jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
	jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
	jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
	jmptab[SD_READ]		 = (long (*)())sd_read;
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

/************************************************************/
/* Do not touch this comment. Reserved for future projects. */
/************************************************************/

int main(void)
{
	// Check whether .bss section is set to zero
	int check = bss_check();

	// Init jump table provided by kernel and bios(ΦωΦ)
	init_jmptab();

	// Init task information (〃'▽'〃)
	init_taskinfo();

	// Output 'Hello OS!', bss check result and OS version
	char output_str[] = "bss check: _ version: _\n\r";
	char output_val[2] = {0};
	int i, output_val_pos = 0;

	output_val[0] = check ? 't' : 'f';
	output_val[1] = version + '0';
	for (i = 0; i < sizeof(output_str); ++i)
	{
		buf[i] = output_str[i];
		if (buf[i] == '_')
		{
			buf[i] = output_val[output_val_pos++];
		}
	}

	bios_putstr(buf);
	bios_putstr("============================\n\r");
	bios_putstr("      Hello " OS_NAME "!\n\r");
	bios_putstr("============================\n\r");


	// TODO: Load tasks by either task id [p1-task3] or task name [p1-task4],
	//   and then execute them.
	while (1)
	{
		int rt;
		int (*ftask)(void);	/* Pointer to function of user app */
		char *cmd = getcmd();

		if ((ftask = (void *)load_task_img(cmd)) != NULL)
		{
				rt = ftask();
				if (rt == 0)
				{
					bios_putstr("Task \"");
					bios_putstr(cmd);
					bios_putstr("\" exited normally\n");
				}
				else
				{
					bios_putstr("Task \"");
					bios_putstr(cmd);
					bios_putstr("\" failed\n");
				}
		}
		else if (strcmp(cmd, "exit") == 0)
		{
			bios_putstr("logout\n");
			return 0;
		}

	}

	// Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
	while (1)
	{
		asm volatile("wfi");
	}

	return 0;
}
