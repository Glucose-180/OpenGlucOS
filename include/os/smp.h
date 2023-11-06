#ifndef SMP_H
#define SMP_H

#define NR_CPUS 2
extern void smp_init();
extern void wakeup_other_hart();
extern uint64_t get_current_cpu_id();

/*
 * If this is secondary CPU, return 1;
 * otherwise, return 0.
 */
extern uint64_t is_scpu(void);

extern void lock_kernel();
extern void unlock_kernel();

pcb_t *cur_cpu(void);

#endif /* SMP_H */
