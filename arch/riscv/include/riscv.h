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

#endif