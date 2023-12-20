/*
 * gfs-path.c:
 * Operations on path string.
 */
#include <os/gfs.h>
#include <os/glucose.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/smp.h>

/*
 * scan_dentry_on_ptr_arr: Scan data blocks holding dir entries
 * pointed by `parr[0]` to `parr[n-1]` for `fname`.
 * Return the `ino` of dir entry if found or `DENTRY_INVALID_INO` if not found.
 */
static unsigned int scan_dentry_on_ptr_arr
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
 * path_anal: analize the string `spath` and find the inode
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
	indblock_buf_t idbbuf;
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
		cur_ino = 0U;	/* starting from "/" */
	else
		if (ccpu->cpath[0] != '/' ||
			(cur_ino = path_anal(ccpu->cpath)) == DENTRY_INVALID_INO)
			panic_g("cpath is %s", ccpu->cpath);
	nfnames = split(path[0] == '/' ? path + 1 : path, '/', fnames, 16U);
	for (i = 0U; i < nfnames; ++i)
	{
		if (GFS_read_inode(cur_ino, &inode) != 0)
			panic_g("cannot find %u-th inode while anal %s at %u",
				cur_ino, spath, i);
		if (inode.type != DIR)
		{
			/* Not a directory */
			if (i + 1U != nfnames)
				return DENTRY_INVALID_INO;
		}
		/* Try direct pointers */
		if ((cur_ino = scan_dentry_on_ptr_arr(inode.dptr, INODE_NDPTR, fnames[i]))
			== DENTRY_INVALID_INO)
		{
			GFS_read_block(inode.idptr, idbbuf);
			cur_ino = scan_dentry_on_ptr_arr(idbbuf, BLOCK_SIZE / sizeof(uint32_t),
				fnames[i]);
		}
		if (cur_ino == DENTRY_INVALID_INO)
			return DENTRY_INVALID_INO;
	}
	return cur_ino;
}
