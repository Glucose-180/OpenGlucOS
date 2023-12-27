#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	char name[20];
	int n, rt;

	if (argc < 3)
	{
		printf("Usage: mkdir-test [path] [count]\n");
		return 1;
	}
	if (sys_changedir(argv[1]) != 0)
	{
		printf("**Error: failed to cd %s\n", argv[1]);
		return 2;
	}
	n = atoi(argv[2]);
	while (n-- > 0)
	{
		itoa(n, name, 20, 10);
		sys_move_cursor(0, 0);
		printf("Testing mkdir %s/%s...", argv[1], name);
		if ((rt = sys_mkdir(name)) != 0)
		{
			printf("**Error: test mkdir %s failed: %d\n", name, rt);
			return 3;
		}
	}
	printf("\nTest pass!\n");
	return 0;
}
