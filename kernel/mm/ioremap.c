#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
	// TODO: [p5-task1] map one specific physical region to virtual address
	uintptr_t va;
	PTE *kpgdir = (PTE *)PGDIR_VA;

	if (phys_addr >= (1UL << 32) || size >= (1UL << 32))
		return NULL;
	
	/* Use 1 GiB large page */
	for (va = io_base + phys_addr; va < io_base + phys_addr + size;
		va += GiB)
	{
		unsigned int vpn2;
		vpn2 = VPN(va, 2U);
		if (get_attribute(kpgdir[vpn2], _PAGE_PRESENT) != 0L &&
			get_attribute(kpgdir[vpn2], _PAGE_XWR) == 0L)
			/* An occupied PTE is found */
			return NULL;
		set_pfn(&kpgdir[vpn2], (phys_addr & ~(GiB - 1UL)) >> NORMAL_PAGE_SHIFT);
		set_attribute(&kpgdir[vpn2],
			_PAGE_XWR | _PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_DIRTY);
	}
	local_flush_tlb_all();	/* I think it is unnecessary */
	return (void *)(io_base + phys_addr);
}

/*
void iounmap(void *io_addr)
{
	// TODO: [p5-task1] a very naive iounmap() is OK
	// maybe no one would call this function?
}
*/