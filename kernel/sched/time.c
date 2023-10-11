#include <os/list.h>
#include <os/sched.h>
#include <type.h>

uint64_t time_elapsed = 0;
uint64_t time_base = 0;

/*
 * The maximum time (seconds).
 * It equals max(uint64_t)/time_base.
 */
uint64_t time_max_sec = 0;

uint64_t get_ticks()
{
	__asm__ __volatile__(
		"rdtime %0"
		: "=r"(time_elapsed));
	return time_elapsed;
}

uint64_t get_timer()
{
	return get_ticks() / time_base;
}

uint64_t get_time_base()
{
	return time_base;
}

void latency(uint64_t time)
{
	uint64_t begin_time = get_timer();

	while (get_timer() - begin_time < time);
	return;
}
