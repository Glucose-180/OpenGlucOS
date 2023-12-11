#ifndef __INCLUDE_NET_H__
#define __INCLUDE_NET_H__

#include <os/list.h>
#include <type.h>

#define PKT_NUM 32

int do_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens);
int do_net_send(const void *txpacket, int length);
int do_net_send_array(const void **vpkt, int *vlen, int npkt);

unsigned int do_net_recv_stream(uint8_t *buf, int len);

void check_sleeping_on_nic(void);

#endif  // !__INCLUDE_NET_H__