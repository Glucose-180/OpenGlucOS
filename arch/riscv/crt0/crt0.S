/*
 * crt0.S: It will be linked together with a user application.
 * When the app starts, code below will be executed first.
*/
#include <asm.h>

.section ".entry_function","ax"
ENTRY(_start)

	/* TODO: [p1-task3] setup C runtime environment for the user program */

	# clear .bss segment
	la	t1, __bss_start	# the start of .bss is the address of __bss_start,
	la	t2, __BSS_END__	# rather than the value at __bss_start!
	add	t0, zero, t1
LC:
	sltu	t3, t0, t2
	beq	zero, t3, LE
	sw	zero, 0(t0)
	addi	t0, t0, 4
	j	LC
LE:
	addi	sp, sp, -16
	sd	ra, 8(sp)	# save current ra(return address)
	sd	fp, 0(sp)	# save OLD fp
	mv	fp, sp
	/* TODO: [p1-task3] enter main function */
	jal	main		# jump to main() of application

	/* TODO: [p1-task3] finish task and return to the kernel, replace this in p3-task2! */
	mv	sp, fp		# clear app stack
	ld	fp, 0(sp)
	ld	ra, 8(sp)	# recover ra
	addi	sp, sp, 16

	call sys_exit

	/*
	 * If this instruction is executed, error has occurred because
	 * sys_exit() is not successful.
	 */
	jr	ra		# jump to GlucOS
	/************************************************************/
	/* Do not touch this comment. Reserved for future projects. */
	/************************************************************/
# while(1) loop, unreachable here
    /* TODO: [p1-task3] finish task and return to the kernel, replace this in p3-task2! */

    /************************************************************/
    /* TODO: [P3] call sys_exit */
    /************************************************************/
// while(1) loop, unreachable here
loop:
    wfi
    j loop

END(_start)
