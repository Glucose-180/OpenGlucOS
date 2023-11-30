#ifndef __GLUCOSE_H__
#define __GLUCOSE_H__

#include <stdarg.h>

#ifndef SECTOR_SIZE
#define SECTOR_SIZE 512U
#endif

/* Return value of bios_getchar() when no key is pressed */
#define NOI (-1)

/*
 * Calc how many sectors are needed to store nbytes bytes
 */
static inline unsigned int nbytes2sectors(unsigned int nbytes)
{
	return (nbytes / SECTOR_SIZE) + (nbytes % SECTOR_SIZE != 0U ? 1U : 0U);
}

/*
 * Calc the sector index of the byte at `lbytes`
 */
static inline unsigned int lbytes2sectors(unsigned int lbytes)
{
	return (lbytes / SECTOR_SIZE);
}

/*
 * Whether ch is CR or LF
 */
static inline int iscrlf(int ch)
{
	return (ch == '\r' || ch == '\n');
}

/*
 * Whether ch is white
 */
static inline int isspace(int ch)
{
	return (ch == '\t' || ch == '\v' || ch == '\f' ||
		ch == ' ' || iscrlf(ch));
}

int getchar(void);
int getline(char* str, const int Len);
char *getcmd(void);
char **split(char *src, const char Sep);
void glucos_brake(void);
void panic(const char *file, const char *func, int line, const char *fmt, ...);
void writelog(const char *fmt, ...);
int do_ulog(const char *str);

/*
 * Call `panic()` function with file name, function name and line number.
 * Print error information and stop GlucOS from running.
 */
#define panic_g(fmt, ...) panic(__FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

/*
 * `##__VA_ARGS__`: the `##` means that this argument is optional.
 * This is learned from ChatGPT.
 */

#endif