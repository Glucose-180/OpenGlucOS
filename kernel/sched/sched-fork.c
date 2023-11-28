#include <os/list.h>
#include <os/pcb-list-g.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <os/malloc-g.h>
#include <os/kernel.h>
#include <os/string.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <os/loader.h>
#include <riscv.h>
#include <asm/regs.h>
#include <csr.h>
#include <os/irq.h>
#include <os/smp.h>

/*
 * do_fork: fork a child process and return the
 * PID of child process or INVALID_PID on error.
 */
pid_t do_fork(void)
{
	pid_t pid;
	unsigned int vpn2, vpn1, vpn0;
	pcb_t *pnew, *qtemp, *ccpu = cur_cpu();
	uintptr_t kernel_stack;
	PTE *pgdir_l1, *pgdir_l0;
	PTE *pgdir_l1_p, *pgdir_l0_p; /* For parent process */

	if ((r_sstatus() & SR_SPP) != 0UL || (reg_t)ccpu->trapframe != ccpu->kernel_sp)
		panic_g("do_fork: trap is likely taken from S-mode:\n"
			"$sstatus 0x%lx, trapframe 0x%lx, kernel_sp 0x%lx",
			r_sstatus(), (uint64_t)ccpu->trapframe, ccpu->kernel_sp);
#if MULTITHREADING != 0
	if (pcb_search_all(ccpu->pid, NULL) > 1U)
		/* Multithreading is not supported */
		return INVALID_PID;
#endif
	if (get_proc_num() >= UPROC_MAX + NCPU || 
		(pid = alloc_pid()) == INVALID_PID ||
		(kernel_stack = (uintptr_t)kmalloc_g(Kstack_size)) == NULL)
		return INVALID_PID;
	qtemp = lpcb_add_node_to_tail(ready_queue, &pnew, &ready_queue);
	if (qtemp == NULL)
	{
		kfree_g((void *)uintptr_t);
		return INVALID_PID;
	}
	ready_queue = qtemp;

	pnew->kernel_sp = kernel_stack + (ccpu->kernel_sp - ccpu->kernel_stack);
	pnew->trapframe = pnew->kernel_sp;
	pnew->user_sp = ccpu->user_sp;
	pnew->seg_start = ccpu->seg_start;
	pnew->seg_end = ccpu->seg_end;
	pnew->cursor_x = ccpu->cursor_x;
	pnew->cursor_y = ccpu->cursor_y;
	pnew->cylim_l = ccpu->cylim_l;
	pnew->cylim_h = ccpu->cylim_h;
	pnew->tid = 0;
	pnew->pid = pid;
	pnew->cpu_mask = ccpu->cpu_mask;
	pnew->kernel_stack = kernel_stack;
	pnew->wait_queue = NULL;
	pnew->status = TASK_READY;
	strncpy(pnew->name, ccpu->name, TASK_NAMELEN);
	pnew->name[TASK_NAMELEN] = '\0';

	if (pcb_table_add(pnew) < 0)
		panic_g("do_fork: Failed to add proc %d to pcb_table[]", pid);

	memcpy((uint8_t*)pnew->trapframe, (uint8_t*)ccpu->trapframe,
		sizeof(regs_context_t));
	pnew->trapframe[OFFSET_REG_TP / sizeof(reg_t)] = (reg_t)pnew;
	/* Return value of child process */
	pnew->trapframe[OFFSET_REG_A0 / sizeof(reg_t)] = 0UL;

	pnew->context[SR_RA] = (reg_t)ret_from_exception;
	pnew->context[SR_SP] = (reg_t)pnew->kernel_sp;

	/* Allocate and copy page table */
	pnew->pgdir_kva = (PTE*)alloc_pagetable(pid);
	share_pgtable(pnew->pgdir_kva, ccpu->pgdir_kva);
	for (vpn2 = 0U; vpn2 < (1U << (PPN_BITS - 1U)); ++vpn2)
	{	/* Scan L2 ccpu->pgdir_kva[] */
		if (get_attribute(ccpu->pgdir_kva[vpn2], _PAGE_PRESENT) != 0L)
		{
			if (get_attribute(ccpu->pgdir_kva[vpn2], _PAGE_XWR) != 0L)
				panic_g("do_fork: invalid L2 PTE %u 0x%lx",
					vpn2, ccpu->pgdir_kva[vpn2]);
			pgdir_l1 = (PTE*)alloc_pagetable(pid);
			pgdir_l1_p = (PTE*)pa2kva(get_pa(ccpu->pgdir_kva[vpn2]));
			set_pfn(&pnew->pgdir_kva[vpn2],
				kva2pa((uintptr_t)pgdir_l1) >> NORMAL_PAGE_SHIFT);
			set_attribute(&pnew->pgdir_kva[vpn2], _PAGE_PRESENT);

			for (vpn1 = 0U; vpn1 < (1U << PPN_BITS); ++vpn1)
			{	/* Scan L1 pgdir_l1_p */
				if (get_attribute(pgdir_l1_p[vpn1], _PAGE_PRESENT) != 0L)
				{
					if (get_attribute(pgdir_l1_p[vpn1], _PAGE_XWR) != 0L)
						panic_g("do_fork: invalid L1 PTE %u %u 0x%lx",
							vpn2, vpn1, pgdir_l1_p[vpn1]);
					pgdir_l0 = (PTE*)alloc_pagetable(pid);
					pgdir_l0_p = (PTE*)pa2kva(get_pa(pgdir_l1_p[vpn1]));
					set_pfn(&pgdir_l1[vpn1],
						kva2pa((uintptr_t)pgdir_l0) >> NORMAL_PAGE_SHIFT);
					set_attribute(&pgdir_l1[vpn1], _PAGE_PRESENT);

					for (vpn0 = 0U; vpn0 < (1U << PPN_BITS); ++vpn0)
					{	/* Scan L0 pgdir_l0_p */
						if (pgdir_l0_p[vpn0] == 0UL)
							continue;
						if (get_attribute(pgdir_l0_p[vpn0], _PAGE_PRESENT) == 0L)
							/* If a page is in disk, swap it to memory. */
							swap_from_disk(&pgdir_l0_p[vpn0], vpn2va(vpn2, vpn1, vpn0));

						if (get_attribute(pgdir_l0_p[vpn0], _PAGE_XWR) == 0L)
							panic_g("do_fork: invalid L0 PTE %u %u %u 0x%lx",
								vpn2, vpn1, vpn0, pgdir_l0_p[vpn0]);
						if (get_attribute(pgdir_l0_p[vpn0], _PAGE_SHARED) != 0L)
						{
							// TODO: Shared page, just copy
						}
						else if (get_attribute(pgdir_l0_p[vpn0], _PAGE_WRITE) == 0L)
						{
							// TODO: Read only page, copy PTE and increase `pg_uva[]`
						}
						else
						{
							// TODO: Normal page, copy PTE and set W to zero.
						}
					}
				}
			}
		}
	}
}