
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/* Command length */
#define CS 126
/* History length */
#define HL 6

unsigned int h_ymr = 0U;

/* The area (range of y) of terminal */
int terminal_begin = 18,
	terminal_end = 27;

/* Auto write log */
char flag_autolog = 0;

const char Terminal[] = 
	"------------------- Terminal -------------------\n";
const char NotFound[] =
	"**glush: command not found: ";

char cmd_history[HL][CS];

int try_syscall(char **cmds);
int try_builtin(char **cmds);
void history_write(const char *cmd);
char *history_read(unsigned int id);
char *getcmd(void);
char **split(char *src, const char Sep);

/*
 * Format of argv[]: glush [terminal_begin] [terminal_end] [flag_autolog]
 */
int main(int argc, char *argv[])
{
	unsigned int history_id;

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
	
	sys_set_cylim(terminal_begin + 1, terminal_end);

	while (1)
	{
		char **words, **words_next;
		char *cmd;
		// ECHO for test
		//printf("%s\n", getcmd());

		// TODO [P3-task1]: call syscall to read UART port
		// TODO [P3-task1]: parse input
		// note: backspace maybe 8('\b') or 127(delete)
		// TODO [P3-task1]: ps, exec, kill, clear
		cmd = getcmd();
		if (cmd[0] == '!')
		{	/* Read history */
			history_id = (unsigned int)atoi(cmd + 1);
			if (history_id < h_ymr)
			{
				cmd = history_read(history_id);
				printf("  %s\n", cmd);
			}
			else
			{
				printf("**glush: !%u: event not found\n", history_id);
				continue;
			}
		}
		else
			history_write(cmd);
		if (flag_autolog != 0)
			printl("glush: %s", cmd);
		words = split(cmd, ' ');
		do {
			/*
			 * We consider strings connected by "&&" are many commands,
			 * so deal with them one by one.
			 */
			for (words_next = words; *words_next != NULL; ++words_next)
				if (strcmp(*words_next, "&&") == 0)
					break;
			if (*words_next != NULL)
				/*
				 * "&&" is found: let *words_next=NULL to mark an end of a command,
				 * and words_next++ to let it points to the start of next command
				 * so that words can be updated to it the next time.
				 */
				*(words_next++) = NULL;
			else
				/*
				 * "&&" is not found: let words_next=NULL as a flag to jump out
				 * of this "while" and get command from input again.
				 */
				words_next = NULL;

			if (words[0] == NULL)
				continue;
			if (try_builtin(words) == 1 && try_syscall(words) == 1)
			{	/* 
				 * Not a syscall. Nor a builtin command.
				 * Consider it as an error.
				 */
				printf("%s%s\n", NotFound, words[0]);
			}
			if (words_next != NULL)
				words = words_next;
		} while (words_next != NULL);

		/************************************************************/
		/* Do not touch this comment. Reserved for future projects. */
		/************************************************************/    
	}

	return 0;
}

/*
 * try_syscall: try to analyse cmds[0] as a syscall.
 * Returns: 0 on success, 1 if not found, 2 on error.
 * "&" can be used to connect multiple syscalls.
 */
int try_syscall(char **cmds)
{
	char **cmds_next;

	do {
		for (cmds_next = cmds; *cmds_next != NULL; ++cmds_next)
			if (strcmp(*cmds_next, "&") == 0)
				break;
		if (*cmds_next != NULL)
			*(cmds_next++) = NULL;
		else
			cmds_next = NULL;

		/* Start with cmds[0]... */
		{
			if (strcmp(cmds[0], "ps") == 0)
			{	/* ps */
				printf("%d user proc in total\n", sys_ps());
				continue;
			}
			else if (strcmp(cmds[0], "clear") == 0)
			{	/* clear */
				sys_clear();
				sys_move_cursor(0, terminal_begin);
				printf("%s", Terminal);
				continue;
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
				continue;
			}
			else if (strcmp(cmds[0], "exec") == 0)
			{	/* exec */
				pid_t pid;

				if (cmds[1] == NULL)
				{
					printf("**glush: too few args for exec\n");
					return 2;
				}
				pid = sys_exec(cmds[1], -1, cmds + 1);
				if (pid == INVALID_PID)
				{
					printf("**glush: failed to exec %s\n", cmds[1]);
					return 2;
				}
				if (cmds_next == NULL)
				{	/* "&" is not found */
					/* Call sys_waitpid() to wait the new process */
					if (sys_waitpid(pid) != pid)
						printf("**glush: failed to wait for %d\n", pid);
				}
				else
					printf("%s: PID is %d\n", cmds[1], pid);
				continue;
			}
			else if (strcmp(cmds[0], "kprint_avail_table") == 0)
			{
				sys_kprint_avail_table();
				continue;
			}
			else if (strcmp(cmds[0], "ulog") == 0)
			{
				if (cmds[1] == NULL)
				{
					printf("**glush: too few args for ulog\n");
					return 2;
				}
				sys_ulog(cmds[1]);
				continue;
			}
			else if (strcmp(cmds[0], "sleep") == 0)
			{
				int stime;
				if (cmds[1] == NULL)
				{
					printf("**glush: too few args for sleep\n");
					return 2;
				}
				if ((stime = atoi(cmds[1])) < 0)
					stime = 0;
				printf("glush: sleeping for %d sec...\n", stime);
				sys_sleep(stime);
				continue;
			}
			else if (strcmp(cmds[0], "taskset") == 0)
			{
				int cmprt;
				int cpu_mask;
				pid_t pid;

				if (cmds[1] == NULL || cmds[2] == NULL ||
					((cmprt = strcmp(cmds[1], "-p")) == 0 && cmds[3] == NULL))
				{
					printf("**glush: too few args for taskset\n");
					return 2;
				}
				if (cmprt != 0)
				{	/* Not "-p", so create */
					cpu_mask = atoi(cmds[1]);
					if ((pid = sys_taskset(1, cmds[2], 0, cpu_mask)) == INVALID_PID)
					{
						printf("**glush: failed to exec %s\n", cmds[2]);
						return 2;
					}
					if (cmds_next == NULL)
					{	/* "&" is not found */
						/* Call sys_waitpid() to wait the new process */
						if (sys_waitpid(pid) != pid)
							printf("**glush: failed to wait for %d\n", pid);
					}
					else
						printf("%s: PID is %d with CPU mask 0x%x\n", cmds[2], pid, cpu_mask);
				}
				else
				{	/* "-p", so just set */
					cpu_mask = atoi(cmds[2]);
					pid = atoi(cmds[3]);
					if (sys_taskset(0, NULL, pid, cpu_mask) == INVALID_PID)
					{
						printf("**glush: failed to set mask 0x%x of proc %d\n", cpu_mask, pid);
						return 2;
					}
					else
						printf("CPU mask of proc %d is set to 0x%x\n", pid, cpu_mask);
				}
				continue;
			}
			else if (strcmp(cmds[0], "mkfs") == 0)
			{
				int rt;
				if (cmds[1] != NULL && strncmp(cmds[1], "-f", 2U) == 0)
					rt = sys_mkfs(1);
				else
					rt = sys_mkfs(0);
				if (rt != 0)
				{
					printf("**glush: failed to initialize GFS: %d\n", rt);
					if (rt != -2)
						printf("There is already %svalid GFS. Try using \"-f\".\n",
							rt < 0 ? "a " : "an in");
					else
						printf("Some files or directories are opened by processes.\n");
				}
				continue;
			}
			else if (strcmp(cmds[0], "statfs") == 0)
			{
				int rt;
				rt = sys_fsinfo();
				if (rt < 0)
					printf("**glush: there is no GFS at all.\n");
				else if (rt > 0)
					printf("**glush: the GFS is invalid: %d\n", rt);
				continue;
			}
			else if (strcmp(cmds[0], "cd") == 0)
			{
				int rt;
				if (cmds[1] == NULL)
					printf("**glush: too few args for cd\n");
				else if ((rt = sys_changedir(cmds[1])) == 1)
					printf("**glush: No such directory\n");
				else if (rt == 2)
					printf("**glush: path is too long\n");
				continue;
			}
			else if (strcmp(cmds[0], "ls") == 0)
			{
				int rt;
				if (cmds[1] != NULL && strncmp(cmds[1], "-l", 2) == 0)
					/* To support "-lh" */
					rt = sys_readdir(cmds[2], cmds[1][2] == 'h' ? 3 : 1);
				else
					rt = sys_readdir(cmds[1], 0);
				if (rt == 0)
					printf("**glush: No such directory\n");
				continue;
			}
			else if (strcmp(cmds[0], "mkdir") == 0)
			{
				int rt;
				if (cmds[1] == NULL)
					printf("**glush: too few args for mkdir\n");
				else if ((rt = sys_mkdir(cmds[1])) == 1 || rt == 2)
					printf("**glush: No such directory\n");
				else if (rt == 3)
					printf("**glush: dir is full\n");
				else if (rt == 4)
					printf("**glush: file already exists\n");
				else if (rt != 0)
					printf("**glush: GFS fault: %d\n", rt);
				continue;
			}
			else if (strcmp(cmds[0], "rm") == 0)
			{
				int rt;
				if (cmds[1] == NULL)
					printf("**glush: too few args for rm\n");
				else if ((rt = sys_rm(cmds[1])) == 1)
					printf("**glush: No such file or directory\n");
				else if (rt == 2)
					printf("**glush: the file or directory is occupied\n");
				else if (rt < 0)
					printf("**glush: GFS fault: %d\n", rt);
				continue;
			}
			else if (strcmp(cmds[0], "touch") == 0)
			{
				int rt;
				if (cmds[1] == NULL)
					printf("**glush: too few args for touch\n");
				else if ((rt = sys_open(cmds[1], O_CREAT)) == -2)
					printf("**glush: No such directory\n");
				else if (rt == -1)
					printf("**glush: %s is a directory\n", cmds[1]);
				else if (rt < 0)
					printf("**glush: GFS fault: %d\n", rt);
				else
					if (sys_close(rt) != 0)
						printf("**glush: failed to close the file\n");
				continue;
			}
			else if (strcmp(cmds[0], "ln") == 0)
			{
				int rt;
				if (cmds[1] == NULL || cmds[2] == NULL)
					printf("**glush: too few args for ln\n");
				else if ((rt = sys_hlink(cmds[1], cmds[2])) == -1)
					printf("**glush: No such file %s\n", cmds[1]);
				else if (rt == -2)
					printf("**glush: hard link not allowed for directory\n");
				else if (rt == -3)
					printf("**glush: links of file %s are full\n", cmds[1]);
				else if (rt == -4 || rt == -5)
					printf("**glush: cannot create link %s: %d\n", cmds[2], rt);
				else if (rt == -6)
					printf("**glush: %s already exists\n", cmds[2]);
				else if (rt < 0)
					printf("**glush: GFS fault: %d\n", rt);
			}
			else
			{	/* Try exec again */
				pid_t pid;

				pid = sys_exec(cmds[0], -1, cmds);
				if (pid == INVALID_PID)
				{	/* Failed */
					return 1;
				}
				if (cmds_next == NULL)
				{	/* "&" is not found */
					/* Call sys_waitpid() to wait the new process */
					if (sys_waitpid(pid) != pid)
						printf("**glush: failed to wait for %d\n", pid);
				}
				else
					printf("%s: PID is %d\n", cmds[0], pid);
				continue;
			}
		}
	} while (cmds_next != NULL && *(cmds = cmds_next) != NULL);
	/*
	 * cmds_next==NULL means that no "&" is found.
	 * *cmds==NULL means that the last "&" has been dealed
	 * and there is nothing after it.
	 */
	return 0;
}

/*
 * try_builtin: try to analyse cmds[0] as a builtin command.
 * Returns: 0 on success, 1 if not found, 2 on error.
 */
int try_builtin(char **cmds)
{
	if (strcmp(cmds[0], "range") == 0)
	{
		int y1, y2;
		if (cmds[1] == NULL || cmds[2] == NULL)
		{
			printf("**glush: too few args for range\n");
			return 2;
		}
		if ((y1 = atoi(cmds[1])) < 0 || (y2 = atoi(cmds[2])) < 0)
		{
			printf("**glush: invalid args for range\n");
			return 2;
		}
		terminal_begin = (y1 > y2 ? y2 : y1);
		terminal_end = (y1 > y2 ? y1 : y2);
		sys_set_cylim(terminal_begin + 1, terminal_end);
		sys_rclear(terminal_begin, terminal_end);
		sys_move_cursor(0, terminal_begin);
		printf("%s", Terminal);
		return 0;
	}
	else if (strcmp(cmds[0], "history") == 0)
	{
		unsigned int i;

		for (i = 0U; i < h_ymr; ++i)
			printf("  %u  %s\n", i, cmd_history[i]);
		return 0;
	}
	else
		return 1;
}

char *getcmd()
{
	static char cbuf[CS];
	char cpath[100];

	sys_getpath(cpath);
	printf("%s@%s:%s$ ", USER_NAME, OS_NAME, cpath);

	getline(cbuf, CS);
	trim(cbuf);
	return cbuf;
}

/*
 * Add a command to history.
 */
void history_write(const char *cmd)
{
	unsigned int i;

	if (h_ymr < HL)
		strcpy(cmd_history[h_ymr++], cmd);
	else
	{
		for (i = 1U; i < HL; ++i)
			strcpy(cmd_history[i - 1U], cmd_history[i]);
		strcpy(cmd_history[HL - 1U], cmd);
	}
}

/*
 * Read a command in history specified by `id`.
 * Return a pointer to it or `NULL` on error.
 */
char *history_read(unsigned int id)
{
	static char buf[CS];

	if (id >= h_ymr)
		return NULL;
	strcpy(buf, cmd_history[id]);
	return buf;
}

/*
 * Split the string src using Sep as seperator and
 * return an array of pointers with end NULL.
 * NOTE: src may be modified!
 */
char **split(char *src, const char Sep)
{
#define CM 20U
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