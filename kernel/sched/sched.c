#include <os/list.h>
#include <os/pcb-list-g.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <os/malloc-g.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <os/loader.h>

//pcb_t pcb[NUM_MAX_TASK];
/*
 * Has been modified by Glucose180
 */
const ptr_t pid0_stack = INIT_KERNEL_STACK;// + PAGE_SIZE;

/*
 * The default size for a user stack and kernel stack.
 * 4 KiB, 2 KiB.
 */
static const uint32_t Ustack_size = 4 * 1024,
	Kstask_size = 2 * 1024;

/*
 * It is used to represent main.c:main()
 */
pcb_t pid0_pcb = {
	.pid = 0,
	.kernel_sp = (ptr_t)pid0_stack,
	.user_sp = (ptr_t)pid0_stack,
	.trapframe = (void *)pid0_stack,
	/* Add more info */
	.status = TASK_RUNNING,
	.cursor_x = 0,
	.cursor_y = 0
};

//LIST_HEAD(ready_queue);
/*
 * Circular queue (linked list) of all READY and RUNNING processes
 */
pcb_t *ready_queue;
//LIST_HEAD(sleep_queue);

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

/*
 * alloc_pid: find a free PID.
 * INVALID_PID will be returned if no free PID is found.
 * NOTE: It can be improved as the algorithm is NOT efficient.
 */
pid_t alloc_pid(void)
{
	pid_t i;
	const pid_t Pid_min = 1, Pid_max = 2 * NUM_MAX_TASK;

	for (i = Pid_min; i <= Pid_max; ++i)
		if (search_node_pid(ready_queue, i) == NULL)
			return i;
	return INVALID_PID;
}

/*
 * Use the name of a task to load it and add it in ready_queue.
 * INVALID_PID will be returned on error.
 */
pid_t create_proc(const char *taskname)
{
	ptr_t entry, user_stack, kernel_stack;
	pcb_t *pnew, *temp, *p0;

	p0 = NULL;
	entry = load_task_img(taskname);
	if (entry == 0U)
		return INVALID_PID;
	user_stack = (ptr_t)umalloc_g(Ustack_size);
	kernel_stack = (ptr_t)kmalloc_g(Kstask_size);
	if (user_stack == 0 || kernel_stack == 0)
		return INVALID_PID;
	if (ready_queue->pid == pid0_pcb.pid)
	{
		ready_queue = del_node(ready_queue, ready_queue, &p0);
		if (ready_queue != NULL || p0 == NULL || p0->pid != pid0_pcb.pid)
			panic_g("create_proc: Failed to remove the proc whose ID is 0");
	}
	if ((temp = add_node_to_tail(ready_queue, &pnew)) == NULL)
	{
		ufree_g((void *)user_stack);
		return INVALID_PID;
	}
	ready_queue = temp;
	if (p0 != NULL)
	{	/* Has removed the 0 proc */
		p0->next = ready_queue;
		if (p0 != current_running)
			panic_g("create_proc: Error happened while removing the proc 0");
	}
	init_pcb_stack(kernel_stack + ROUND(Kstask_size, ADDR_ALIGN),
		user_stack + ROUND(Ustack_size, ADDR_ALIGN), entry, pnew);
	return pnew->pid;
}

void do_scheduler(void)
{
	pcb_t *p, *q;
	// TODO: [p2-task3] Check sleep queue to wake up PCBs

	/************************************************************/
	/* Do not touch this comment. Reserved for future projects. */
	/************************************************************/

	/*
	// It is used to test whether kfree_g can detect invalid address
	static int tested = 0;
	if (tested == 0 && current_running->pid > 0)
	{
		kfree_g(current_running->user_sp - Ustack_size + 8);
		// Test whether panic will happen
		tested = 1;
	}
	*/

	// TODO: [p2-task1] Modify the current_running pointer.
	// TODO: [p2-task1] switch_to current_running
	while (1)
	{
		for (p = current_running->next; p != current_running; p = p->next)
		{	/* Search the linked list and find a READY process */
			if (p->status == TASK_READY)
			{
				current_running->status = TASK_READY;
				q = current_running;
				current_running = p;
				current_running->status = TASK_RUNNING;
				switch_to(&(q->context), &(current_running->context));
				return;
			}
		}
		return;
	}
}

void do_sleep(uint32_t sleep_time)
{
	// TODO: [p2-task3] sleep(seconds)
	// NOTE: you can assume: 1 second = 1 `timebase` ticks
	// 1. block the current_running
	// 2. set the wake up time for the blocked task
	// 3. reschedule because the current_running is blocked.
}

void check_sleeping(void)
{
	// TODO: [p2-task3] Pick out tasks that should wake up from the sleep queue
}

//void do_block(list_node_t *pcb_node, list_head *queue)
/*
 * Insert *Pt to tail of Queue and return the new head of Queue.
 * NULL will be returned on error (It is unlike to happen).
 */
pcb_t *do_block(pcb_t * const Pt, pcb_t * const Queue)
{
	// TODO: [p2-task2] block the pcb task into the block queue
	Pt->status = TASK_SLEEPING;
	return insert_node(Queue, Pt, NULL);
}

//void do_unblock(list_node_t *pcb_node)
/*
 * Remove the head from Queue, set its status to be READY
 * and insert it into ready_queue AFTER current_running.
 * Return: new head of Queue.
 */
pcb_t *do_unblock(pcb_t * const Queue)
{
	// TODO: [p2-task2] unblock the `pcb` from the block queue
	pcb_t *prec, *nq, *temp;

	nq = del_node(Queue, Queue, &prec);
	if (prec == NULL)
		panic_g("do_unblock: Failed to remove the head of queue 0x%lx", (long)Queue);
	prec->status = TASK_READY;
	temp = insert_node(ready_queue, prec, current_running);
	if (temp == NULL)
		panic_g("do_unblock: Failed to insert process (PID=%d) to ready_queue", prec->pid);
	ready_queue = temp;
	return nq;
}

/************************************************************/
void init_pcb_stack(
	ptr_t kernel_sp, ptr_t user_sp, ptr_t entry_point,
	pcb_t *pcb)
{
	 /* TODO: [p2-task3] initialization of registers on kernel stack
	  * HINT: sp, ra, sepc, sstatus
	  * NOTE: To run the task in user mode, you should set corresponding bits
	  *     of sstatus(SPP, SPIE, etc.).
	  */
	//regs_context_t *pt_regs =
		//(regs_context_t *)(kernel_stack - sizeof(regs_context_t));


	/* TODO: [p2-task1] set sp to simulate just returning from switch_to
	 * NOTE: you should prepare a stack, and push some values to
	 * simulate a callee-saved context.
	 */
	//switchto_context_t *pt_switchto =
		//(switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));

	pcb->context.regs[SR_RA] = entry_point;
	pcb->context.regs[SR_SP] = user_sp;
	pcb->kernel_sp = kernel_sp;
	pcb->user_sp = user_sp;
	pcb->trapframe = NULL;	/* Flag for a new task */
	pcb->status = TASK_READY;
	pcb->cursor_x = pcb->cursor_y = 0;
	if ((pcb->pid = alloc_pid()) == INVALID_PID)
		panic_g("init_pcb_stack: No invalid PID can be used");
}