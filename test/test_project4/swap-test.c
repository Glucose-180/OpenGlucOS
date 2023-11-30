#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int rwpgs = 10;
const unsigned long MiB = 1024U * 1024U;	/* 1 MiB */
const int Pgsize = 4096;	/* page size */

int main(int argc, char *argv[])
{
	char *p;
	int i;

	if (argc > 1)
		rwpgs = atoi(argv[1]);
	sys_move_cursor(0, 2);
	p = sys_sbrk(rwpgs * Pgsize);
	if (p == NULL)
	{
		printf("**Failed to sbrk %d pages\n", rwpgs);
		return 1;
	}
	if (rwpgs < 512)
	{	/* size less than 2 MiB */
		printf("Testing reading and writing %d pages at 0x%lx\n",
			rwpgs, (long)p);
		for (i = 0; i < rwpgs; ++i)
		{
			//p[i * Pgsize] = 'A' + i;
			strncpy(p + i * Pgsize, "Genshin", 8U);
			/*
			* Because `sys_sbrk()` returns an 8 bytes aligned address,
			* copy 8 bytes will only write 1 page.
			*/
		}
		for (i = 0; i < rwpgs; ++i)
		{
			//if (p[i * Pgsize] != (char)('A' + i))
			sys_move_cursor(0, 3);
			/*
			* Try to cause page fault after trapped to kernel.
			*/
			//printf("%c%cnshin", p[i * Pgsize], p[i * Pgsize + 1]);
			sys_screen_write(p + i * Pgsize, strlen("Genshin"));
			if (strncmp(p + i * Pgsize, "Genshin", 8U) != 0)
			{
				printf("\n**Test failed at %d: 0x%lx!\n", i, (long)(p + i * Pgsize));
				return 1;
			}
		}
	}
	else
	{
		printf("Testing about %d MiB...\n", rwpgs * Pgsize / MiB);
		for (i = 0; i < rwpgs; ++i)
		{
			p[i * Pgsize] = 'A' + i % 26;
			if ((i % 256) == 0)	// 1 MiB
			{
				sys_move_cursor(0, 3);
				printf("%d MiB has been written...\n", i / 256);
			}
		}
		for (i = 0; i < rwpgs; ++i)
			if (p[i * Pgsize] != (char)('A' + i % 26))
			{
				printf("Test failed at %d: 0x%lx!\n", i, (long)(p + i * Pgsize));
				return 1;
			}
			else if ((i % 256) == 0)
			{
				sys_move_cursor(0, 4);
				printf("%d MiB has been read...\n", i / 256);
			}
				
	}
	printf("\nSwapping test passed!\n");
	return 0;
}