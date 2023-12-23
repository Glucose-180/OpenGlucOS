/*
 * gfs-flist.c: operations on linked list that records
 * the opened files and directories.
 */

#include <os/gfs.h>
#include <os/glucose.h>
#include <os/string.h>
#include <os/malloc-g.h>
#include <printk.h>

/*
 * list node for "/".
 * `bewr`, `nproc` are meaningless for it.
 */
static flist_node_t flh;
static flist_node_t *flist_head = &flh;

/*
 * flist_init: initialize the linked list.
 * It is called when the GFS is init.ed.
 */
void flist_init(void)
{
	flist_node_t *p, *q;

	flist_head = &flh;
	flist_head->ino = 0U;

	p = flist_head->next;
	while (p != NULL)
	{
		q = p->next;
		kfree_g(p);
		p = q;
	}
	flist_head->next = NULL;
}

/*
 * flist_inc_fnode: if `ino` is found in the list,
 * the node's `ino` is increased; otherwise,
 * a new node is created and added in the list.
 * If `wr` is not 0, `bewr` will be set.
 * NOTE: if `ino` is 0 ("/"), nothing will be done.
 * Return 0 on success, or 1 on error.
 */
int flist_inc_fnode(uint32_t ino, int wr)
{
	flist_node_t *p;

	if (ino == 0U)
		return 0;
	if (ino >= GFS_superblock.inode_num)
		return 1;
	for (p = flist_head; p->next != NULL; p = p->next)
		if (p->next->ino == ino)
			break;
	if (p->next == NULL)
	{
		if ((p->next = kmalloc_g(sizeof(flist_node_t))) == NULL)
		{
			GFS_panic("flist_inc_fnode: no enough memory for list");
			return 1;
		}
		p->next->ino = ino;
		p->next->bewr |= wr;
		p->next->next = NULL;
		p->next->nproc = 1U;
		GFS_read_inode(ino, &(p->next->inode));
	}
	else
	{
		p->next->bewr |= wr;
		++(p->next->nproc);
	}
	return 0;
}

/*
 * flist_dec_fnode: search `ino` in the list.
 * `.nproc` is decreased by 1. If it becomes 0,
 * the node will be deleted.
 * If `cwr` is not zero, `.bewr` will be cleared.
 * NOTE: if `ino` is 0 ("/"), nothing will be done.
 * Return 0 on success, or 1 if `ino` not found.
 */
int flist_dec_fnode(uint32_t ino, int cwr)
{
	flist_node_t *p, *q;

	if (ino == 0U)
		return 0;
	if (ino >= GFS_superblock.inode_num)
		return 1;
	for (p = flist_head; p->next != NULL; p = p->next)
		if (p->next->ino == ino)
			break;
	if (p->next == NULL)
		return 1;
	p->next->bewr &= (cwr ? 0 : ~0);
	if (--(p->next->nproc) <= 0)
	{
		q = p->next->next;
		kfree_g(p->next);
		p->next = q;
	}
	return 0;
}

/*
 * flist_search: search `ino` in the list.
 * Return the pointer to it or `NULL` if not found.
 */
flist_node_t *flist_search(uint32_t ino)
{
	flist_node_t *p;

	for (p = flist_head; p != NULL; p = p->next)
		if (p->ino == ino)
			break;
	return p;
}
