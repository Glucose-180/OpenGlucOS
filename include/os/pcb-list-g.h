/* Operations for CIRCULAR linked list of PCB */
#ifndef __PCB_LIST_G_H__
#define __PCB_LIST_G_H__

#include <os/sched.h>

typedef pcb_t clist_node_t;

clist_node_t *add_node_to_tail(clist_node_t * const Head, clist_node_t * volatile *ppnew);
clist_node_t *insert_node(clist_node_t * const Head, clist_node_t * const Pnew, clist_node_t * const Pprior);
clist_node_t *search_node_pid(clist_node_t * const Head, pid_t const Pid);
clist_node_t *del_node(clist_node_t * const Head, clist_node_t * const T, clist_node_t **ppdel);


#endif