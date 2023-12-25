/*
 * gfs-file.c:
 * Operations on files
 */
#include <os/sched.h>
#include <common.h>
#include <os/glucose.h>
#include <os/string.h>
#include <printk.h>
#include <os/smp.h>
#include <os/malloc-g.h>

/*
 * fd_init: Init the file descriptors of process `p`.
 * If `s` is `NULL`, `p->fds[]` will be cleared.
 * Otherwise, `p->fds[]` will be copied from `s->fds[]`,
 * and all open files will be increased in file list.
 */
void fd_init(pcb_t *p, pcb_t *s)
{
	unsigned int i;

	if (s == NULL)
		for (i = 0U; i < OFILE_MAX; ++i)
			p->fds[i].fnode = NULL;
	else
		for (i = 0U; i < OFILE_MAX; ++i)
			if (s->fds[i].fnode != NULL)
			{
				p->fds[i] = s->fds[i];
				p->fds[i].fnode->nproc++;
			}
			else
				p->fds[i].fnode = NULL;
}

/*
 * do_open: Open the file at `fpath` with flags `oflags`.
 * Return the file descriptor index on success or
 * negative number on error:
 * -1: the file doesn't exist (if `fpath` points to a dir, also);
 * -2: the parent directory doesn't exist;
 * -3: no free file descriptor;
 * -4: the parent dir is full;
 * -5: no free block on the disk;
 * -8: other error in GFS.
 */
long do_open(const char *fpath, int oflags)
{
	char tpath[PATH_LEN + 1];
	char *tname;
	unsigned int ppino, tino, i;
	long fd;
	GFS_inode_t pinode, tinode;
	int adrt;
	flist_node_t *pfnode;
	pcb_t *ccpu = cur_cpu();

	if ((ppino = path_anal_2(fpath, tpath, &tname)) == DENTRY_INVALID_INO)
		return -2L;
	if (tname[0] == '\0')
		return -1L;
	/* Search unused file descriptor */
	for (fd = 0L; fd < (long)OFILE_MAX; ++fd)
		if (ccpu->fds[fd].fnode == NULL)
			break;
	if (fd >= (long)OFILE_MAX)
		return -3L;
	
	if (GFS_read_inode(ppino, &pinode) != 0)
	{
		GFS_panic("do_open: invalid ino %u", ppino);
		return -8;
	}
	if ((tino = search_dentry_in_dir_inode(&pinode, tname))
		== DENTRY_INVALID_INO)
	{
		if ((oflags & O_CREATE) != 0)
		{	/* Create the target file */
			if (GFS_alloc_in_bitmap(1U, &tino, GFS_superblock.inode_bitmap_loc,
				GFS_superblock.block_bitmap_loc) != 1U)
				/* Allocate an inode for the new file */
				return -4;
			/* Init the inode */
			tinode.nlink = 1U;
			tinode.size = 0U;
			tinode.type = IT_FILE;
			for (i = 0U; i < INODE_NDPTR; ++i)
				tinode.dptr[i] = INODE_INVALID_PTR;
			tinode.idptr = tinode.diptr = INODE_INVALID_PTR;
			switch ((adrt = GFS_add_dentry(&pinode, tname, tino)))
			{
			case 0:	/* Success */
				break;
			case 1: case 2:
				/* Parent dir is full or no free block */
				GFS_free_in_bitmap(tino, GFS_superblock.inode_bitmap_loc,
					GFS_superblock.block_bitmap_loc);
				return -3 - adrt;
				break;
			default:
				GFS_panic("do_open: GFS_add_dentry(., %s, %u) returns %d",
					tname, tino, adrt);
				return -8;
				break;
			}
			GFS_write_inode(tino, &tinode);
			GFS_write_inode(ppino, &pinode);
		}
		else
			return -1;
	}
	else
	{
		GFS_read_inode(tino, &tinode);
		if (tinode.type != IT_FILE)
			/* Don't open a dir */
			return -1L;
	}
	if ((pfnode = flist_inc_fnode(tino, oflags & O_WRONLY)) == NULL)
		return -8;
	ccpu->fds[fd].oflags = oflags;
	ccpu->fds[fd].pos = 0U;
	ccpu->fds[fd].fnode = pfnode;
	return fd;
}

/*
 * do_close: Close the file and free the file descriptor.
 * Return the file descriptor on success or -1
 * if the descriptor at `fd` is invalid.
 */
long do_close(long fd)
{
	pcb_t *ccpu = cur_cpu();

	if (fd < 0L || fd >= (long)OFILE_MAX)
		return -1L;
	if (ccpu->fds[fd].fnode == NULL)
		return -1L;
	if (flist_dec_fnode(ccpu->fds[fd].fnode->ino,
		ccpu->fds[fd].oflags & O_WRONLY) != 0)
	{
		GFS_panic("do_close: proc %d has invalid .fds[%d].fnode->ino %u",
			ccpu->pid, (int)fd, ccpu->fds[fd].fnode->ino);
		return -1L;
	}
	ccpu->fds[fd].fnode = NULL;
	return fd;
}
