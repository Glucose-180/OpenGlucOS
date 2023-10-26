#ifndef __INCLUDE_TASK_H__
#define __INCLUDE_TASK_H__

#include <type.h>

#define TASK_MEM_BASE    0x52000000
#define TASK_MAXNUM      16
#define TASK_SIZE        0x10000


#define SECTOR_SIZE 512
#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

#define TASK_NAMELEN 31	/* Max length of app's name */

/*
 * How many sectors can taskinfo occupies.
 * It equals to the space size between start of bootloader and kernel,
 * as taskinfo would be written into the space of bootloader.
 */
#define TASKINFO_SECTORS_MAX ((0x50201000 - 0x50200000) / SECTOR_SIZE)

/* TODO: [p1-task4] implement your own task_info_t! */
typedef struct {	/* It should be the same as struct in createimage.c */
	uint32_t offs;	/* Offset in image file */
	uint32_t size;	/* Size in image file */
	uint32_t addr;	/* Start addr, or vaddr of segment 0 */
	uint32_t entr;	/* Entry addr in main memory */
	char name[TASK_NAMELEN + 1];
} task_info_t;
extern unsigned int tasknum;

extern task_info_t taskinfo[TASK_MAXNUM];

#endif
