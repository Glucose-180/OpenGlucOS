#include <asm/biosdef.h>
#include <csr.h>

# os size location (os_size could be sectors [p1-task3] or bytes [p1-task4])
.equ os_size_loc, 0x50200104 # address of kernel size, unit: sector

# kernel address (move kernel to here, and jmp here to start kernel)
.equ kernel, 0x50202000

# BIOS function entry (jump here to use BIOS APIs)
.equ bios_func_entry, 0x50150000

.text
.global main

main:
	# fence on all memory and I/O
	fence
    // a0 is mhartid
	bnez a0, secondary

	/* TODO: [p1-task1] call BIOS to print string "It's bootblock!" */
	addi	a7, zero, BIOS_PUTSTR
	la	a0, msg
	jal	bios_func_entry


	/* TODO: [p1-task2] call BIOS to read kernel in SD card */
	li	a0, os_size_loc
	lhu	a1, 0(a0)	# num_of_blocks
	li	a7, BIOS_SDREAD
	li	a0, kernel	# mem_address
	li	a2, 1		# block_id
	jal	bios_func_entry


	/* TODO: [p1-task4] load task-related arguments and pass them to kernel */


	/* TODO: [p1-task2] jump to kernel to start UCAS-OS */
	jal	kernel
	/************************************************************/
secondary:
	/* TODO [P3-task3]: 
	 * 1. Mask all interrupts
	 * 2. let stvec pointer to kernel_main
	 * 3. enable software interrupt for ipi
	 */
	csrw	CSR_SIE, zero
	csrw	CSR_SIP, zero

	li	t0, kernel
	not	t1, zero
	slli	t1, t1, 2	/* $t1 := 0b1...100 */
	and	t0, t0, t1	/* Clear low 2 bits to represent Direct mode */
	csrw	CSR_STVEC, t0

	li	t0, SR_SSIE
	csrs	CSR_SIE, t0
	li	t0, SR_SIE
	csrs	CSR_SSTATUS, t0

wait_for_wakeup:
	wfi
	j wait_for_wakeup

	/************************************************************/
/* while(1) --> stop here */
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

