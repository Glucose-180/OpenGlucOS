#include <sys/syscall.h>
#include <os/glucose.h>
#include <printk.h>
#include <os/smp.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
	/* TODO: [p2-task3] handle syscall exception */
	/**
	 * HINT: call syscall function like syscall[fn](arg0, arg1, arg2),
	 * and pay attention to the return value and sepc
	 */
	long sysno = regs->regs[17];	/* Use a7 as syscall number */
	long *args = (long *)(regs->regs + 10);
	long (*func)(long, long , long, long, long);
	long rt;
	if (sysno >= NUM_SYSCALLS || (void *)syscall[sysno] == (void *)invalid_syscall)
		invalid_syscall(sysno, args[0], args[1], args[2], args[3], args[4]);
	/*else if (interrupt != regs->sepc)
		panic_g("handle_syscall: stval 0x%lx is not equals to trapframe->sepc 0x%lx",
			interrupt, regs->sepc);*/
	func = (long (*)(long, long, long, long, long))(syscall[sysno]);
	rt = func(args[0], args[1], args[2], args[3], args[4]);
	args[0] = rt;	/* a0 := return value */
}

void invalid_syscall(long sysno, long arg0, long arg1, long arg2, long arg3, long arg4)
{
	if (cur_cpu()->pid != 0 && cur_cpu()->pid != 1)
	{
		printk("**Invalid syscall %ld\n", sysno);
		do_exit();
	}
	else
		panic_g(
			"invalid_syscall: invalid syscall %ld of pid 0\n"
			"arg0: 0x%lx\targ1: 0x%lx\targ2: 0x%lx\n"
			"arg3: 0x%lx\targ4: 0x%lx\n",
			sysno, arg0, arg1, arg2, arg3, arg4
		);
}
