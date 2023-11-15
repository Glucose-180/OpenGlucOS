#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <os/sched.h>
#include <type.h>
#include <os/glucose.h>

const uint32_t App_addr = TASK_MEM_BASE,	/* App1 address */
	App_size = TASK_SIZE;	/* App2 addr - App1 addr */

uint64_t load_task_img(const char *taskname)
{
	/**
	* TODO:
	* 1. [p1-task3] load task from image via task id, and return its entrypoint
	* 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
	*/
	unsigned int i;
	uint64_t vaddr, entry;
	uint32_t taski_start_sector, taski_end_sector,
		taski_sectors;

	for (i = 0U; i < tasknum; ++i)
	{	/* Loading via name */
		if (strcmp(taskname, taskinfo[i].name) == 0)
			break;
	}
	if (i >= tasknum)
		return 0UL;	/* Not found */

	vaddr = taskinfo[i].v_addr;
	entry = taskinfo[i].v_entr;

	/*
	 * Temporary patch for multiple CPU before virtual memory is implemented.
	 * Suppose that CPU0 is running a user process `add` in User mode,
	 * and CPU1 is calling do_exec() to create an `add` again,
	 * then illegal instruction might be detected as the memory storing
	 * instructions is changed.
	 */
	if (pcb_search_name(taskname) != NULL)
		return entry;

	taski_start_sector = lbytes2sectors(taskinfo[i].offset);
	taski_end_sector = lbytes2sectors(taskinfo[i].offset + taskinfo[i].f_size);
	taski_sectors = taski_end_sector - taski_start_sector + 1U;

	bios_sd_read(vaddr, taski_sectors, taski_start_sector);

	memcpy((uint8_t *)vaddr, (uint8_t *)(uint64_t)
		(vaddr + (taskinfo[i].offset - taski_start_sector * SECTOR_SIZE)),
		taskinfo[i].f_size);

    return entry;
}
