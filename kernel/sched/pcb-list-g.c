/*
 * pcb-list-g.c: by Glucose180
 * Operations for CIRCULAR linked list of PCB.
 */

#include <os/list.h>
#include <os/lock.h>
#include <os/pcb-list-g.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <os/malloc-g.h>

/*
 * add_node_to_tail: Add a new node to the tail of list pointed by Head,
 * and returns head pointer of new list.
 * Then, *ppnew will points to the new node.
 * NULL will be returned on error.
 */
/*
 * TODO: Check NUM_MAX_TASK.
 */
clist_node_t *add_node_to_tail(clist_node_t * const Head, clist_node_t * volatile *ppnew)
{
	clist_node_t *p;

	*ppnew = kmalloc_g(sizeof(clist_node_t));
	if (*ppnew == NULL)
		return NULL;
	if (Head == NULL)
	{	/* Empty list */
		(*ppnew)->next = *ppnew;
		return *ppnew;
	}
	for (p = Head; p->next != Head; p = p->next)
		;
	p->next = *ppnew;
	(*ppnew)->next = Head;
	return Head;
}

/*
 * insert_node: insert a node after the node pointed by Pprior.
 * If Pprior is NULL, *Pnew will be inserted to tail.
 * Otherwise, if Pprior is not found in this list, NULL will be returned.
 */
clist_node_t *insert_node(clist_node_t * const Head, clist_node_t * const Pnew, clist_node_t * const Pprior)
{
	clist_node_t *p;

	if (Head == NULL)
	{	/* Empty list */
		if (Pprior == NULL)
			return Pnew->next = Pnew;
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
	return Head;
}

/*
 * search_node_pid: Search a node according to member "pid".
 * Return pointer to it, or NULL if not found.
 */
clist_node_t *search_node_pid(clist_node_t * const Head, pid_t const Pid)
{
	clist_node_t *p;

	if (Head == NULL || Head->pid == Pid)
		return Head;
	for (p = Head->next; p != Head; p = p->next)
		if (p->pid == Pid)
			return p;
	return NULL;
}

/*
 * del_node: Delete a node pointed by T,
 * and returns head pointer to new list.
 * Then, *ppdel will pointes to the deleted node.
 * If the node is not found, *ppdel will be NULL.
 * (*ppdel)->next is NOT defined.
 */
clist_node_t *del_node(clist_node_t * const Head, clist_node_t * const T, clist_node_t **ppdel)
{
	clist_node_t *p;

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