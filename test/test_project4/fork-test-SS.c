/*
 * Complex fork test by Glucose180 for GlucOS:
 * Shared memory, and swapping, and multithreading.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifndef MULTITHREADING
#define MULTITHREADING 1
#endif

/* Original printing location */
#define LOC0 0

/* Test shared memory */
#define MAGIC 0x1230695306L
#define SHMKEY 2

/* Test swapping */
#define SWAP_TEST "swap-test"

const char *str[] = {
	"This train is bound for Universal Resort.",
	"The next station is Gongzhufen.",
	"Change here for Line 10.",
	"We are now arriving at Gongzhufen.",
	"                                           "
};
char *q;
int loc = LOC0;

void thread_write(void* arg);
char *test_fork(void);

int main(int argc, char *argv[])
{
	char *p;
	long *pshared;
	int barrier;

	sys_move_cursor(0, loc);

	barrier = sys_barrier_init(1, 4);
	if (barrier < 0)
	{
		printf("Barrier failed.");
		return 1;
	}

	q = sys_shmpageget(SHMKEY);
	if (q == NULL || sys_shmpagedt(q) < 0)
	{
		printf("Shared memory 1 failed.");
		return 1;
	}
	/*
	 * By getting a shared page and then destroying
	 * it, we make `q` point to a new private page.
	 * This is used to test copy-on-write for multithreading
	 * after sys_fork().
	 */
	strcpy(q, "Buy tickets at 95306!");

	pshared = sys_shmpageget(SHMKEY);
	if (pshared == NULL)
	{
		printf("Shared memory 2 failed.");
		return 1;
	}
	*pshared = MAGIC;

	/* Now test swapping */
	if (argc >= 2)
	{
		pid_t pidswap;
		char *args[] = {SWAP_TEST, argv[1]};
		pidswap = sys_exec(SWAP_TEST, 2, args);
		if (pidswap < 0)
			printf("Failed to start " SWAP_TEST ".");
		else
		{
			printf("Testing swapping...");
			if (sys_waitpid(pidswap) < 0)
				printf("Failed to wait %d!", pidswap);
			else
			{
				sys_move_cursor(0, loc);
				printf("Swapping is done!");
			}
		}
		sys_sleep(2);
	}

	p = test_fork();

	/* 4 processes will reach here! */
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

		/* A child process should get the page first */
		if (loc != LOC0)
		{
			pshared = sys_shmpageget(SHMKEY);
			if (pshared == NULL)
			{
				sys_move_cursor(3, loc);
				printf("Shared memory 3 failed.");
				return 1;
			}
		}
		/* Check the value at shared page */
		if (*pshared != MAGIC)
		{
			sys_move_cursor(3, loc);
			printf("Shared memory 3 failed.");
			return 1;
		}

		sys_move_cursor(3, loc);
		printf("Shared memory test passed!%s", str[4]);
		sys_sleep(2);

#if MULTITHREADING != 0
		pthread_t tid;
		if (pthread_create(&tid, thread_write, NULL) != 0 ||
			pthread_join(tid) != 0)
		{
			sys_move_cursor(3, loc);
			printf("Multithreading test failed!%s", str[4]);
			return 1;
		}

		sys_move_cursor(3, loc);
		printf("%s           ", q);
		sys_sleep(5);
#endif

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
			loc = LOC0 + 1;
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
		loc = LOC0 + 2;
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
			loc = LOC0 + 3;
			pid2 = sys_getpid();
			sys_move_cursor(0, loc);
			printf("%d: I am child process of %d.", pid2, pid1);
			strcpy(p, str[3]);
			sys_sleep(3);
			sys_move_cursor(3, loc);
			printf("%s", str[4]);
			sys_sleep(8);
		}
	}
	return p;
}

/*
 * Use a child thread to write string to test
 * copy-on-write.
 */
void thread_write(void* arg)
{
	/*
	 * `q` points to a new page. Modify the page to
	 * test copy-on-write for multithreading.
	 */
	switch (loc - LOC0)
	{
	case 0:
		break;
	case 1:
		strcpy(q, "Deliver goods at 12306!");
		break;
	case 2:
		strcpy(q, "China railway wishes you");
		break;
	case 3:
		strcpy(q, " a pleasant journey!");
		break;
	}
	sys_sleep(1);
	return;
}