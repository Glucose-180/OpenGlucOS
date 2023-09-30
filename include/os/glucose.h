#ifndef __GLUCOSE_H__
#define __GLUCOSE_H__

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
 * Calc the sector number of the byte at lbytes
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

void trim(char *const Str);
int getchar(void);
int getline(char* str, const int Len);
char *getcmd(void);
void panic_g(const char *fmt, ...);

#endif