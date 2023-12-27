/*
 * rwfile2 -w/-r [file_name] -n[size_in_KiB]
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/* Size of the buffer, or R/W one time */
#define BS 1024

char junk = 0x5;
const char *fname;
char flag_r = 1, flag_w = 0;
int32_t size = 1024;	/* KiB */
char buf[BS];

int main(int argc, char *argv[])
{
	int fd, rt, i, j;

	while (--argc)
	{
		++argv;
		if (strcmp(*argv, "-w") == 0)
			flag_w = 1, flag_r = 0;
		else if (strcmp(*argv, "-r") == 0)
			flag_r = 1, flag_w = 0;
		else if (strncmp(*argv, "-n", 2) == 0)
			size = atoi(*argv + 2);
		else
			fname = *argv;
	}
	if (fname == NULL)
	{
		printf("Usage: rwfile2 -w/-r [file_name] -n[size_in_KiB]\n");
		return 2;
	}
	if ((fd = sys_open(fname, O_RDWR | O_CREAT)) < 0)
	{
		printf("Failed to open file %s: %d\n", fname, fd);
		return 2;
	}
	if (flag_w != 0)
	{
		for (i = 0; i < BS; ++i)
			buf[i] = junk;
		for (i = 0; i < size; ++i)
		{
			sys_move_cursor(0, 1);
			rt = sys_write(fd, buf, BS);
			if (rt == BS)
				printf("%d * %d Bytes have been written\n", i + 1, BS);
			else
			{
				printf("%d: failed to write %d Bytes: %d!", i, BS, rt);
				return 3;
			}
		}
	}
	else if (flag_r != 0)
	{
		for (i = 0; i < size; ++i)
		{
			sys_move_cursor(0, 1);
			rt = sys_read(fd, buf, BS);
			if (rt == BS)
			{
				for (j = 0; j < BS; ++j)
					if (buf[j] != junk)
						break;
				if (j < BS)
				{
					printf("%d: wrong data at %d: 0x%x", i, j, (int)buf[j]);
					return 4;
				}
				else
					printf("%d * %d Bytes have been read\n", i + 1, BS);
			}
			else
			{
				printf("%d: failed to read %d Bytes: %d!", i, BS, rt);
				return 5;
			}
		}
	}
	rt = sys_close(fd);
	if (rt != fd)
		printf("Close %d failed: %d\n", fd, rt);
	else
		printf("Test passed!\n");
	return 0;
}