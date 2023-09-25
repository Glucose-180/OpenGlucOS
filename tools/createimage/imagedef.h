#ifndef __IMAGEDEF_H__
#define __IMAGEDEF_H__

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512
#define BOOT_LOADER_SIG_OFFSET 0x1fe
#define BOOT_LOADER_SIG_1 0x55
#define BOOT_LOADER_SIG_2 0xaa

#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))
#define TASK_MAXNUM 16

#define TASK_NAMELEN 31	/* Max length of app's name */
/* TODO: [p1-task4] design your own task_info_t */
typedef struct {
	uint32_t offs;	/* Offset in image file */
	uint32_t size;	/* Size in image file */
	uint32_t entr;	/* Entry addr in main memory */
	char name[TASK_NAMELEN + 1];
} task_info_t;

static const long Bootloader_sig_offset = BOOT_LOADER_SIG_OFFSET,
	Image_size_offset = 256;
//	Kernel_size_offset = 260, Taskinfo_offset_offset = 264, Tasknum_offset = 268;

#endif