#include <asm.h>
#include <csr.h>

ENTRY(get_current_cpu_id)
	csrr a0, CSR_MHARTID
	jr ra
ENDPROC(get_current_cpu_id)

ENTRY(is_scpu)
	csrr	a0, CSR_MHARTID
	beq	a0, zero, L0
	li	a0, 1
L0:
	ret
ENDPROC(is_scpu)