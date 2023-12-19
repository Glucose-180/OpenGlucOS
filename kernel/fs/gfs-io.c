/*
 * gfs-io.c:
 * Disk IO functions to make checking and cache convenient.
 */
#include <os/gfs.h>
#include <os/mm.h>
#include <common.h>
#include <os/glucose.h>


int disk_read_sec(unsigned int sec_idx_in_GFS, unsigned int nsec, uintptr_t kva)
{
	if (GFS_base_sec == 0U)
		panic_g("invalid GFS base");
	// TODO: support reading more than 64 sectors
	return sd_read((unsigned int)kva2pa(kva), nsec, GFS_base_sec + sec_idx_in_GFS);
}