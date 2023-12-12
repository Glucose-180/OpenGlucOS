/*
 * Callings of vt100_move_cursor has been modified by Glucose180.
 * As I find that the cursor on the screen is started from 1 rather than 0,
 * the arguments for calling vt100_move_cursor should be 1 greater than
 * cur_cpu()->cursor_x/y.
 */
#include <screen.h>
#include <printk.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/irq.h>
#include <os/kernel.h>
#include <os/ctype.h>

#include <os/smp.h>

#define SCREEN_WIDTH    80
#define SCREEN_HEIGHT   50
#define SCREEN_LOC(x, y) ((y) * SCREEN_WIDTH + (x))

/* screen buffer */
char new_screen[SCREEN_HEIGHT * SCREEN_WIDTH] = {0};
char old_screen[SCREEN_HEIGHT * SCREEN_WIDTH] = {0};

/* cursor position */
static void vt100_move_cursor(int x, int y)
{
	// \033[y;xH
	printv("%c[%d;%dH", 27, y, x);
}

/* clear screen */
static void vt100_clear()
{
	// \033[2J
	printv("%c[2J", 27);
}

/* hidden cursor */
static void vt100_hidden_cursor()
{
	// \033[?25l
	printv("%c[?25l", 27);
}

/* Show cursor: added by Glucose180 */
static void vt100_show_cursor()
{
	printv("%c[?25h", 27);
}

/* write a char */
void screen_write_ch(char ch)
{
	char flag_limited;
	pcb_t * ccpu = cur_cpu();

	if (ch == '\n')
	{
write_lf:
		ccpu->cursor_x = 0;
		if (ccpu->cursor_y < SCREEN_HEIGHT - 1)
		{
			ccpu->cursor_y++;
			flag_limited = 0;
		}
		else
			flag_limited = 1;
		if (ccpu->cylim_l >= 0 &&	/* Auto scroll */
			ccpu->cylim_h >= ccpu->cylim_l &&
			(
				ccpu->cursor_y > ccpu->cylim_h ||
				flag_limited == 1
			)
		)
		{
			int y, x;
			for (y = ccpu->cylim_l; y < ccpu->cylim_h; ++y)
			{
				for (x = 0; x < SCREEN_WIDTH; ++x)
					new_screen[SCREEN_LOC(x, y)] = new_screen[SCREEN_LOC(x, y + 1)];
			}
			ccpu->cursor_y = y;
			for (x = 0; x < SCREEN_WIDTH; ++x)
				new_screen[SCREEN_LOC(x, ccpu->cursor_y)] = ' ';
		}
		return;
	}
	else if (ch == '\b' || ch == '\177')
	{	/* Backspace: by Glucose180 */
		if (ccpu->cursor_x > 0)
			ccpu->cursor_x--;
		else if (ccpu->cursor_y > 0)
		{
			ccpu->cursor_y--;
			ccpu->cursor_x = SCREEN_WIDTH - 1;
		}
	}
	else
	{
		new_screen[SCREEN_LOC(ccpu->cursor_x, ccpu->cursor_y)] = ch;
		if (++ccpu->cursor_x >= SCREEN_WIDTH)
		{
			goto write_lf;
		}
	}
}

void init_screen(void)
{
	vt100_hidden_cursor();
	screen_clear();
	vt100_show_cursor();	// Added by Glucose180
}

void screen_clear(void)
{
	int i, j;

	vt100_clear();
	for (i = 0; i < SCREEN_HEIGHT; i++)
	{
		for (j = 0; j < SCREEN_WIDTH; j++)
		{
			new_screen[SCREEN_LOC(j, i)] = ' ';
			old_screen[SCREEN_LOC(j, i)] = ' ';
		}
	}
	cur_cpu()->cursor_x = 0;
	cur_cpu()->cursor_y = 0;
	screen_reflush();
}

/* Clear screen from ybegin to yend */
void screen_rclear(int ybegin, int yend)
{
	int i, j;

	if (ybegin >= 0 && ybegin < SCREEN_HEIGHT &&
		yend >= 0 && yend < SCREEN_HEIGHT)
	{
		for (i = ybegin; i < yend; ++i)
			for (j = 0; j < SCREEN_WIDTH; ++j)
				new_screen[SCREEN_LOC(j, i)] = ' ';
	}
	screen_reflush();
}

void screen_move_cursor(int x, int y)
{
	if (x >= SCREEN_WIDTH)
		x = SCREEN_WIDTH - 1;
	else if (x < 0)
		x = 0;
	if (y >= SCREEN_HEIGHT)
		y = SCREEN_HEIGHT - 1;
	else if (y < 0)
		y = 0;
	cur_cpu()->cursor_x = x;
	cur_cpu()->cursor_y = y;
	vt100_move_cursor(x + 1, y + 1);
}

/*
 * `screen_write`: write `len` chars from `buff`.
 * Return: bytes written.
 */
unsigned int screen_write(char *buff, unsigned int len)
{
/*	int i = 0;
	int l = strlen(buff);

	for (i = 0; i < l; i++)
	{
		screen_write_ch(buff[i]);
	}*/
	unsigned int rt;

	if (len > SCR_WRITE_MAX)
		len = SCR_WRITE_MAX;
	rt = len;
	while (len-- > 0U)
		screen_write_ch(*(buff++));
	screen_reflush();
	return rt;
}

/*
 * `do_screen_write`: this is for syscall from user.
 * `buff` is checked to ensure that it is from user space.
 */
unsigned int do_screen_write(char *buff, unsigned int len)
{
	if ((uintptr_t)buff >= KVA_MIN)
		return 0U;
	else
		return screen_write(buff, len);
}

/*
 * This function is used to print the serial port when the clock
 * interrupt is triggered. However, we need to pay attention to
 * the fact that in order to speed up printing, we only refresh
 * the characters that have been modified since this time.
 */
void screen_reflush(void)
{
	int i, j, i0 = -1, j0 = -2;

	/* here to reflush screen buffer to serial port */
	for (i = 0; i < SCREEN_HEIGHT; i++)
	{
		for (j = 0; j < SCREEN_WIDTH; j++)
		{
			/* We only print the data of the modified location. */
			if (new_screen[SCREEN_LOC(j, i)] != old_screen[SCREEN_LOC(j, i)])
			{
				if (!(i == i0 && j == j0 + 1))
				/* If they two are consecutive, don't move cursor. */
					vt100_move_cursor(j + 1, i + 1);
				bios_putchar(new_screen[SCREEN_LOC(j, i)]);
				old_screen[SCREEN_LOC(j, i)] = new_screen[SCREEN_LOC(j, i)];
				i0 = i;
				j0 = j;
			}
		}
	}

	/* recover cursor position */
	vt100_move_cursor(cur_cpu()->cursor_x + 1,
		cur_cpu()->cursor_y + 1);
}

/* Set cursor y limit */
void screen_set_cylim(int cylim_l, int cylim_h)
{
	cur_cpu()->cylim_l = cylim_l;
	cur_cpu()->cylim_h = cylim_h;
}