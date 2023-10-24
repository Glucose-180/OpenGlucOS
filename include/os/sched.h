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
#include <os/list.h>

#define NUM_MAX_TASK 16
#define INVALID_PID (-1)

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
} task_status_t;

typedef pid_t tid_t;
/* Thread control block */
typedef struct tcb {
	tid_t tid;
	switchto_context_t context;
	struct tcb *next;
} tcb_t;

/* Process Control Block */
typedef struct pcb
{
	/* register context */
	// NOTE: this order must be preserved, which is defined in regs.h!!
	reg_t kernel_sp;
	reg_t user_sp;
	/*
	 * NOTE: user_sp and kernel_sp of PCB of PID 0:
	 * After SAVE_CONTEXT, user_sp and trapframe->sp equal
	 * $sp just at the beginning of the trap, and kernel_sp
	 * equals $sp after SAVE_CONTEXT. After RESTORE_CONTEXT,
	 * they three are all equal to $sp just at the beginning.
	 */
	regs_context_t *trapframe;

	/*
	 * the start (lowest) address of kernel and user stack.
	 * Different from kernel/user_sp which are stack pointers,
	 * as stack grows from high address to low.
	 */
	reg_t kernel_stack;
	reg_t user_stack;

	/* previous, next pointer */
	//list_node_t list;
	/* next pointer */
	struct pcb *next;

	/* process id */
	pid_t pid;

	/* SLEEPING | READY | RUNNING */
	task_status_t status;

	/* cursor position */
	int cursor_x;
	int cursor_y;

	/* time(seconds) to wake up sleeping PCB */
	uint64_t wakeup_time;

	/* Saved regs */
	switchto_context_t context;

	/* Pointer to list of child threads */
	tcb_t *pcthread;
	/* Pointer to current thread */
	tcb_t *cur_thread;
} pcb_t;

/* ready queue to run */
//extern list_head ready_queue;
extern pcb_t *ready_queue;

/* sleep queue to be blocked in */
//extern list_head sleep_queue;
extern pcb_t *sleep_queue;

/* current running task PCB */
extern pcb_t * volatile current_running;
extern pid_t process_id;

//extern pcb_t pcb[NUM_MAX_TASK];
extern pcb_t pid0_pcb;
extern const ptr_t pid0_stack;

/* The type of arguments has been modified from pcb_t* to switchto_context_t* */
extern void switch_to(switchto_context_t *prev, switchto_context_t *next);

pid_t alloc_pid(void);
pid_t create_proc(const char *taskname);

void do_scheduler(void);
void do_sleep(uint32_t);
void check_sleeping(void);
void wake_up(pcb_t * const T);

pcb_t *do_block(pcb_t * const Pt, pcb_t * const Queue);
pcb_t *do_unblock(pcb_t * const Queue);

void init_pcb_stack(
	ptr_t kernel_sp, ptr_t user_sp, ptr_t entry_point,
	pcb_t *pcb);
void set_preempt(void);

/************************************************************/
/* Do not touch this comment. Reserved for future projects. */
/************************************************************/

#endif
