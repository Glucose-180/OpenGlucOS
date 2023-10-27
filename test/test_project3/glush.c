
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/* The area (range of y) of terminal */
int terminal_begin = 18,
	terminal_end = 27;

char *getcmd(void);

int main(void)
{
	sys_move_cursor(0, terminal_begin);
	printf("------------------- Terminal -------------------\n");

	while (1)
	{
		// ECHO for test
		printf("%s\n", getcmd());
		// TODO [P3-task1]: call syscall to read UART port
		
		// TODO [P3-task1]: parse input
		// note: backspace maybe 8('\b') or 127(delete)

		// TODO [P3-task1]: ps, exec, kill, clear    

		/************************************************************/
		/* Do not touch this comment. Reserved for future projects. */
		/************************************************************/    
	}

	return 0;
}

char *getcmd()
{
#define CS 126
	static char cbuf[CS];

	printf("%s@%s:~$ ", USER_NAME, OS_NAME);

	getline(cbuf, CS);
	trim(cbuf);
	return cbuf;
#undef CS
}