#include <os/mm.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/glucose.h>
#include <os/malloc-g.h>

/*
 * Use "char map" (like bitmap) to manage page frames and page tables.
 * When `DEBUG_EN` is not zero, declear these variables as external
 * so that we can watch their values.
 */
#if DEBUG_EN != 0
const uintptr_t	Pgtb_base = PGDIR_VA;
volatile uint8_t pgtb_charmap[NPT];
/* index of first free page frame/table */
volatile unsigned int pg_ffree, pgtb_ffree;
#else
static const uintptr_t Pgtb_base = PGDIR_VA;
static volatile uint8_t pgtb_charmap[NPT];
/* index of first free page frame/table */
static volatile unsigned int pg_ffree, pgtb_ffree;
#endif

const uintptr_t Pg_base = ROUND(FREEMEM_KERNEL, PAGE_SIZE);
/*
 * pg_charmap[]: pg_charmap[i] being CMAP_FREE means that i-th page is free.
 * Otherwise, pg_charmap[i] means the PID of the process occupying it.
 */
volatile uint8_t pg_charmap[NPF];

/*
 * Record the UVA of a page frame if it is normal.
 * If the page frame is shared, pg_uva[#] is the
 * number of processes using it.
 */
volatile uintptr_t *pg_uva;

void init_mm(void)
{
	void malloc_init(void);
	void init_swap(void);
	unsigned int i;

	malloc_init();
	init_swap();

	pg_ffree = 0U;
	if (CMAP_FREE == 0xffu && (NPF & 0x7U) == 0U)
		for (i = 0U; i < NPF / 8U; ++i)
			/* use int64_t to speed up initialization */
			((int64_t*)pg_charmap)[i] = (int64_t)-1;
	else
		for (i = 0U; i < NPF; ++i)
			pg_charmap[i] = CMAP_FREE;
	
	pgtb_ffree = 2U;
	if (CMAP_FREE == 0xffu && (NPT & 0x7U) == 0U)
		for (i = 0U; i < NPT / 8U; ++i)
			((int64_t*)pgtb_charmap)[i] = (int64_t)-1;
	else
		for (i = 0U; i < NPT; ++i)
			pgtb_charmap[i] = CMAP_FREE;
	/* The first two page tables are used by kernel */
	pgtb_charmap[0] = pgtb_charmap[1] = 0U;
	pg_uva = kmalloc_g(sizeof(uintptr_t) * NPF);
	if (pg_uva == NULL)
		panic_g("Failed to alloc `pg_uva`");
}

/*
 * alloc_page: Allocate ONE page frame for process `pid`.
 * Return the KVA of it or PANIC!
 * `uva` is used to recorded the UVA for the page frame
 * so that the coresponding PTE is easy to find.
 */
uintptr_t alloc_page(unsigned int npages, pid_t pid, uintptr_t uva)
{
	uintptr_t rt;
	if (npages != 1U)
		panic_g("only supports 1 page up to now");
	if ((uint8_t)pid == CMAP_FREE)
		panic_g("pid is invalid: 0x%x", (int)CMAP_FREE);
	if (pg_ffree >= NPF)
	{	/* No free page */
		pg_ffree = swap_to_disk();
		/* A flag of swapped */
		npages = 0U;
	}
	rt = Pg_base + pg_ffree * NORMAL_PAGE_SIZE;
	pg_charmap[pg_ffree] = (uint8_t)pid;
	pg_uva[pg_ffree] = (uva >> NORMAL_PAGE_SHIFT) << NORMAL_PAGE_SHIFT;
	/* Update `pg_ffree` */
	if (npages != 0U)
	{	/* Use this flag to avoid unnecessary scanning */
		for (; pg_ffree < NPF; ++pg_ffree)
			if (pg_charmap[pg_ffree] == CMAP_FREE)
				break;
	}
	else
		pg_ffree = NPF;
	return rt;
}

void free_page(uintptr_t pg_kva)
{
	// TODO [P4-task1] (design you 'freePage' here if you need):
	unsigned int pg_idx;

	//pg_idx = (pg_kva - Pg_base) / PAGE_SIZE;
	pg_idx = (pg_kva - Pg_base) >> NORMAL_PAGE_SHIFT;
	if (pg_kva < Pg_base || (pg_kva & (PAGE_SIZE - 1UL)) != 0UL || pg_idx >= NPF)
		panic_g("invalid addr of page: 0x%lx", pg_kva);
	if (pg_charmap[pg_idx] == CMAP_FREE)
		panic_g("page 0x%lx is already free", pg_kva);
	pg_charmap[pg_idx] = CMAP_FREE;
	/* Update `pg_ffree` */
	pg_ffree = (pg_idx < pg_ffree ? pg_idx : pg_ffree);
}

/*
 * alloc_pagetable: virtual memory version of alloc_page()
 * in boot.c.
 * Allocate a page as a pagetable for the process with PID `pid`,
 * CLEAR it and return its KERNEL VIRTUAL ADDRESS.
 */
uintptr_t alloc_pagetable(pid_t pid)
{
	uintptr_t rt;

	if ((uint8_t)pid == CMAP_FREE)
		panic_g("pid is invalid: %d", pid);
	if (pgtb_ffree >= NPT)
		/* No free page table */
		panic_g("no free page table for proc %d", pid);
	rt = Pgtb_base + pgtb_ffree * NORMAL_PAGE_SIZE;
	clear_pgdir(rt);
	pgtb_charmap[pgtb_ffree] = (uint8_t)pid;
	/* Update `pgtb_ffree` */
	for (; pgtb_ffree < NPT; ++pgtb_ffree)
		if (pgtb_charmap[pgtb_ffree] == CMAP_FREE)
			break;
	return rt;
}

void free_pagetable(uintptr_t pgtb_kva)
{
	unsigned pgtb_idx;

	pgtb_idx = (pgtb_kva - Pgtb_base) >> NORMAL_PAGE_SHIFT;
	if (pgtb_kva < Pgtb_base || (pgtb_kva & (PAGE_SIZE - 1UL)) != 0UL ||
		pgtb_idx >= NPT)
		panic_g("invalid addr of page table: 0x%lx", pgtb_kva);
	if (pgtb_charmap[pgtb_idx] == 0U)
		panic_g("cannot free page table 0x%lx used by kernel", pgtb_kva);
	if (pgtb_charmap[pgtb_idx] == CMAP_FREE)
		panic_g("page table 0x%lx is already free", pgtb_kva);
	pgtb_charmap[pgtb_idx] = CMAP_FREE;
	/* Update `pgtb_free` */
	pgtb_ffree = (pgtb_idx < pgtb_ffree ? pgtb_idx : pgtb_ffree);
}

/*
 * free_pages_of_proc: When a process is terminated,
 * use this function to free pages and page tables occupied
 * by it. `pgdir` is the KVA of its L2 page table.
 * `pid` is for calling `shm_page_dt()`.
 * Return the number of page frames freed.
 */
unsigned int free_pages_of_proc(PTE* pgdir, pid_t pid)
{
	unsigned int i, j, k, f_ymr = 0U;
	PTE * pgdir_l1, * pgdir_l0;

	/* Scan L2 page table */
	for (i = 0U; i < (1U << (PPN_BITS - 1U)); ++i)
	{	/* pgdir[256] and higher point to kernel page tables! */
		if (get_attribute(pgdir[i], _PAGE_PRESENT) != 0L)
		{
			if (get_attribute(pgdir[i], _PAGE_XWR) != 0L)
				panic_g("L2 pgdir[%u] has a PTE 0x%lx that X, W, R are not all zero, "
				"pgdir is 0x%lx", i, (uint64_t)pgdir[i], (uint64_t)pgdir);
			pgdir_l1 = (PTE*)pa2kva(get_pa(pgdir[i]));
			/* Scan L1 page table */
			for (j = 0U; j < (1U << PPN_BITS); ++j)
			{
				if (get_attribute(pgdir_l1[j], _PAGE_PRESENT) != 0L)
				{
					if (get_attribute(pgdir_l1[j], _PAGE_XWR) != 0L)
						panic_g("pgdir_l1[%u] has a PTE 0x%lx that X, W, R are not all zero, "
						"pgdir_l1 is 0x%lx", j, (uint64_t)pgdir_l1[j], (uint64_t)pgdir_l1);
					pgdir_l0 = (PTE*)pa2kva(get_pa(pgdir_l1[j]));
					/* Scan L0 page table */
					for (k = 0U; k < (1U << PPN_BITS); ++k)
					{
						if (get_attribute(pgdir_l0[k], _PAGE_PRESENT) != 0L)
						{
							if (get_attribute(pgdir_l0[k], _PAGE_XWR) == 0L)
								panic_g("pgdir_l0[%u] has a PTE 0x%lx that X, W, R are all zero, "
								"pgdir_l0 is 0x%lx", k, (uint64_t)pgdir_l0[k], (uint64_t)pgdir_l0);
							if (get_attribute(pgdir_l0[k], _PAGE_SHARED) != 0L)
							{	/* Don't free it directly if it is shared */
								int drt = shm_page_dt(vpn2va(i, j, k), pid, pgdir);
								if (drt < 0)
									panic_g("Failed to free shared "
										"pages with UVA 0x%lx and PTE 0x%lx",
										vpn2va(i, j, k), pgdir_l0[k]);
								else if (drt > 0)
									++f_ymr;
							}
							else if (get_attribute(pgdir_l0[k], _PAGE_COW) != 0L)
							{	/* COW page */
								unsigned int pgidx = get_pgidx(pgdir_l0[k]);
								if (pg_uva[pgidx] == 0UL || pg_uva[pgidx] > UPROC_MAX)
									panic_g("page %u is COW "
										"but is 0x%lx in pg_uva[], 0x%lx of proc %d",
										pgidx, pg_uva[pgidx], vpn2va(i, j, k), pid);
								if (--pg_uva[pgidx] == 0UL)
								{
									free_page(pa2kva(get_pa(pgdir_l0[k])));
									++f_ymr;
								}
							}
							else
							{
								free_page(pa2kva(get_pa(pgdir_l0[k])));
								++f_ymr;
							}
						}
						else if (pgdir_l0[k] != 0U)
						{	/* PTE is not 0 but V is 0, so the page is on disk */
							free_swap_page(get_pfn(pgdir_l0[k]));
							++f_ymr;
						}
					}
					free_pagetable((uintptr_t)pgdir_l0);
				}
			}
			free_pagetable((uintptr_t)pgdir_l1);
		}
	}
	free_pagetable((uintptr_t)pgdir);
	return f_ymr;
}

/*
 * share_pgtable: mapping kernel virtual address into user page table.
 * Note that dest_pgdir and src_pgdir are KVA.
 */
void share_pgtable(PTE* dest_pgdir, PTE* src_pgdir)
{
	// TODO [P4-task1] share_pgtable:
	unsigned int i;
	/*
	 * i starts from 256 and ends at 511.
	 * The highest (38th) bit of kernel virtual address is 1,
	 * so VPN2 starts from 256.
	 */
	for (i = (1U << (PPN_BITS - 1U)); i < (1U << PPN_BITS); ++i)
		dest_pgdir[i] = src_pgdir[i];
}

/*
 * alloc_page_helper: allocate a physical page for `va`,
 * mapping it into `pgdir_kva`, and return the kernel virtual address of it.
 * `pid` is the PID of the process for which the page is used.
 * This function either succeeds, or causes panic.
 * NOTE: if the page corresponding with `va` is allocated (the PTE is not 0),
 * panic will happen!
 */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir_kva, pid_t pid)
{
	PTE* ppte;
	uint64_t lpte;

	lpte = va2pte(va, (PTE*)pgdir_kva, 1, pid);
	ppte = (PTE*)(lpte & ~7UL);
	lpte &= 7UL;

	if (lpte != 0U)
		panic_g("va2pte(0x%lx, 0x%lx, 1, %d) returns invalid address: 0x%lx",
			va, pgdir_kva, pid, (uint64_t)ppte + lpte);
	if (*ppte != 0UL)
		panic_g("VA 0x%lx is already mapped to PTE 0x%lx in 0x%lx of %d",
			va, *ppte, pgdir_kva, pid);
	lpte = alloc_page(1U, pid, va);	/* Reuse `lpte` */
	set_pfn(ppte, kva2pa(lpte) >> NORMAL_PAGE_SHIFT);
	set_attribute(ppte, _PAGE_VURWXAD);
	return lpte;
}

/*
 * va2pte: look up the page table at `pgdir_kva`
 * and find the PTE of virtual address `va`.
 * Return the KVA of the PTE ADDed by the level
 * of it. The lowest 2 bits:
 * 0b10: PTE of L2 page table;
 * 0b01: PTE of L1 page table;
 * 0b00: PTE of L0 page table.
 * NOTE: the V of PTE may not be 1, which means that
 * the page of `va` is not in main memory!
 * If `alloc` is 0: if a PTE with V 0 is found while looking up,
 * the looking process is stopped and the KVA of it is returned.
 * Otherwise, necessary PTEs and L1 or L0 page tables would be allocated
 * for process `pid`. `pid` is ignored if `alloc` is 0.
 * This is learned from XV6: vm.c: walk().
 */
uintptr_t va2pte(uintptr_t va, PTE* pgdir, int alloc, pid_t pid)
{
	int level;
	PTE* ppte;

	for (level = 2; level >= 0; --level)
	{
		unsigned int vpn = VPN(va, level);
		ppte = &pgdir[vpn];
		if (get_attribute(*ppte, _PAGE_PRESENT) != 0L)
		{
			if (get_attribute(*ppte, _PAGE_XWR) == 0L)
				/* Point to next level of page table */
				pgdir = (PTE*)pa2kva(get_pa(*ppte));
			else
				/*
				 * Why does it stop if V is not 0 even if `level` is not 0?
				 * This would make this function useful for kernel page table
				 * because the kernel uses 2 MiB large page.
				 */
				return (uintptr_t)ppte + (uint64_t)level;
		}
		else
		{
			if (level > 0 && alloc != 0)
			{
				if (*ppte != 0UL)
					panic_g("Level %d PTE 0x%lx at 0x%lx is not 0 but V is 0",
						level, *ppte, (uint64_t)ppte);
				pgdir = (PTE*)alloc_pagetable(pid);
				set_pfn(ppte, kva2pa((uintptr_t)pgdir) >> NORMAL_PAGE_SHIFT);
				set_attribute(ppte, _PAGE_PRESENT);
			}
			else
				return (uintptr_t)ppte + (uint64_t)level;
		}
	}
	panic_g("Valid level 0 PTE 0x%lx at 0x%lx has zero X, W, R",
		*ppte, (uint64_t)ppte);
	return 0UL;
}

/*
 * va2kva: look up the page table at `pgdir_kva`
 * and translate the virtual address `va` to
 * kernel virtual address.
 * Return the KVA on success and 0UL on error.
 */
uintptr_t va2kva(uintptr_t va, PTE* pgdir_kva)
{
	uint64_t vpn2, vpn1, vpn0, offset;
	PTE *ppte;
	uint64_t lpte;	/* Level */

	va &= VA_MASK;
	vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
	vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
	vpn0 = (va >> NORMAL_PAGE_SHIFT) & ~(~0UL << NORMAL_PAGE_SHIFT);
	offset = va & (NORMAL_PAGE_SIZE - 1U);

	lpte = va2pte(va, pgdir_kva, 0, 0);
	ppte = (PTE*)(lpte & ~7UL);
	lpte &= 7UL;

	if (get_attribute(*ppte, _PAGE_PRESENT) != 0L)
	{
		switch (lpte)
		{
		case 2UL:
			return pa2kva(get_pa(*ppte) + (vpn1 << (NORMAL_PAGE_SHIFT + PPN_BITS))
				+ (vpn0 << NORMAL_PAGE_SHIFT) + offset);
			break;
		case 1UL:
			return pa2kva(get_pa(*ppte) + (vpn0 << NORMAL_PAGE_SHIFT) + offset);
			break;
		case 0UL:
			return pa2kva(get_pa(*ppte) + offset);
			break;
		default:
			panic_g("va2pte() returned invalid KVA: 0x%lx",
				(uintptr_t)ppte + lpte);
			return 0UL;
			break;
		}
	}
	else	/* V of the PTE is 0 */
		return 0UL;
}

/*
 * do_sbrk: enlarge the segment by `size`. Just increase `seg_end`.
 * Return: old `seg_end` on success and 0 on error.
 */
uintptr_t do_sbrk(uint64_t size)
{
	uintptr_t rt;
	pcb_t *ccpu = cur_cpu();

	size = ROUND(size, 0x8);
	rt = ccpu->seg_end;
	/*
	 * If the user process passes a negative value to `size`,
	 * 0 will be returned as `USEG_MAX` is NOT so great that
	 * the higest bit of it is 1.
	 */
	if (size > USEG_MAX || rt + size - ccpu->seg_start > USEG_MAX)
		return 0UL;
	else
	{
#if MULTITHREADING != 0
		unsigned int nth, i;
		tcb_t *farr[TID_MAX + 1];
		nth = pcb_search_all(ccpu->pid, farr);
		for (i = 0U; i < nth; ++i)
			farr[i]->seg_end += size;
#else
		ccpu->seg_end += size;
#endif
		return rt;
	}
}
