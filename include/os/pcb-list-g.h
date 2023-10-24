/* Operations for CIRCULAR linked list of PCB */
#ifndef __PCB_LIST_G_H__
#define __PCB_LIST_G_H__

#include <os/sched.h>

pcb_t *lpcb_add_node_to_tail(pcb_t * const Head, pcb_t * volatile *ppnew);
pcb_t *lpcb_insert_node(pcb_t * const Head, pcb_t * const Pnew, pcb_t * const Pprior);
pcb_t *lpcb_search_node(pcb_t * const Head, pid_t const Pid);
pcb_t *lpcb_del_node(pcb_t * const Head, pcb_t * const T, pcb_t **ppdel);


#endif