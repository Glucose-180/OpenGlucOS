/*
 * gfs0.c:
 * Basic operations (such as initialization) for Glucose File System.
 */
#include <os/gfs.h>
#include <os/sched.h>
#include <common.h>

/*
 * The index of the first sector for GFS.
 * It is initialized in `init_swap()` and should be
 * 8-Sec aligned.
 */
unsigned int GFS_base_sec;

const uint8_t GFS_Magic[24U] = "Z53/4 Beijingxi-Kunming";

