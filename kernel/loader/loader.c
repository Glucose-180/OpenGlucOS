#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <os/sched.h>
#include <type.h>
#include <os/glucose.h>
#include <os/malloc-g.h>

const uint32_t App_addr = TASK_MEM_BASE,	/* App1 address */
	App_size = TASK_SIZE;	/* App2 addr - App1 addr */

uint64_t load_task_img(const char *taskname, PTE* pgdir_kva, pid_t pid)
{
	/**
	* TODO:
	* 1. [p1-task3] load task from image via task id, and return its entrypoint
	* 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
	*/
	unsigned int i;
	uint64_t vaddr, entry;
	uint32_t fsize, msize, uoffset;
	uint32_t taski_start_sector, taski_end_sector,
		taski_sectors;
	/* Used to store user program loaded from image temporarily */
	int8_t *ut_buf, *pg_kva = NULL;

	for (i = 0U; i < tasknum; ++i)
	{	/* Loading via name */
		if (strcmp(taskname, taskinfo[i].name) == 0)
			break;
	}
	if (i >= tasknum)
		return 0UL;	/* Not found */

	vaddr = taskinfo[i].v_addr;
	entry = taskinfo[i].v_entr;
	fsize = taskinfo[i].f_size;
	msize = taskinfo[i].m_size;

	if ((vaddr & (PAGE_SIZE - 1U)) != 0)
		/* vaddr is not 4 KiB aligned */
		return 0U;

	taski_start_sector = lbytes2sectors(taskinfo[i].offset);
	taski_end_sector = lbytes2sectors(taskinfo[i].offset + taskinfo[i].f_size);
	taski_sectors = taski_end_sector - taski_start_sector + 1U;

	if (taski_sectors > 64U)
#if DEBUG_EN != 0
		panic_g("load_task_img: %s is too large (%u sectors)",
		taskname, taski_sectors);
#else
		return 0UL;
#endif
	
	if ((ut_buf = kmalloc_g(taski_sectors * SECTOR_SIZE)) == NULL)
		return 0UL;

	/*
	 * Should argument mem_address be physical address?
	 * This is to be considered...
	 */
	bios_sd_read(kva2pa((uintptr_t)ut_buf), taski_sectors, taski_start_sector);

	uoffset = taskinfo[i].offset - taski_start_sector * SECTOR_SIZE;

	/* Allocate physical pages and copy user program in them */
	for (i = 0U; i < fsize; ++i)
	{
		//if (i % PAGE_SIZE == 0U)
		if ((i & (PAGE_SIZE - 1U)) == 0)	/* & is faster than % */
			pg_kva = (int8_t*)alloc_page_helper(vaddr + i, (uintptr_t)pgdir_kva, pid);
		pg_kva[i & (PAGE_SIZE - 1U)] = ut_buf[uoffset + i];
	}

	/* Allocate physical pages without filling it, such as for .bss section */
	if ((i & (PAGE_SIZE - 1U)) != 0)
		i = (i & ~(PAGE_SIZE - 1U)) + PAGE_SIZE;
	for (; i < msize; i += PAGE_SIZE)
		pg_kva = (int8_t*)alloc_page_helper(vaddr + i, (uintptr_t)pgdir_kva, pid);

	__sync_synchronize();

	kfree_g(ut_buf);
    return entry;
}
