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
static GFS_cache_block_t *gfsc_blocks;
GFS_inode_t *gfsc_inodes;
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

/* Performance counters for GFS cache */
uint64_t gfsc_wrhit, gfsc_rdhit, gfsc_wrmiss, gfsc_rdmiss;

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
	gfsc_wrhit = gfsc_wrmiss = 0UL;
	gfsc_rdhit = gfsc_rdmiss = 0UL;
}

/*
 * block_copy64: copy a block using 64-bit unit.
 */
static void block_copy64(uint64_t *dest, const uint64_t *src)
{
	unsigned int i;

	for (i = 0U; i < BLOCK_SIZE / sizeof(uint64_t); ++i)
		dest[i] = src[i];
}

/*
 * glru_scan_or_evict: scan all cache blocks to
 * find an invalid (`.tag==0`) block and set its `.tag`
 * to `tag`.
 * If found, return its index; otherwise, return the
 * index of the LRU block (eviction).
 */
static unsigned int glru_scan_or_evict(uint32_t tag)
{
	unsigned int i;

	for (i = 0U; i < gfsc_blocks_num; ++i)
		if (gfsc_blocks[i].tag == 0U)
			break;
	if (i >= gfsc_blocks_num)
		/* LRU */
		i = gfsc_lru_aseq[gfsc_blocks_num - 1U];
	
	gfsc_blocks[i].tag = tag;
	return i;
}

/*
 * gfsc_scan_and_inv: scan all cache blocks and invalidate
 * the block whose `.tag` is `tag`.
 * Reserved for future use.
 * Return 0 if found, or 1 if not found.
 */
int gfsc_scan_and_inv(uint32_t tag)
{
	unsigned int i;

	for (i = 0U; i < gfsc_blocks_num; ++i)
		if (gfsc_blocks[i].tag == tag)
		{
			gfsc_blocks[i].tag = 0U;
			return 0;
		}
	
	return 1;
}

/*
 * glru_aseq_move: move the entry in `gfsc_lru_aseq[]` whose
 * value is `cbidx` to the head (MRU).
 */
static void glru_aseq_move(uint16_t cbidx)
{
	unsigned int i;

	for (i = gfsc_blocks_num - 1U; i > 0U; --i)
		if (gfsc_lru_aseq[i] == cbidx)
			break;
	if (gfsc_lru_aseq[i] != cbidx)
		GFS_panic("glru_aseq_move: cannot find %u in LRU array",
			(unsigned int)cbidx);
	
	for (; i > 0U; --i)
		gfsc_lru_aseq[i] = gfsc_lru_aseq[i - 1U];
	gfsc_lru_aseq[0] = cbidx;
}

/*
 * gfsc_search: scan all cache blocks and return the
 * index of the hit block (`.tag==tag`).
 * If not found, return `UINT32_MAX`.
 */
static uint32_t gfsc_search(uint32_t tag)
{
	uint32_t i;

	for (i = 0U; i < gfsc_blocks_num; ++i)
		if (gfsc_blocks[i].tag == tag)
			return i;
	return UINT32_MAX;
}

/*
 * gfsc_read_block: read a block whose index relative to GFS is
 * `bidx` to memory address `kva`. If hit, read the block in cache;
 * otherwise load the block to cache (eviction may happen).
 * Return 0 on hit, 1 on miss or negative number on error.
 */
int gfsc_read_block(uint32_t bidx, void *kva)
{
	unsigned int he;
	int flag_miss = 0;

	he = gfsc_search(bidx);
	if (he >= gfsc_blocks_num)
	{	/* Miss */
		he = glru_scan_or_evict(bidx);
		GFS_read_sec(bidx * SEC_PER_BLOCK, SEC_PER_BLOCK, gfsc_blocks[he].block);
		flag_miss = 1;
		++gfsc_rdmiss;
	}
	else
		++gfsc_rdhit;

	glru_aseq_move((uint16_t)he);

	if (((uintptr_t)kva & 7UL) == 0UL)
		block_copy64(kva, gfsc_blocks[he].block);
	else
		memcpy(kva, (const void *)gfsc_blocks[he].block, BLOCK_SIZE);

	return flag_miss;
}

/*
 * gfsc_write_block: write a block whose index relative to GFS is
 * `bidx` from memory address `kva`. If hit, write the cache block
 * and disk; otherwise, write the cache and disk too but eviction
 * may also happen.
 * Return 0 on hit, 1 on miss or negative number on error.
 */
int gfsc_write_block(uint32_t bidx, const void *kva)
{
	unsigned int he;
	int flag_miss = 0;

	he = gfsc_search(bidx);
	if (he >= gfsc_blocks_num)
	{
		he = glru_scan_or_evict(bidx);
		flag_miss = 1;
		++gfsc_wrmiss;
	}
	else
		++gfsc_wrhit;

	if (((uintptr_t)kva & 7UL) == 0UL)
		block_copy64(gfsc_blocks[he].block, kva);
	else
		memcpy((void *)gfsc_blocks[he].block, kva, BLOCK_SIZE);

	glru_aseq_move((uint16_t)he);
	GFS_write_sec(bidx * SEC_PER_BLOCK, SEC_PER_BLOCK, gfsc_blocks[he].block);
	return flag_miss;
}
