#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>
#include <os/glucose.h>

mutex_lock_t mlocks[LOCK_NUM];
spin_lock_t slocks[SPINLOCK_NUM];

semaphore_t semaphores[SEMAPHORE_NUM];

void init_locks(void)
{
	/* TODO: [p2-task2] initialize mlocks */
	int i;

	for (i = 0; i < LOCK_NUM; ++i)
	{
		spin_lock_init(&(mlocks[i].slock));
		mlocks[i].status = UNLOCKED;
		mlocks[i].block_queue = NULL;
		mlocks[i].key = 0;	/* TEMPorarily Unused */
		mlocks[i].opid = INVALID_PID;
	}
	for (i = 0; i < SPINLOCK_NUM; ++i)
		slocks[i].status = UNLOCKED;
	
	init_semaphores();
}

void spin_lock_init(spin_lock_t *lock)
{
	lock->status = UNLOCKED;
}

/*
 * Try to acquire a spin lock.
 * Returns: 0 on success, 1 on failed.
 */
int spin_lock_try_acquire(spin_lock_t *lock)
{
	/* TODO: [p2-task2] try to acquire spin lock */
	if (__sync_lock_test_and_set(&(lock->status), LOCKED) == LOCKED)
		return 1;
	else
		return 0;
}

void spin_lock_acquire(spin_lock_t *lock)
{
	/* NOTE: This is learned from XV6. */
	/*
	 * On RISC-V, __sync_lock_test_and_set turns into an atomic swap:
	 * a5 = LOCKED, s1 = &lock->status, amoswap.w.aq a5, a5, (s1);
	 */
	while (__sync_lock_test_and_set(&(lock->status), LOCKED) == LOCKED)
		;
	/*
	 * Tell the C compiler and the processor to not move loads or stores
	 * past this point, to ensure that the critical section's memory
	 * references happen strictly after the lock is acquired.
	 * On RISC-V, this emits a "fence" instruction.
	 */
	__sync_synchronize();
}

void spin_lock_release(spin_lock_t *lock)
{
	if (lock->status == UNLOCKED)
	/*
	 * After supporting multicore, the condition may should be changed.
	 * If lock->status is LOCKED but it is not held by this CPU,
	 * panic_g() should also be called. See XV6: spinlock.c: release() and holding().
	 */
		panic_g("Trying to release an unlocked spin lock 0x%lx",
			(long)lock);
	/* NOTE: This is learned from XV6. */
	/*
	 * Tell the C compiler and the CPU to not move loads or stores
	 * past this point, to ensure that all the stores in the critical
	 * section are visible to other CPUs before the lock is released,
	 * and that loads in the critical section occur strictly before
	 * the lock is released.
	 * On RISC-V, this emits a fence instruction.
	 */
	__sync_synchronize();
	/*
	 * Release the lock, equivalent to lk->locked = 0.
	 * This code doesn't use a C assignment, since the C standard
	 * implies that an assignment might be implemented with
	 * multiple store instructions.
	 * On RISC-V, __sync_lock_release turns into an atomic swap:
	 * s1 = &lock->status, amoswap.w zero, zero, (s1)
	 */
	__sync_lock_release(&(lock->status));
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
	//pcb_t *p, *q;
	mutex_lock_t *ptlock;

	if (mlock_idx >= LOCK_NUM || mlock_idx < 0)
		return -1;	/* Invalid mlock_idx */
	ptlock = mlocks + mlock_idx;	/* Pointer to target lock */
	/*
	 * Only when we acquire the spin lock of the mutex lock,
	 * can we do operations on it.
	 */
	spin_lock_acquire(&(ptlock->slock));

	while (ptlock->status == LOCKED && ptlock->opid != current_running->pid)
	{
		/* Should be blocked */
		if (do_block(&(ptlock->block_queue), &(ptlock->slock)) != 0)
		/*
		 * block current_running process in block_queue and release the lock.
		 * The lock will be reacquired after being unblocked.
		 */
		{
			panic_g("do_mutex_lock_acquire: lock %d is LOCKED"
				" but no ready process is found", mlock_idx);
			return -1;
		}
	}
	/* Acquire the mutex lock */
	ptlock->status = LOCKED;
	ptlock->opid = current_running->pid;
	/* Remember to release the spin lock */
	spin_lock_release(&(ptlock->slock));
	return mlock_idx;
}

/*
 * Release mutex lock with ID mlock_idx.
 * Return mlock_idx or negative number on error.
 */
int do_mutex_lock_release(int mlock_idx)
{
	/* TODO: [p2-task2] release mutex lock */
	mutex_lock_t *ptlock;

	if (mlock_idx >= LOCK_NUM || mlock_idx < 0)
		return -1;	/* Invalid mlock_idx */
	ptlock = mlocks + mlock_idx;

	spin_lock_acquire(&(ptlock->slock));

	if (ptlock->status == LOCKED &&
		ptlock->opid != current_running->pid)
	{
		spin_lock_release(&(ptlock->slock));
		return -2;	/* The lock is LOCKED by other process */
	}
	if (ptlock->status == UNLOCKED)
	{
		spin_lock_release(&(ptlock->slock));
		return -3;
	}

	/*
	 * Unlock the mutex lock and unblock a process in the block_queue,
	 * in other word, "Signal" operation.
	 */
	ptlock->status = UNLOCKED;
	if (ptlock->block_queue != NULL)
		ptlock->block_queue = do_unblock(ptlock->block_queue);

	spin_lock_release(&(ptlock->slock));
	return mlock_idx;
}

/*
 * Release all resources (mutex locks, semaphores, ...)
 * occupied by proc kpid. Used when a proc is killed.
 */
void ress_release_killed(pid_t kpid)
{
	int i;

	for (i = 0; i < LOCK_NUM; ++i)
	{
		spin_lock_acquire(&(mlocks[i].slock));
		if (mlocks[i].status == LOCKED && mlocks[i].opid == kpid)
		{
			if (mlocks[i].block_queue == NULL)
				mlocks[i].status = UNLOCKED;
			else
			{
				pcb_t *temp;
				temp = mlocks[i].block_queue;
				mlocks[i].block_queue = do_unblock(mlocks[i].block_queue);
				mlocks[i].opid = temp->pid;
			}
		}
		spin_lock_release(&(mlocks[i].slock));
	}

	for (i = 0; i < SEMAPHORE_NUM; ++i)
	{
		spin_lock_acquire(&(semaphores[i].slock));
		if (semaphores[i].opid == kpid)
		{
			semaphores[i].opid = INVALID_PID;
			semaphores[i].value = 0;
			while (semaphores[i].block_queue != NULL)
				semaphores[i].block_queue = do_unblock(semaphores[i].block_queue);
		}
		spin_lock_release(&(semaphores[i].slock));
	}
}

void init_semaphores(void)
{
	int i;

	for (i = 0; i < SEMAPHORE_NUM; ++i)
	{
		spin_lock_init(&(semaphores[i].slock));
		semaphores[i].opid = INVALID_PID;
		semaphores[i].block_queue = NULL;
		semaphores[i].value = 0;
	}
}

/*
 * Init the semaphore specified by `key` with value `value`,
 * and return the index of the semaphore.
 * -1 will be returned on error.
 */
int do_semaphore_init(int key, int value)
{
	int sidx;

	/* Simply do this */
	sidx = (unsigned int)key % (unsigned int)SEMAPHORE_NUM;
	spin_lock_acquire(&(semaphores[sidx].slock));
	if (semaphores[sidx].opid != INVALID_PID &&
		semaphores[sidx].opid != current_running->pid)
	{	/* semaphore is occupied by other process */
		spin_lock_release(&(semaphores[sidx].slock));
		return -1;
	}
	semaphores[sidx].opid = current_running->pid;
	semaphores[sidx].value = value;
	spin_lock_release(&(semaphores[sidx].slock));
	return sidx;
}

/*
 * V operation of a semaphore.
 * Returns: value after operation and before spin lock is released,
 * or INT32_MIN on error.
 */
int do_semaphore_up(int sidx)
{
	semaphore_t *ptsema;
	int val = INT32_MIN;

	if (sidx >= SEMAPHORE_NUM || sidx < 0)
		return INT32_MIN;
	ptsema = semaphores + sidx;
	spin_lock_acquire(&(ptsema->slock));
	if (ptsema->opid == INVALID_PID)
		/* It is an invalid semaphore */
		goto err;
	if ((ptsema->value)++ < 0)
	{
		if (ptsema->block_queue == NULL)
			/*
			 * If a process in the block_queue is killed,
			 * this situation might happen, because it is removed from
			 * the queue but the value is not increased.
			 * Just set the value to 0 as I haven't found a better solution.
			 */
			ptsema->value = 0;
		else
			ptsema->block_queue = do_unblock(ptsema->block_queue);
	}
	/* Do a check */
	if (ptsema->value >= 0 && ptsema->block_queue != NULL)
		panic_g("do_semaphore_up: semaphore %d has nonnegative value %d "
			"but non-empty block_queue", sidx, ptsema->value);
	val = ptsema->value;
err:
	spin_lock_release(&(ptsema->slock));
	return val;
}

/*
 * P operation of a semaphore.
 * Returns: value before spin lock is released,
 * or INT32_MIN on error.
 */
int do_semaphore_down(int sidx)
{
	semaphore_t *ptsema;
	int val = INT32_MIN;

	if (sidx >= SEMAPHORE_NUM || sidx < 0)
		return INT32_MIN;
	ptsema = semaphores + sidx;
	spin_lock_acquire(&(ptsema->slock));
	if (ptsema->opid == INVALID_PID)
		/* It is an invalid semaphore */
		goto err;
	if (--ptsema->value < 0)
		do_block(&(ptsema->block_queue), &(ptsema->slock));
	val = ptsema->value;
err:
	spin_lock_release(&(ptsema->slock));
	return val;
}

/*
 * Destroy a semaphore and wakeup all processes in the queue.
 * Returns: sidx on success and INT32_MIN on error.
 */
int do_semaphore_destroy(int sidx)
{
	semaphore_t *ptsema;

	if (sidx >= SEMAPHORE_NUM || sidx < 0)
		return INT32_MIN;
	ptsema = semaphores + sidx;
	spin_lock_acquire(&(ptsema->slock));
	ptsema->opid = INVALID_PID;
	ptsema->value = 0;
	while (ptsema->block_queue != NULL)
		ptsema->block_queue = do_unblock(ptsema->block_queue);
	spin_lock_release(&(ptsema->slock));
	return sidx;
}