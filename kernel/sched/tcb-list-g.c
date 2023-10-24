/*
 * tcb-list-g.c: by Glucose180
 * Operations for single (NOT circular) linked list of TCB.
 */
#include <os/sched.h>
#include <os/malloc-g.h>

/*
 * ltcb_add_node_to_tail: Add a new node to the tail of list pointed by Head,
 * and returns head pointer of new list.
 * Then, *ppnew will points to the new node.
 * NULL will be returned on error.
 */
tcb_t *ltcb_add_node_to_tail(tcb_t * const Head, tcb_t * volatile *ppnew)
{
	tcb_t *p;

	*ppnew = kmalloc_g(sizeof(tcb_t));
	if (*ppnew == NULL)
		return NULL;
	if (Head == NULL)
	{	/* Empty list */
		(*ppnew)->next = NULL;
		return *ppnew;
	}
	for (p = Head; p->next != NULL; p = p->next)
		;
	p->next = *ppnew;
	(*ppnew)->next = NULL;
	return Head;
}

/*
 * ltcb_search_node: Search a node according to member "tid".
 * Return pointer to it, or NULL if not found.
 */
tcb_t *ltcb_search_node(tcb_t * const Head, tid_t const Tid)
{
	tcb_t *p;

	if (Head == NULL || Head->tid == Tid)
		return Head;
	for (p = Head->next; p != NULL; p = p->next)
		if (p->tid == Tid)
			return p;
	return NULL;
}

/*
 * ltcb_del_node: Delete a node pointed by T,
 * and returns head pointer to new list.
 * Then, *ppdel will pointes to the deleted node.
 * If the node is not found, *ppdel will be NULL.
 * (*ppdel)->next is NOT defined.
 */
tcb_t *ltcb_del_node(tcb_t * const Head, tcb_t * const T, tcb_t **ppdel)
{
	tcb_t *p;

	if (Head == NULL)
	{	/* Empty node */
		*ppdel = NULL;
		return NULL;
	}
	else if (Head->next == NULL)
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
		*ppdel = T;
		return Head->next;
	}
	for (p = Head; p->next != NULL; p = p->next)
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