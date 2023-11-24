#include <os/mm.h>
#include <os/kernel.h>
#include <os/glucose.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/task.h>

#if DEBUG_EN != 0

volatile uint8_t spg_charmap[NPSWAP];
unsigned int swap_start;
volatile unsigned int clock_pt;
volatile unsigned int spg_ffree;
#else

static volatile uint8_t spg_charmap[NPSWAP];
/* Start sector ID of swap partition */
static unsigned int swap_start;

/*
 * `clock_pt`: the clock pointer for clock algorithm.
 * It equals the index of the oldest page frame.
 */
volatile static unsigned int clock_pt;
volatile static unsigned int spg_ffree;
#endif

/* Return the sector index of swap page with index `spg_idx` */
static inline unsigned int get_sec_idx(unsigned int spg_idx)
{
	return swap_start +
		(NORMAL_PAGE_SIZE / SECTOR_SIZE) * spg_idx;
}

void init_swap()
{
	uint32_t taskinfo_offset = /* unit: sector */
		*(uint32_t *)(BOOTLOADER_VADDR + TINFO_OFFSET),
		i;
	uint32_t tasknum = *(uint32_t *)(BOOTLOADER_VADDR + TNUM_OFFSET);
	uint32_t taskinfo_size = tasknum * sizeof(task_info_t);
	/*
	 * Use the sector after the last sector that contains
	 * task info as the first sector of swap partition.
	 */
	swap_start = lbytes2sectors(taskinfo_offset + taskinfo_size - 1U) + 1U;
	clock_pt = spg_ffree = 0U;
	if (CMAP_FREE == 0xffU && (NPSWAP & 0x7U) == 0U)
		for (i = 0U; i < NPSWAP / 8U; ++i)
			((int64_t*)spg_charmap)[i] = (int64_t)-1;
	else
		for (i = 0U; i < NPSWAP; ++i)
			spg_charmap[i] = CMAP_FREE;
}

/*
 * alloc_swap_page: allocate a swap page on disk
 * for process with PID `pid` and returns its index.
 * If no free page exists, `UINT32_MAX` is returned.
 */
static unsigned int alloc_swap_page(pid_t pid)
{
	unsigned int rt;

	if ((uint8_t)pid == CMAP_FREE)
		panic_g("alloc_swap_page: pid is invalid: 0x%x", (int)CMAP_FREE);
	if (spg_ffree >= NPSWAP)
		return UINT32_MAX;
	rt = spg_ffree;
	spg_charmap[spg_ffree] = (uint8_t)pid;
	/* Update `spg_free` */
	for (; spg_ffree < NPSWAP; ++spg_ffree)
		if (spg_charmap[spg_ffree] == CMAP_FREE)
			break;
	return rt;
}

/*
 * free_swap_page: free a swap page on disk
 * with index `spg_idx`.
 */
void free_swap_page(unsigned int spg_idx)
{
	if (spg_idx >= NPSWAP)
		panic_g("free_swap_page: invalid index %u", spg_idx);
	if (spg_charmap[spg_idx] == CMAP_FREE)
		panic_g("free_swap_page: page %u is already free", spg_idx);
	spg_charmap[spg_idx] = CMAP_FREE;
	spg_ffree = (spg_idx < spg_ffree ? spg_idx : spg_ffree);
}

/*
 * swap_to_disk: Use clock algorithm to select a page frame,
 * write it to disk, record its index (on disk) to its PTE,
 * clear V bit of the PTE and free the page frame.
 * Return the index of the swapped page frame.
 */
unsigned int swap_to_disk()
{
	PTE *ppte;
	uint64_t lpte;
	pid_t pid;
	pcb_t *p;
	unsigned int spidx;

	while (1)
	{
		p = pcb_search(pid = pg_charmap[clock_pt]);
		/* PANIC will also happen if a free page is found. */
		if (p == NULL)
			panic_g("swap_to_disk: invalid PID %d for page frame %u",
				pid, clock_pt);
		/*
		 * Don't swap the page of a process running on another CPU!
		 * Because the TLB of it can not be flushed in time.
		 */
		if (p->status != TASK_RUNNING || p->pid == cur_cpu()->pid)
		{
			lpte = va2pte(pg_uva[clock_pt], p->pgdir_kva);
			ppte = (PTE*)(lpte & ~7UL);
			lpte &= 7UL;
			if (lpte != 0UL || get_attribute(*ppte, _PAGE_PRESENT) == 0L)
				panic_g("swap_to_disk: invalid PTE 0x%lx at 0x%lx", *ppte, (uintptr_t)ppte);
			if (get_attribute(*ppte, _PAGE_ACCESSED) != 0U)
				*ppte &= ~_PAGE_ACCESSED;	/* Clear its A bit */
			else
				break;
		}
		/* Move the clock pointer by 1 step */
		if (++clock_pt >= NPF)
			clock_pt = 0U;
	}
	spidx = alloc_swap_page(pid);
	if (spidx >= NPSWAP)
		/*
		 * Just handle this situation by panic.
		 * In future design, we can terminate a process to save the OS.
		 */
		panic_g("swap_to_disk: No free page on disk for %d", pid);
	/* Write the page to disk */
	bios_sd_write((unsigned int)kva2pa(Pg_base + (clock_pt << NORMAL_PAGE_SHIFT)),
		NORMAL_PAGE_SIZE / SECTOR_SIZE, get_sec_idx(spidx));
	*ppte &= ~_PAGE_PRESENT;
	/* Record the index of page on disk in PTE */
	set_pfn(ppte, (uint64_t)spidx);
	/*
	 * Don't forget to flush TLB! If that doesn't work, try:
	__sync_synchronize();
	local_flush_tlb_all();
	 */
	__sync_synchronize();
	local_flush_tlb_page(pg_uva[clock_pt]);
	if (cur_cpu()->pid == p->pid)
		/* Flush I-Cache if a page of the current process is swapped */
		local_flush_icache_all();
	/* 0 PTE will be considered as not allocated */
	if (*ppte == 0UL)
		panic_g("swap_to_disk: PTE at 0x%lx of %d becomes 0",
			(uintptr_t)ppte, pid);
#if DEBUG_EN != 0
	writelog("Page %u of proc %d is swapped to disk at %u",
		clock_pt, pid, spidx);
#endif
	spidx = clock_pt;	/* Hold the value temporarily */
	if (++clock_pt >= NPF)
		clock_pt = 0U;
	return spidx;
}

/*
 * swap_from_disk: Read the page index on disk of `*ppte`,
 * load the page from disk, free the page on disk and set
 * A, V bits and PFN field of `*ppte`. The new page frame
 * will be set as occupied by the current process, and the
 * UVA is specified by `uva`. `uva` is also used to flush TLB.
 * Return the KVA of new page frame or 0 if the PTE is error.
 */
uintptr_t swap_from_disk(PTE *ppte, uintptr_t uva)
{
	unsigned spidx;
	uintptr_t pg_kva;

	spidx = get_pfn(*ppte);
	if (get_attribute(*ppte, _PAGE_PRESENT) != 0L || spidx >= NPSWAP)
		return 0UL;
	pg_kva = alloc_page(1U, cur_cpu()->pid, uva);
	if (pg_kva == 0UL)
		panic_g("swap_from_disk: alloc_page() returns 0");
	bios_sd_read((unsigned int)kva2pa(pg_kva),
		NORMAL_PAGE_SIZE / SECTOR_SIZE, get_sec_idx(spidx));
	free_swap_page(spidx);
	set_attribute(ppte, _PAGE_PRESENT | _PAGE_ACCESSED);
	set_pfn(ppte, kva2pa(pg_kva) >> NORMAL_PAGE_SHIFT);
	//local_flush_tlb_page(uva);
#if DEBUG_EN != 0
	writelog("Proc %d caused page %u swapped from disk to PA 0x%lx for UVA 0x%lx",
		cur_cpu()->pid, spidx, kva2pa(pg_kva), uva);
#endif
	return pg_kva;
}