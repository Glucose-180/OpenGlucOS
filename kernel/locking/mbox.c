#include <os/lock.h>

static mailbox_t mailboxes[MBOX_NUM];

void init_mbox();
int do_mbox_open(char *name);
void do_mbox_close(int midx);
int do_mbox_send(int midx, uint8_t* msg, unsigned int msg_length);
int do_mbox_recv(int midx, uint8_t* msg, unsigned int msg_length);
