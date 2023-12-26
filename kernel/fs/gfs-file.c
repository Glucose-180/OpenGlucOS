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

static uint8_t data_block_buf[BLOCK_SIZE];

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
		if ((oflags & O_CREAT) != 0)
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
 * read_file_on_ptr_arr: read at most `len` bytes from the blocks
 * pointed by `parr[0]` to `parr[n-1]` to `buf` starting from `start_pos`.
 * Return the number of bytes read successfully.
 * NOTE: `start_pos` is the offset relative to the block pointed by `parr[0]`!
 */
static int64_t read_file_on_ptr_arr(const uint32_t *parr,
	unsigned int n, uint32_t start_pos, uint32_t len, uint8_t *buf)
{
	unsigned int start_pidx, end_pidx, pidx;
	int64_t r_ymr = 0L;
	/* Offset in a block */
	unsigned int boffset;

	if (len == 0U || n == 0U)
		return 0UL;
	start_pidx = start_pos / BLOCK_SIZE;
	end_pidx = (start_pos + len - 1U) / BLOCK_SIZE;
	if (end_pidx > n - 1U)
		end_pidx = n - 1U;
	/* Read from blocks pointed by `parr[start_pidx]` to `parr[end_pidx]` */
	for (pidx = start_pidx, boffset = start_pos % BLOCK_SIZE;
		pidx <= end_pidx; ++pidx)
	{
		if (parr[pidx] == INODE_INVALID_PTR)
			break;
		GFS_read_block(parr[pidx], data_block_buf);
		for (; boffset < BLOCK_SIZE && r_ymr < (int64_t)len; ++boffset, ++r_ymr)
			buf[r_ymr] = data_block_buf[boffset];
		boffset = 0U;
	}
	return r_ymr;
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

/*
 * do_read: read the file specified by `fd` at most
 * `len` bytes and write them in `buf`.
 * Return the number of bytes read successfully,
 * or negative number on error:
 * -1: invalid `fd`;
 * -2: invalid `buf`;
 * ...
 * -8, -9: severe error on GFS.
 */
long do_read(long fd, uint8_t *buf, uint32_t len)
{
	pcb_t *ccpu = cur_cpu();
	unsigned int i;
	int64_t rpos, r_ymr = 0L;
	const GFS_inode_t *pinode;
	indblock_buf_t idbbuf, dibbuf;

	if ((unsigned long)fd >= OFILE_MAX || ccpu->fds[fd].fnode == NULL
		|| (ccpu->fds[fd].oflags & O_RDONLY) == 0)
		return -1L;
	if ((uintptr_t)buf >= KVA_MIN)
		return -2L;
	pinode = &(ccpu->fds[fd].fnode->inode);
	if (len == 0U || ccpu->fds[fd].pos >= pinode->size)
		return 0L;
	if (pinode->size - ccpu->fds[fd].pos < len)
		/* If the length of the remaining file is less than `len`... */
		len = pinode->size - ccpu->fds[fd].pos;
	rpos = (int64_t)(ccpu->fds[fd].pos);
	/* Try reading data pointed by direct pointers */
	r_ymr += read_file_on_ptr_arr(pinode->dptr,
		INODE_NDPTR, (uint32_t)rpos, len, buf);
	if (r_ymr >= (int64_t)len)
	{
		ccpu->fds[fd].pos += len;
		return r_ymr;
	}
	/* Not enough? Try reading data pointed by indiret pointer. */
	rpos = ((int64_t)ccpu->fds[fd].pos + r_ymr - INODE_NDPTR * BLOCK_SIZE);
	if (rpos < 0 || pinode->idptr == INODE_INVALID_PTR)
	{
		GFS_panic("file %d of proc %d has size %u but only %u bytes "
			"are successfully read from %u, with rpos %ld", (int)fd, ccpu->pid,
			pinode->size, (unsigned int)r_ymr, ccpu->fds[fd].pos, rpos);
		return -8L;
	}
	GFS_read_block(pinode->idptr, idbbuf);
	r_ymr += read_file_on_ptr_arr(idbbuf, BLOCK_SIZE / sizeof(uint32_t),
		rpos, len - (uint32_t)r_ymr, buf + r_ymr);
	if (r_ymr >= (int64_t)len)
	{
		ccpu->fds[fd].pos += len;
		return r_ymr;
	}
	/* Try reading data pointed by indiret pointer. */
	if (pinode->diptr == INODE_INVALID_PTR)
	{
		GFS_panic("file %d of proc %d has size %u but only %u bytes "
			"are successfully read from %u, with rpos %ld", (int)fd, ccpu->pid,
			pinode->size, (unsigned int)r_ymr, ccpu->fds[fd].pos, rpos);
		return -9L;
	}
	GFS_read_block(pinode->diptr, dibbuf);
	for (i = 0U; i < BLOCK_SIZE / sizeof(uint32_t) &&
		dibbuf[i] != INODE_INVALID_PTR; ++i)
	{
		rpos = (int64_t)ccpu->fds[fd].pos + r_ymr -
			((int64_t)INODE_NDPTR * (int64_t)BLOCK_SIZE +
			(int64_t)(i + 1U) * (int64_t)BLOCK_SIZE *
			(int64_t)(BLOCK_SIZE / sizeof(uint32_t)));
		if (rpos < 0L)
		{
			GFS_panic("file %d of proc %d has size %u but only %u bytes "
				"are successfully read from %u, with rpos %ld", (int)fd, ccpu->pid,
				pinode->size, (unsigned int)r_ymr, ccpu->fds[fd].pos, rpos);
			return -10L;
		}
		GFS_read_block(dibbuf[i], idbbuf);
		r_ymr += read_file_on_ptr_arr(idbbuf, BLOCK_SIZE / sizeof(uint32_t),
			(uint32_t)rpos, len - (uint32_t)r_ymr, buf + r_ymr);
	}
	if (r_ymr == (int64_t)len)
	{
		ccpu->fds[fd].pos += len;
		return r_ymr;
	}
	else
	{
		GFS_panic("file %d of proc %d has size %u but only %u bytes "
			"are successfully read from %u, with rpos %ld", (int)fd, ccpu->pid,
			pinode->size, (unsigned int)r_ymr, ccpu->fds[fd].pos, rpos);
		return -11L;
	}
}
