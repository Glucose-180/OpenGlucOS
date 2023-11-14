/*
 * glucose.c: library of frequently-used functions
 * written by myself (Glucose180)
 */
#include <os/kernel.h>
#include <os/glucose.h>
#include <stdarg.h>
#include <printk.h>
#include <os/irq.h>
#include <os/string.h>
#include <os/time.h>
#include <os/smp.h>

/*
 * Remove white spaces at the beginning and end of Str.
 * This function has been moved to string.c.
 */
/*void trim(char *const Str)
{
	char *s = Str, *t = Str;

	while (isspace(*t))
		++t;
	while ((*s++ = *t++))
		;
	while (s-- > Str && (*s == '\0' || isspace(*s)))
		;
	*++s = '\0';
}*/

int getchar()
{
#define QS 128	/* Size of queue */
#define QEMPTY (qhead == qtail)
#define QFULL ((qhead - qtail) % QS == 1)

	/* Input buffer. It's a CIRCULAR queue. */
	static signed char ibuf[QS];
	static unsigned int qhead = 0,
		qtail = 0;	/* pointers of buffer */
	int ch;

	if (!QEMPTY)
	{
		ch = ibuf[qhead];	/* Depart */
		qhead = (qhead + 1) % QS;
		return ch;
	}

	while (1)
	{
		while ((ch = bios_getchar()) == NOI)
			;
		if (ch == '\b' || ch == '\177')
		{	/* backspace */
			if (!QEMPTY)
			{
				/* Remove the char at end of buffer */
				qtail = (qtail + QS - 1) % QS;
				//bios_putstr("\b \b");
				printk("%c %c", ch, ch);
			}
			continue;
		}
		else if ((iscrlf(ch) && !QEMPTY) || (!iscrlf(ch) && QFULL))
		{	/* Get CR/LF when buffer is not empty, or buffer is full */
			//bios_putchar('\n');
			printk("\n");
			ch = ibuf[qhead];	/* Depart */
			qhead = (qhead + 1) % QS;
			ibuf[qtail] = '\n';	/* Enter */
			qtail = (qtail + 1) % QS;
			return ch;
		}
		else if (iscrlf(ch))
		{	/* Get CR/LF but buffer is empty */
			//bios_putchar('\n');
			printk("\n");
			return '\n';
		}
		else
		{	/* Get normal char and buffer is not full */
			ibuf[qtail] = ch;	/* Enter */
			qtail = (qtail + 1) % QS;
			//bios_putchar(ch);
			printk("%c", ch);
		}
	}

#undef QEMPTY
#undef QFULL
#undef QS
}

/*
 * Read a line and return its length.
 * Please set Len as the size of str[].
 */
int getline(char* str, const int Len)
{
	int i, c;
	
	for (i = 0; i < Len - 1 && (c = getchar()) != NOI && c != '\n'; ++i)
		str[i] = c;
	if (c == '\n')
		str[i++] = c;
	str[i] = '\0';
	return i;
}

/*
 * Print command promot and read a line of command.
 */
char *getcmd()
{
#define CS 126
	static char cbuf[CS];

	printk("%s@%s:~$ ", USER_NAME, OS_NAME);

	getline(cbuf, CS);
	trim(cbuf);
	return cbuf;
#undef CS
}

/*
 * Split the string src using Sep as seperator and
 * return an array of pointers with end NULL.
 * NOTE: src may be modified!
 */
char **split(char *src, const char Sep)
{
#define CM 10U
	unsigned int i;
	static char *rt[CM + 1];
	char *p;
	char flag_new = 1;

	for (i = 0U, p = src; *p != '\0'; ++p)
	{
		if (flag_new != 0)
			rt[i++] = p, flag_new = 0;
		if (*p == Sep)
		{
			*p = '\0', flag_new = 1;
			if (i >= CM)
				break;
		}
	}
	rt[i] = NULL;
	return rt;
#undef CM
}

/*
 * Disenable interrupt and stop running.
 */
void glucos_brake(void)
{
	disable_interrupt();
#if DEBUG_EN != 0
	writelog("CPU %lu applied brake", get_current_cpu_id());
#endif
	while (1)
		__asm__ volatile("wfi");
}

/*
 * Print error information
 */
void panic_g(const char *fmt, ...)
{
	va_list va;
	int _vprint(const char *fmt, va_list _va, void (*output)(char*));

	disable_interrupt();
	printv("\n**CPU %lu Panic: ", get_current_cpu_id());

#if DEBUG_EN != 0
	uint64_t time;
	time = get_timer();
	printl("[t=%04lus] **CPU %lu Panic: ", time, get_current_cpu_id());
#endif

	va_start(va, fmt);
	_vprint(fmt, va, bios_putstr);
#if DEBUG_EN != 0
	_vprint(fmt, va, bios_logging);
	printl("**\n");
#endif
	va_end(va);

	printv("**\n");
	glucos_brake();
	while (1)
		;
}

/*
 * writelog: write log in the specified file
 * with time lable.
 */
void writelog(const char *fmt, ...)
{
	va_list va;
	uint64_t time;
	int _vprint(const char *fmt, va_list _va, void (*output)(char*));

	time = get_timer();
	printl("[t=%04lus] ", time);

	va_start(va, fmt);
	_vprint(fmt, va, bios_logging);
	va_end(va);

	printl("\n");
}

/*
 * do_ulog: provide a syscall for user program to
 * write log. Only write a time lable and a string.
 */
int do_ulog(const char *str)
{
	uint64_t time;

	time = get_timer();
	return printl("[t=%04lus] %s\n", time, str);
}