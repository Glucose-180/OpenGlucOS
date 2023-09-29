/*
 * pcb-list-g.c: by Glucose180
 * Operations for linked list of PCB.
 */

#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>

