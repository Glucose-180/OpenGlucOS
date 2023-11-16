#include <os/mm.h>
#include <os/glucose.h>

/*
 * Use "char map" (like bitmap) to manage page frames.
 */
#if DEBUG_EN != 0
const uintptr_t Pg_base = ROUND(FREEMEM_KERNEL, PAGE_SIZE);
volatile uint8_t pg_charmap[NPF];
unsigned int pg_ffree;
volatile uintptr_t pgtable_base = PGDIR_VA + NORMAL_PAGE_SIZE + NORMAL_PAGE_SIZE;
#else
static const uintptr_t Pg_base = ROUND(FREEMEM_KERNEL, PAGE_SIZE);
/*
 * pg_charmap[]: pg_charmap[i] being CMAP_FREE means that i-th page is free.
 * Otherwise, pg_charmap[i] means the PID of the process occupying it.
 */
static volatile uint8_t pg_charmap[NPF];
static unsigned int pg_ffree;	/* index of first free page */
#endif

void init_mm(void)
{
	void malloc_init(void);
	unsigned int i;

	malloc_init();

	pg_ffree = 0U;
	if (CMAP_FREE == 0xffu)
		for (i = 0U; i < NPF / 8U; ++i)
			/* use int64_t to speed up initialization */
			((int64_t*)pg_charmap)[i] = (int64_t)-1;
	else
		for (i = 0U; i < NPF; ++i)
			pg_charmap[i] = CMAP_FREE;
	
}

/*
 * alloc_page: Allocate ONE page frame for process `pid`.
 * Return the KVA of it or 0UL on error.
 */
uintptr_t alloc_page(unsigned int npages, pid_t pid)
{
	uintptr_t rt;
	if (npages != 1U)
		panic_g("alloc_page: only supports 1 page up to now");
	// align PAGE_SIZE
/*	ret = ROUND(kernel_mem_curr, PAGE_SIZE);
	kernel_mem_curr = ret + npages * PAGE_SIZE;
	if (kernel_mem_curr > FREEMEM_KERNEL + 0x0a000000UL)
		panic_g("alloc_page: 0x%lx overflow!", kernel_mem_curr);
*/
	if ((uint8_t)pid == CMAP_FREE)
		panic_g("alloc_page: pid is invalid: 0x%x", (int)CMAP_FREE);
	if (pg_ffree >= NPF)
		/* No free page */
		return 0UL;
	rt = Pg_base + pg_ffree * PAGE_SIZE;
	pg_charmap[pg_ffree] = (uint8_t)pid;
	/* Update pg_ffree */
	for (; pg_ffree < NPF; ++pg_ffree)
		if (pg_charmap[pg_ffree] == CMAP_FREE)
			break;
	return rt;
}

// NOTE: Only need for S-core to alloc 2MB large page
#ifdef S_CORE
static ptr_t largePageMemCurr = LARGE_PAGE_FREEMEM;
ptr_t allocLargePage(int numPage)
{
	// align LARGE_PAGE_SIZE
	ptr_t ret = ROUND(largePageMemCurr, LARGE_PAGE_SIZE);
	largePageMemCurr = ret + numPage * LARGE_PAGE_SIZE;
	return ret;    
}
#endif

void freePage(ptr_t baseAddr)
{
	// TODO [P4-task1] (design you 'freePage' here if you need):
}

/*
 * alloc_pagetable: virtual memory version of alloc_page()
 * in boot.c.
 * Allocate a page as a pagetable, CLEAR it and return its
 * KERNEL VIRTUAL ADDRESS.
 */
uintptr_t alloc_pagetable()
{
	/*
	 * Three pages have been used in while boot_kernel(),
	 * and one of them will be revoked, so set pgtable_base like this.
	 * NOTE: if boot.c is modified, pay attention to
	 * the number of paged used by it!!!
	 */
#if DEBUG_EN == 0
	static volatile uintptr_t pgtable_base = PGDIR_VA +
		NORMAL_PAGE_SIZE + NORMAL_PAGE_SIZE;
#endif
	pgtable_base += NORMAL_PAGE_SIZE;
	if (pgtable_base > 0xffffffc051f00000UL)
		panic_g("alloc_pagetable: 0x%lx overflow!", pgtable_base);
	clear_pgdir(pgtable_base - NORMAL_PAGE_SHIFT);
	return pgtable_base - NORMAL_PAGE_SIZE;
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
 * allocate a physical page for `va`, mapping it into `pgdir_kva`,
 * return the kernel virtual address for the page.
 * `pid` is the PID of the process for which the page is used.
 */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir_kva, pid_t pid)
{
	// TODO [P4-task1] alloc_page_helper:
	uint64_t vpn2, vpn1, vpn0;
	PTE* pgdir = (PTE*)pgdir_kva, *pgdir_l1 = NULL, *pgdir_l0 = NULL;
	uintptr_t pg_kva;
	
	va &= VA_MASK;
	vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
	vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
	vpn0 = (va >> NORMAL_PAGE_SHIFT) & ~(~0UL << PPN_BITS);

	if (pgdir[vpn2] == 0UL)
	{
		pgdir_l1 = (PTE*)alloc_pagetable();
		set_pfn(&pgdir[vpn2], kva2pa((uintptr_t)pgdir_l1) >> NORMAL_PAGE_SHIFT);
		set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
	}
	else
	{
		pgdir_l1 = (PTE*)pa2kva(get_pa(pgdir[vpn2]));
		if (get_attribute(pgdir[vpn2], _PAGE_PRESENT) == 0U)
			panic_g("alloc_page_helper: pgdir[vpn2]: 0x%lx is not 0 but V is 0,\n"
				"pgdir is 0x%lx, vpn2 is %lu", pgdir[vpn2], pgdir_kva, vpn2);
	}

	if (pgdir_l1[vpn1] == 0UL)
	{
		pgdir_l0 = (PTE*)alloc_pagetable();
		set_pfn(&pgdir_l1[vpn1], kva2pa((uintptr_t)pgdir_l0) >> NORMAL_PAGE_SHIFT);
		set_attribute(&pgdir_l1[vpn1], _PAGE_PRESENT);
	}
	else
	{
		pgdir_l0 = (PTE*)pa2kva(get_pa(pgdir_l1[vpn1]));
		if (get_attribute(pgdir_l1[vpn1], _PAGE_PRESENT) == 0U)
			panic_g("alloc_page_helper: pgdir_l1[vpn1]: 0x%lx is not 0 but V is 0,\n"
				"pgdir_l1 is 0x%lx, vpn1 is %lu",
				pgdir_l1[vpn1], (uintptr_t)pgdir_l1, vpn1);
	}
#define _PAGE_VURWXAD (_PAGE_PRESENT | _PAGE_USER | _PAGE_READ | _PAGE_WRITE |\
			_PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY)
	if (pgdir_l0[vpn0] == 0UL)
	{
		pg_kva = alloc_page(1U, pid);
		set_pfn(&pgdir_l0[vpn0], kva2pa(pg_kva) >> NORMAL_PAGE_SHIFT);
		set_attribute(&pgdir_l0[vpn0], _PAGE_VURWXAD);
	}
	else
	{
		pg_kva = pa2kva(get_pa(pgdir_l0[vpn0]));
		if (get_attribute(pgdir_l0[vpn0], _PAGE_VURWXAD) != _PAGE_VURWXAD)
			panic_g("alloc_page_helper: pgdir_l0[vpn0]: 0x%lx is not 0 but has wrong attrib,\n"
				"pgdir_l0 is 0x%lx, vpn0 is %lu",
				pgdir_l0[vpn0], (uintptr_t)pgdir_l0, vpn0);
	}
	return pg_kva;
}

/*
 * alloc_pages: allocate `npages` pages for virtual address `va`
 * and map va to va + npages * 4KiB - 1 into pgdir_kva.
 * Return the KVA of the first page and 0 on error.
uintptr_t alloc_pages(uintptr_t va, unsigned npages, PTE* pgdir_kva)
{

}
 */

/*
 * uva2kva: look up the page table at `pgdir_kva`
 * and translate the virtual address `va` to
 * kernel virtual address.
 * Return the KVA on success and 0UL on error.
 */
uintptr_t va2kva(uintptr_t va, PTE* pgdir_kva)
{
	uint64_t vpn2, vpn1, vpn0, offset;
	PTE *pgdir_l1 = NULL, *pgdir_l0 = NULL;

#define _PAGE_XWR (_PAGE_EXEC | _PAGE_WRITE | _PAGE_READ)

	va &= VA_MASK;
	vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
	vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
	vpn0 = (va >> NORMAL_PAGE_SHIFT) & ~(~0UL << NORMAL_PAGE_SHIFT);
	offset = va & (NORMAL_PAGE_SIZE - 1U);
	if (get_attribute(pgdir_kva[vpn2], _PAGE_PRESENT) != 0L)
	{
		if (get_attribute(pgdir_kva[vpn2], _PAGE_XWR) == 0L)
			/* Point to next level of page table */
			pgdir_l1 = (PTE *)pa2kva(get_pa(pgdir_kva[vpn2]));
		else
			/* It is leave page table */
			return pa2kva((pgdir_kva[vpn2]) + (vpn1 << (NORMAL_PAGE_SHIFT + PPN_BITS)) +
				(vpn0 << NORMAL_PAGE_SHIFT) + offset);
	}
	else
		return 0UL;	/* Failed */
	if (get_attribute(pgdir_l1[vpn1], _PAGE_PRESENT) != 0L)
	{
		if (get_attribute(pgdir_l1[vpn1], _PAGE_XWR) == 0L)
			/* Point to next level of page table */
			pgdir_l0 = (PTE *)pa2kva(get_pa(pgdir_l1[vpn1]));
		else
			/* It is leave page table */
			return pa2kva(get_pa(pgdir_l1[vpn1]) +
				(vpn0 << NORMAL_PAGE_SHIFT) + offset);
	}
	else
		return 0UL;
	if (get_attribute(pgdir_l0[vpn0], _PAGE_PRESENT) != 0L)
	{
		if (get_attribute(pgdir_l0[vpn0], _PAGE_XWR) == 0L)
			/* It's error as X, W, R are all 0! */
			return 0UL;
		else
			return pa2kva(get_pa(pgdir_l0[vpn0]) + offset);
	}
	else
		return 0UL;
}

uintptr_t shm_page_get(int key)
{
	// TODO [P4-task4] shm_page_get:
	return 0;
}

void shm_page_dt(uintptr_t addr)
{
	// TODO [P4-task4] shm_page_dt:
}