/* Operations for CIRCULAR linked list of PCB */
#ifndef __PCB_LIST_G_H__
#define __PCB_LIST_G_H__

#include <sched.h>

typedef pcb_t clist_node_t;

clist_node_t *add_node_to_tail(clist_node_t * const Head, clist_node_t **ppnew);
clist_node_t *search_node_pid(clist_node_t * const Head, pid_t const Pid);
clist_node_t *del_node(clist_node_t * const Head, clist_node_t * const T, clist_node_t **ppdel);


#endif