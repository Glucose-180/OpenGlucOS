/*
 * gfs-cache.c:
 * The cache of GFS.
 */
#include <os/sched.h>
#include <common.h>
#include <os/malloc-g.h>
#include <os/smp.h>
#include <os/string.h>
	//#include <os/glucose.h>
	//#include <printk.h>

static const uintptr_t Gfsc_base = GFS_CACHE_BASE;
/* The first sector is for super block */
static const uintptr_t Gfsc_inode_bitmap_base = GFS_CACHE_BASE + SECTOR_SIZE;
static uintptr_t gfsc_block_bitmap_base;
static volatile GFS_cache_block_t *gfsc_blocks;
volatile GFS_inode_t *gfsc_inodes;
/* The number of blocks in cache, typically 3 k ~ 4 k */
static unsigned int gfsc_blocks_num;

/*
 * The access sequence for LRU.
 * `gfsc_lru_aseq[i]` being `j` means that the access seq
 * of block `j` is `i`. In other word, `gfsc_lru_aseq[0]`
 * is the MRU (most recently used) and the last index is
 * the LRU block.
 */
static uint16_t *gfsc_lru_aseq;

/*
 * GFS_cache_init: calc the addresses and load necessary data
 * (bitmaps and inodes).
 * It must be called after the super block is inited.
 */
void GFS_cache_init(void)
{
	unsigned int i;

	if (FREEMEM_KERNEL + NPF * NORMAL_PAGE_SIZE > GFS_CACHE_BASE)
		panic_g("Page frames exceed the boundary");
	/* Init the addresses */
	gfsc_block_bitmap_base = Gfsc_base +
		GFS_superblock->block_bitmap_loc * SECTOR_SIZE;
	gfsc_inodes = (void *)(Gfsc_base +
		GFS_superblock->inode_loc * SECTOR_SIZE);
	gfsc_blocks = (void *)(Gfsc_base +
		GFS_superblock->data_loc * SECTOR_SIZE);
	if ((uint64_t)gfsc_blocks >= GFS_CACHE_END)
		panic_g("Cache blocks exceed the boundary");
	gfsc_blocks_num = (GFS_CACHE_END - (uint64_t)gfsc_blocks)
		/ sizeof(GFS_cache_block_t);
#if DEBUG_EN != 0
	writelog("GFS cache: block bitmap at 0x%lx, inode at 0x%lx,"
		" %u blocks at 0x%lx", gfsc_block_bitmap_base,
		gfsc_inodes, gfsc_blocks_num, gfsc_blocks);
#endif
	/* Load bitmaps and inodes from disk */
	GFS_read_sec(GFS_superblock->inode_bitmap_loc,
		GFS_superblock->data_loc - GFS_superblock->inode_bitmap_loc,
		(void *)Gfsc_inode_bitmap_base);
	if (gfsc_lru_aseq == NULL)
		if ((gfsc_lru_aseq = kmalloc_g(gfsc_blocks_num * sizeof(uint16_t))) == NULL)
			panic_g("Failed to alloc %u bytes", gfsc_blocks_num * sizeof(uint16_t));
	/* Init all blocks and seq array */
	for (i = 0U; i < gfsc_blocks_num; ++i)
	{
		gfsc_blocks[i].tag = 0U;
		gfsc_lru_aseq[i] = (uint16_t)i;
	}
}