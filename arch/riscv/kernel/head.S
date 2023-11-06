/*
 * head.S: The boot loader will execute code below first.
 * This program clear .bss segment and reset sp, then jump to
 * main() of GlucOS.
*/

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

#define KERNEL_STACK		0x50500000
#define KERNEL_STACK_S		0x50400000	/* for secondary CPU */

.section ".entry_function","ax"
ENTRY(_start)
	/* Mask all interrupts */
	csrw	CSR_SIE, zero
	csrw	CSR_SIP, zero

	csrr	t0, CSR_MHARTID
	bne		t0, zero, secondary

	/* TODO: [p1-task2] clear BSS for flat non-ELF images */
	la	t1, __bss_start	# the start of .bss is the ADDRESS of __bss_start,
	la	t2, __BSS_END__	# rather than the value at __bss_start!
	add	t0, zero, t1
LC:
	sltu	t3, t0, t2
	beq	zero, t3, LE
	sw	zero, 0(t0)
	addi	t0, t0, 4
	j	LC
LE:

	/* TODO: [p1-task2] setup C environment */
	li	sp, KERNEL_STACK
	mv	fp, sp
	jal	main	# jump to main() of GlucOS
loop:
	wfi
	j loop

secondary:
	li	sp, KERNEL_STACK_S
	mv	fp, sp
	jal	main_s
loop_s:
	wfi
	j	loop_s

END(_start)