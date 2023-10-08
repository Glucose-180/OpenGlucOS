#ifndef INCLUDE_PRINTK_H_
#define INCLUDE_PRINTK_H_

#include <stdarg.h>

int vprintk(const char *fmt, va_list _va);

/* kernel print */
int printk(const char *fmt, ...);

/* vt100 print */
int printv(const char *fmt, ...);

/* (QEMU-only) save print content to logfile */
int printl(const char *fmt, ...);

#endif
