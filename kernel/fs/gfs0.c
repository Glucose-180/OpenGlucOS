/*
 * gfs0.c:
 * Basic operations (such as initialization) for Glucose File System.
 */
#include <os/gfs.h>
#include <os/sched.h>
#include <common.h>
#include <os/string.h>
#include <os/mm.h>
#include <os/glucose.h>
#include <printk.h>
#include <os/time.h>

/*
 * The index of the first sector for GFS.
 * It is initialized in `init_swap()` and should be
 * 8-Sec aligned.
 * NOTE: it must be initialized first before GFS is used!
 */
unsigned int GFS_base_sec;

/*
 * Keep the basic information of GFS on the disk.
 * It might be the already existing super block,
 * or the superblock initialized just now.
 */
GFS_superblock_t *GFS_superblock = (void *)GFS_CACHE_BASE;

const uint8_t GFS_Magic[24U] = "Z53/4 Beijingxi-Kunming";

/*
 * GFS_check: check GFS on the disk.
 * Return: 0 if there is already a valid GFS,
 * 1, 2, 3, 4 if there is an invalid GFS,
 * or -1 if there is no GFS at all.
 */
int GFS_check()
{
	uint64_t superblock_buf[SECTOR_SIZE / sizeof(uint64_t)];
	GFS_superblock_t *psb = (GFS_superblock_t*)superblock_buf;

	GFS_read_sec(0U, 1U, superblock_buf);
	if (strcmp((void *)psb->GFS_magic, (void *)GFS_Magic) != 0)
		/* No GFS */
		return -1;
	*GFS_superblock = *psb;
	if ((psb->inode_loc - psb->block_bitmap_loc) * SECTOR_SIZE * 8U * BLOCK_SIZE
		!= psb->GFS_size)
		return 1;
	if ((psb->block_bitmap_loc - psb->inode_bitmap_loc) * SECTOR_SIZE * 8U
		!= psb->inode_num)
		return 2;
	if ((psb->data_loc - psb->inode_loc) * SECTOR_SIZE
		!= psb->inode_num * sizeof(GFS_inode_t))
		return 3;
	if (((GFS_base_sec + psb->data_loc) & (SEC_PER_BLOCK - 1U))
		!= 0U)
		/* Address of data blocks is not one block aligned */
		return 4;
	return 0;
}

/*
 * GFS_init: create a GFS on the disk NO MATTER there is
 * a GFS or not, and then mount it.
 * Return: 0 on success, 1 on error (if some files or directories
 * except `/` are opened).
 */
int GFS_init()
{
	sector_buf_t superblock_buf;
	GFS_dentry_t sector_buf[SECTOR_SIZE / sizeof(GFS_dentry_t)];
	GFS_superblock_t *psb = (GFS_superblock_t*)superblock_buf;
	unsigned int sidx, i;
	/* Root dir inode and block */
	unsigned int rdiidx = 0U, rdbidx;
	GFS_inode_t rdinode;	/* Root dir inode */

	if (flist_head->next != NULL)
		return 1;

	/* Write super block */
	strcpy((void *)psb->GFS_magic, (void *)GFS_Magic);
	psb->GFS_size = GFS_SIZE;
	psb->inode_num = NINODE;
	psb->inode_bitmap_loc = INODE_BITMAP_LOC;
	psb->block_bitmap_loc = BLOCK_BITMAP_LOC;
	psb->inode_loc = INODE_LOC;
	psb->data_loc = DATA_LOC;
	GFS_write_sec(0U, 1U, superblock_buf);
	*GFS_superblock = *psb;

	/* Clear inode bitmap and block bitmap */
	memset(sector_buf, 0U, sizeof(sector_buf));
	for (sidx = INODE_BITMAP_LOC; sidx < INODE_LOC; ++sidx)
		GFS_write_sec(sidx, 1U, sector_buf);
	
	GFS_mount();

	/* Allocate an inode for directory "/" */
	if (GFS_alloc_in_bitmap(1U, &rdiidx, GFS_superblock->inode_bitmap_loc,
		GFS_superblock->block_bitmap_loc) != 1U || rdiidx != 0U)
		panic_g("failed to allocate inode for \"/\": %u", rdiidx);
	rdinode.nlink = 1U;	/* Dir cannot be hard linked */
	rdinode.size = 2U;	/* "." and ".." */
	rdinode.type = IT_DIR;
	/* Allocate a block for it */
	if (GFS_alloc_in_bitmap(1U, &rdbidx, GFS_superblock->block_bitmap_loc,
		GFS_superblock->inode_loc) != 1U)
		panic_g("failed to allocate block for \"/\"");
	/* Write data for "." and ".." */
	strcpy((char*)(sector_buf[0].fname), ".");
	sector_buf[0].ino = rdiidx;
	strcpy((char*)(sector_buf[1].fname), "..");
	sector_buf[1].ino = rdiidx;
	for (i = 2U; i < SECTOR_SIZE / sizeof(GFS_dentry_t); ++i)
		sector_buf[i].ino = DENTRY_INVALID_INO;
	/* Writing the first sector of the block */
	GFS_write_sec(GFS_superblock->data_loc + rdbidx * SEC_PER_BLOCK, 1U, sector_buf);
	sector_buf[0].ino = sector_buf[1].ino = DENTRY_INVALID_INO;
	for (i = 1U; i < SEC_PER_BLOCK; ++i)
		GFS_write_sec(GFS_superblock->data_loc + rdbidx * SEC_PER_BLOCK + i,
			1U, sector_buf);
	/* Set and write inode of "/" */
	rdinode.dptr[0] = rdbidx + GFS_DATALOC_BLOCK;
	for (i = 1U; i < INODE_NDPTR; ++i)
		rdinode.dptr[i] = INODE_INVALID_PTR;
	rdinode.idptr = INODE_INVALID_PTR;
	rdinode.diptr = INODE_INVALID_PTR;
	GFS_write_inode(rdiidx, &rdinode);
#if DEBUG_EN != 0
	writelog("GFS is initialized starting on sector %u", GFS_base_sec);
#endif
	return 0;
}

/*
 * GFS_mount: init the file list and the cache.
 */
void GFS_mount(void)
{
	if (strcmp((char *)GFS_superblock->GFS_magic, (char *)GFS_Magic) != 0)
		panic_g("GFS is not checked or initialized");
	GFS_cache_init();
	flist_init();
}

/*
 * GFS_alloc_in_bitmap: Allocate `n` free entries in a bitmap (inode or block),
 * whose range (sector index relative to `GFS_base_Sec`) is
 * [`start_sec`, `end_sec`), and write their
 * indexes in `iarr[]` (ensure that there is enough space).
 * Return the number of entries allocated successfully.
 */
unsigned int GFS_alloc_in_bitmap(unsigned int n, unsigned int iarr[],
	unsigned int start_sec, unsigned int end_sec)
{
	/* Count successfully allocated entries */
	unsigned int a_ymr;
	unsigned int sidx;	/* Sector index */
	unsigned int i, j;
	//sector_buf_t sector_buf;
	/*
	 * After GFS cache is implemented, it is unnecessary to read
	 * the disk as bitmaps have been loaded into the cache.
	 */
	volatile uint64_t *sector_buf;
	uint64_t mask;
	char flag_modified;	/* A sector is modified */

	for (a_ymr = 0U, sidx = start_sec; sidx < end_sec && a_ymr < n; ++sidx)
	{	/* Scan a sector (`sidx`) in bitmap */
		//GFS_read_sec(sidx, 1U, sector_buf);
		sector_buf = (volatile uint64_t*)(GFS_CACHE_BASE + sidx * SECTOR_SIZE);
		flag_modified = 0;
		for (i = 0U; i < SECTOR_SIZE / sizeof(uint64_t) && a_ymr < n; ++i)
		{
			if (sector_buf[i] != ~0UL)
			{
				for (j = 0U; j < 64U && a_ymr < n; ++j)
				{
					mask = ((uint64_t)1UL << 63) >> j;
					if ((sector_buf[i] & mask) == 0UL)
					{
						sector_buf[i] |= mask;
						iarr[a_ymr++] =
							(sidx - start_sec) * SECTOR_SIZE * 8U + i * 64U + j;
					}
				}
				flag_modified = 1;
			}
		}
		if (flag_modified != 0)
			GFS_write_sec(sidx, 1U, sector_buf);
	}
	return a_ymr;
}

/*
 * GFS_free_in_bitmap: clear the `bidx`-th bit in the bitmap
 * whose range (sector index relative to `GFS_base_Sec`) is
 * [`start_sec`, `end_sec`).
 * Return 0 on success or 1 if the `idx` is out of range.
 */
int GFS_free_in_bitmap(unsigned int bidx,
	unsigned int start_sec, unsigned int end_sec)
{
	unsigned int sidx;
	//sector_buf_t sector_buf;
	/*
	 * After GFS cache is implemented, it is unnecessary to read
	 * the disk as bitmaps have been loaded into the cache.
	 */
	volatile uint64_t *sector_buf;
	uint64_t mask;

	if (end_sec <= start_sec ||
		bidx >= (end_sec - start_sec) * SECTOR_SIZE * 8U)
		return 1;
	sidx = start_sec + (bidx / (SECTOR_SIZE * 8U));
	bidx = bidx % (SECTOR_SIZE * 8U);
	//GFS_read_sec(sidx, 1U, sector_buf);
	sector_buf = (volatile uint64_t*)(GFS_CACHE_BASE + sidx * SECTOR_SIZE);

	mask = ((uint64_t)1UL << 63) >> (bidx % 64U);
	sector_buf[bidx / 64U] &= ~mask;

	GFS_write_sec(sidx, 1U, sector_buf);
	return 0;
}

/*
 * GFS_read_inode: read `ino`-th inode from GFS disk
 * and write it to `pinode`.
 * Return 0 on success or -1 if the `ino` is illegal.
 */
int GFS_read_inode(unsigned int ino, GFS_inode_t *pinode)
{
/*
	unsigned int sidx;
	sector_buf_t sector_buf;
	flist_node_t *pfn;

	if (ino >= GFS_superblock->inode_num)
		return -1;

	if ((pfn = flist_search(ino)) != NULL)
		*pinode = pfn->inode;
	else
	{
		sidx = GFS_superblock->inode_loc + lbytes2sectors(ino * sizeof(GFS_inode_t));
		GFS_read_sec(sidx, 1U, sector_buf);
		*pinode = ((GFS_inode_t*)sector_buf)
			[ino % (sizeof(sector_buf) / sizeof(GFS_inode_t))];
	}
*/
	if (ino >= GFS_superblock->inode_num)
		return -1;
	*pinode = gfsc_inodes[ino];
	return 0;
}

/*
 * GFS_write_inode: write `ino`-th inode to GFS disk.
 * Return 0 on success or -1 if the `ino` is illegal.
 */
int GFS_write_inode(unsigned int ino, volatile GFS_inode_t *pinode)
{
	unsigned int sidx;
/*
	sector_buf_t sector_buf;
	flist_node_t *pfn;

	if (ino >= GFS_superblock->inode_num)
		return -1;

	if ((pfn = flist_search(ino)) != NULL)
		pfn->inode = *pinode;

	sidx = GFS_superblock->inode_loc + lbytes2sectors(ino * sizeof(GFS_inode_t));
	GFS_read_sec(sidx, 1U, sector_buf);
	((GFS_inode_t*)sector_buf)[ino % (sizeof(sector_buf) / sizeof(GFS_inode_t))]
		= *pinode;
	GFS_write_sec(sidx, 1U, sector_buf);
*/	
	if (ino >= GFS_superblock->inode_num)
		return -1;
	gfsc_inodes[ino] = *pinode;
	sidx = GFS_superblock->inode_loc + lbytes2sectors(ino * sizeof(GFS_inode_t));
	GFS_write_sec(sidx, 1U,
		(void *)ROUNDDOWN((uintptr_t)(gfsc_inodes + ino), SECTOR_SIZE));

	return 0;
}

/*
 * GFS_count_in_bitmap: count occupied bits in a bitmap (inode or block),
 * whose range (sector index relative to `GFS_base_Sec`) is
 * [`start_sec`, `end_sec`).
 * Return the number of `1` bits.
 */
unsigned int GFS_count_in_bitmap(unsigned int start_sec, unsigned int end_sec)
{
	unsigned int sidx;
	unsigned int o_ymr = 0U;	/* Number of ones */
	unsigned int i;
	uint64_t l;
	//sector_buf_t sector_buf;
	/*
	 * After GFS cache is implemented, it is unnecessary to read
	 * the disk as bitmaps have been loaded into the cache.
	 */
	volatile uint64_t *sector_buf;

	for (sidx = start_sec; sidx < end_sec; ++sidx)
	{
		//GFS_read_sec(sidx, 1U, sector_buf);
		sector_buf = (volatile uint64_t*)(GFS_CACHE_BASE + sidx * SECTOR_SIZE);
		for (i = 0U; i < sizeof(sector_buf_t) / sizeof(uint64_t); ++i)
		{
			l = sector_buf[i];
			while (l > 0UL)
			{
				l &= (l - 1UL);	// Clear the lowest `1` bit
				++o_ymr;
			}
			//o_ymr += (unsigned int)__builtin_popcountl(sector_buf[i]);
		}
	}
	return o_ymr;
}

/*
 * do_mkfs: initialize GFS on the disk.
 * If `force` is not zero, initialze GFS even if there is already
 * a file system. Otherwise, initialization will fail if there is
 * already a file system, no matter valid or not.
 * Return: 0 on success, -1 if there is already a valid GFS,
 * -2 if some files or directories except `/` are opened,
 * or 1, 2, 3, 4 if there is already an invalid GFS.
 */
int do_mkfs(int force)
{
	int crt;

	if (force != 0 || (crt = GFS_check()) < 0)
	{
		printk("Initializing Glucose File System...\n");
		if (GFS_init() != 0)
			return -2;
		printk(
			"GFS base sector : %u\n"
			"GFS magic       : \"%s\"\n"
			"GFS size        : %u MiB\n"
			"Number of inodes: %u\n"
			"inode bitmap loc: %u Sec\n"
			"Block bitmap loc: %u Sec\n"
			"inode loc       : %u Sec\n"
			"Data blocks loc : %u Sec\n",
			GFS_base_sec, GFS_superblock->GFS_magic,
			GFS_superblock->GFS_size >> 20,
			GFS_superblock->inode_num,
			GFS_superblock->inode_bitmap_loc,
			GFS_superblock->block_bitmap_loc,
			GFS_superblock->inode_loc,
			GFS_superblock->data_loc
		);
		return 0;
	}
	else if (crt == 0)
		/* Already a valid GFS */
		return -1;
	else
		/* Already an invalid GFS */
		return crt;
}

/*
 * do_fsinfo: read information of GFS on the disk.
 * Return value is the same as `GFS_check()`.
 */
int do_fsinfo()
{
	int crt;
	/* Number of used inodes and blocks */
	unsigned int nui, nub;

	crt = GFS_check();
	if (crt == 0)
	{
		nui = GFS_count_in_bitmap(GFS_superblock->inode_bitmap_loc,
			GFS_superblock->block_bitmap_loc);
		nub = GFS_count_in_bitmap(GFS_superblock->block_bitmap_loc,
			GFS_superblock->inode_loc);
		printk(
			"GFS base sector : %u\n"
			"GFS magic       : \"%s\"\n"
			"GFS size        : %u MiB\n"
			//"Number of inodes: %u\n"
			"inode bitmap loc: %u Sec\n"
			"Block bitmap loc: %u Sec\n"
			"inode loc       : %u Sec\n"
			"Data blocks loc : %u Sec\n",
			GFS_base_sec, GFS_superblock->GFS_magic,
			GFS_superblock->GFS_size >> 20,
			//GFS_superblock->inode_num,
			GFS_superblock->inode_bitmap_loc,
			GFS_superblock->block_bitmap_loc,
			GFS_superblock->inode_loc,
			GFS_superblock->data_loc
		);
		printk(
			"Used inodes     : %u / %u (%u B)\n"
			"Used data blocks: %u / %u (%u B)\n",
			nui, GFS_superblock->inode_num, sizeof(GFS_inode_t),
			nub, GFS_superblock->GFS_size / BLOCK_SIZE, BLOCK_SIZE
		);
	}
	return crt;
}

/*
 * GFS_panic: print error information and suggest the user
 * reset the file system.
 * It is called when severe error is detected in GFS.
 */
void GFS_panic(const char *fmt, ...)
{
	va_list va;
	uint64_t time;
	extern int _vprint(const char *fmt, va_list _va, void (*output)(char*));
	extern int vprintk(const char *fmt, va_list _va);

	printk("\n**Severe error is detected in GFS:\n");

	time = get_timer();
	printl("[t=%04lus] Severe error is detected in GFS: ", time);

	va_start(va, fmt);
	vprintk(fmt, va);
//#if DEBUG_EN != 0
	_vprint(fmt, va, qemu_logging);
	printl("\n");
//#endif
	va_end(va);

	printk("\nTry \"mkfs -f\" to reset GFS or \"statfs\""
		"to show basic information...\n");
}