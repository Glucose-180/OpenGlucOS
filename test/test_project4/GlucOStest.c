#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define NOI (-1)	/* No input */

void test_access_kernel(void);
void test_syscall(void);
void test_unaligned(void);
void test_zerodivision(void);
void test_stack(void);
void test_sbrk(void);
void test_argv(int argc, char *argv[]);
void test_getpid(void);
void test_exit(void);
void test_illegalinst(void);
void test_access_wrong_addr_in_kernel(void);

int main(int argc, char *argv[])
{
	int c;
	int flag_retry;

	printf(
		"== This program is to test the Operating System ==\n"
		"\t0 - Try to access kernel memory\n"
		"\t1 - Check GPRs after a Syscall\n"
		"\t2 - Try to access an unaligned address\n"
		"\t3 - Use big stack\n"
	);
	printf(
		"\t4 - Call sys_sbrk() and access\n"
		"\t5 - Command line args\n"
		"\t6 - Call sys_getpid()\n"
		"\t7 - Call sys_exit()\n"
	);
	printf(
		"\t8 - Illegal instruction\n"
		"\t9 - Access wrong address in kernel\n"
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
			test_stack();
			break;
		case 4:
			test_sbrk();
			break;
		case 5:
			test_argv(argc, argv);
			break;
		case 6:
			test_getpid();
			break;
		case 7:
			test_exit();
			break;
		case 8:
			test_illegalinst();
			break;
		case 9:
			test_access_wrong_addr_in_kernel();
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
	long *p = (void *)0xffffffc052000000;
	printf("Load from 0x%lx: 0x%lx.\n", (long)p, *p);
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
	int ar[4];
	long *p = (long *)((char *)ar + 1);
	long d;

	printf("Trying to access 0x%lx...\n", (long)p);
	__asm__ volatile
	(
		"li		t5, 5033\n\t"
		"sd		t5, 0(%1)\n\t"
		"ld		%0, 0(%1)"
		: "=r" (d)
		: "r" (p)
		: "t5"
	);
	if (d == 5033)
		printf("Test passed!\n");
	else
		printf("Test failed: %ld\n", d);
}

void test_stack(void)
{
	char buf[4096];
	uint64_t sp;

	strcpy(buf, "Test buffer bigger than 1 page passed!\n");
	buf[4095] = '\0';

	__asm__ volatile(
		"mv		%0, sp\n\t"
		: "=r"(sp)
		:
	);

	printf("%s$sp is 0x%lx.\n", buf, sp);

	/* Try to increase the stack by 3 pages */
	__asm__ volatile(
		"li		t0, 12288\n\t"
		"sub	sp, sp, t0\n\t"
		"sd		t0, 0(sp)\n\t"
		"add	sp, sp, t0\n\t"
		:
		:
		: "memory", "t0"
	);
	printf("Test failed! No exception happened.\n");
}

void test_sbrk()
{
	const unsigned long Size = 1024UL * 1024UL;	/* 1MiB */
	volatile char *p = sys_sbrk(Size);

	printf("sys_sbrk(1 MiB) returns 0x%lx\n", (long)p);
	if (p != (void *)0L)
	{
		p[0] = 'g';
		p[Size - 1U] = 'e';
		if (p[0] == 'g' && p[Size - 1U] == 'e')
			printf("Test 1 MiB passed!\n");
		else
			printf("Test 1 MiB failed!\n");
	}
	p = sys_sbrk(Size * 1024UL);
	printf("sys_sbrk(1 GiB) returns 0x%lx\n", (long)p);
	if (p != (void *)0L)
	{
		p[Size] = 'g';
		p[Size * 1024U - 1U] = 'e';
		if (p[Size] == 'g' && p[Size * 1024U - 1U] == 'e')
			printf("Test 1 GiB passed!\n");
		else
			printf("Test 1 GiB failed!\n");
	}
}

void test_argv(int argc, char *argv[])
{
	int i;
	
	printf("argc is %d\n", argc);
	for (i = 0; i < argc; ++i)
	{
		printf("argv[%d] is \"%s\"\n", i, argv[i]);
	}
}

void test_getpid(void)
{
	pid_t pid;

	pid = sys_getpid();
	printf("pid is %d\n", pid);
}

void test_exit(void)
{
	sys_exit();
	printf("Failed!\n");
}

void test_illegalinst(void)
{
	__asm__ volatile (
		"csrs		sie, zero\n\t"
		"fadd.s		f2, f1, f0\n\t"
	);
}

void test_access_wrong_addr_in_kernel(void)
{
	sys_write((char *)0xf00010000UL);
}