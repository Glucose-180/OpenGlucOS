/*
 * This program is to test blocking of sending and TXQE interrupt.
 * Usage: send3 [NPKT] [REP]
 * where `[NPKT]` is the number of packets and `[REP]` is
 * repeating times.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define NMSG 10

const char *msg[NMSG] = {
	"The train bound for Universal Resort is arriving!",
	"This train is bound for Universal Resort.",
	"The next station is Gongzhufen.",
	"Change here for Line 10.",
	"We are now arriving at Gongzhufen.",

	"The train is arriving, please mind the gap.",
	"This train stops service at Zhangguozhuang station.",

	"Buy tickets at 95306!",
	"Deliver goods at 12306!",
	"China railway wishes you a pleasant journey!"

};

int msglen[NMSG];

int npkt = 10, rep = 1;

int main(int argc, char *argv[])
{
	int i;
	int rt;
	char **vpkt;
	int *vlen;

	if (argc > 2)
	{
		npkt = atoi(argv[1]);
		rep = atoi(argv[2]);
	}
	if (npkt <= 0 || rep <= 0)
		return 1;
	
	for (i = 0; i < NMSG; ++i)
		msglen[i] = strlen(msg[i]);

	sys_move_cursor(0, 0);

	vpkt = (char **)malloc(npkt * sizeof(char *));
	vlen = (int *)malloc(npkt * sizeof(int));
	if (vpkt == NULL || vlen == NULL)
	{
		printf("**Failed to allocate memory for `vpkt` or `vlen`!");
		return 2;
	}

	for (i = 0; i < npkt; ++i)
		if ((vpkt[i] = (char*)malloc(vlen[i] = msglen[i % NMSG])) == NULL)
		{
			printf("**Failed to allocate %d bytes memory for `vpkt[%d]`!",
				vlen[i], i);
			return 2;
		}
		else
			strncpy(vpkt[i], msg[i % NMSG], vlen[i]);
	
	for (i = 0; i < rep; ++i)
	{
		sys_move_cursor(0, 0);
		rt = sys_net_send_array((const void**)vpkt, vlen, npkt);
		if (rt > 0)
			printf("> [SEND] %d packets (len %d) have been sent %d times!",
				npkt, rt, i + 1);
		else
		{
			printf("> [SEND] Failed to send %d-th packet!", i);
			return -1;
		}
	}
	return 0;
}