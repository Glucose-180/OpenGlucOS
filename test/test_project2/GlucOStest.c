#include <stdio.h>
#include <unistd.h>

/*
 * Using this file is dangerous.
 * Only for test.
 */
#include <kernel.h>

#define NOI (-1)	/* No input */

void test_access_kernel(void);
void test_syscall(void);
void test_unaligned(void);
void test_zerodivision(void);
void test_printf(void);

int main()
{
	int c;
	int flag_retry;

	printf(
		"== This program is to test the Operating System ==\n"
		"\t0 - Try to access kernel memory\n"
		"\t1 - Check GPRs after a Syscall\n"
		"\t2 - Try to access an unaligned address\n"
		"\t3 - Try to divide an integer by ZERO\n"
		"\t4 - printf different kinds of integers\n"
	);
	while (1)
	{
		while ((c = sys_bios_getchar()) == NOI)
			;
		flag_retry = 0;
		switch (c - '0')
		{
		case 0:
			test_access_kernel();
			break;
		case 1:
			test_syscall();
			break;
		case 2:
			test_unaligned();
			break;
		case 3:
			test_zerodivision();
			break;
		case 4:
			test_printf();
			break;
		default:
			flag_retry = 1;
			printf("**Error: invalid input %c, try arain\n", c);
			break;
		}
		if (flag_retry == 0)
			sys_yield();
	}
}

void test_access_kernel(void)
{
	/*
	 * Call jump table in kernel memory.
	 * See whether exception will happen.
	 */
	bios_putchar('x');
	printf("Test failed! No exception happened.\n");
}

void test_syscall(void)
{
	long sysno = 23;	/* sys_reflush */
	long res;
	__asm__ volatile
	(
		/* 
		 * Write some data to
		 * GPRs and *sp.
		 */
		"li		s2, 2\n\t"
		"li		s8, 8\n\t"
		"li		t0, 10\n\t"
		"li		t5, 15\n\t"
		"li		a2, 22\n\t"
		"addi	sp, sp, -16\n\t"
		"sd		s2, 0(sp)\n\t"
		/*
		 * Do syscall: reflush
		 */
		"mv		a7, %1\n\t"
		"ecall\n\t"
		/*
		 * Check whether these data
		 * are restored.
		 */
		"mv		%0, zero\n\t"
		"xori	s2, s2, 2\n\t"
		"or		%0, %0, s2\n\t"
		"xori	s8, s8, 8\n\t"
		"or		%0, %0, s8\n\t"
		"xori	t0, t0, 10\n\t"
		"or		%0, %0, t0\n\t"
		"xori	t5, t5, 15\n\t"
		"or		%0, %0, t5\n\t"
		"xori	a2, a2, 22\n\t"
		"or		%0, %0, a2\n\t"
		"ld		s2, 0(sp)\n\t"
		"xori	s2, s2, 2\n\t"
		"or		%0, %0, s2\n\t"
		"addi	sp, sp, 16"
		: "=r"(res)
		: "r"(sysno)
		: "s2", "s8", "t0", "t5", "a2"
	);
	if (res != 0)
		printf("Test failed: %lx\n", res);
	else
		printf("Test passed!\n");
}

void test_unaligned(void)
{
	int ar[2];
	int *p = (int *)((char *)ar + 1);

	printf("Trying to access 0x%lx...\n", (long)p);
	__asm__ volatile
	(
		"sw		zero, 0(%0)\n\t"
		"lw		t0, 0(%0)"
		:
		: "r"(p)
		: "t0"
	);
	printf("Test failed! No exception happened.\n");
}

void test_zerodivision(void)
{
	int dividend = 5, quo;

	quo = dividend / 0;
	if (quo == -1)
		printf("Test passed! The quotient is -1.\n");
	else
		printf("Test failed! No exception happened "
			"and the quotient is %d.\n", quo);
}

void test_printf(void)
{
	int iv = -5033;
	unsigned int uiv = 2147483648;
	long lv = -4294967296;
	unsigned long ulv = 4294967296;

	printf(
		"(int)-5033                 : %d\n"
		"(unsigned)2147483648       : %u\n"
		"(long)-4294967296          : %ld\n"
		"(unsigned long)4294967296  : %lu\n",
		iv, uiv, lv, ulv
		);
}