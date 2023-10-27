
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define SHELL_BEGIN 20

int main(void)
{
	sys_move_cursor(0, SHELL_BEGIN);
	printf("------------------- Terminal -------------------\n");
	printf("> root@UCAS_OS: ");

	while (1)
	{
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
