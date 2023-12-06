#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/**
 * The ascii airplane is designed by Joan Stark
 * from: https://www.asciiart.eu/vehicles/airplanes
 */

static char blank[] = {"                                                                   "};
static char plane1[] = {"         _       "};
static char plane2[] = {"       -=\\`\\     "};
static char plane3[] = {"   |\\ ____\\_\\__  "};
static char plane4[] = {" -=\\c`\"\"\"\"\"\"\" \"`)"};
static char plane5[] = {"    `~~~~~/ /~~` "};
static char plane6[] = {"      -==/ /     "};
static char plane7[] = {"        '-'      "};

/**
 * NOTE: bios APIs is used for p2-task1 and p2-task2. You need to change
 * to syscall APIs after implementing syscall in p2-task3!
*/
int main(int argc, char *argv[])
{
	int j = 10;

	int rep_ymr = INT32_MAX;

	if (argc > 1)
		rep_ymr = atoi(argv[1]);

	while (rep_ymr-- > 0)
	{
		for (int i = 0; i < 50; i++)
		{
			/* move */
			sys_move_cursor(i, j + 0);
			printf("%s", plane1);

			sys_move_cursor(i, j + 1);
			printf("%s", plane2);

			sys_move_cursor(i, j + 2);
			printf("%s", plane3);

			sys_move_cursor(i, j + 3);
			printf("%s", plane4);

			sys_move_cursor(i, j + 4);
			printf("%s", plane5);

			sys_move_cursor(i, j + 5);
			printf("%s", plane6);

			sys_move_cursor(i, j + 6);
			printf("%s", plane7);
		}

		sys_move_cursor(0, j + 0);
		printf("%s", blank);

		sys_move_cursor(0, j + 1);
		printf("%s", blank);

		sys_move_cursor(0, j + 2);
		printf("%s", blank);

		sys_move_cursor(0, j + 3);
		printf("%s", blank);

		sys_move_cursor(0, j + 4);
		printf("%s", blank);

		sys_move_cursor(0, j + 5);
		printf("%s", blank);

		sys_move_cursor(0, j + 6);
		printf("%s", blank);
	}
	return 0;
}
