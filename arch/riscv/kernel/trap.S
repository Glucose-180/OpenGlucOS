#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)

	/* TODO: [p2-task3] save exception_handler_entry into STVEC */
	la	t0, exception_handler_entry
	not	t1, zero
	slli	t1, t1, 2	/* $t1 := 0b1...100 */
	and	t0, t0, t1	/* Clear low 2 bits to represent Direct mode */
	csrw	CSR_STVEC, t0

	/* TODO: [p2-task4] enable interrupts globally */
	addi	sp, sp, -16
	sd	ra, 0(sp)
	jal enable_interrupt

	ld	ra, 0(sp)
	addi	sp, sp, 16
	jr	ra
ENDPROC(setup_exception)
