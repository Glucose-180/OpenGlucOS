/*
 * riscv.h: Provide inline functions about RISC-V architecture.
 * It is learned from XV6.
 */
#ifndef __RISCV_H__
#define __RISCV_H__

#include <type.h>

static inline reg_t r_sstatus()
{
	reg_t x;
	__asm__ volatile("csrr %0, sstatus" : "=r" (x) );
	return x;
}

static inline void w_sstatus(reg_t x)
{
	__asm__ volatile("csrw sstatus, %0" : : "r" (x));
}

static inline reg_t r_sepc()
{
	reg_t x;
	__asm__ volatile("csrr %0, sepc" : "=r" (x) );
	return x;
}

static inline void w_sepc(reg_t x)
{
	__asm__ volatile("csrw sepc, %0" : : "r" (x));
}

extern reg_t get_current_cpu_id(void);

/*
 * If this is secondary CPU, return 1;
 * otherwise, return 0.
 */
extern uint64_t is_scpu(void);

#endif