#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int rwpgs = 10;
unsigned long size = 1024U * 1024U;	/* 1 MiB */
int step = 4096;	/* page size */

int main(int argc, char *argv[])
{
	char *p;
	int i;

	if (argc > 1)
		rwpgs = atoi(argv[1]);
	sys_move_cursor(0, 2);
	p = sys_sbrk(size);
	printf("Testing reading and writing %d pages at 0x%lx\n",
		rwpgs, (long)p);
	for (i = 0; i < rwpgs; ++i)
	{
		//p[i * step] = 'A' + i;
		strncpy(p + i * step, "Genshin", 8U);
		/*
		 * Because `sys_sbrk()` returns an 8 bytes aligned address,
		 * copy 8 bytes will only write 1 page.
		 */
	}
	for (i = 0; i < rwpgs; ++i)
	{
		//if (p[i * step] != (char)('A' + i))
		sys_move_cursor(0, 3);
		/*
		 * Try to cause page fault after trapped to kernel.
		 */
		//printf("%c%cnshin", p[i * step], p[i * step + 1]);
		sys_write(p + i * step);
		if (strncmp(p + i * step, "Genshin", 8U) != 0)
		{
			printf("\nTest failed at %d: 0x%lx!\n", i, (long)p);
			return 1;
		}
	}
	printf("\nTest passed!\n");
	return 0;
}