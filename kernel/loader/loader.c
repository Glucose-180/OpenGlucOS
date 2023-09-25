#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
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
	uint64_t entry;
	uint32_t taski_start_sector, taski_end_sector,
		taski_sectors;

	for (i = 0U; i < tasknum; ++i)
	{	/* Loading via name */
		if (strcmp(taskname, taskinfo[i].name) == 0)
			break;
	}
	if (i >= tasknum)
		return 0UL;	/* Not found */

	entry = taskinfo[i].entr;
	taski_start_sector = lbytes2sectors(taskinfo[i].offs);
	taski_end_sector = lbytes2sectors(taskinfo[i].offs + taskinfo[i].size);
	taski_sectors = taski_end_sector - taski_start_sector + 1U;

	bios_sd_read(entry, taski_sectors, taski_start_sector);

	memcpy((uint8_t *)entry, (uint8_t *)(uint64_t)
		(entry + (taskinfo[i].offs - taski_start_sector * SECTOR_SIZE)),
		taskinfo[i].size);

    return entry;
}
