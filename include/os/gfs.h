/*
 * gfs.h:
 * Definitions and declarations about Glucose File System.
 */
#ifndef __GFS_H__
#define __GFS_H__

//#include <os/sched.h>
//#include <common.h>
#include <type.h>

/*
 * NOTE: Don't modify these definitions without thinking twice,
 * because they are depended by many other definitions!
 */

/* Units */
#define KiB 1024U
#define MiB (1024U * KiB)
#define GiB (1024U * MiB)

/* Size of a disk block: 4 KiB */
#define BLOCK_SIZE 4096U
/* Size of GFS: 1 GiB */
#define GFS_SIZE (1U * GiB)
/* Start sector of inode bitmap in GFS */
#define INODE_BITMAP_LOC 1U
/* Start sector of block bitmap in GFS */
#define BLOCK_BITMAP_LOC 8U
/* Start sector of inodes */
#define INODE_LOC 72U
/* Start sector of data blocks */
#define DATA_LOC 3656U
/* Number of inodes: 7S * 512B/S * 8/B */
#define NINODE 28672U

extern unsigned int GFS_base_sec;
const uint8_t GFS_Magic[24U];

/* Header in GFS super block */
typedef struct {
	uint8_t GFS_magic[24U];
	uint32_t GFS_size;
	uint32_t inode_num;
	uint32_t inode_bitmap_loc;
	uint32_t block_bitmap_loc;
	uint32_t inode_loc;
	uint32_t data_loc;
} GFS_superblock_t;

typedef struct {
	/*
	 * Basic info of a file or directory.
	 * NOTE: `mode`, `owner`, `timestamp`
	 * are reserved for future.
	 */
	uint32_t mode, owner;
	uint32_t size, timestamp;
	/*
	 * Direct pointer to file: the index of data block
	 * RELATIVE TO `GFS_base_sec`(/8). 4 KiB max for one.
	 */
	uint32_t dptr[10U];
	/* Indirect pointer 4 MiB max */
	uint32_t idptr;
	/* Double indirect pointer: 4 GiB max */
	uint32_t diptr;
} GFS_inode_t;

#endif
