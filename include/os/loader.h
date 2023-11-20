#ifndef __INCLUDE_LOADER_H__
#define __INCLUDE_LOADER_H__

#include <type.h>
#include <os/mm.h>

uint64_t load_task_img(const char *taskname, PTE* pgdir_kva, pid_t pid,
	 uintptr_t* pstart, uintptr_t* pend);

#endif