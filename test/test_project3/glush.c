
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/* The area (range of y) of terminal */
int terminal_begin = 18,
	terminal_end = 27;

/* Auto write log */
char flag_autolog = 0;

const char Terminal[] = 
	"------------------- Terminal -------------------\n";
const char NotFound[] =
	"**glush: command not found: ";

int try_syscall(char **cmds);

char *getcmd(void);
char **split(char *src, const char Sep);

/*
 * Format of argv[]: glush [terminal_begin] [terminal_end] [flag_autolog]
 */
int main(int argc, char *argv[])
{
	if (argc > 1)
		terminal_begin = atoi(argv[1]);
	if (argc > 2)
		terminal_end = atoi(argv[2]);
	if (argc > 3)
		flag_autolog = atoi(argv[3]);

	sys_move_cursor(0, terminal_begin);
	printf("%s", Terminal);

	if (flag_autolog != 0)
		printl("glush starts: terminal_begin is %d, terminal_end is %d",
			terminal_begin, terminal_end);

	while (1)
	{
		char **words;
		char *cmd;
		// ECHO for test
		//printf("%s\n", getcmd());

		// TODO [P3-task1]: call syscall to read UART port
		// TODO [P3-task1]: parse input
		// note: backspace maybe 8('\b') or 127(delete)
		// TODO [P3-task1]: ps, exec, kill, clear
		cmd = getcmd();
		printl("glush: %s", cmd);
		words = split(cmd, ' ');
		if (words[0] == NULL)
			continue;
		if (try_syscall(words) == 1)
		{	/* 
			 * Not a syscall. Try to execute a user program directly.
			 * But now, consider it as an error first.
			 */
			printf("%s%s\n", NotFound, words[0]);
		}

		/************************************************************/
		/* Do not touch this comment. Reserved for future projects. */
		/************************************************************/    
	}

	return 0;
}

/*
 * try_syscall: try to analyse cmds[0] as a syscall.
 * Returns: 0 on success, 1 if not found, 2 on error.
 */
int try_syscall(char **cmds)
{

	if (strcmp(cmds[0], "ps") == 0)
	{	/* ps */
		printf("%d user proc in total\n", sys_ps());
		return 0;
	}
	else if (strcmp(cmds[0], "clear") == 0)
	{	/* clear */
		sys_clear();
		sys_move_cursor(0, terminal_begin);
		printf("%s", Terminal);
		return 0;
	}
	else if (strcmp(cmds[0], "kill") == 0)
	{
		pid_t pid;

		if (cmds[1] == NULL)
		{
			printf("**glush: too few args for kill\n");
			return 2;
		}
		pid = atoi(cmds[1]);
		if (sys_kill(pid) != pid)
		{
			printf("**glush: failed to terminate proc %d\n", pid);
			return 2;
		}
		printf("proc %d is terminated\n", pid);
		return 0;
	}
	else if (strcmp(cmds[0], "exec") == 0)
	{	/* exec */
		pid_t pid;
		int i;
		char flag_background = 0;

		for (i = 1; cmds[i] != NULL; ++i)
			if (cmds[i][0] == '&')
			{
				flag_background = 1;
				cmds[i] = NULL;
			}

		if (cmds[1] == NULL)
		{
			printf("**glush: too few args for exec\n");
			return 2;
		}
		if (cmds[2] == NULL)
			/*
			 * No command argument:
			 * Test whether sys_exec() works when argv is NULL.
			 */
			pid = sys_exec(cmds[1], 2, NULL);
		else
			/* Test whether sys_exec() can determine argc auto. */
			pid = sys_exec(cmds[1], -1, cmds + 1);
		if (pid == INVALID_PID)
		{
			printf("**glush: failed to exec %s\n", cmds[1]);
			return 2;
		}
		if (flag_background == 0)
		{
			/*
			 * Call sys_waitpid() to wait the new process
			 */
		}
		return 0;
	}
	else if (strcmp(cmds[0], "kprint_avail_table") == 0)
	{
		sys_kprint_avail_table();
		return 0;
	}
	else if (strcmp(cmds[0], "ulog") == 0)
	{
		if (cmds[1] == NULL)
		{
			printf("**glush: too few args for ulog\n");
			return 2;
		}
		sys_ulog(cmds[1]);
		return 0;
	}
	else
		return 1;
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
		if (flag_new != 0 && *p != Sep)
		/*
		 * "*p != Sep" is added to avoid empty strings
		 * when two or more Sep are consecutive.
		 */
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