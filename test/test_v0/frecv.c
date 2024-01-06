/*
 * This program is used to receive a file and
 * save it in the GFS (file system).
 * Usage: frecv -l<len> file
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

const char *fname;
int len;

int main(int argc, char *argv[])
{
	char *buf;
	int rrt, wrt, fd;

	while (--argc)
	{
		++argv;
		if (strncmp(*argv, "-l", 2) == 0)
			len = atoi(*argv + 2);
		else
			fname = *argv;
	}

	if (fname == NULL || len <= 0)
	{
		printf("Usage: frecv -l<len> file\n");
		return 2;
	}

	if ((buf = malloc(len)) == NULL)
	{
		printf("Failed to allocate %d bytes buffer\n", len);
		return 2;
	}

	if ((fd = sys_open(fname, O_RDWR | O_CREAT)) < 0)
	{
		printf("Failed to open file %s\n", fname);
		return 3;
	}

	printf("Waiting for %d bytes data...\n", len);
	if ((rrt = sys_net_recv_stream((void*)buf, len)) > 0)
	{
		buf += 4;
		printf("%d bytes are received.\n"
			"Saving %d bytes to file %s...\n", rrt, rrt - 4, fname);
		rrt -= 4;
		wrt = sys_write(fd, buf, rrt);
		if (wrt == rrt)
			printf("%d bytes have been written!\n", wrt);
		else
			printf("Failed to write %d bytes: %d\n", rrt, wrt);
		sys_close(fd);
		return (rrt == wrt ? 0 : 1);
	}
	else
	{
		printf("Failed to receive %d bytes: %d\n", len, rrt);
		sys_close(fd);
		return 5;
	}
}