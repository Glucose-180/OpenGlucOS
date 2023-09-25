/*
 * glucose.c: library of frequently-used functions
 * written by myself (Glucose180)
 */
#include <os/kernel.h>
#include <os/glucose.h>

/*
 * Remove white spaces at the beginning and end of Str
 */
void trim(char *const Str)
{
	char *s = Str, *t = Str;

	while (isspace(*t))
		++t;
	while ((*s++ = *t++))
		;
	while (s-- > Str && (*s == '\0' || isspace(*s)))
		;
	*++s = '\0';
}

int getchar()
{
#define QS 32	/* Size of queue */
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
		if (ch == '\b')
		{	/* backspace */
			if (!QEMPTY)
			{
				/* Remove the char at end of buffer */
				qtail = (qtail + QS - 1) % QS;
				bios_putstr("\b \b");
			}
			continue;
		}
		else if ((iscrlf(ch) && !QEMPTY) || (!iscrlf(ch) && QFULL))
		{	/* Get CR/LF when buffer is not empty, or buffer is full */
			bios_putchar('\n');
			ch = ibuf[qhead];	/* Depart */
			qhead = (qhead + 1) % QS;
			ibuf[qtail] = '\n';	/* Enter */
			qtail = (qtail + 1) % QS;
			return ch;
		}
		else if (iscrlf(ch))
		{	/* Get CR/LF but buffer is empty */
			bios_putchar('\n');
			return '\n';
		}
		else
		{	/* Get normal char and buffer is not full */
			ibuf[qtail] = ch;	/* Enter */
			qtail = (qtail + 1) % QS;
			bios_putchar(ch);
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
#define CS 30
	static char cbuf[CS];

	bios_putstr(USER_NAME);
	bios_putchar('@');
	bios_putstr(OS_NAME);
	bios_putstr(":~$ ");

	getline(cbuf, CS);
	trim(cbuf);
	return cbuf;
#undef CS
}