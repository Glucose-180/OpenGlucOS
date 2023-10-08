#include <asm.h>
#include <csr.h>
#include <asm/regs.h>

.macro SAVE_CONTEXT
	/* TODO: [p2-task3] save all general purpose registers here! */
	/* HINT: Pay attention to the function of tp and sp, and save them carefully! */

	sd	t0, -8(sp)	/* Save t0 in user stack temporarily */
	la	t0, current_running
	ld	t0, 0(t0)	/* Now t0 equals current_running, or c.r. */

	sd	sp, PCB_USER_SP(t0)	/* c.r.->user_sp := $sp */
	ld	sp, PCB_KERNEL_SP(t0)	/* $sp := c.r.->kernel_sp */
	addi	sp, OFFSET_SIZE		/* $sp := $sp - sizeof(regs_context_t) */

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
	sd	t1, OFFSET_REG_SCAUSE

	ld	t1, PCB_USER_SP(t0)	/* $t1 := c.r.->user_sp */
	sd	t1, OFFSET_REG_SP(sp)

	sd	sp, PCB_KERNEL_SP(t0)	/* c.r.->kernel_sp := $sp */
	sd	sp, PCB_TRAPFRAME(t0)	/* c.r.->trapframe := $sp */

	/* Restore $t0 and save it */
	//ld	t0, PCB_USER_SP(t0)	/* $t0 := c.r.->user_sp */
	mv	t0, t1	/* $t0 := c.r.->user_sp */
	ld	t0, -8(t0)

	/*
	 * Disable user-mode memory access as it should only be set in the
	 * actual user copy routines.
	 *
	 * Disable the FPU to detect illegal usage of floating point in kernel
	 * space.
	 */
	 /* See riscv-privileged-20211203.pdf: Page 79, 4.1.1.2 and Page 40, 3.1.1.6 */
	li t0, SR_SUM | SR_FS
	csrc	CSR_SSTATUS, t0

	/* TODO: [p2-task3] save sstatus, sepc, stval and scause on kernel stack */

.endm

.macro RESTORE_CONTEXT
	/* TODO: Restore all general purpose registers and sepc, sstatus */
	/* HINT: Pay attention to sp again! */
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
	//addi sp, sp, -(SWITCH_TO_SIZE)

	/* TODO: [p2-task1] save all callee save registers on kernel stack,
	 * see the definition of `struct switchto_context` in sched.h*/
	beq	zero, a0, RS	// a0 (&current_running->context) is NULL
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
RS:
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
	la	t0, current_running
	ld	tp, 0(t0)
	//addi sp, sp, SWITCH_TO_SIZE
	jr	ra
ENDPROC(switch_to)

ENTRY(ret_from_exception)
	/* TODO: [p2-task3] restore context via provided macro and return to sepc */
	/* HINT: remember to check your sp, does it point to the right address? */
ENDPROC(ret_from_exception)

ENTRY(exception_handler_entry)

	/* TODO: [p2-task3] save context via the provided macro */


	/* TODO: [p2-task3] load ret_from_exception into $ra so that we can return to
	 * ret_from_exception when interrupt_help complete.
	 */


	/* TODO: [p2-task3] call interrupt_helper
	 * NOTE: don't forget to pass parameters for it.
	 */


ENDPROC(exception_handler_entry)
