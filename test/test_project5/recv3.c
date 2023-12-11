/*
 * This program is used to test reliable data transmission with GNTP.
 * Usage: recv3 -l[LEN] [-c] [-p[PLOC]]
 * where [LEN] is the number of bytes to be received,
 * [PLOC] is the bottom of printing area,
 * and `-c` means that print data in char format rather than hex.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int print_location = 15;
int recv_len = 48;
int flag_char = 0;

#define LINEMAX_CHAR 48
#define LINEMAX_HEX 16

int main(int argc, char *argv[])
{
	char *rbuf;
	int rt;
	int i, j;

	while (--argc)
	{
		++argv;
		if (strcmp("-c", *argv) == 0)
			flag_char = 1;
		else if (strncmp("-l", *argv, 2) == 0)
			recv_len = atoi(*argv + 2);
		else if (strncmp("-p", *argv, 2) == 0)
			print_location = atoi(*argv + 2);
	}
	if (print_location < 0 || recv_len <= 0)
	{
		printf("**Invalid argument!\n");
		return 1;
	}
	sys_set_cylim(0, print_location);
	if ((rbuf = malloc(recv_len + 16)) == NULL)
	{
		printf("**Failed to allocate memory!\n");
		return 2;
	}
	printf("Waiting for %d bytes data...", recv_len);
	while ((rt = sys_net_recv_stream((void*)rbuf, recv_len)) > 0)
	{
		sys_move_cursor(0, 0);
		printf("%d bytes of %u B file are received using GNTP:\n", rt, *(uint32_t*)rbuf);
		if (flag_char != 0)
		{
			for (i = j = 0; i < recv_len; ++i)
			{
				int c = rbuf[i];
				if (c >= 32 && c <= 126)
				{
					printf("%c", c);
					++j;
				}
				else if (c == '\n' || c == '\r' || j >= LINEMAX_CHAR)
				{
					printf("\n");
					j = 0;
				}
				else
				{
					printf(".");
					++j;
				}
			}
		}
		else
		{
			for (i = 0; i < (recv_len + LINEMAX_HEX - 1) / LINEMAX_HEX; ++i)
			{
				for (j = 0; j < LINEMAX_HEX && (i * LINEMAX_HEX + j < recv_len); ++j)
					printf("%02x ", (int)rbuf[i * LINEMAX_HEX + j]);
				printf("\n");
			}
		}
	}
	printf("**Failed to receive %d bytes\n", recv_len);
	return 3;
}