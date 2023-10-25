#include <stdio.h>
#include <unistd.h>

unsigned long v0, v1;

void v0_inc(unsigned long step);
void v1_inc(unsigned long step);

int main()
{
	unsigned long yield_ymr = 0U;

	sys_thread_create((void *(*)())v0_inc, 100);
	sys_thread_create((void *(*)())v1_inc, 100);
	while (1)
	{
		sys_thread_yield();
		++yield_ymr;
		sys_move_cursor(0, 8);
		printf("> [TASK] Every thread has yielded %lu times", yield_ymr);
	}
	return 0;
}

void v0_inc(unsigned long step)
{
	while (1)
	{
		for (; v0 < v1 + step; ++v0)
		{
			sys_move_cursor(0, 6);
			printf("> [TASK] This is thread v0_inc(): %lu", v0);
		}
		sys_thread_yield();
	}
}

void v1_inc(unsigned long step)
{
	while (1)
	{
		for (; v1 < v0 + step; ++v1)
		{
			sys_move_cursor(0, 7);
			printf("> [TASK] This is thread v1_inc(): %lu", v1);
		}
		sys_thread_yield();
	}
}