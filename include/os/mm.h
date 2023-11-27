/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Memory Management
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */
#ifndef MM_H
#define MM_H

#include <type.h>
#include <pgtable.h>

#define MAP_KERNEL 1
#define MAP_USER 2
#define MEM_SIZE 32
#define PAGE_SIZE 4096 // 4K
//#define INIT_KERNEL_STACK 0xffffffc052000000
//#define FREEMEM_KERNEL (INIT_KERNEL_STACK+PAGE_SIZE)
#define FREEMEM_KERNEL 0xffffffc052000000

#ifndef USTACK_NPG
/*
 * How many pages are allocated for
 * user stack of a user process.
 * NOTE: also defined in `sched.h`.
 */
#define USTACK_NPG 16U
#endif

#ifndef USEG_MAX
/*
 * Max size for user segment (bytes).
 * It is used in `do_sbrk()`.
 */
#define USEG_MAX (8UL * 1024UL * 1024UL)
#endif

#ifndef NPF
/*
 * Number of page frames: these page frames will occupy
 * 0x52000000 to 0x5e800000-1 (physical address)
 */
#define NPF (50U * 1024U)
#endif

#ifndef NPSWAP
/*
 * Number of pages in swap partition of disk
 */
#define NPSWAP 128U
#endif

/*
 * Number of page tables: these page tables will occupy
 * 0x51000000 to 0x51f00000-1 (physical address)
 */
#define NPT 3840U

/* Free flag in "charmap" */
#define CMAP_FREE (uint8_t)0xff
/* Shared page flag */
#define CMAP_SHARED (uint8_t)0xfe

/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

/* How many processes can use a shared memory */
#define NPSHM 24U
/* How many shared pages */
#define NSHM 3U

/* Shared memory control */
typedef struct {
	unsigned int nproc;		/* How many processes are using this page */
	unsigned int pgidx;		/* The index of the physical page frame */
	pid_t opid[NPSHM];		/* The PIDs of processes using it */
#if DEBUG_EN != 0
	uintptr_t uva[NPSHM];	/* The UVAs of the page for processes using it */
#endif
	/*
	 * NOTE: `uva` might be useless;
	 * if `nproc` is 0, this shared memory page is invalid and other
	 * data (`pgidx`, ...) should be ignored!
	 */
} shm_ctrl_t;

extern volatile shm_ctrl_t shm_ctrl[NSHM];

extern const uintptr_t Pg_base;
extern volatile uint8_t pg_charmap[NPF];
//extern volatile uintptr_t pg_uva[NPF];
extern volatile uintptr_t *pg_uva;

void init_mm(void);
extern uintptr_t alloc_page(unsigned int npages, pid_t pid, uintptr_t uva);
// TODO [P4-task1] */
void free_page(uintptr_t pg_kva);

// #define S_CORE
// NOTE: only need for S-core to alloc 2MB large page
#ifdef S_CORE
#define LARGE_PAGE_FREEMEM 0xffffffc056000000
#define USER_STACK_ADDR 0x400000
extern ptr_t allocLargePage(int numPage);
#else
// NOTE: A/C-core
#define USER_STACK_ADDR 0xf00010000UL
#endif

uintptr_t alloc_pagetable(pid_t pid);
void free_pagetable(uintptr_t pgtb_kva);
unsigned int free_pages_of_proc(PTE* pgdir, pid_t pid);
extern void share_pgtable(PTE* dest_pgdir, PTE* src_pgdir);
extern uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir_kva, pid_t pid);

uintptr_t va2pte(uintptr_t va, PTE* pgdir_kva);
uintptr_t va2kva(uintptr_t va, PTE* pgdir_kva);

uintptr_t do_sbrk(uint64_t size);

void free_swap_page(unsigned int spg_idx);
unsigned int swap_to_disk(void);
uintptr_t swap_from_disk(PTE *ppte, uintptr_t uva);

// TODO [P4-task4]: shm_page_get/dt */
uintptr_t shm_page_get(int key);
int shm_page_dt(uintptr_t addr, pid_t mpid, PTE* pgdir);
int do_shm_page_dt(uintptr_t addr);

#endif /* MM_H */
