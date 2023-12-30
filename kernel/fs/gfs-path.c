/*
 * gfs-path.c:
 * Operations on path string.
 */
#include <os/glucose.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/smp.h>

/*
 * search_dentry_on_ptr_arr: Scan data blocks holding dir entries
 * pointed by `parr[0]` to `parr[n-1]` for `fname`.
 * Return the `ino` of dir entry if found or `DENTRY_INVALID_INO` if not found.
 */
static unsigned int search_dentry_on_ptr_arr
	(const uint32_t *parr, unsigned int n, const char *fname)
{
	unsigned int i, j;
	GFS_dentry_t debbuf[DENT_PER_BLOCK];

	for (i = 0U; i < n; ++i)
	{
		if (parr[i] == INODE_INVALID_PTR)
			continue;
		GFS_read_block(parr[i], debbuf);
		for (j = 0U; j < DENT_PER_BLOCK; ++j)
			if (debbuf[j].ino != DENTRY_INVALID_INO &&
				strcmp((char *)debbuf[j].fname, fname) == 0)
				return debbuf[j].ino;
	}
	return DENTRY_INVALID_INO;
}

/*
 * search_dentry_in_dir_inode: search an entry in a directory inode.
 * Return the `ino` of dir entry if found or `DENTRY_INVALID_INO` if not found.
 */
unsigned int search_dentry_in_dir_inode
	(const GFS_inode_t *pinode, const char *fname)
{
	indblock_buf_t idbbuf;
	unsigned int ino;

	if (pinode->type != IT_DIR)
		return DENTRY_INVALID_INO;
	if ((ino = search_dentry_on_ptr_arr(pinode->dptr, INODE_NDPTR, fname))
		== DENTRY_INVALID_INO && pinode->idptr != INODE_INVALID_PTR)
	{
		GFS_read_block(pinode->idptr, idbbuf);
		ino = search_dentry_on_ptr_arr(idbbuf, BLOCK_SIZE / sizeof(uint32_t),
			fname);
	}
	return ino;
}

/*
 * path_anal: analize the string `spath` and find the inode.
 * If it starts with `/`, it is absolute path;
 * otherwise it is considered as relative path.
 * Return the index of inode found (`ino` in dir entry)
 * or `DENTRY_INVALID_INO` if not found.
 * NOTE: the length of string is limited,
 * and pay attention to the kernel stack again!
 */
unsigned int path_anal(const char *spath)
{
	char path[PATH_LEN + 1];
	/* inode index that is just found */
	unsigned int cur_ino;
	char *fnames[16];
	unsigned int nfnames, i;
	GFS_inode_t inode;
	pcb_t *ccpu = cur_cpu();

	if (strlen(spath) > PATH_LEN)
		/* Path is too long */
		return DENTRY_INVALID_INO;
	strcpy(path, spath);
	if (path[0] == '/')
		cur_ino = 0U;	/* Start from "/" */
	else	/* Start from the current path */
		cur_ino = ccpu->cur_ino;
	nfnames = split(path[0] == '/' ? path + 1 : path, '/', fnames, 16U);
	for (i = 0U; i < nfnames; ++i)
	{
		if (GFS_read_inode(cur_ino, &inode) != 0)
		{
			GFS_panic("cannot find %u-th inode while anal %s at %u",
				cur_ino, spath, i);
			return DENTRY_INVALID_INO;
		}
		if (inode.type != IT_DIR)
			/* Not a directory */
			return DENTRY_INVALID_INO;
		cur_ino = search_dentry_in_dir_inode(&inode, fnames[i]);
		if (cur_ino == DENTRY_INVALID_INO)
			return DENTRY_INVALID_INO;
	}
	if (cur_ino >= GFS_superblock->inode_num)
	{
		GFS_panic("path_anal: %s got illegal ino %u", spath, cur_ino);
		return DENTRY_INVALID_INO;
	}
	return cur_ino;
}

/*
 * do_changedir: change the process's current path.
 * Return 0 on success, 1 if the directory is not found,
 * or 2 if the path is too long.
 */
int do_changedir(const char *tpath)
{
	unsigned int target_ino;
	GFS_inode_t inode;
	pcb_t *ccpu = cur_cpu();
	/* current path len and target path len */
	int cplen = strlen(ccpu->cpath);
	int tplen = strlen(tpath);

	if (tplen > PATH_LEN || (tpath[0] != '/' && cplen + tplen + 1 > PATH_LEN))
		return 2;
	target_ino = path_anal(tpath);
	if (target_ino == DENTRY_INVALID_INO)
		return 1;
	GFS_read_inode(target_ino, &inode);
	/* check of `target_ino` has been done in `path_anal()`. */
	//if (GFS_read_inode(target_ino, &inode) != 0)
	//	panic_g("path_anal(\"%s\") returned invalid %u", tpath, target_ino);
	if (inode.type != IT_DIR)
		return 1;
	if (tpath[0] == '/')
		/* Absolute path */
		strcpy(ccpu->cpath, tpath);
	else
	{	/* Relative path */
		if (ccpu->cpath[cplen - 1] != '/')
		{
			ccpu->cpath[cplen++] = '/';
			ccpu->cpath[cplen] = '\0';
		}
		strcat(ccpu->cpath, tpath);
	}
	if (target_ino != ccpu->cur_ino)
	{
		if (flist_dec_fnode(ccpu->cur_ino, 0) != 0)
			GFS_panic("do_changedir: failed to dec %u in file list", ccpu->cur_ino);
		flist_inc_fnode(target_ino, 0);
		ccpu->cur_ino = target_ino;
	}
	if (path_squeeze(ccpu->cpath) == 0U)
		panic_g("invalid abs path: %s", ccpu->cpath);
	return 0;
}

/*
 * do_getpath: get the process's current path and write it to `path`.
 * Return the index of the current directory or `DENTRY_INVALID_INO`
 * if the `path` address is illegal.
 */
unsigned int do_getpath(char *path)
{
	if ((uintptr_t)path >= KVA_MIN)
		return DENTRY_INVALID_INO;
	strcpy(path, cur_cpu()->cpath);
	return cur_cpu()->cur_ino;
}

/*
 * path_squeeze: squeeze a path string with "." and "..".
 * For example, "/home/glucoes/./../tiepi" will become "/home/tiepi".
 * NOTE: `path` should be an absolute path (starting with '/').
 * Return the actual length of the path after squeezed
 * or 0 if `path[0]` is not '/'.
 */
unsigned int path_squeeze(char *path)
{
	unsigned int n;
	char *fnames[20U];
	unsigned int i, l;
	int j;

	if (path[0] != '/')
		return 0U;
	++path;	/* Skip '/' */
	n = split(path, '/', fnames, 20U);
	/*
	 * Ignore `.`, `..` and the name before it by setting the
	 * pointers in `fnames[]` to them to `NULL`.
	 */
	for (j = -1, i = 0U; i < n; ++i)
	{	/* Scan every file name */
	/* `j` points to the last valid name before `i` */
		if (strcmp(fnames[i], "..") == 0)
		{
			fnames[i] = NULL;
			if (j >= 0)
				fnames[j--] = NULL;
		}
		else if (strcmp(fnames[i], ".") == 0)
			fnames[i] = NULL;
		else
			j = (int)i;
	}
	/*
	 * Connect the remaining names and calculate the
	 * total length.
	 */
	for (l = 0U, i = 0U; i < n; ++i)
	{
		if (fnames[i] == NULL)
			continue;
		while (*(fnames[i]) != '\0')
			path[l++] = *(fnames[i]++);
		path[l++] = '/';
	}
	if (l > 0U)
		--l;
	path[l] = '\0';
	return l + 1U;
}

/*
 * path_anal_2: copy (to `tpath`) and analyse the path in `spath`
 * to get its parent dir path, parent dir inode index (ino) and its target name.
 * Return the parent inode index and write the target name pointer in `*ptname`.
 * In other word, `tpath` will keep the parent dir path and
 * `*ptname` will point to the target name.
 * If the parent dir is not found, `DENTRY_INVALID_INO` will be returned.
 * If `spath` only has a slash '/' at the beginning or doesn't has '/',
 * `tpath` will be meaningless!
 * For example, if `spath` is "/home/tiepi", then `tpath` will be
 * "/home", `*ptname` will be "tiepi" and the return value is inode index
 * of "/home".
 */
unsigned int path_anal_2(const char *spath, char *tpath, char **ptname)
{
	/* index of last slash ('/') */
	int ils;
	/* parent path inode index */
	unsigned int ppino;

	strncpy(tpath, spath, PATH_LEN);
	tpath[PATH_LEN] = '\0';
	for (ils = strlen(tpath); ils >= 0; --ils)
		if (tpath[ils] == '/')
		{
			tpath[ils] = '\0';
			break;
		}
	*ptname = tpath + (ils + 1);
	if (ils == 0)
		ppino = 0U;	/* "/" */
	else if (ils < 0)
		/* `stpath` is just a name */
		ppino = cur_cpu()->cur_ino;
	else
		ppino = path_anal(tpath);
	return ppino;
}
