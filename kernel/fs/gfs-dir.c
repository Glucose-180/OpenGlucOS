/*
 * gfs-dir.c:
 * Operations on directories
 */
#include <os/gfs.h>
#include <os/sched.h>
#include <common.h>
#include <os/glucose.h>
#include <os/string.h>
#include <printk.h>

/*
 * GFS_add_dentry: Add an entry in the directory whose inode is pointed
 * by `pinode`. The entry is specified by `fname` and `ino`.
 * Return: 0 on success, 1 if the directory is full, 2 if no free block,
 * -1 if `pinode` points to a file, -2 if `fname` already exists,
 * and -3 on error (severe error of file system).
 * NOTE: this function do not write inode to disk!
 */
int GFS_add_dentry(GFS_inode_t *pinode, const char *fname, unsigned int ino)
{
	unsigned int i, j;
	/*
	 * Buffers of block holding dir entries and indirect block.
	 * NOTE: Their total size is 8 KiB. Pay attention to the kernel stack!
	 */
	GFS_dentry_t debbuf[DENT_PER_BLOCK];
	indblock_buf_t idbbuf;
	unsigned int idbidx, bidx;

	if (pinode->type != DIR)
		return -1;
	if (pinode->size >= NDENTRIES)
		return 1;
	/*
	 * NOTE: Searching and then scanning makes many repeated
	 * reading operations of disk. This is not efficient.
	 * After cache is implemented, it might be alleviated.
	 */
	if (search_dentry_in_dir_inode(pinode, fname) != DENTRY_INVALID_INO)
		/* already exists */
		return -2;
	/* Scan all director pointers */
	for (i = 0U; i < INODE_NDPTR; ++i)
	{
		if (pinode->dptr[i] == INODE_INVALID_PTR)
			break;
		GFS_read_block(pinode->dptr[i], debbuf);
		for (j = 0U; j < DENT_PER_BLOCK; ++j)
			if (debbuf[j].ino == DENTRY_INVALID_INO)
				break;
	}
	if (i < INODE_NDPTR)
	{	/* There is free space in direct pointers */
		if (pinode->dptr[i] == INODE_INVALID_PTR)
		{	/* We need to allocate a new data block */
			if (GFS_alloc_in_bitmap(1U, &bidx, GFS_superblock.block_bitmap_loc,
				GFS_superblock.inode_loc) != 1U)
				/* No free data block */
				return 2;
			strncpy((char*)debbuf[0].fname, fname, FNLEN);
			debbuf[0].fname[FNLEN] = 0U;
			debbuf[0].ino = ino;
			for (j = 1U; j < DENT_PER_BLOCK; ++j)
				debbuf[j].ino = DENTRY_INVALID_INO;
			pinode->dptr[i] = bidx;
		}
		else
		{	/* There is a space in an existing data block */
			bidx = pinode->dptr[i];
			strncpy((char*)debbuf[j].fname, fname, FNLEN);
			debbuf[j].fname[FNLEN] = 0U;
			debbuf[j].ino = ino;
		}
		GFS_write_block(bidx, debbuf);
		pinode->size++;
		return 0;
	}
	else
	{	/* Try to search the indirect pointer */
		if (pinode->idptr == INODE_INVALID_PTR)
		{	/* Allocate a new block for indirect pointers */
			if (GFS_alloc_in_bitmap(1U, &idbidx, GFS_superblock.block_bitmap_loc,
				GFS_superblock.inode_loc) != 1U)
				return 2;
			for (i = 0U; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
				idbbuf[i] = INODE_INVALID_PTR;
			GFS_write_block(idbidx, idbbuf);
			pinode->idptr = idbidx;
		}
		else
			GFS_read_block((idbidx = pinode->idptr), idbbuf);
		/*
		 * Now `idbidx` is equal to `pinode->idptr`, and
		 * `idbbuf` is the content of the block pointed by it.
		 */
		/* Now repeat what we have done on `pinode->dptr[]`... */
		for (i = 0U; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
		{
			if (idbbuf[i] == INODE_INVALID_PTR)
				break;
			GFS_read_block(idbbuf[i], debbuf);
			for (j = 0U; j < DENT_PER_BLOCK; ++j)
				if (debbuf[j].ino == DENTRY_INVALID_INO)
					break;
		}
		if (i < BLOCK_SIZE / sizeof(uint32_t))
		{	/* There is free space */
			if (idbbuf[i] == INODE_INVALID_PTR)
			{	/* We need to allocate a new data block */
				if (GFS_alloc_in_bitmap(1U, &bidx, GFS_superblock.block_bitmap_loc,
					GFS_superblock.inode_loc) != 1U)
					/* No free data block */
					return 2;
				strncpy((char*)debbuf[0].fname, fname, FNLEN);
				debbuf[0].fname[FNLEN] = 0U;
				debbuf[0].ino = ino;
				for (j = 1U; j < DENT_PER_BLOCK; ++j)
					debbuf[j].ino = DENTRY_INVALID_INO;
				idbbuf[i] = bidx;
			}
			else
			{	/* There is a space in an existing data block */
				bidx = idbbuf[i];
				strncpy((char*)debbuf[j].fname, fname, FNLEN);
				debbuf[j].fname[FNLEN] = 0U;
				debbuf[j].ino = ino;
			}
			GFS_write_block(bidx, debbuf);
			GFS_write_block(idbidx, idbbuf);
			pinode->size++;
			return 0;
		}
		else
			/* `pinode->size` is not large, but there's no free space! */
			return -3;
	}
}

