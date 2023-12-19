/*
 * gfs-io.c:
 * Disk IO functions to make checking and cache convenient.
 */
#include <os/gfs.h>
#include <os/mm.h>
#include <common.h>
#include <os/glucose.h>

static inline unsigned int umin(unsigned int x, unsigned int y)
{
	return (x < y ? x : y);
}

/*
 * GFS_read_sec: Read `nsec` sectors from GFS in disk to main memory `kva`.
 * `sec_idx_in_GFS` is the index of starting sector relative to `GFS_base_sec`.
 * Return: 0 (reserved).
 */
int GFS_read_sec(unsigned int sec_idx_in_GFS, unsigned int nsec, void* kva)
{
	unsigned int sidx;

	if (GFS_base_sec == 0U)
		panic_g("invalid GFS base");
	for (sidx = GFS_base_sec + sec_idx_in_GFS;
		sidx < GFS_base_sec + sec_idx_in_GFS + nsec; sidx += 64U, kva += 64U * SECTOR_SIZE)
	{
		sd_read((unsigned int)kva2pa((uintptr_t)kva),
			umin(GFS_base_sec + sec_idx_in_GFS + nsec - sidx, 64U), sidx);
	}
	return 0;
}

/*
 * GFS_write_sec: Write `nsec` sectors to GFS in disk from main memory `kva`.
 * `sec_idx_in_GFS` is the index of starting sector relative to `GFS_base_sec`.
 * Return: 0 (reserved).
 */
int GFS_write_sec(unsigned int sec_idx_in_GFS, unsigned int nsec, void* kva)
{
	unsigned int sidx;

	for (sidx = GFS_base_sec + sec_idx_in_GFS;
		sidx < GFS_base_sec + sec_idx_in_GFS + nsec; sidx += 64U, kva += 64U * SECTOR_SIZE)
	{
		sd_write((unsigned int)kva2pa((uintptr_t)kva),
			umin(GFS_base_sec + sec_idx_in_GFS + nsec - sidx, 64U), sidx);
	}
	return 0;
}

