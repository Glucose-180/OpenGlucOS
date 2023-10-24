#ifndef __TCB_LIST_G__
#define __TCB_LIST_G__

tcb_t *ltcb_add_node_to_tail(tcb_t * const Head, tcb_t * volatile *ppnew);
tcb_t *ltcb_search_node(tcb_t * const Head, tid_t const Tid);
tcb_t *ltcb_del_node(tcb_t * const Head, tcb_t * const T, tcb_t **ppdel);

#endif