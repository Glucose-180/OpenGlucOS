/*
 * gfs-dir.c:
 * Operations on directories
 */
#include <os/sched.h>
#include <common.h>
#include <os/glucose.h>
#include <os/string.h>
#include <printk.h>
#include <os/smp.h>
#include <os/malloc-g.h>

/*
 * GFS_add_dentry: Add an entry in the directory whose inode is pointed
 * by `pinode`. The entry is specified by `fname` and `ino`.
 * Return: 0 on success, 1 if the directory is full, 2 if no free block,
 * -1 if `pinode` points to a file, //////-2 if `fname` already exists,
 * and -3 on error (severe error of file system).
 * NOTE: This function do not write inode to disk! It doesn't check
 * whether `fname` is valid (not empty, and doesn't exist) neither.
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

	if (pinode->type != IT_DIR)
		return -1;
	if (pinode->size >= NDENTRIES)
		return 1;
	/*
	 * NOTE: Searching and then scanning makes many repeated
	 * reading operations of disk. This is not efficient.
	 * After cache is implemented, it might be alleviated.
	 * 
	 * It is ignored as this check is done in `do_mkdir()`.
	 */
	//if (search_dentry_in_dir_inode(pinode, fname) != DENTRY_INVALID_INO)
		/* already exists */
	//	return -2;
	/* Scan all director pointers */
	for (i = 0U; i < INODE_NDPTR; ++i)
	{
		if (pinode->dptr[i] == INODE_INVALID_PTR)
			break;
		GFS_read_block(pinode->dptr[i], debbuf);
		for (j = 0U; j < DENT_PER_BLOCK; ++j)
			if (debbuf[j].ino == DENTRY_INVALID_INO)
				break;
		if (j < DENT_PER_BLOCK)
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
			bidx += GFS_DATALOC_BLOCK;
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
			idbidx += GFS_DATALOC_BLOCK;
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
			if (j < DENT_PER_BLOCK)
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
				bidx += GFS_DATALOC_BLOCK;
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

/*
 * do_mkdir: make a directory specified by `stpath`.
 * `stpath` can be absolute path or relative path, according to
 * whether `stpath[0]` is '/' or not.
 * Return 0 on success, 1 if the parent dir is not found,
 * 2 if the dir name is invalid (empty),
 * 3 if the parent dir is full, 4 if the dir already exists,
 * 5 if inode is not allocated successfully,
 * -1 on severe error.
 */
int do_mkdir(const char *stpath)
{
	GFS_inode_t inode, tinode;
	char tpath[PATH_LEN + 1];
	/* target name */
	char *tname;
	/* parent path inode index */
	unsigned int ppino;
	/* target dir inode index */
	unsigned int tino;
	int adrt;
	unsigned int i;

	ppino = path_anal_2(stpath, tpath, &tname);
	if (ppino == DENTRY_INVALID_INO)
		return 1;
	if (tname[0] == '\0')
		return 2;

	if (GFS_read_inode(ppino, &inode) != 0)
	{
		GFS_panic("do_mkdir: target path %s caused invalid parent ino %u.",
			stpath, ppino);
		return -1;
	}
	if (inode.type != IT_DIR)
		return 1;
	if (inode.size >= NDENTRIES)
		return 3;
	if (search_dentry_in_dir_inode(&inode, tname) != DENTRY_INVALID_INO)
		return 4;
	if (GFS_alloc_in_bitmap(1U, &tino, GFS_superblock.inode_bitmap_loc,
		GFS_superblock.block_bitmap_loc) != 1U)
		return 5;
	switch (adrt = GFS_add_dentry(&inode, tname, tino))
	{
	case 0:
		/* success */
		break;
	case 2:
		/* no free block */
		if (GFS_free_in_bitmap(tino, GFS_superblock.inode_bitmap_loc,
			GFS_superblock.block_bitmap_loc) != 0)
			GFS_panic("do_mkdir: failed to free %u inode", tino);
		return 3;
		break;
	default:
		/* severe error */
		GFS_panic("do_mkdir: GFS_add_dentry(., %s, %u) returns %d",
			tname, tino, adrt);
		return -1;
		break;
	}
	/* Init `tinode` */
	tinode.nlink = 1U;
	tinode.type = IT_DIR;
	tinode.size = 0U;
	for (i = 0U; i < INODE_NDPTR; ++i)
		tinode.dptr[i] = INODE_INVALID_PTR;
	tinode.idptr = tinode.diptr = INODE_INVALID_PTR;
	switch (adrt = GFS_add_dentry(&tinode, ".", tino))
	{
	case 0:
		break;
	case 2:
		GFS_free_in_bitmap(tino, GFS_superblock.inode_bitmap_loc,
			GFS_superblock.block_bitmap_loc);
		GFS_panic("do_mkdir: No free block in GFS!");
		/*
		 * As the parent inode has not been wriiten to disk,
		 * just free the target dir inode in bitmap.
		 * NOTE: this situation may not be dealt properly,
		 * as I do not have enough time and energy...
		 */
		return -1;
		break;
	default:
		GFS_panic("do_mkdir: GFS_add_dentry(., %s, %u) returns %d",
			".", tino, adrt);
		return -1;
		break;
	}
	switch (adrt = GFS_add_dentry(&tinode, "..", ppino))
	{
	case 0:
		break;
	case 2:
		GFS_free_in_bitmap(tino, GFS_superblock.inode_bitmap_loc,
			GFS_superblock.block_bitmap_loc);
		GFS_panic("do_mkdir: No free block in GFS!");
		/*
		 * As the parent inode has not been wriiten to disk,
		 * just free the target dir inode in bitmap.
		 * NOTE: this situation may not be dealt properly,
		 * as I do not have enough time and energy...
		 */
		return -1;
		break;
	default:
		GFS_panic("do_mkdir: GFS_add_dentry(., %s, %u) returns %d",
			"..", tino, adrt);
		return -1;
		break;
	}
	GFS_write_inode(ppino, &inode);
	if (GFS_write_inode(tino, &tinode) != 0)
	{
		GFS_panic("do_mkdir: \"%s\" has invalid ino %u", tname, tino);
		return -1;
	}
	return 0;
}

/*
 * scan_dentries_on_ptr_arr: Scan `parr[0]` to `parr[n-1]`
 * and print all dir entries. If `det`, details will be printed.
 * If `det` is greater than 2, the size will be displayed in
 * unit that is easy to read.
 * Return the number of entries printed.
 */
static unsigned int scan_dentries_on_ptr_arr
	(const uint32_t *parr, unsigned int n, int det)
{
	unsigned int i, j, e_ymr;
	GFS_dentry_t debbuf[DENT_PER_BLOCK];
	GFS_inode_t inode;

	for (i = 0U, e_ymr = 0U; i < n; ++i)
	{
		if (parr[i] == INODE_INVALID_PTR)
			continue;
		GFS_read_block(parr[i], debbuf);
		for (j = 0U; j < DENT_PER_BLOCK; ++j)
			if (debbuf[j].ino != DENTRY_INVALID_INO)
			{
				++e_ymr;
				if (det != 0)
				{
					// TODO: print details
					GFS_read_inode(debbuf[j].ino, &inode);
					printk("%u   %s   ", debbuf[j].ino, debbuf[j].fname);
					if (inode.type == IT_DIR)
						printk("%u entries\n", inode.size);
					else
					{
						if (inode.size >= 10U * MiB && det > 2)
							printk("%u MiB\n", inode.size / MiB);
						else if (inode.size >= 10U * KiB && det > 2)
							printk("%u KiB\n", inode.size / KiB);
						else
							printk("%u B\n", inode.size);
					}
				}
				else
				{
					printk("%s   ", debbuf[j].fname);
					if (e_ymr % 4U == 0U)
						printk("\n");
				}
			}
	}
	return e_ymr;
}

/*
 * do_readdir: Read the directory specified by `stpath` and print.
 * This is for `ls` command supporting `-l` option.
 * If `stpath` is `NULL`, it will read the current dir.
 * If `det` is not zero, details of entries will be printed.
 * Return the number of entries or 0 if not found.
 */
unsigned int do_readdir(const char *stpath, int det)
{
	unsigned int tino, e_ymr;
	GFS_inode_t tinode;
	indblock_buf_t idbbuf;

	if (stpath == NULL)
		tino = cur_cpu()->cur_ino;
	else if ((tino = path_anal(stpath)) == DENTRY_INVALID_INO)
		return 0U;
	if (GFS_read_inode(tino, &tinode) != 0)
	{
		GFS_panic("do_readdir: \"%s\" has invalid ino %u",
			stpath != NULL ? stpath : cur_cpu()->cpath, tino);
		return 0U;
	}
	if (tinode.type != IT_DIR)
		return 0U;
	e_ymr = scan_dentries_on_ptr_arr(tinode.dptr, INODE_NDPTR, det);
	if (tinode.idptr != INODE_INVALID_PTR)
	{
		GFS_read_block(tinode.idptr, idbbuf);
		e_ymr += scan_dentries_on_ptr_arr(idbbuf, BLOCK_SIZE / sizeof(uint32_t), det);
	}
	printk("\n");
	return e_ymr;
}

/*
 * rm_dentries_on_ptr_arr: Scan `parr[0]` to `parr[n-1]` and remove
 * dir entries in the data blocks pointed by them.
 * NOTE: if an entry is opened, it will not be removed.
 * Return the number of entries removed.
 */
static unsigned int rm_dentries_on_ptr_arr(uint32_t *parr, unsigned int n)
{
	/* NOTE: consider using `kmalloc_g()` to save the stack! */
	//GFS_dentry_t debbuf[DENT_PER_BLOCK];
	GFS_dentry_t *debbuf;
	unsigned int i, j;
	int rmit, flag_left;
	unsigned int rm_ymr = 0U;

	if ((debbuf = kmalloc_g(BLOCK_SIZE)) == NULL)
	{
		GFS_panic("rm_dentries_on_ptr_arr: no free memory");
		return 0U;
	}

	for (i = 0U; i < n; ++i)
	{
		if (parr[i] == INODE_INVALID_PTR)
			continue;
		GFS_read_block(parr[i], debbuf);
		flag_left = 0;
		for (j = 0U; j < DENT_PER_BLOCK; ++j)
		{
			if (debbuf[j].ino != DENTRY_INVALID_INO)
			{
				if (strcmp((char *)debbuf[j].fname, ".") == 0 ||
					strcmp((char *)debbuf[j].fname, "..") == 0)
					/* Ignore "." and ".." */
					continue;
				rmit = GFS_remove_file_or_dir(debbuf[j].ino);
				if (rmit == 0 || rmit == 1)
				{
					++rm_ymr;
					debbuf[j].ino = DENTRY_INVALID_INO;
				}
				else if (rmit == 2)
					flag_left = 1;
				else if (rmit < 0)
				{
					flag_left = 1;
					GFS_panic("rm_dentries_on_ptr_arr: failed %d to remove "
						"dir entry %s, %u", rmit, debbuf[j].fname, debbuf[j].ino);
				}
			}
		}
		if (flag_left != 0)
			GFS_write_block(parr[i], debbuf);
		else
		{
			if (GFS_free_in_bitmap(parr[i] - GFS_DATALOC_BLOCK,
				GFS_superblock.block_bitmap_loc, GFS_superblock.inode_loc) != 0)
				GFS_panic("rm_dentries_on_ptr_arr: invalid parr[%u]: %u",
					i, parr[i]);
			parr[i] = INODE_INVALID_PTR;
		}
	}
	kfree_g(debbuf);
	return rm_ymr;
}

/*
 * GFS_remove_file_or_dir: Remove the file or dir corresponding with
 * `ino`-th inode. A dir will be removed recursively. But it doesn't
 * care the entry in the parent directory.
 * Return: 0 if the file or dir is removed, 1 if the file is linked
 * so that only the `nlink` is decreased, 2 if the file or dir is occupied,
 * -1 if `ino` is invalid, -2 if no free memory,
 * -3, -4, -5, ... if invalid pointer is found,
 */
int GFS_remove_file_or_dir(unsigned int ino)
{
	GFS_inode_t inode;
	//indblock_buf_t idbbuf;
	/* Indirect block buffer and double indirect */
	uint32_t *idbbuf = NULL, *dibbuf = NULL;
	unsigned int rdprt1, rdprt2, i, j;
	int rt = -100;

	if (flist_search(ino) != NULL)
		return 2;
	if (GFS_read_inode(ino, &inode) != 0)
		return -1;
	if (inode.type == IT_DIR)
	{	/* Recursively */
		rdprt1 = rm_dentries_on_ptr_arr(inode.dptr, INODE_NDPTR);
		if (inode.idptr != INODE_INVALID_PTR)
		{
			if ((idbbuf = kmalloc_g(BLOCK_SIZE)) == NULL)
			{
				//GFS_panic("GFS_remove_file_or_dir: no free memory");
				return -2;
			}
			GFS_read_block(inode.idptr, idbbuf);
			rdprt2 = rm_dentries_on_ptr_arr(idbbuf,
				BLOCK_SIZE / sizeof(uint32_t));
			for (i = 0U; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
				if (idbbuf[i] != INODE_INVALID_PTR)
					break;
			if (i < BLOCK_SIZE / sizeof(uint32_t))
				/* Not all dir entries in the indirect block are removed */
				GFS_write_block(inode.idptr, idbbuf);
			else
			{
				if (GFS_free_in_bitmap(inode.idptr - GFS_DATALOC_BLOCK,
					GFS_superblock.block_bitmap_loc, GFS_superblock.inode_loc) != 0)
				{
					//GFS_panic("GFS_remove_file_or_dir: invalid .idptr: %u",
					//	inode.idptr);
					return -3;
				}
				inode.idptr = INODE_INVALID_PTR;
			}
		}
		else
			rdprt2 = 0U;
		if ((inode.size -= (rdprt1 + rdprt2)) > 2U)
		{	/* Not all dir entries are removed */
			GFS_write_inode(ino, &inode);
			rt = 2;
			goto rm_f_d_end;
		}
		else
		{
			GFS_free_in_bitmap(ino, GFS_superblock.inode_bitmap_loc,
				GFS_superblock.block_bitmap_loc);
			rt = 0;
			goto rm_f_d_end;
		}
	}
	else
	{	/* Remove file */
		if (inode.nlink >= 2U)
		{
			--inode.nlink;
			GFS_write_inode(ino, &inode);
			return 1;
		}
		/* Free all blocks in director pointers */
		for (i = 0U; i < INODE_NDPTR; ++i)
		{
			if (inode.dptr[i] != INODE_INVALID_PTR)
				if (GFS_free_in_bitmap(inode.dptr[i] - GFS_DATALOC_BLOCK,
					GFS_superblock.block_bitmap_loc, GFS_superblock.inode_loc) != 0)
				{
					//GFS_panic("GFS_remove_file_or_dir: invalid dptr[%u]: %u",
					//	i, inode.dptr[i]);
					return -4;
				}
		}

		if (inode.idptr != INODE_INVALID_PTR)
		{	/* Free data blocks in indirect pointers */
			if ((idbbuf = kmalloc_g(BLOCK_SIZE)) == NULL)
			{
				//GFS_panic("GFS_remove_file_or_dir: no free memory");
				return -2;
			}
			GFS_read_block(inode.idptr, idbbuf);
			for (i = 0U; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
			{
				if (idbbuf[i] != INODE_INVALID_PTR)
					if (GFS_free_in_bitmap(idbbuf[i] - GFS_DATALOC_BLOCK,
						GFS_superblock.block_bitmap_loc, GFS_superblock.inode_loc) != 0)
					{
						rt = -5;
						goto rm_f_d_end;
					}
			}
			if (GFS_free_in_bitmap(inode.idptr - GFS_DATALOC_BLOCK,
				GFS_superblock.block_bitmap_loc, GFS_superblock.inode_loc) != 0)
				{
					rt = -6;
					goto rm_f_d_end;
				}
		}

		if (inode.diptr != INODE_INVALID_PTR)
		{	/* Double indirect block */
			if ((dibbuf = kmalloc_g(BLOCK_SIZE)) == NULL)
			{
				rt = -2;
				goto rm_f_d_end;
			}
			GFS_read_block(inode.diptr, dibbuf);
			for (j = 0U; j < BLOCK_SIZE / sizeof(uint32_t); ++j)
			{
				if (dibbuf[j] == INODE_INVALID_PTR)
					continue;
				if (idbbuf == NULL)
					idbbuf = kmalloc_g(BLOCK_SIZE);
				if (idbbuf == NULL)
				{
					rt = -2;
					goto rm_f_d_end;
				}
				GFS_read_block(dibbuf[j], idbbuf);
				for (i = 0U; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
				{
					if (idbbuf[i] != INODE_INVALID_PTR)
						if (GFS_free_in_bitmap(idbbuf[i] - GFS_DATALOC_BLOCK,
							GFS_superblock.block_bitmap_loc, GFS_superblock.inode_loc) != 0)
						{
							rt = -7;
							goto rm_f_d_end;
						}
				}
				if (GFS_free_in_bitmap(dibbuf[j] - GFS_DATALOC_BLOCK,
					GFS_superblock.block_bitmap_loc, GFS_superblock.inode_loc) != 0)
				{
					rt = -8;
					goto rm_f_d_end;
				}
			}
			if (GFS_free_in_bitmap(inode.diptr - GFS_DATALOC_BLOCK,
				GFS_superblock.block_bitmap_loc, GFS_superblock.inode_loc) != 0)
			{
				rt = -9;
				goto rm_f_d_end;
			}
		}
		GFS_free_in_bitmap(ino, GFS_superblock.inode_bitmap_loc,
			GFS_superblock.block_bitmap_loc);
		rt = 0;
		goto rm_f_d_end;
	}
rm_f_d_end:
	if (dibbuf != NULL)
		kfree_g(dibbuf);
	if (idbbuf != NULL)
		kfree_g(idbbuf);
	return rt;
}

/*
 * remove_dentry_on_ptr_arr: Scan `parr[0]` to `parr[n-1]` and
 * remove the entry that has `ino`.
 * Return the number of entries removed.
 * NOTE: normally there cannot be two or more entries that have
 * the same `ino`. So if the return value is greater than 1,
 * error might have happened!
 */
static unsigned int remove_dentry_on_ptr_arr
	(const uint32_t *parr, unsigned int n, unsigned int ino)
{
	unsigned int i, j, r_ymr;
	GFS_dentry_t debbuf[BLOCK_SIZE / sizeof(GFS_dentry_t)];
	int flag_removed;

	for (i = 0U, r_ymr = 0U; i < n; ++i)
	{
		if (parr[i] == INODE_INVALID_PTR)
			continue;
		GFS_read_block(parr[i], debbuf);
		flag_removed = 0;
		for (j = 0U; j < DENT_PER_BLOCK; ++j)
		{
			if (debbuf[j].ino == ino)
			{
				debbuf[j].ino = DENTRY_INVALID_INO;
				++r_ymr;
				flag_removed = 1;
			}
		}
		if (flag_removed != 0)
			GFS_write_block(parr[i], debbuf);
	}
	return r_ymr;
}

/*
 * remove_dentry_in_dir_inode: Remove the dir entry in the inode
 * that has `ino` and decrease its size.
 * Return the number of entries removed.
 * NOTE: normally there cannot be two or more entries that have
 * the same `ino`. So if the return value is greater than 1,
 * error might have happened!
 */
unsigned int remove_dentry_in_dir_inode
	(GFS_inode_t *pinode, unsigned int ino)
{
	indblock_buf_t idbbuf;
	unsigned int r_ymr = 0U;

	if (pinode->type != IT_DIR)
		return 0U;
	r_ymr += remove_dentry_on_ptr_arr(pinode->dptr, INODE_NDPTR, ino);
	if (pinode->idptr != INODE_INVALID_PTR)
	{
		GFS_read_block(pinode->idptr, idbbuf);
		r_ymr += remove_dentry_on_ptr_arr(idbbuf,
			BLOCK_SIZE / sizeof(uint32_t), ino);
	}
	if (pinode->size < r_ymr + 2U)
		GFS_panic("remove_dentry_in_dir_inode: "
			"invalid size and r_ymr %u, %u", pinode->size, r_ymr);
	else
		pinode->size -= r_ymr;
	return r_ymr;
}

/*
 * do_remove: remove the file or directory (recursively)
 * specified by `stpath`.
 * Return 0 on success, 1 if the target file is not found,
 * 2 if the file is occupied, or -1 on error.
 */
int do_remove(const char *stpath)
{
	GFS_inode_t pinode;
	int rmdrt;
	unsigned int rmert;
	char tpath[PATH_LEN + 1];
	/* target name */
	char *tname;
	/* parent path inode index */
	unsigned int ppino;
	/* target file ino */
	unsigned int tino;

	ppino = path_anal_2(stpath, tpath, &tname);
	if (ppino == DENTRY_INVALID_INO)
		/* Parent dir is not found */
		return 1;
	if (tname[0] == '\0' || strcmp(tname, ".") == 0
		|| strcmp(tname, "..") == 0)
		return 1;	/* invalid name */
	/*
	 * Now `ppino` is the parent dir ino,
	 * and `tname` is the target file name.
	 */
	if (GFS_read_inode(ppino, &pinode) != 0)
	{
		GFS_panic("do_remove: invalid ppino %u", ppino);
		return -1;
	}
	if ((tino = search_dentry_in_dir_inode(&pinode, tname))
		== DENTRY_INVALID_INO)
		/* Not found */
		return 1;
	switch (rmdrt = GFS_remove_file_or_dir(tino))
	{
	case 0: case 1: /* Removed */
		break;
	case 2:	/* Occupied */
		return 2;
		break;
	default:	/* Error */
		GFS_panic("do_remove: failed to remove %s (%u): %d",
			stpath, tino, rmdrt);
		return -1;
		break;
	}
	/* Removed */
	if ((rmert = remove_dentry_in_dir_inode(&pinode, tino)) != 1U)
	{
		GFS_panic("do_remove: %u entries (%u) are removed in %s",
			rmert, tino, tpath);
		return -1;
	}
	GFS_write_inode(ppino, &pinode);
	return 0;
}
