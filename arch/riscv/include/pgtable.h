#ifndef PGTABLE_H
#define PGTABLE_H

#include <type.h>

#define SATP_MODE_SV39 8
#define SATP_MODE_SV48 9

#define SATP_ASID_SHIFT 44lu
#define SATP_MODE_SHIFT 60lu

#define NORMAL_PAGE_SHIFT 12lu
#define NORMAL_PAGE_SIZE (1lu << NORMAL_PAGE_SHIFT)
#define LARGE_PAGE_SHIFT 21lu
#define LARGE_PAGE_SIZE (1lu << LARGE_PAGE_SHIFT)

/*
 * Flush entire local TLB.  'sfence.vma' implicitly fences with the instruction
 * cache as well, so a 'fence.i' is not necessary.
 */
static inline void local_flush_tlb_all(void)
{
	__asm__ __volatile__ ("sfence.vma" : : : "memory");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long vaddr)
{
	__asm__ __volatile__ ("sfence.vma %0" : : "r" (vaddr) : "memory");
}

static inline void local_flush_icache_all(void)
{
	asm volatile ("fence.i" ::: "memory");
}

static inline void set_satp(
	unsigned mode, unsigned asid, unsigned long ppn)
{
	unsigned long __v =
		(unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
	__asm__ __volatile__("sfence.vma\ncsrw satp, %0" : : "rK"(__v) : "memory");
}

#define PGDIR_PA 0x51000000lu  // use 51000000 page as PGDIR
#define PGDIR_VA 0xffffffc051000000UL

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

#define _PAGE_PRESENT (1UL << 0)
#define _PAGE_READ (1UL << 1)     /* Readable */
#define _PAGE_WRITE (1UL << 2)    /* Writable */
#define _PAGE_EXEC (1UL << 3)     /* Executable */
#define _PAGE_USER (1UL << 4)     /* User */
#define _PAGE_GLOBAL (1UL << 5)   /* Global */
#define _PAGE_ACCESSED (1UL << 6) /* Set by hardware on any access */
#define _PAGE_DIRTY (1UL << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT (1UL << 8)     /* Reserved for software */

#define _PAGE_PFN_SHIFT 10lu
#define _PAGE_RESERVED_SHIFT 54U

#define _PAGE_PPN_MASK ((~0UL << _PAGE_PFN_SHIFT) & ~(~0UL << _PAGE_RESERVED_SHIFT))
#define _PAGE_ATTRIBUTE_MASK ~(~0UL << 8)

#define VA_MASK ((1lu << 39) - 1UL)

#define PPN_BITS 9lu
#define NUM_PTE_ENTRY (1U << PPN_BITS)

typedef uint64_t PTE;

/* Translation between physical addr and kernel virtual addr */
static inline uintptr_t kva2pa(uintptr_t kva)
{
	/* TODO: [P4-task1] */
	return kva - (0xffffffc0UL << 32);
}

static inline uintptr_t pa2kva(uintptr_t pa)
{
	/* TODO: [P4-task1] */
	return pa + (0xffffffc0UL << 32);
}

/* get physical page addr from PTE 'entry' */
static inline uintptr_t get_pa(PTE entry)
{
	/* TODO: [P4-task1] */
	return (entry & _PAGE_PPN_MASK) <<
		(NORMAL_PAGE_SHIFT - _PAGE_PFN_SHIFT);
}

/* Get/Set page frame number of the `entry` */
static inline uintptr_t get_pfn(PTE entry)
{
	/* TODO: [P4-task1] */
	return (entry & _PAGE_PPN_MASK) >> _PAGE_PFN_SHIFT;
}
static inline void set_pfn(PTE *entry, uint64_t pfn)
{
	/* TODO: [P4-task1] */
	*entry = (*entry & ~_PAGE_PPN_MASK) |
		((pfn << _PAGE_PFN_SHIFT) & ~_PAGE_PPN_MASK);
}

/* Get/Set attribute(s) of the `entry` */
static inline long get_attribute(PTE entry, uint64_t mask)
{
	/* TODO: [P4-task1] */
	return entry & mask & _PAGE_ATTRIBUTE_MASK;
}
static inline void set_attribute(PTE *entry, uint64_t bits)
{
	/* TODO: [P4-task1] */
	*entry |= (bits & _PAGE_ATTRIBUTE_MASK);
}

static inline void clear_pgdir(uintptr_t pgdir_addr)
{
	/* TODO: [P4-task1] */
	unsigned int ne = NUM_PTE_ENTRY;
	while (ne-- > 0U)
		((PTE *)pgdir_addr)[ne] = 0UL;
}

#endif  // PGTABLE_H
