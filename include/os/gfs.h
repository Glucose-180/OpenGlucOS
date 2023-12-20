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
#ifndef MiB
#define MiB (1024U * KiB)
#endif
#ifndef GiB
#define GiB (1024U * MiB)
#endif

#ifndef SECTOR_SIZE
#define SECTOR_SIZE 512U
#endif
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
/* Max length of file name */
#define FNLEN 59U
/* Number of direct pointers in an inode */
#define INODE_NDPTR 10U
/* Invalid block pointer in inode */
#define INODE_INVALID_PTR 0U
/* Invalid index of inode */
#define DENTRY_INVALID_INO ~0U

#define SEC_PER_BLOCK (BLOCK_SIZE / SECTOR_SIZE)

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

enum Inode_type {FILE, DIR};

typedef struct {
	/*
	 * Basic info of a file or directory.
	 * NOTE: `mode`, `owner`, `timestamp`
	 * are reserved for future.
	 */
	uint8_t nlink, type;
	uint16_t mode;
	uint32_t owner;
	/*
	 * For normal file, `size` is its size (B);
	 * for directory, `size` is the number of its files
	 * and sub directories.
	 */
	uint32_t size, timestamp;
	/*
	 * Direct pointer to file: the index of data block
	 * RELATIVE TO `GFS_base_sec`(/8). 4 KiB max for one.
	 */
	uint32_t dptr[INODE_NDPTR];
	/* Indirect pointer 4 MiB max */
	uint32_t idptr;
	/* Double indirect pointer: 4 GiB max */
	uint32_t diptr;
} GFS_inode_t;

typedef struct {
	/* The name of the file or sub directory */
	uint8_t fname[FNLEN + 1U];
	/* The index of its inode */
	uint32_t ino;
} dir_entry_t;

/* An 512 B buffer whose address is 8-B aligned */
typedef uint64_t sector_buf_t[SECTOR_SIZE / sizeof(uint64_t)];


extern unsigned int GFS_base_sec;
extern GFS_superblock_t GFS_superblock;
const uint8_t GFS_Magic[24U];

int GFS_read_sec(unsigned int sec_idx_in_GFS, unsigned int nsec, void* kva);
int GFS_write_sec(unsigned int sec_idx_in_GFS, unsigned int nsec, void* kva);

int GFS_check(void);
int GFS_init(void);
unsigned int GFS_alloc_in_bitmap(unsigned int n, unsigned int iarr[],
	unsigned int start_sec, unsigned int end_sec);
int GFS_read_inode(unsigned int ino, GFS_inode_t *pinode);
int GFS_write_inode(unsigned int ino, const GFS_inode_t *pinode);
unsigned int GFS_count_in_bitmap(unsigned int start_sec, unsigned int end_sec);

int do_mkfs(int force);
int do_fsinfo(void);

#endif
