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
#include <os/smp.h>

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
			pinode->dptr[i] = bidx + GFS_superblock.data_loc / SEC_PER_BLOCK;
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
			pinode->idptr = idbidx + GFS_superblock.data_loc / SEC_PER_BLOCK;
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
				idbbuf[i] = bidx + GFS_superblock.data_loc / SEC_PER_BLOCK;
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
	/* target name and parent path */
	char *tname, *ppath;
	/* index of last slash ('/') */
	int ils;
	/* parent path inode index */
	unsigned int ppino;
	/* target dir inode index */
	unsigned int tino;
	int adrt;
	unsigned int i;

	strncpy(tpath, stpath, PATH_LEN);
	tpath[PATH_LEN] = '\0';
	for (ils = strlen(tpath); ils >= 0; --ils)
		if (tpath[ils] == '/')
		{
			tpath[ils] = '\0';
			break;
		}
	tname = tpath + (ils + 1);
	if (tname[0] == '\0')
		return 2;	/* invalid name */
	ppath = tpath;
	if (ils == 0)
		ppino = 0U;	/* "/" */
	else if (ils < 0)
		/* `stpath` is just a name */
		ppino = cur_cpu()->cur_ino;
	else if ((ppino = path_anal(ppath)) == DENTRY_INVALID_INO)
		return 1;

	if (GFS_read_inode(ppino, &inode) != 0)
	{
		GFS_panic("do_mkdir: target path %s caused invalid parent ino %u.",
			stpath, ppino);
		return -1;
	}
	if (inode.type != DIR)
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
	tinode.type = DIR;
	tinode.size = 0U;
	for (i = 0U; i < INODE_NDPTR; ++i)
		tinode.dptr[i] = INODE_INVALID_PTR;
	tinode.idptr = tinode.diptr = INODE_INVALID_PTR;
	switch (adrt = GFS_add_dentry(&tinode, ".", tino))
	{
	case 0:
		break;
	case 2:
		GFS_panic("do_mkdir: No free block in GFS!");
		// TODO: remove the new dir
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
		GFS_panic("do_mkdir: No free block in GFS!");
		// TODO: remove the new dir
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
 * Return the number of entries printed.
 */
static unsigned int scan_dentries_on_ptr_arr
	(const uint32_t *parr, unsigned int n, int det)
{
	unsigned int i, j, e_ymr;
	GFS_dentry_t debbuf[DENT_PER_BLOCK];

	for (i = 0U, e_ymr = 0U; i < n; ++i)
	{
		if (parr[i] == INODE_INVALID_PTR)
			continue;
		GFS_read_block(parr[i], debbuf);
		for (j = 0U; j < DENT_PER_BLOCK; ++j)
			if (debbuf[j].ino != DENTRY_INVALID_INO)
			{
				printk("%s   ", debbuf[j].fname);
				++e_ymr;
				if (det != 0)
				{
					// TODO: print details
				}
				else
				{
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
		tino = 0U;
	else if ((tino = path_anal(stpath)) == DENTRY_INVALID_INO)
		return 0U;
	if (GFS_read_inode(tino, &tinode) != 0)
	{
		GFS_panic("do_readdir: \"%s\" has invalid ino %u",
			stpath != NULL ? stpath : cur_cpu()->cpath, tino);
		return 0U;
	}
	e_ymr = scan_dentries_on_ptr_arr(tinode.dptr, INODE_NDPTR, det);
	if (tinode.idptr != INODE_INVALID_PTR)
	{
		GFS_read_block(tinode.idptr, idbbuf);
		e_ymr += scan_dentries_on_ptr_arr(idbbuf, BLOCK_SIZE / sizeof(uint32_t), det);
	}
	printk("\n");
	return e_ymr;
}
