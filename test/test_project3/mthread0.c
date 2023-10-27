#include <stdio.h>
#include <unistd.h>

unsigned long v0, v1;

const unsigned long Step = 20;

void v0_inc(unsigned long step);
void v1_inc(unsigned long step);

int main()
{
	unsigned long yield_ymr = 0U;
	int t0, t1;

	while (1)
	{
		v0 = v1 = 0U;
		yield_ymr = 0U;
		t0 = sys_thread_create((void *(*)())v0_inc, Step);
		t1 = sys_thread_create((void *(*)())v1_inc, Step);
		while (1)
		{
			sys_thread_yield();
			++yield_ymr;
			sys_move_cursor(0, 8);
			if (yield_ymr == 20)
			{
				if (sys_thread_kill(t0) == t0)
				{
					printf("> [TASK] thread %d has been killed           ", t0);
					v0 += (Step << 1);
				}
				else
					printf("> [TASK] failed to kill thread %d            ", t0);
			}
			else if (yield_ymr > 20 && yield_ymr < 40)
				v0 += (Step << 1);
			else if (yield_ymr == 40)
			{
				if (sys_thread_kill(t1) == t1)
				{
					printf("> [TASK] thread %d has been killed           ", t1);
					break;
				}
				else
					printf("> [TASK] failed to kill thread %d            ", t1);
			}
			else
				printf("> [TASK] Every thread has yielded %lu times  ", yield_ymr);
		}
		sys_sleep(5);
	}
	return 0;
}

void v0_inc(unsigned long step)
{
	while (1)
	{
		do {
			++v0;
			sys_move_cursor(0, 6);
			printf("> [TASK] This is thread v0_inc(): %lu     ", v0);
		} while (v0 < v1 + step);
		sys_thread_yield();
	}
}

void v1_inc(unsigned long step)
{
	while (1)
	{
		do {
			++v1;
			sys_move_cursor(0, 7);
			printf("> [TASK] This is thread v1_inc(): %lu     ", v1);
		} while (v1 < v0 + step);
		sys_thread_yield();
	}
}