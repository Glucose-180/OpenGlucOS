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

int rep = 1;

int main(int argc, char *argv[])
{
	int i;
	int s_ymr = 0;

	if (argc > 1)
		rep = atoi(argv[1]);
	
	for (i = 0; i < NMSG; ++i)
		msglen[i] = strlen(msg[i]);
	
	while (rep-- > 0)
	{
		for (i = 0; i < NMSG; ++i)
		{
			sys_move_cursor(0, 0);
			if (sys_net_send(msg[i], msglen[i]) > 0)
			{
				++s_ymr;
				printf("> [SEND] totally send packet %d!  ", s_ymr);
			}
			else
			{
				printf("\n  Failed to send packet %d!   ", i);
				return 1;
			}
		}
	}
	return 0;
}