#include <stdio.h>
#include <stdint.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

const unsigned long MiB = 1024UL * 1024UL;	/* 1MiB */

int main(int argc, char* argv[])
{
	assert(argc > 1);
	srand(clock());
	long mem2 = 0;
	uintptr_t mem1 = 0;
	int i;
	volatile char *p;

	sys_move_cursor(2, 2);
	/* Try 1 GiB */
	p = sys_sbrk(1024U * MiB);
	printf("sys_sbrk(1 GiB) returns 0x%lx\n", (long)p);
	if (p == (void *)0L)
	{	/* If failed, try 2 MiB */
		p = sys_sbrk(2U * MiB);
		printf("sys_sbrk(2 MiB) returns 0x%lx\n", (long)p);
	}

	for (i = 1; i < argc; i++)
	{
		mem1 = atol(argv[i]);
		// sys_move_cursor(2, curs+i);
		mem2 = rand();
		*(long*)mem1 = mem2;
		printf("0x%lx, %ld\n", mem1, mem2);
		if (*(long*)mem1 != mem2) {
			printf("Error!\n");
		}
	}
	//Only input address.
	//Achieving input r/w command is recommended but not required.
	printf("Success!\n");
	return 0;
}
