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

#ifndef PATH_LEN
/*
 * Max length of path.
 * also defined in `sched.h`.
 */
#define PATH_LEN 71
#endif

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
	/*
	 * Double indirect pointer: 4 GiB max.
	 * It is not used in a directory inode.
	 */
	uint32_t diptr;
} GFS_inode_t;

/* Directory entry */
typedef struct {
	/* The name of the file or sub directory */
	uint8_t fname[FNLEN + 1U];
	/* The index of its inode */
	uint32_t ino;
} GFS_dentry_t;

/*
 * How many entries can a directory hold (including "." and "..").
 * Typically it is (10+1024)*64=66176.
 */
#define NDENTRIES ((INODE_NDPTR + BLOCK_SIZE / sizeof(uint32_t)) * \
	(BLOCK_SIZE / sizeof(GFS_dentry_t)))

/* Number of dir entries in a data block */
#define DENT_PER_BLOCK (BLOCK_SIZE / sizeof(GFS_dentry_t))

/* A 512 B buffer whose address is 8-B aligned */
typedef uint64_t sector_buf_t[SECTOR_SIZE / sizeof(uint64_t)];

/* A 4 KiB buffer of a indirect block */
typedef uint32_t indblock_buf_t[BLOCK_SIZE / sizeof(uint32_t)];

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

unsigned int search_dentry_in_dir_inode
	(const GFS_inode_t *pinode, const char *fname);
unsigned int path_anal(const char *spath);
int do_changedir(const char *tpath);
unsigned int do_getpath(char *path);
unsigned int path_squeeze(char *path);

int GFS_add_dentry(GFS_inode_t *pinode, const char *fname, unsigned int ino);

static inline int GFS_write_block(unsigned int bidx_in_GFS, void *kva)
{
	return GFS_write_sec(bidx_in_GFS * SEC_PER_BLOCK, SEC_PER_BLOCK, kva);
}

static inline int GFS_read_block(unsigned int bidx_in_GFS, void *kva)
{
	return GFS_read_sec(bidx_in_GFS * SEC_PER_BLOCK, SEC_PER_BLOCK, kva);
}

#endif
