#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

int getchar(void)
{
#define QS 128	/* Size of queue */
#define QEMPTY (qhead == qtail)
#define QFULL ((qhead - qtail) % QS == 1)

/*
 * If it has been 90 sec since the user pressed a key,
 * lower the scanning frequency to save power.
 */
#define WTIME 90U

	/* Input buffer. It's a CIRCULAR queue. */
	static signed char ibuf[QS];
	static unsigned int qhead = 0,
		qtail = 0;	/* pointers of buffer */
	int ch;
	static unsigned long time_base = 0UL, tlast = 0UL;

	if (time_base == 0UL)
	{	/* Initialization */
		time_base = (unsigned long)sys_get_timebase();
		tlast = sys_get_tick() / time_base;
	}

	if (!QEMPTY)
	{
		ch = ibuf[qhead];	/* Depart */
		qhead = (qhead + 1) % QS;
		return ch;
	}

	while (1)
	{
		while ((ch = sys_bios_getchar()) == NOI)
		{
			if (sys_get_tick() / time_base - tlast >= WTIME)
				sys_sleep(2U);
		}
		
		tlast = sys_get_tick() / time_base;
		if (ch == '\b' || ch == '\177')
		{	/* backspace */
			if (!QEMPTY)
			{
				/* Remove the char at end of buffer */
				qtail = (qtail + QS - 1) % QS;
				//bios_putstr("\b \b");
				printf("%c %c", ch, ch);
			}
			continue;
		}
		else if ((iscrlf(ch) && !QEMPTY) || (!iscrlf(ch) && QFULL))
		{	/* Get CR/LF when buffer is not empty, or buffer is full */
			printf("\n");
			ch = ibuf[qhead];	/* Depart */
			qhead = (qhead + 1) % QS;
			ibuf[qtail] = '\n';	/* Enter */
			qtail = (qtail + 1) % QS;
			return ch;
		}
		else if (iscrlf(ch))
		{	/* Get CR/LF but buffer is empty */
			printf("\n");
			return '\n';
		}
		else
		{	/* Get normal char and buffer is not full */
			ibuf[qtail] = ch;	/* Enter */
			qtail = (qtail + 1) % QS;
			//bios_putchar(ch);
			printf("%c", ch);
		}
	}

#undef QEMPTY
#undef QFULL
#undef QS
}