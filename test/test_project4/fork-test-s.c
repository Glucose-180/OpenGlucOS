/*
 * Simple fork test by Glucose180 for GlucOS.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char str[30];

int main(int argc, char *argv[])
{
	int loc = 2;
	pid_t pid;

	sys_move_cursor(0, loc);
	pid = sys_fork();
	if (pid < 0)
	{
		printf("sys_fork() failed\n");
		return 1;
	}
	if (pid > 0)
		strcpy(str, "I am parent process of %d");
	else
		strcpy(str, "\n\nI am child process got %d");

	printf(str, pid);
	return 0;
}
