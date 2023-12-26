/*
 * Test file operations (lseek() and write()) on Linux.
 * Usage: foptest [file_name] [-w] [-l[location]]
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int flag_wr = 0, seek_loc = 0;

const char *wdata = "tiepi";
const char *fname = "t";
int rdata;

int main(int argc, char *argv[])
{
	int fd, rt;

	while (--argc)
	{
		++argv;
		if (strcmp(*argv, "-w") == 0)
			flag_wr = 1;
		else if (strncmp(*argv, "-l", 2) == 0)
			seek_loc = atoi(*argv + 2);
		else
			fname = *argv;
	}
	fd = sys_open(fname, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		printf("Failed to open file %s: %d", fname, fd);
		return 2;
	}
	rt = sys_lseek(fd, seek_loc, SEEK_SET);
	printf("lseek(%d, %d, SEEK_SET) returned %d\n", fd, seek_loc, rt);
	if (flag_wr != 0)
	{
		rt = sys_write(fd, wdata, 5U);
		printf("write() returned %d\n", rt);
	}
	else
	{	/* read */
		rt = sys_read(fd, (char *)&rdata, 1U);
		printf("read() returned %d\n", rt);
		printf("read data: 0x%x\n", rdata);
	}
	rt = sys_close(fd);
	printf("close() returned %d\n", rt);

	return 0;
}
