#include <os/mm.h>
#include <os/kernel.h>
#include <os/glucose.h>
#include <os/sched.h>

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
	/*
	 * Use the sector after the first sector that contains
	 * task info as the first sector of swap partition,
	 * because the first sector that contains task info
	 * also has other data.
	 */
	swap_start = lbytes2sectors(taskinfo_offset) + 1U;
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
	if (spg_ffree > NPSWAP)
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
static void free_swap_page(unsigned int spg_idx)
{
	if (spg_charmap[spg_idx] == CMAP_FREE)
		panic_g("free_swap_page: page %u is already free", spg_idx);
	spg_charmap[spg_idx] = CMAP_FREE;
	spg_ffree = (spg_idx < spg_ffree ? spg_idx : spg_ffree);
}

/*
 * swap_to_disk: Use clock algorithm to select a page frame,
 * write it to disk, record its index (on disk) to its PTE,
 * clear V bit of the PTE and free the page frame.
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
		if (p == NULL)
			panic_g("swap_to_disk: invalid PID %d for page frame %u",
				pid, clock_pt);
		lpte = va2pte(pg_uva[clock_pt], p->pgdir_kva);
		ppte = (PTE*)(lpte & ~7UL);
		lpte &= 7UL;
		if (lpte != 0UL || get_attribute(*ppte, _PAGE_PRESENT) == 0L)
			panic_g("swap_to_disk: invalid PTE 0x%lx at 0x%lx", *ppte, (uintptr_t)ppte);
		if (get_attribute(*ppte, _PAGE_ACCESSED) != 0U)
		{
			*ppte &= ~_PAGE_ACCESSED;	/* Clear its A bit */
			/* Move the clock pointer by 1 step */
			if (++clock_pt >= NPSWAP)
				clock_pt = 0U;
		}
		else
			break;
	}
	spidx = alloc_swap_page(pid);
	if (spidx > NPSWAP)
		/*
		 * Just handle this situation by panic.
		 * In future design, we can terminate a process to save the OS.
		 */
		panic_g("swap_to_disk: No free page on disk for %d", pid);
	/* Write the page to disk */
	bios_sd_write(kva2pa(Pg_base + (clock_pt << NORMAL_PAGE_SHIFT)),
		NORMAL_PAGE_SIZE / SECTOR_SIZE, get_sec_idx(spidx));
	*ppte &= ~_PAGE_PRESENT;
	/* Record the index of page on disk in PTE */
	set_pfn(ppte, spidx);
	/* 0 PTE will be considered as not allocated */
	if (*ppte == 0UL)
		panic_g("swap_to_disk: PTE at 0x%lx of %d becomes 0",
			(uintptr_t)ppte, pid);
	spidx = clock_pt;	/* Hold the value temporarily */
	if (++clock_pt >= NPSWAP)
		clock_pt = 0U;
	return spidx;
}

//TODO: swap_from_disk()