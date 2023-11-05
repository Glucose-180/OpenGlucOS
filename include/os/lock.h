/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Thread Lock
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

#ifndef INCLUDE_LOCK_H_
#define INCLUDE_LOCK_H_

#include <os/sched.h>

#define LOCK_NUM 16

typedef enum {
	UNLOCKED,
	LOCKED,
} lock_status_t;

typedef struct spin_lock
{
	volatile lock_status_t status;
} spin_lock_t;

typedef struct mutex_lock
{
	/* Every mutex lock needs a spin lock to protect it */
	spin_lock_t slock;
	/* This status is the status of this mutex lock */
	lock_status_t status;
	pcb_t* block_queue;
	int key;
	pid_t opid;	/* Owner's PID */
} mutex_lock_t;

enum Spin_locks {
	/* ... */
	SPINLOCK_NUM
};

//extern mutex_lock_t mlocks[LOCK_NUM];
//extern spin_lock_t slocks[SPINLOCK_NUM];

void init_locks(void);

void spin_lock_init(spin_lock_t *lock);
int spin_lock_try_acquire(spin_lock_t *lock);
void spin_lock_acquire(spin_lock_t *lock);
void spin_lock_release(spin_lock_t *lock);

int do_mutex_lock_init(int key);
int do_mutex_lock_acquire(int mlock_idx);
int do_mutex_lock_release(int mlock_idx);
void ress_release_killed(pid_t kpid);

/* Moved from sched.h */
/*
 * Insert *current_running to tail of *Pqueue, RELEASE spin lock slock
 * (if it is not NULL) and then reschedule.
 * The lock will be reacquired after being unblocked.
 * Returns: 0 if switch_to() is called, 1 otherwise.
 */
int do_block(pcb_t ** const Pqueue, spin_lock_t *slock);

/************************************************************/
typedef struct barrier
{
    // TODO [P3-TASK2 barrier]
	/* Spin lock to protect it */
	spin_lock_t slock;
	/* The PID of the process who init this sema */
	pid_t opid;
	/*
	 * come: count of processes that have come to barrier.
	 * goal: total number of processes that should come.
	 */
	int goal, come;
	pcb_t *block_queue;
} barrier_t;

#define BARRIER_NUM 16

void init_barriers(void);
int do_barrier_init(int key, int goal);
int do_barrier_wait(int bidx);
int do_barrier_destroy(int bidx);

typedef struct condition
{
    // TODO [P3-TASK2 condition]
} condition_t;

#define CONDITION_NUM 16

void init_conditions(void);
int do_condition_init(int key);
void do_condition_wait(int cond_idx, int mutex_idx);
void do_condition_signal(int cond_idx);
void do_condition_broadcast(int cond_idx);
void do_condition_destroy(int cond_idx);

typedef struct semaphore
{
    // TODO [P3-TASK2 semaphore]
	/* Spin lock to protect it */
	spin_lock_t slock;
	/* The PID of the process who init this sema */
	pid_t opid;
	/* Value of semaphore */
	int value;
	/* Processes being blocked */
	pcb_t* block_queue;
} semaphore_t;

#define SEMAPHORE_NUM 16

void init_semaphores(void);
int do_semaphore_init(int key, int value);
int do_semaphore_up(int sidx);
int do_semaphore_down(int sidx);
int do_semaphore_destroy(int sidx);

//#define MAX_MBOX_LENGTH (64)
#define MAX_MBOX_LENGTH (71)

typedef struct mailbox
{
    // TODO [P3-TASK2 mailbox]
	spin_lock_t slock;
	pid_t opid;
	/*
	 * Use a circular queue as buffer,
	 * buf_head and buf_tail are pointers of it.
	 */
	uint8_t buf[MAX_MBOX_LENGTH + 1];
	unsigned int buf_head, buf_tail;
	/*
	 * Two queues are needed. Senders are blocked in
	 * block_queue_s until buffer has enough space, and
	 * receivers are blocked in block_queue_r until buffer
	 * has enough data.
	 */
	pcb_t* block_queue_s, * block_queue_r;
} mailbox_t;

#define MBOX_NUM 16
void init_mbox(void);
int do_mbox_open(const char *name);
int do_mbox_close(int midx);
int do_mbox_send(int midx, const uint8_t* msg, unsigned int msg_length);
int do_mbox_recv(int midx, uint8_t* msg, unsigned int msg_length);

/************************************************************/

#endif
