#ifndef INCLUDE_STDIO_H_
#define INCLUDE_STDIO_H_

#include <stdarg.h>

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list va);
int printl(const char *fmt, ...);
int getchar(void);
int getline(char* str, const int Len);

/* No input value returned by bios_getchar() */
#define NOI (-1)

#endif
