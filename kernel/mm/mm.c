#include <os/mm.h>
#include <os/glucose.h>

// NOTE: A/C-core
#if DEBUG_EN != 0
ptr_t kernel_mem_curr = FREEMEM_KERNEL;
uintptr_t pg_base = PGDIR_VA + NORMAL_PAGE_SIZE + NORMAL_PAGE_SIZE;
#endif

ptr_t alloc_page(unsigned int npages)
{
#if DEBUG_EN == 0
	static ptr_t kernel_mem_curr = FREEMEM_KERNEL;
#endif
	// align PAGE_SIZE
	ptr_t ret = ROUND(kernel_mem_curr, PAGE_SIZE);
	kernel_mem_curr = ret + npages * PAGE_SIZE;
	if (kernel_mem_curr > FREEMEM_KERNEL + 0x0a000000UL)
		panic_g("alloc_page: 0x%lx overflow!", kernel_mem_curr);
	return ret;
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

void *kmalloc(size_t size)
{
	// TODO [P4-task1] (design you 'kmalloc' here if you need):
	return NULL;
}

/*
 * alloc_pagetable: virtual memory version of alloc_page()
 * in boot.c.
 * Allocate a page as a pagetable and return its
 * KERNEL VIRTUAL ADDRESS.
 */
uintptr_t alloc_pagetable()
{
	/*
	 * Three pages have been used in while boot_kernel(),
	 * and one of them will be revoked, so set pg_base like this.
	 * NOTE: if boot.c is modified, pay attention to
	 * the number of paged used by it!!!
	 */
#if DEBUG_EN == 0
	static uintptr_t pg_base = PGDIR_VA +
		NORMAL_PAGE_SIZE + NORMAL_PAGE_SIZE;
#endif
	pg_base += NORMAL_PAGE_SIZE;
	if (pg_base > 0xffffffc051f00000UL)
		panic_g("alloc_pagetable: 0x%lx overflow!", pg_base);
	return pg_base - NORMAL_PAGE_SIZE;
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
	// TODO [P4-task1] share_pgtable:
}

/* allocate physical page for `va`, mapping it into `pgdir_kva`,
   return the kernel virtual address for the page
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir_kva)
{
	// TODO [P4-task1] alloc_page_helper:
	uint64_t vpn2, vpn1, vpn0;
	PTE* pgdir = (PTE*)pgdir_kva, *pgdir_l1 = NULL, *pgdir_l0 = NULL;
	uintptr_t pg_kva;
	
	va &= VA_MASK;
	vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
	vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
	vpn0 = (va >> NORMAL_PAGE_SHIFT) & ~(~0UL << NORMAL_PAGE_SHIFT);

	if (pgdir[vpn2] == 0UL)
	{
		pgdir_l1 = (PTE*)alloc_pagetable();
		set_pfn(&pgdir[vpn2], kva2pa((uintptr_t)pgdir_l1) >> NORMAL_PAGE_SHIFT);
		set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
		clear_pgdir((uintptr_t)pgdir_l1);
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
		set_attribute(&pgdir[vpn1], _PAGE_PRESENT);
		clear_pgdir((uintptr_t)pgdir_l0);
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
		pg_kva = alloc_page(1U);
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

uintptr_t shm_page_get(int key)
{
	// TODO [P4-task4] shm_page_get:
	return 0;
}

void shm_page_dt(uintptr_t addr)
{
	// TODO [P4-task4] shm_page_dt:
}