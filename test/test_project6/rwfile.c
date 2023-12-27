#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static char buff[64];
const char *fname = NULL;
const char *wdata = "Hello world!\n";

int main(int argc, char *argv[])
{
	int len;
	int rwrt;
	int fd;

	if (argc >= 2)
		fname = argv[1];
	if (argc >= 3)
		wdata = argv[2];

	if (fname == NULL)
	{
		printf("Usage: rwfile [file_name] [writedata]");
		return 2;
	}

	len = strlen(wdata);
	//int fd = sys_fopen("1.txt", O_RDWR);
	fd = sys_open(fname, O_RDWR | O_CREAT);

	printf("sys_open(\"%s\") returns %d\n", fname, fd);
	if (fd < 0)
		return 1;

	// write 'hello world!' * 10
	for (int i = 0; i < 10; i++)
	{
		//sys_fwrite(fd, "hello world!\n", 13);
		rwrt = sys_write(fd, wdata, len);
		if (rwrt != len)
		{
			printf("%d: sys_write(\"%s\") returns %d\n", i, wdata, rwrt);
			return 2;
		}
	}
	rwrt = sys_close(fd);
	if (rwrt != fd)
		printf("sys_close(%d) returns %d\n", rwrt);

	fd = sys_open(fname, O_RDWR);

	printf("sys_open(\"%s\") returns %d\n", fname, fd);
	if (fd < 0)
		return 1;
	// read
	for (int i = 0; i < 10; i++)
	{
		//sys_fread(fd, buff, 13);
		rwrt = sys_read(fd, buff, len);
		if (rwrt != len)
		{
			printf("%d: sys_read() returns %d\n", i, rwrt);
			return 3;
		}
		buff[len] = '\0';
		printf("%s", buff);
	}

	//sys_fclose(fd);
	rwrt = sys_close(fd);
	if (rwrt != fd)
		printf("sys_close(%d) returns %d\n", rwrt);
	return 0;
}