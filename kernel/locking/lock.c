#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>
#include <os/glucose.h>

mutex_lock_t mlocks[LOCK_NUM];

void init_locks(void)
{
	/* TODO: [p2-task2] initialize mlocks */
	int i;

	for (i = 0; i < LOCK_NUM; ++i)
	{
		mlocks[i].lock.status = UNLOCKED;
		mlocks[i].block_queue = NULL;
		mlocks[i].key = 0;	/* TEMPorarily Unused */
		mlocks[i].opid = INVALID_PID;
	}
}

void spin_lock_init(spin_lock_t *lock)
{
	/* TODO: [p2-task2] initialize spin lock */
}

int spin_lock_try_acquire(spin_lock_t *lock)
{
	/* TODO: [p2-task2] try to acquire spin lock */
	return 0;
}

void spin_lock_acquire(spin_lock_t *lock)
{
	/* TODO: [p2-task2] acquire spin lock */
}

void spin_lock_release(spin_lock_t *lock)
{
	/* TODO: [p2-task2] release spin lock */
}

int do_mutex_lock_init(int key)
{
	/* TODO: [p2-task2] initialize mutex lock */
	unsigned int mlock_id;

	mlock_id = (unsigned int)key % LOCK_NUM;
	return (int)mlock_id;
}

/*
 * Acquire mutex lock with ID mlock_idx.
 * Return mlock_idx or -1 on error.
 */
int do_mutex_lock_acquire(int mlock_idx)
{
	/* TODO: [p2-task2] acquire mutex lock */
	pcb_t *p, *q;
	mutex_lock_t *ptlock;

	if (mlock_idx >= LOCK_NUM || mlock_idx < 0)
		return -1;	/* Invalid mlock_idx */
	ptlock = mlocks + mlock_idx;	/* Pointer to target lock */
	if (ptlock->lock.status == UNLOCKED || 
		ptlock->opid == current_running->pid)
	{	/* Acquire the lock */
		ptlock->lock.status = LOCKED;
		ptlock->opid = current_running->pid;
		return mlock_idx;
	}
	/* Should be blocked */
	for (p = current_running->next; p != current_running; p = p->next)
	{
		if (p->status == TASK_READY)
		{
			q = current_running;
			current_running = p;
			current_running->status = TASK_RUNNING;
			ready_queue = lpcb_del_node(ready_queue, q, &p);
			if (p != q)
				panic_g("do_mutex_lock_acquire: Failed to remove"
					" current_running from ready_queue");
			ptlock->block_queue = do_block(p, ptlock->block_queue);
			switch_to(&(p->context), &(current_running->context));
			return mlock_idx;
		}
	}
	panic_g("do_mutex_lock_acquire: lock %d is LOCKED"
		" but no ready process is found", mlock_idx);
	return -1;
}

/*
 * Release mutex lock with ID mlock_idx.
 * Return mlock_idx or negative number on error.
 */
int do_mutex_lock_release(int mlock_idx)
{
	/* TODO: [p2-task2] release mutex lock */
	mutex_lock_t *ptlock;
	pcb_t *temp;

	if (mlock_idx >= LOCK_NUM || mlock_idx < 0)
		return -1;	/* Invalid mlock_idx */
	ptlock = mlocks + mlock_idx;
	if (ptlock->lock.status == LOCKED &&
		ptlock->opid != current_running->pid)
		return -2;	/* The lock is LOCKED by other process */
	if (ptlock->lock.status == UNLOCKED)
		return -3;
	
	if (ptlock->block_queue == NULL)
		ptlock->lock.status = UNLOCKED;
	else
	{
		temp = ptlock->block_queue;	/* Points to the head */
		ptlock->block_queue = do_unblock(ptlock->block_queue);
		/* Now *temp acquires the lock */
		ptlock->opid = temp->pid;
	}
	return mlock_idx;
}
