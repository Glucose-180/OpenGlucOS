#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int loc = 1;
	unsigned int size = 2U * 1024U * 1024U;	/* 2 MiB */
	long *pshm;

	if (argc > 1)
		loc = atoi(argv[1]);
	if (argc > 2)
		size = atoi(argv[2]);
	sys_sbrk(size);
	pshm = sys_shmpageget(1);
	sys_move_cursor(0, loc);
	printf("Shared page at 0x%lx: 0x%lx", (long)pshm, *pshm);
	*pshm = 0x12306L;
	sys_sleep(10);
	sys_move_cursor(0, loc);
	printf("Exited!                           ");
	return 0;
}
