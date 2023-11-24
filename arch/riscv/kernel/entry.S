#include <asm.h>
#include <csr.h>
#include <asm/regs.h>

.macro GET_CURRENT_RUNNING_TO_T0
	csrr	t0, CSR_MHARTID
	bne	t0, zero, 1f
	la	t0, current_running
	ld	t0, 0(t0)	/* Now $t0 equals current_running[0], or c.r. */
	j	2f
1:
	la	t0, current_running
	ld	t0, 8(t0)	/* Now $t0 equals current_running[1], or c.r. */
2:
.endm

/*
 * SAVE_CONTEXT: When the trap is taken from U-mode:
 * 1. $sp is saved in `user_sp` of PCB;
 * 2. $sp is set to the `kernel_sp` of PCB;
 * 3. $sp and `kernel_sp` are decresed by sizeof(regs_context_t);
 * 4. GPRs except $sp and $t0 and some CSRs are saved in the space in kernel stack;
 * 5. `trapframe` of PCB is set to $sp after saveing;
 * 6. `trapframe->regs[#sp]` is set to `user_sp`, or original $sp;
 * 7. $t0 is saved in the corresponding space of the frame in kernel stack.
 * However, When the trap is taken from S-mode:
 * only 3, 4 and 7 above are executed, and ((regs_context_t*)$sp)->regs[#sp]
 * is set to the original $sp (before 3. decreasement).
 * This is to suppport traps such as page fault that are taken
 * from S-mode!
 */
.macro SAVE_CONTEXT
	/* TODO: [p2-task3] save all general purpose registers here! */
	/* HINT: Pay attention to the function of tp and sp, and save them carefully! */

	csrw	CSR_SSCRATCH, t0	/* Save $t0 in $sscratch */

	csrr	t0, CSR_SSTATUS
	andi	t0, t0, SR_SPP
	/*
	 * SPP of $sstatus being 1 means that the
	 * trap(exception) is NOT from U-Mode.
	 */
	bne	t0, zero, Exc_from_S_mode

Exc_from_U_mode:
	GET_CURRENT_RUNNING_TO_T0
	sd	sp, PCB_USER_SP(t0)	/* c.r.->user_sp := $sp */
	ld	sp, PCB_KERNEL_SP(t0)	/* $sp := c.r.->kernel_sp */
	addi	sp, sp, -OFFSET_SIZE		/* $sp := $sp - sizeof(regs_context_t) */
	j	Save_gprs

Exc_from_S_mode:
	/*
	 * Exception from S-Mode:
	 * Don't load `kernel_sp` from PCB, and don't save $sp
	 * in `user_sp` and `trapframe` of PCB!
	 */
	GET_CURRENT_RUNNING_TO_T0
	addi	sp, sp, -OFFSET_SIZE		/* $sp := $sp - sizeof(regs_context_t) */

Save_gprs:
	/* Save GPRs */
	sd	zero, OFFSET_REG_ZERO(sp)
	sd	ra, OFFSET_REG_RA(sp)
	//sd	sp, OFFSET_REG_SP(sp)
	sd	gp, OFFSET_REG_GP(sp)
	sd	tp, OFFSET_REG_TP(sp)
	//sd	t0, OFFSET_REG_T0(sp)
	sd	t1, OFFSET_REG_T1(sp)
	sd	t2, OFFSET_REG_T2(sp)
	sd	s0, OFFSET_REG_S0(sp)
	sd	s1, OFFSET_REG_S1(sp)
	sd	a0, OFFSET_REG_A0(sp)
	sd	a1, OFFSET_REG_A1(sp)
	sd	a2, OFFSET_REG_A2(sp)
	sd	a3, OFFSET_REG_A3(sp)
	sd	a4, OFFSET_REG_A4(sp)
	sd	a5, OFFSET_REG_A5(sp)
	sd	a6, OFFSET_REG_A6(sp)
	sd	a7, OFFSET_REG_A7(sp)
	sd	s2, OFFSET_REG_S2(sp)
	sd	s3, OFFSET_REG_S3(sp)
	sd	s4, OFFSET_REG_S4(sp)
	sd	s5, OFFSET_REG_S5(sp)
	sd	s6, OFFSET_REG_S6(sp)
	sd	s7, OFFSET_REG_S7(sp)
	sd	s8, OFFSET_REG_S8(sp)
	sd	s9, OFFSET_REG_S9(sp)
	sd	s10, OFFSET_REG_S10(sp)
	sd	s11, OFFSET_REG_S11(sp)
	sd	t3, OFFSET_REG_T3(sp)
	sd	t4, OFFSET_REG_T4(sp)
	sd	t5, OFFSET_REG_T5(sp)
	sd	t6, OFFSET_REG_T6(sp)
	
	/* Save CSRs */
	csrr	t1, CSR_SSTATUS
	sd	t1, OFFSET_REG_SSTATUS(sp)
	csrr	t1, CSR_SEPC
	sd	t1, OFFSET_REG_SEPC(sp)
	csrr	t1, CSR_STVAL
	sd	t1, OFFSET_REG_SBADADDR(sp)
	csrr	t1, CSR_SCAUSE
	sd	t1, OFFSET_REG_SCAUSE(sp)

	csrr	t1, CSR_SSTATUS
	andi	t1, t1, SR_SPP
	bne	t1, zero, Exc_from_S_mode_2

Exc_from_U_mode_2:
	ld	t1, PCB_USER_SP(t0)	/* $t1 := c.r.->user_sp */
	sd	t1, OFFSET_REG_SP(sp)
	sd	sp, PCB_TRAPFRAME(t0)	/* c.r.->trapframe := $sp */
	j	Save_gpr_t0

Exc_from_S_mode_2:
	addi	t2, sp, OFFSET_SIZE	/* $t2 := $sp + sizeof(regs_context_t) */
	sd	t2, OFFSET_REG_SP(sp)	/* Save original $sp in trapframe->sp */
Save_gpr_t0:
	sd	sp, PCB_KERNEL_SP(t0)	/* c.r.->kernel_sp := $sp */
	/* Restore $t0 and save it */
	csrr	t0, CSR_SSCRATCH
	sd	t0, OFFSET_REG_T0(sp)

	/*
	 * Disable user-mode memory access as it should only be set in the
	 * actual user copy routines.
	 *
	 * Disable the FPU to detect illegal usage of floating point in kernel
	 * space.
	 */
	 /* See riscv-privileged-20211203.pdf: Page 79, 4.1.1.2 and Page 40, 3.1.1.6 */
	//li t0, SR_SUM | SR_FS
	//csrc	CSR_SSTATUS, t0

.endm

/*
 * RESTORE_CONTEXT: When it is returning to U-mode:
 * 1. some CSRs and all GPRs except $sp, $t0 and $t1
 * are restored from *(regs_context_t*)$sp (`trapframe` of PCB is not used);
 * 2. $sp are incresed by sizeof(regs_context_t) and then saved in `kernel_sp`;
 * 3. $sp is set to `user_sp` of PCB;
 * 4. $t0 and $t1 are restored;
 * However, when it is returning to S-mode,
 * only 1, 2 and 4 are executed, which means that $sp is not set as `user_sp`.
 */
.macro RESTORE_CONTEXT
	/* TODO: Restore all general purpose registers and sepc, sstatus */
	/* HINT: Pay attention to sp again! */
	GET_CURRENT_RUNNING_TO_T0

	//ld	sp, PCB_TRAPFRAME(t0)	/* $sp := c.r.->trapframe */

	/*
	 * $sstatus, $sepc and $scause should also be restored.
	 * $scause is not necessary UP TO NOW.
	 */
	ld	t1, OFFSET_REG_SSTATUS(sp)
	ld	t2, OFFSET_REG_SEPC(sp)
	ld	t3, OFFSET_REG_SCAUSE(sp)
	csrw	CSR_SSTATUS, t1
	csrw	CSR_SEPC, t2
	csrw	CSR_SCAUSE, t3

	/* Restore GPRs */
	//ld	zero, OFFSET_REG_ZERO(t0)
	ld	ra, OFFSET_REG_RA(sp)
	//ld	sp, OFFSET_REG_SP(sp)
	ld	gp, OFFSET_REG_GP(sp)
	ld	tp, OFFSET_REG_TP(sp)
	//ld	t0, OFFSET_REG_T0(sp)
	//ld	t1, OFFSET_REG_T1(sp)
	ld	t2, OFFSET_REG_T2(sp)
	ld	s0, OFFSET_REG_S0(sp)
	ld	s1, OFFSET_REG_S1(sp)
	ld	a0, OFFSET_REG_A0(sp)
	ld	a1, OFFSET_REG_A1(sp)
	ld	a2, OFFSET_REG_A2(sp)
	ld	a3, OFFSET_REG_A3(sp)
	ld	a4, OFFSET_REG_A4(sp)
	ld	a5, OFFSET_REG_A5(sp)
	ld	a6, OFFSET_REG_A6(sp)
	ld	a7, OFFSET_REG_A7(sp)
	ld	s2, OFFSET_REG_S2(sp)
	ld	s3, OFFSET_REG_S3(sp)
	ld	s4, OFFSET_REG_S4(sp)
	ld	s5, OFFSET_REG_S5(sp)
	ld	s6, OFFSET_REG_S6(sp)
	ld	s7, OFFSET_REG_S7(sp)
	ld	s8, OFFSET_REG_S8(sp)
	ld	s9, OFFSET_REG_S9(sp)
	ld	s10, OFFSET_REG_S10(sp)
	ld	s11, OFFSET_REG_S11(sp)
	ld	t3, OFFSET_REG_T3(sp)
	ld	t4, OFFSET_REG_T4(sp)
	ld	t5, OFFSET_REG_T5(sp)
	ld	t6, OFFSET_REG_T6(sp)

	addi	sp, sp, OFFSET_SIZE	/* $sp := $sp + sizeof(regs_context_t) */
	sd	sp, PCB_KERNEL_SP(t0)	/* c.r.->kernel_sp := $sp */

	csrr	t1, CSR_SSTATUS
	andi	t1, t1, SR_SPP
	bne	t1, zero, Ret_to_S_mode
Ret_to_U_mode:
	ld	sp, PCB_USER_SP(t0)	/* $sp := c.r.->user_sp */
	ld	t0, PCB_TRAPFRAME(t0)	/* $t0 := c.r.->trapframe */
	ld	t1, OFFSET_REG_T1(t0)
	ld	t0, OFFSET_REG_T0(t0)	/* $t0 := c.r.->trapframe->regs[#t0] */
	j	Ret_continue
Ret_to_S_mode:	/* Don't load `user_sp`of PCB! */
	ld	t0, (OFFSET_REG_T0 - OFFSET_SIZE)(sp)
	ld	t1, (OFFSET_REG_T1 - OFFSET_SIZE)(sp)
Ret_continue:
.endm

ENTRY(enable_preempt)
	not t0, x0
	csrs CSR_SIE, t0
	jr ra
ENDPROC(enable_preempt)

ENTRY(disable_preempt)
	csrw CSR_SIE, zero
	jr ra
ENDPROC(disable_preempt)

ENTRY(enable_interrupt)
	li t0, SR_SIE
	csrs CSR_SSTATUS, t0
	jr ra
ENDPROC(enable_interrupt)

ENTRY(disable_interrupt)
	li t0, SR_SIE
	csrc CSR_SSTATUS, t0
	jr ra
ENDPROC(disable_interrupt)

// NOTE: the address of previous pcb in a0
// NOTE: the address of next pcb in a1
/* The type of arguments has been modified from pcb_t* to switchto_context_t* */
// extern void switch_to(switchto_context_t *prev, switchto_context_t *next);
ENTRY(switch_to)

	/* TODO: [p2-task1] save all callee save registers on kernel stack,
	 * see the definition of `struct switchto_context` in sched.h*/
	sd	ra, SWITCH_TO_RA(a0)
	sd	sp, SWITCH_TO_SP(a0)
	sd	s0, SWITCH_TO_S0(a0)
	sd	s1, SWITCH_TO_S1(a0)
	sd	s2, SWITCH_TO_S2(a0)
	sd	s3, SWITCH_TO_S3(a0)
	sd	s4, SWITCH_TO_S4(a0)
	sd	s5, SWITCH_TO_S5(a0)
	sd	s6, SWITCH_TO_S6(a0)
	sd	s7, SWITCH_TO_S7(a0)
	sd	s8, SWITCH_TO_S8(a0)
	sd	s9, SWITCH_TO_S9(a0)
	sd	s10, SWITCH_TO_S10(a0)
	sd	s11, SWITCH_TO_S11(a0)

	/* TODO: [p2-task1] restore all callee save registers from kernel stack,
	 * see the definition of `struct switchto_context` in sched.h*/
	ld	ra, SWITCH_TO_RA(a1)
	ld	sp, SWITCH_TO_SP(a1)
	ld	s0, SWITCH_TO_S0(a1)
	ld	s1, SWITCH_TO_S1(a1)
	ld	s2, SWITCH_TO_S2(a1)
	ld	s3, SWITCH_TO_S3(a1)
	ld	s4, SWITCH_TO_S4(a1)
	ld	s5, SWITCH_TO_S5(a1)
	ld	s6, SWITCH_TO_S6(a1)
	ld	s7, SWITCH_TO_S7(a1)
	ld	s8, SWITCH_TO_S8(a1)
	ld	s9, SWITCH_TO_S9(a1)
	ld	s10, SWITCH_TO_S10(a1)
	ld	s11, SWITCH_TO_S11(a1)
/*
 * The guide_book_p2 Page5.2 says that current_running
 * should be saved to tp.
 * NOTE: please ensure that the current_running points to
 * the new process's PCB before calling switch_to.
 */
	GET_CURRENT_RUNNING_TO_T0
	mv	tp, t0
	/* Update $satp and refresh TLB */
	csrr	t0, satp
	li	t1, (1 << 60) - 1
	not	t1, t1
	and	t0, t1, t0	/* Clear ASID and PPN */
	lwu	t2, PCB_PID(tp)	
	sll	t2, t2, 44
	sll	t1, t1, 60
	not	t1, t1
	and	t2, t1, t2
	or	t0, t2, t0	/* Set ASID, or PID */
	lwu	t2, PCB_PGDIR(tp) /* use lwu to load physical addr */
	srl	t2, t2, 12	/* Physical page number */
	or	t0, t2, t0
	csrw	satp, t0
	sfence.vma
	fence.i
	ret

ENDPROC(switch_to)

ENTRY(ret_from_exception)
	/* TODO: [p2-task3] restore context via provided macro and return to sepc */
	/* HINT: remember to check your sp, does it point to the right address? */

	/*
	 * Should I enable interrupt by setting
	 * SIE of $sstatus manully?
	 */
	//call	enable_interrupt
	ld	t2, OFFSET_REG_SCAUSE(sp)	/* $t2 := c.r.->trapframe->scause */
	li	t1, 8
	bne	t2, t1, NOT_SYSCALL

//IS_SYSCALL:
	/* sepc += 4 */
	ld	t3, OFFSET_REG_SEPC(sp)	/* $t3:= c.r.->trapframe->sepc */
	addi	t3, t3, 4
	sd	t3, OFFSET_REG_SEPC(sp)
NOT_SYSCALL:
	/*
	 * If a trap is taken from S-mode for a user process,
	 * do not unlock kernel as we are returning to S-mode.
	 */
	GET_CURRENT_RUNNING_TO_T0
	lw	t0, PCB_PID(t0)
	addi	t1, zero, 2
	blt	t0, t1, Unlock_kernel

	//csrr	t2, CSR_SSTATUS
	ld	t2, OFFSET_REG_SSTATUS(sp)
	andi	t2, t2, SR_SPP
	beq	t2, zero, Unlock_kernel
	
	j	Bypass_unlock
Unlock_kernel:
	call	unlock_kernel
Bypass_unlock:
	RESTORE_CONTEXT
	sret

ENDPROC(ret_from_exception)

/*
 * Using global kernel spin lock:
 * Note that RESTORE_CONTEXT and SVAE_CONTEXT are not
 * included in critical section, so we must ensure that
 * one CPU canNOT modify the PCB pointed by current_running
 * of another CPU except `next` and `status`.
 */

ENTRY(exception_handler_entry)

	/* TODO: [p2-task3] save context via the provided macro */
	SAVE_CONTEXT

	GET_CURRENT_RUNNING_TO_T0
	lw	t0, PCB_PID(t0)
	addi	t1, zero, 2
	blt	t0, t1, Lock_kernel

	csrr	t2, CSR_SSTATUS
	andi	t2, t2, SR_SPP
	beq	t2, zero, Lock_kernel
	
	j	Bypass_lock
	/*
	 * If a trap is taken from S-mode for a user process,
	 * do not lock kernel as it is already locked.
	 */
Lock_kernel:
	call	lock_kernel
Bypass_lock:

	/* TODO: [p2-task3] load ret_from_exception into $ra so that we can return to
	 * ret_from_exception when interrupt_help complete.
	 */
	la	ra, ret_from_exception
	addi	sp, sp, -16
	sd	ra, 0(sp)

	/* TODO: [p2-task3] call interrupt_helper
	 * NOTE: don't forget to pass parameters for it.
	 */
	GET_CURRENT_RUNNING_TO_T0
	ld	a0, PCB_KERNEL_SP(t0)	/* $a0 := c.r.->kernel_sp */
	/*
	 * Shouldn't be c.r.->trapframe! Because when trap is taken from
	 * S-mode, `trapframe` is not kept as $sp!
	 */
	ld	a1, OFFSET_REG_SBADADDR(a0)
	ld	a2, OFFSET_REG_SCAUSE(a0)
	call	interrupt_helper

	ld	ra, 0(sp)
	addi	sp, sp, 16
	jr	ra
ENDPROC(exception_handler_entry)
