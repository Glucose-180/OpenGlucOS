/*
 * gfs0.c:
 * Basic operations (such as initialization) for Glucose File System.
 */
#include <os/gfs.h>
#include <os/sched.h>
#include <common.h>
#include <os/glucose.h>
#include <os/string.h>

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
GFS_superblock_t GFS_superblock;

const uint8_t GFS_Magic[24U] = "Z53/4 Beijingxi-Kunming";

/*
 * GFS_check: check GFS on the disk.
 * Return: 0 if there is already a valid GFS,
 * 1, 2, 3 if there is an invalid GFS,
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
	GFS_superblock = *psb;
	if ((psb->inode_loc - psb->block_bitmap_loc) * SECTOR_SIZE * 8U * BLOCK_SIZE
		!= psb->GFS_size)
		return 1;
	if ((psb->block_bitmap_loc - psb->inode_bitmap_loc) * SECTOR_SIZE * 8U
		!= psb->inode_num)
		return 2;
	if ((psb->data_loc - psb->inode_loc) * SECTOR_SIZE
		!= psb->inode_num * sizeof(GFS_inode_t))
		return 3;
	if (((GFS_base_sec + psb->data_loc) & (BLOCK_SIZE / SECTOR_SIZE - 1U))
		!= 0U)
		/* Address of data blocks is not one block alogned */
		return 4;
	return 0;
}

/*
 * GFS_init: create a GFS on the disk NO MATTER there is
 * a GFS or not.
 * Return: 0 (reserved).
 */
int GFS_init()
{
	sector_buf_t superblock_buf, sector_buf;
	GFS_superblock_t *psb = (GFS_superblock_t*)superblock_buf;
	unsigned int sidx;

	/* Write super block */
	strcpy((void *)psb->GFS_magic, (void *)GFS_Magic);
	psb->GFS_size = GFS_SIZE;
	psb->inode_num = NINODE;
	psb->inode_bitmap_loc = INODE_BITMAP_LOC;
	psb->block_bitmap_loc = BLOCK_BITMAP_LOC;
	psb->inode_loc = INODE_LOC;
	psb->data_loc = DATA_LOC;
	GFS_write_sec(0U, 1U, superblock_buf);
	GFS_superblock = *psb;

	/* Clear inode bitmap and block bitmap */
	memset(sector_buf, 0U, sizeof(sector_buf));
	for (sidx = INODE_BITMAP_LOC; sidx < INODE_LOC; ++sidx)
		GFS_write_sec(sidx, 1U, sector_buf);
	
	// TODO: allocate 0 inode for directory "/"

	return 0;
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
	sector_buf_t sector_buf;
	uint64_t mask;
	char flag_modified;	/* A sector is modified */

	for (a_ymr = 0U, sidx = start_sec; sidx < end_sec && a_ymr < n; ++sidx)
	{	/* Scan a sector (`sidx`) in bitmap */
		GFS_read_sec(sidx, 1U, sector_buf);
		flag_modified = 0;
		for (i = 0U; i < SECTOR_SIZE / sizeof(uint64_t); ++i)
		{
			if (sector_buf[i] != ~0UL)
			{
				for (j = 0U; j < 64U; ++j)
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
 * GFS_read_inode: read `ino`-th inode from GFS disk
 * and write it to `pinode`.
 * Return 0 on success or -1 if the `ino` is illegal.
 */
int GFS_read_inode(unsigned int ino, GFS_inode_t *pinode)
{
	unsigned int sidx;
	sector_buf_t sector_buf;

	if (ino >= GFS_superblock.inode_num)
		return -1;
	sidx = GFS_superblock.inode_loc + lbytes2sectors(ino * sizeof(GFS_inode_t));
	GFS_read_sec(sidx, 1U, sector_buf);
	*pinode = ((GFS_inode_t*)sector_buf)
		[sidx % (sizeof(sector_buf) / sizeof(GFS_inode_t))];
	return 0;
}
