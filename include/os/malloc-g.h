/*
 * Memory allocation without compression.
 * Modified from the program of "Data Structure" course by Glucose180.
 */
#ifndef __MALLOC_G__
#define __MALLOC_G__

#include <type.h>
#include <os/mm.h>
#include <printk.h>
#include <os/glucose.h>

/* Total memory size: unit: PAGE (default: 4 KiB) */
#ifndef KSL
	#define KSL 512	/* 2 MiB */
#endif
#ifndef USL
	#define USL 4096	/* 16 MiB */
#endif
/* Size less than EU will not be preserved */
#ifndef EU
	#define EU 64U
#endif

/* Alignment of address returned from Xmalloc_g */
#define ADDR_ALIGN 0x8

/* Alignment of stack pointer in RISC-V 64 */
#define SP_ALIGN 0x10

enum Tag_t { FREE,OCCUPIED };

typedef struct header {
	union {
		/* the last(left) node */
		struct header* last;
		/* the head of this node */
		struct header* head;
	};
	int32_t tag;
	uint32_t size;
	/* the next(right) node */
	struct header* next;
} header;

void malloc_init(void);
void* kmalloc_g(const uint32_t Size);
void kfree_g(void* const P);
void kprint_avail_table(void);
void* umalloc_g(const uint32_t Size);
void ufree_g(void* const P);
//void uprint_avail_table(void);

#endif
