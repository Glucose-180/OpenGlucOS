/*
 * pcb-list-g.c: by Glucose180
 * Operations for CIRCULAR linked list of PCB.
 */
#include <os/sched.h>
#include <os/malloc-g.h>

/*
 * lpcb_add_node_to_tail: Add a new node to the tail of list pointed by Head,
 * and returns head pointer of new list.
 * Then, *ppnew will points to the new node.
 * NULL will be returned on error.
 */
pcb_t *lpcb_add_node_to_tail(pcb_t * const Head, pcb_t * volatile *ppnew,
	pcb_t ** const Phead)
{
	pcb_t *p;

	*ppnew = kmalloc_g(sizeof(pcb_t));
	if (*ppnew == NULL)
		return NULL;
	if (Head == NULL)
	{	/* Empty list */
		(*ppnew)->next = *ppnew;
		(*ppnew)->phead = Phead;
		return *ppnew;
	}
	for (p = Head; p->next != Head; p = p->next)
		;
	p->next = *ppnew;
	(*ppnew)->next = Head;
	(*ppnew)->phead = Phead;
	return Head;
}

/*
 * lpcb_insert_node: insert a node after the node pointed by Pprior.
 * If Pprior is NULL, *Pnew will be inserted to tail.
 * Otherwise, if Pprior is not found in this list, NULL will be returned.
 */
pcb_t *lpcb_insert_node(pcb_t * const Head, pcb_t * const Pnew, pcb_t * const Pprior,
	pcb_t ** const Phead)
{
	pcb_t *p;

	if (Head == NULL)
	{	/* Empty list */
		if (Pprior == NULL)
		{
			Pnew->phead = Phead;
			return Pnew->next = Pnew;
		}
		else
			return NULL;
	}
	/* Search the list */
	for (p = Head; p->next != Head; p = p->next)
		if (p == Pprior)
			break;
	if (p != Pprior && Pprior != NULL)
		return NULL;
	Pnew->next = p->next;
	p->next = Pnew;
	Pnew->phead = Phead;
	return Head;
}

/*
 * lpcb_search_node: Search a node according to `pid`.
 * Return pointer to it, or NULL if not found.
 */
pcb_t *lpcb_search_node(pcb_t * const Head, pid_t const Pid)
{
	pcb_t *p;

	if (Head == NULL || Head->pid == Pid)
		return Head;
	for (p = Head->next; p != Head; p = p->next)
		if (p->pid == Pid)
			return p;
	return NULL;
}

#if MULTITHREADING != 0

/*
 * lpcb_search_node: Search a node according to `pid` AND `tid`.
 * Return pointer to it, or NULL if not found.
 * This function is used for multithreading.
 */
pcb_t *lpcb_search_node_tcb(pcb_t * const Head, pid_t const Pid, tid_t const Tid)
{
	pcb_t *p;

	if (Head == NULL || (Head->pid == Pid && Head->tid == Tid))
		return Head;
	for (p = Head->next; p != Head; p = p->next)
		if (p->pid == Pid && p->tid == Tid)
			return p;
	return NULL;
}
#endif

/*
 * lpcb_del_node: Delete a node pointed by T,
 * and returns head pointer to new list.
 * Then, *ppdel will pointes to the deleted node.
 * If the node is not found, *ppdel will be NULL.
 * (*ppdel)->next and (*ppdel)->phead is NOT defined.
 */
pcb_t *lpcb_del_node(pcb_t * const Head, pcb_t * const T, pcb_t **ppdel)
{
	pcb_t *p;

	if (Head == NULL)
	{	/* Empty node */
		*ppdel = NULL;
		return NULL;
	}
	else if (Head->next == Head)
	{	/* Only 1 node */
		if (Head == T)
			*ppdel = Head;
		else
			*ppdel = NULL;
		return NULL;
	}
	/* 2 or more nodes */
	if (Head == T)
	{
		for (p = Head; p->next != Head; p = p->next)
			/* Find the tail */;
		p->next = Head->next;
		*ppdel = Head;
		return p->next;
	}
	for (p = Head; p->next != Head; p = p->next)
		if (p->next == T)
		{	/* Find the prior */
			*ppdel = T;
			p->next = T->next;
			return Head;
		}
	/* Not found */
	*ppdel = NULL;
	return Head;
}