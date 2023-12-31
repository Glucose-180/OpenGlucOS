/*
 * Test file operations (lseek() and write()).
 * Usage: foptest [file_name] [-w] [-l[location]] [-n[cnt]] -p[PLOC]
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int flag_wr = 0, seek_loc = 0;

const char Wdata[] = "tiepi";
const int Len = sizeof(Wdata) - 1;
const char *fname;
/* Write bytes count */
int cnt = Len;
int ploc = 9;

int main(int argc, char *argv[])
{
	int fd, rt, i, j;
	char *buf;

	while (--argc)
	{
		++argv;
		if (strcmp(*argv, "-w") == 0)
			flag_wr = 1;
		else if (strncmp(*argv, "-l", 2) == 0)
			seek_loc = atoi(*argv + 2);
		else if (strncmp(*argv, "-n", 2) == 0)
			cnt = atoi(*argv + 2);
		else if (strncmp(*argv, "-p", 2) == 0)
			ploc = atoi(*argv + 2);
		else
			fname = *argv;
	}

	if (fname == NULL || cnt <= 0 || seek_loc < 0)
	{
		printf("Usage: foptest [file_name] [-w] [-l[location]] [-n[cnt]] -p[PLOC]\n");
		return 2;
	}

	sys_move_cursor(0, 0);
	sys_set_cylim(2, ploc);

	buf = malloc(cnt);
	if (buf == NULL)
	{
		printf("Failed to allocate %d bytes buffer\n", cnt);
		return 2;
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
	/* Fill the buffer with `Wdata[]` */
		for (i = j = 0; i < cnt / Len; ++i)
			strcpy(buf + Len * i, Wdata), j += Len;
		for (i = j; i < cnt; ++i)
			buf[i] = Wdata[j % Len];

		rt = sys_write(fd, buf, cnt);
		printf("write() returned %d\n", rt);
	}
	else
	{	/* read */
		rt = sys_read(fd, buf, cnt);
		printf("read() returned %d\n", rt);
		printf("read data: \n");
		sys_screen_write(buf, cnt);
	}
	rt = sys_close(fd);
	printf("\nclose() returned %d\n", rt);

	return 0;
}
