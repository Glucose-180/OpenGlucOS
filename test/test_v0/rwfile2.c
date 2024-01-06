/*
 * rwfile2 -w/-r [file_name] -n[size_in_KiB]/-m[size_in_MiB]
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

/* Size of the buffer, or R/W one time */
#define BS_K 1024
#define BS_M (BS_K*1024)

char junk = 0x5;
const char *fname;
char flag_r = 1, flag_w = 0, flag_mib = 0;
int32_t size, unit;
char *buf;

int main(int argc, char *argv[])
{
	int fd, rt, i, j;
	uint64_t time_base, tk0, tused = 0UL;

	time_base = (uint64_t)sys_get_timebase();
	while (--argc)
	{
		++argv;
		if (strcmp(*argv, "-w") == 0)
			flag_w = 1, flag_r = 0;
		else if (strcmp(*argv, "-r") == 0)
			flag_r = 1, flag_w = 0;
		else if (strncmp(*argv, "-n", 2) == 0)
			size = atoi(*argv + 2);
		else if (strncmp(*argv, "-m", 2) == 0)
		{
			if ((*argv)[2] != '\0')
				size = atoi(*argv + 2);
			flag_mib = 1;
		}
		else
			fname = *argv;
	}

	if (size <= 0)
		size = (flag_mib ? 1 : 1024);	/* Default size */
	unit = (flag_mib ? BS_M : BS_K);
	buf = malloc(unit);

	if (fname == NULL)
	{
		printf("Usage: rwfile2 -w/-r [file_name] -n[size_in_KiB]/-m[size_in_MiB]\n");
		return 2;
	}
	if ((fd = sys_open(fname, O_RDWR | O_CREAT)) < 0)
	{
		printf("Failed to open file %s: %d\n", fname, fd);
		return 2;
	}

	if (flag_w != 0)
	{
		for (i = 0; i < unit; ++i)
			buf[i] = junk;
		for (i = 0; i < size; ++i)
		{
			sys_move_cursor(0, 1);
			tk0 = clock();
			rt = sys_write(fd, buf, unit);
			tused += clock() - tk0;
			if (rt == unit)
				printf("%d %s have been written\n", i + 1, flag_mib ? "MiB" : "KiB");
			else
			{
				printf("%d: failed to write 1 %s: %d!", i, flag_mib ? "MiB" : "KiB", rt);
				return 3;
			}
		}
	}
	else if (flag_r != 0)
	{
		for (i = 0; i < size; ++i)
		{
			sys_move_cursor(0, 1);
			tk0 = clock();
			rt = sys_read(fd, buf, unit);
			tused += clock() - tk0;
			if (rt == unit)
			{
				for (j = 0; j < unit; ++j)
					if (buf[j] != junk)
						break;
				if (j < unit)
				{
					printf("%d: wrong data at %d: 0x%x", i, j, (int)buf[j]);
					return 4;
				}
				else
					printf("%d %s have been read\n", i + 1, flag_mib ? "MiB" : "KiB");
			}
			else
			{
				printf("%d: failed to read 1 %s: %d!", i, flag_mib ? "MiB" : "KiB", rt);
				return 5;
			}
		}
	}


	rt = sys_close(fd);
	if (rt != fd)
		printf("Close %d failed: %d\n", fd, rt);
	else
		printf("Test passed!\nTime elapsed: %lu ms in file RW\n",
			tused / (time_base / 1000UL));
	return 0;
}