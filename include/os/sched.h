/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include <type.h>
//#include <os/lock.h>
#include <pgtable.h>

#define UPROC_MAX 16
#define TASK_NAMELEN 31
#define INVALID_PID (-1)

#if NCPU != 2
#define NCPU 1
#endif

#ifndef USTACK_NPG
/*
 * How many pages are allocated for
 * user stack of a user process.
 * NOTE: also defined in `mm.h`.
 */
#define USTACK_NPG 16U
#endif

#ifndef MULTITHREADING
/*
 * Support multithreading or not.
 * It appears in many header files!
 */
#define MULTITHREADING 1
#endif

/*
 * used to save register infomation:
 * NOTE: this order must be preserved,
 * which is defined in regs.h!!
 */
typedef struct regs_context
{
	/* Saved main processor registers.*/
	reg_t regs[32];

	/* Saved special registers. */
	reg_t sstatus;
	reg_t sepc;
	reg_t sbadaddr;
	reg_t scause;
} regs_context_t;

/* used to save register infomation in switch_to */
typedef struct switchto_context
{
	/*
	 * Callee saved registers:
	 * ra, sp, s0~s11.
	 */
	reg_t regs[14];
} switchto_context_t;

enum Saved_regs {
	SR_RA,SR_SP,SR_S0,SR_S1,SR_S2,SR_S3,SR_S4,
	SR_S5,SR_S6,SR_S7,SR_S8,SR_S9,SR_S10,SR_S11
};

typedef enum {
	TASK_SLEEPING,
	TASK_RUNNING,
	TASK_READY,
	TASK_EXITED,
#if NCPU == 2
	TASK_EXITING
	/*
	 * If a process on a CPU kills a process running
	 * on another CPU, TASK_EXITING will be set and it will
	 * do_exit() once it enters kernel.
	 */
#endif
} task_status_t;

typedef pid_t tid_t;

/* Process Control Block */
typedef struct pcb
{
	/* register context */
	// NOTE: this order must be preserved, which is defined in regs.h!!
	/* --- order preserved starts --- */
	/*
	 * kernel_sp is KVA while user_sp is UVA.
	 * "TP" means this elem is private for every thread.
	 */
	reg_t kernel_sp, user_sp;	/* TP */
	regs_context_t *trapframe;	/* TP */
	/*
	 * pgdir_kva is the kernel virtual address of page
	 * directory of this process.
	 */
	PTE* pgdir_kva;
	/* process id */
	pid_t pid;
#if MULTITHREADING != 0
	pthread_t tid;	/* TP */
#endif
	/* --- order preserved ends --- */
	/*
	 * `kernel_stack` is the base address of kernel stack.
	 * It is KVA.
	 */
	uintptr_t kernel_stack;	/* TP */
	/*
	 * `seg_start` is the start of valid user virtual address.
	 * `seg_end` is the end address of valid segment, and it
	 * can be added by `do_sbrk()` syscall. Access to address
	 * EQUAL or GREATER than it will cause segmentation fault.
	 */
	uintptr_t seg_start, seg_end;
	/*
	 * Processes waiting for this proc.
	 */
	struct pcb *wait_queue;	/* TP */
	/* next pointer */
	struct pcb *next;	/* TP */
	/* SLEEPING | READY | RUNNING */
	task_status_t status;	/* TP */
	/* cursor position */
	int cursor_x, cursor_y;	/* TP */
	/*
	 * limit of cursor_y, used for auto scroll.
	 * < 0 means  invalid.
	 */
	int cylim_l, cylim_h;	/* TP */
	/* time(seconds) to wake up sleeping PCB */
	uint64_t wakeup_time;	/* TP */
	/* Saved regs */
	switchto_context_t context;	/* TP */
	/* Name of this process */
	char name[TASK_NAMELEN + 1];
	/*
	 * phead: points to the head pointer of the queue
	 * to which this PCB is belonging. This is used to
	 * find the queue.
	 */
	struct pcb ** phead;	/* TP */
	/*
	 * req_len is used to store the length of mailbox
	 * request when blocked in queue of mailbox.
	 */
	unsigned int req_len;	/* TP */
	/*
	 * cpu_mask is used to set CPU affinity. If a CPU has ID i,
	 * this process can run on it if and only if (1<<i)&cpu_mask is not zero.
	 */
	unsigned int cpu_mask;	/* TP */
} pcb_t;

extern const uintptr_t User_sp;
extern const uint32_t //Ustack_size,
	Kstack_size;

/* ready queue to run */
//extern list_head ready_queue;
extern pcb_t *ready_queue;

/* sleep queue to be blocked in */
//extern list_head sleep_queue;
extern pcb_t *sleep_queue;

/* current running task PCB */
extern pcb_t * volatile current_running[NCPU];

//extern pcb_t pcb[UPROC_MAX];
extern pcb_t pid0_pcb, pid1_pcb;
extern const ptr_t pid0_stack, pid1_stack;

/* The type of arguments has been modified from pcb_t* to switchto_context_t* */
extern void switch_to(switchto_context_t *prev, switchto_context_t *next);

pid_t alloc_pid(void);
pid_t create_proc(const char *taskname, unsigned int cpu_mask);

void do_scheduler(void);
void do_sleep(uint32_t);
void check_sleeping(void);
void wake_up(pcb_t * const T);

//int do_block(pcb_t ** const Pqueue, spin_lock_t *slock);
/*
 * Declaration of do_unblock() is moved to lock.h
 * to avoid hazard about including.
 */

/*
 * Remove the head from Queue, set its status to be READY
 * and insert it into ready_queue AFTER current_running.
 * Return: new head of Queue.
 */
pcb_t *do_unblock(pcb_t * const Queue);

void init_pcb_stack(
	uintptr_t kernel_stack, uintptr_t entry_point, pcb_t *pcb);
void set_preempt(void);

/************************************************************/
/* TODO [P3-TASK1] exec exit kill waitpid ps*/
#ifdef S_CORE
extern pid_t do_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2);
#else
extern pid_t do_exec(const char *name, int argc, char *argv[]);
#endif
extern void do_exit(void);
extern int do_kill(pid_t pid);
extern pid_t do_waitpid(pid_t pid);
extern int do_process_show(void);
extern pid_t do_getpid(void);
pid_t do_taskset(int create, char *name, pid_t pid, unsigned int cpu_mask);
/************************************************************/

extern pcb_t *pcb_table[UPROC_MAX + NCPU];
int pcb_table_add(pcb_t *p);
int pcb_table_del(pcb_t *p);
int get_proc_num(void);
pcb_t *pcb_search(pid_t pid);
pcb_t *pcb_search_name(const char *name);


#if MULTITHREADING != 0
#include <os/sched-thread.h>
#endif

#endif
