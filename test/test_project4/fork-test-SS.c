/*
 * Complex fork test by Glucose180 for GlucOS:
 * Shared memory, and swapping, and multithreading.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *str[] = {
	"This train is bound for Universal Resort.",
	"The next station is Gongzhufen.",
	"Change here for Line 10.",
	"We are now arriving at Gongzhufen.",
	"                                           "
};
int loc = 0;

char *test_fork(void);

int main(int argc, char *argv[])
{
	char *p;
	int barrier;
	// Open shared memory;
	// Start swap-test.
	barrier = sys_barrier_init(1, 4);
	if (barrier < 0)
	{
		printf("Barrier failed.");
		return 1;
	}
	p = test_fork();
	sys_move_cursor(3, loc);
	if (p == NULL)
	{
		printf("Test failed.");
		return 1;
	}
	else
	{
		printf("%s", p);
		sys_barrier_wait(barrier);
		sys_sleep(2);
		sys_move_cursor(3, loc);
		printf("Exited!%s", str[4]);
		return 0;
	}
}

/*
 * Return a valid pointer to string
 * or NULL on error.
 */
char *test_fork(void)
{
	char *p;
	pid_t pid0, pid1, pid2;

	pid0 = sys_getpid();
	if ((p = malloc(64U)) == NULL)
		return NULL;
	if ((pid1 = sys_fork()) < 0)
		return NULL;	/* Failed */
	else if (pid1 > 0)
	{	/* Parent */
		sys_move_cursor(0, loc);
		printf("%d: I forked child process %d.", pid0, pid1);
		sys_sleep(3);
		if ((pid2 = sys_fork()) < 0)
			return NULL;
		else if (pid2 > 0)
		{	/* Parent */
			sys_move_cursor(0, loc);
			printf("%d: I forked child process %d.", pid0, pid2);
			strcpy(p, str[0]);
			sys_sleep(3);
			sys_move_cursor(3, loc);
			printf("%s", str[4]);
			sys_sleep(1);
		}
		else
		{	/* Child */
			loc = 1;
			pid2 = sys_getpid();
			sys_move_cursor(0, loc);
			printf("%d: I am child process of %d.", pid2, pid0);
			strcpy(p, str[1]);
			sys_sleep(3);
			sys_move_cursor(3, loc);
			printf("%s", str[4]);
			sys_sleep(2);
		}
	}
	else
	{	/* Child */
		loc = 2;
		pid1 = sys_getpid();
		sys_move_cursor(0, loc);
		printf("%d: I am child process of %d.", pid1, pid0);
		sys_sleep(3);
		if ((pid2 = sys_fork()) < 0)
			return NULL;
		else if (pid2 > 0)
		{	/* Parent */
			sys_move_cursor(0, loc);
			printf("%d: I forked child process %d.", pid1, pid2);
			strcpy(p, str[2]);
			sys_sleep(3);
			sys_move_cursor(3, loc);
			printf("%s", str[4]);
			sys_sleep(3);
		}
		else
		{	/* Child */
			loc = 3;
			pid2 = sys_getpid();
			sys_move_cursor(0, loc);
			printf("%d: I am child process of %d.", pid2, pid0);
			strcpy(p, str[3]);
			sys_sleep(3);
			sys_move_cursor(3, loc);
			printf("%s", str[4]);
			sys_sleep(8);
		}
	}
	return p;
}