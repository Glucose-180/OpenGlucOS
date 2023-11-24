#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

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
		p[i * step] = 'A' + i;
	}
	for (i = 0; i < rwpgs; ++i)
		if (p[i * step] != 'A' + i)
		{
			printf("Test failed at %d: 0x%lx!\n", i, (long)p);
			return 1;
		}
	printf("Test passed!\n");
	return 0;
}