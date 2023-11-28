#include <os/pcb-list-g.h>
#include <os/sched.h>
#include <os/lock.h>
#include <os/glucose.h>
#include <os/string.h>
#include <os/smp.h>

/*
 * Use a external array to store pointers to all PCB.
 * This array is maintained by functions in this source file.
 * Every time a PCB is created, pcb_table_add should be called;
 * every time a PCB is deleted, pcb_table_del should be called!
 */
pcb_t *pcb_table[UPROC_MAX + NCPU];

/* Count of proc */
static int proc_ymr = 0;

/*
 * It equals the index of [used entry whose index is max]
 * ADDED 1.
 */
static int free_pt = 0;

static void pcb_table_rearrange(void);

/*
 * Add an entry to pcb_table and increase `proc_ymr`.
 * The index will be returned and -1 on error (not found).
 */
int pcb_table_add(pcb_t *p)
{
	if (proc_ymr >= UPROC_MAX + NCPU)
		return -1;
	if (free_pt >= UPROC_MAX + NCPU)
		pcb_table_rearrange();
	pcb_table[free_pt++] = p;
	++proc_ymr;
	return free_pt - 1;
}

/*
 * Delete an entry in pcb_table and decrese proc_ymr.
 * The index will be returned and -1 on error (not found).
 */
int pcb_table_del(pcb_t *p)
{
	int i;

	for (i = 0; i < free_pt; ++i)
		if (pcb_table[i] == p)
			break;
	if (i < free_pt)
	{
		pcb_table[i] = NULL;
		--proc_ymr;
		return i;
	}
	else	/* Not found */
		return -1;
}

int get_proc_num(void)
{
	return proc_ymr;
}

/*
 * Search a PCB according to PID.
 * Returns pointer to it on success, or
 * NULL if not found.
 */
pcb_t *pcb_search(pid_t pid)
{
	int i;

	for (i = 0; i < free_pt; ++i)
		if (pcb_table[i] != NULL && pcb_table[i]->pid == pid)
			return pcb_table[i];
	return NULL;
}

#if MULTITHREADING != 0
/*
 * Search a "PCB"(TCB) of the current process with
 * TID `tid`. Returns pointer to it on success, or
 * NULL if not found.
 */
tcb_t *pcb_search_tcb(tid_t tid)
{
	int i;
	pid_t pid = cur_cpu()->pid;

	for (i = 0; i < free_pt; ++i)
		if (pcb_table[i] != NULL &&
			pcb_table[i]->pid == pid && pcb_table[i]->tid == tid)
			return pcb_table[i];
	return NULL;
}

/*
 * Search all "PCB"(TCB) of a process and write them in `ppcb` (if not `NULL`).
 * Return the number of PCB found.
 * NOTE: ensure that `farr` has enough space!
 */
unsigned int pcb_search_all(pid_t pid, pcb_t **farr)
{
	unsigned int found_ymr = 0U;
	int i;

	for (i = 0; i < free_pt; ++i)
		if (pcb_table[i] != NULL && pcb_table[i]->pid == pid)
		{
			if (farr != NULL)
				farr[found_ymr++] = pcb_table[i];
			else
				++found_ymr;
		}
	return found_ymr;
}

#endif

/*
 * Search a PCB according to its name.
 * Returns pointer to it on success, or
 * NULL if not found.
 */
pcb_t *pcb_search_name(const char *name)
{
	int i;

	for (i = 0; i < free_pt; ++i)
		if (pcb_table[i] != NULL &&
			strncmp(name, pcb_table[i]->name, TASK_NAMELEN) == 0)
			return pcb_table[i];
	return NULL;
}

/*
 * Frequently adding and deleting may cause spaces in pcb_table.
 * Use this function to rearrange them.
 */
static void pcb_table_rearrange(void)
{
	int i, j;

	for (i = j = 0; i < free_pt; ++i)
	{
		if (pcb_table[i] != NULL)
			pcb_table[j++] = pcb_table[i];
	}
	if (j != proc_ymr)
		panic_g("pcb_table_rearrange: algorithm error while rearranging");
	free_pt = j;
	while (j < UPROC_MAX + NCPU)
		pcb_table[j++] = NULL;
}