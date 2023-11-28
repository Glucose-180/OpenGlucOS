#include <os/mm.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/glucose.h>

volatile shm_ctrl_t shm_ctrl[NSHM];

/*
 * shm_page_get: allocate a new page or get a being used page
 * according to `key`. Return the UVA for current process of it
 * on success or 0 on error.
 * NOTE: the new page will be cleared.
 */
uintptr_t shm_page_get(int key)
{
	unsigned int shmid = (unsigned)key % NSHM;
	unsigned int i;
	/* The index such that `.opid[pidid]` is `INVALID_PID` */
	unsigned int pidid;
	/* UVA of the shared page for this process */
	uintptr_t uva;
	uintptr_t pg_kva, pg_kva2;
	PTE* ppte;
	uint64_t lpte;
	pcb_t *ccpu = cur_cpu();
	volatile shm_ctrl_t *ptsc = shm_ctrl + shmid;

	if (ptsc->nproc >= NPSHM)
		/* Quota of the page is full */
		return 0UL;
	else if (ptsc->nproc > 0U)
		for (i = 0U; i < NPSHM; ++i)
			if (ptsc->opid[i] == ccpu->pid)
				/* This process is already using this shared page. */
				return 0UL;
	uva = ROUND(ccpu->seg_end, NORMAL_PAGE_SIZE);
	if (uva + NORMAL_PAGE_SIZE - ccpu->seg_start > USEG_MAX)
		/* The heap of the process is too large */
		return 0UL;
	ccpu->seg_end = uva;
#if MULTITHREADING != 0
	pcb_t *farr[TID_MAX + 1];
	unsigned int nth = pcb_search_all(ccpu->pid, farr);
	for (i = 0U; i < nth; ++i)
		farr[i]->seg_end = uva + NORMAL_PAGE_SIZE;
#else
	ccpu->seg_end = uva + NORMAL_PAGE_SIZE;
#endif
	if (ptsc->nproc == 0U)
	{	/* It is the first process using it */
		pg_kva = alloc_page_helper(uva, (uintptr_t)ccpu->pgdir_kva, CMAP_SHARED);
		/*
		 * If it is the first process using this page,
		 * clear it.
		 */
		clear_pgdir(pg_kva);
		ptsc->pgidx = (pg_kva - Pg_base) >> NORMAL_PAGE_SHIFT;
		pidid = 0U;
		ptsc->opid[0U] = ccpu->pid;
		for (i = 1U; i < NPSHM; ++i)
			ptsc->opid[i] = INVALID_PID;
	}
	else
	{
		for (pidid = 0U; pidid < NPSHM; ++pidid)
			if (ptsc->opid[pidid] == INVALID_PID)
				break;
		if (pidid >= NPSHM)
			panic_g("shm_page_get: Cannot find pidid such that "
				"opid[pidid] is INVALID_PID for shm_ctrl[%u], proc %d",
				shmid, ccpu->pid);
		ptsc->opid[pidid] = ccpu->pid;
		pg_kva = Pg_base + (ptsc->pgidx << NORMAL_PAGE_SHIFT);
	}
	ptsc->opid[pidid] = ccpu->pid;
	ptsc->nproc++;
#if DEBUG_EN != 0
	ptsc->uva[pidid] = uva;
#endif
	/*
	 * We do not need a new page frame. We just need a valid PTE in L0 page table.
	 * If it is the first process using this shared memory, `pg_kva2` must equal
	 * `pg_kva` as the `uva` has just been used to allocate a page.
	 * Otherwise, they must be different because the `uva` is greater than the original
	 * `seg_end` of this process, which means that it is not allocated.
	 * In this case, we should free the page frame as we just need the PTE.
	 */
	pg_kva2 = alloc_page_helper(uva, (uintptr_t)ccpu->pgdir_kva, CMAP_SHARED);
	if ((ptsc->nproc > 1U) != (pg_kva2 != pg_kva))
		panic_g("shm_ctrl[%u].nproc is %u but pg_kva is 0x%lx, pg_kva2 is 0x%lx",
			shmid, ptsc->nproc, pg_kva, pg_kva2);
	if (ptsc->nproc > 1U)
		/* Don't forget that we just need the PTE. */
		free_page(pg_kva2);
	lpte = va2pte(uva, ccpu->pgdir_kva);
	ppte = (PTE*)(lpte & ~7UL);
	lpte &= 7UL;
	if (lpte != 0UL)
		panic_g("shm_page_get: va2pte() returned invalid KVA: 0x%lx",
			(uintptr_t)ppte + lpte);
	*ppte = 0UL;
	set_attribute(ppte, _PAGE_SHARED | _PAGE_VURWXAD);
	set_pfn(ppte, kva2pa(pg_kva) >> NORMAL_PAGE_SHIFT);
	local_flush_tlb_page(uva);
	return uva;
}

/*
 * shm_page_dt: Cancel a shared page according to its UVA `addr`
 * of a certain process.
 * If `mpid` is `INVALID_PID`, `addr` is considered as the current process
 * and `pgdir` is ignored; otherwise, `addr` is considered belonging to
 * process `mpid` and `pgdir` is its page directory (KVA).
 * If no process is using it, it will be destroyed.
 * Return the number of page freed (0 or 1) or -1 on error.
 */
int shm_page_dt(uintptr_t addr, pid_t mpid, PTE* pgdir)
{
	PTE* ppte;
	uint64_t lpte;
	uintptr_t shm_page_kva;
	volatile shm_ctrl_t *ptsc;
	unsigned int pgidx, i;
	pid_t pid = (mpid == INVALID_PID ? cur_cpu()->pid : mpid);

	if (mpid == INVALID_PID)
		lpte = va2pte(addr, cur_cpu()->pgdir_kva);
	else
		lpte = va2pte(addr, pgdir);
	ppte = (PTE*)(lpte & ~7UL);
	lpte &= 7UL;

	if (lpte != 0UL || get_attribute(*ppte, _PAGE_SHARED) == 0L)
		/* Not a shared page */
		return -1;
	if (get_attribute(*ppte, _PAGE_PRESENT) == 0L)
		panic_g("shm_page_dt: PTE at 0x%lx is shared but not present: 0x%lx",
			(uintptr_t)ppte, *ppte);
	shm_page_kva = pa2kva(get_pa(*ppte));
	*ppte = 0UL;
	if (mpid == INVALID_PID)
		local_flush_tlb_page(addr);
	pgidx = (shm_page_kva - Pg_base) >> NORMAL_PAGE_SHIFT;

	for (i = 0U; i < NSHM; ++i)
		if (shm_ctrl[i].nproc > 0U && shm_ctrl[i].pgidx == pgidx)
			break;
	if (i >= NSHM)
		panic_g("shm_page_dt: shared page %u with UVA 0x%lx is not found",
			pgidx, addr);
	ptsc = shm_ctrl + i;

	for (i = 0U; i < NPSHM; ++i)
		if (ptsc->opid[i] == pid)
			break;
	if (i >= NPSHM || ptsc->nproc == 0U)
		panic_g("shm_page_dt: Cannot find current process in "
			"shm_ctrl[%d].opid[], .nproc %u", (int)(ptsc - shm_ctrl), ptsc->nproc);
	ptsc->opid[i] = INVALID_PID;
	if (--(ptsc->nproc) == 0U)
	{	/* This is the last process using it */
		free_page(shm_page_kva);
		return 1;
	}
	return 0;
}

/* Syscall */
int do_shm_page_dt(uintptr_t addr)
{
	return shm_page_dt(addr, INVALID_PID, NULL);
}