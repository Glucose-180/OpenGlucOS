/*
 * Memory allocation without compression.
 * Modified from the program of "Data Structure" course by Glucose180.
 * NOTE: You'd better add checking code in the kfree_g() functions,
 * to check whether the memory is allocated by kmalloc_g().
 */
#include <os/malloc-g.h>

/*
 * Use a 4 MiB space in .bss section of GlucOS kernel.
 * It is in .bss section, so clearing it is unnecessary and
 * time consuming. Just let kallocbuf point to an address
 * after __BSS_END__ can be considered.
 */
//static uint64_t _kallocbuf[KSL / 8U];

/*
 * Use a free space after .bss section instead
 * to avoid spending time clearing it.
 */
extern int __BSS_END__[];

static int8_t* kallocbuf;

static header* kpav;

/* point to the foot of the node p points */
static inline header* foot_loc(header* p)
{
	return (header*)((int8_t*)p + p->size - sizeof(header));
}

void malloc_init()
{
	header* p, *q;

	//kallocbuf = (void *)_kallocbuf;
	kallocbuf = (void *)ROUND((uintptr_t)__BSS_END__ + 16UL, 16UL);

	if ((uintptr_t)kallocbuf + KSL > 0xffffffc050e00000UL)
		panic_g("no enough space for kallocbuf: 0x%lx",
			(uintptr_t)kallocbuf);

	kpav = (header*)kallocbuf;
	p = kpav;

	p->next = p;
	p->size = KSL;
	p->tag = FREE;
	p->last = p;

	q = foot_loc(p);
	q->head = p;
	q->tag = FREE;
}

/* kmalloc: First fit */
void* kmalloc_g(const uint32_t Size)
{
	uint32_t n;
	header* h, *f;

	n = Size + 2U * sizeof(header);
	n = ROUND(n, ADDR_ALIGN);	/* ensure that n%8==0 */
	for (h = kpav; h != NULL && h->size < n && h->next != kpav; h = h->next)
		;
	if (h == NULL || h->size < n)
		return NULL;	/* Not found */
	f = foot_loc(h);
	kpav = h->next;	/* Convention */
	if (h->size - n < EU)
	{	/* allocate the single block */
		if (h == kpav)
			/* no free space left */
			kpav = NULL;
		else
		{
			h->last->next = kpav;
			kpav->last = h->last;
		}
		h->tag = f->tag = OCCUPIED;
	}
	else
	{	/* allocate the last n bytes */
		f->tag = OCCUPIED;
		h->size -= n;
		f = foot_loc(h);
		f->tag = FREE;
		f->head = h;
		h = f + 1;
		h->size = n;
		h->tag = OCCUPIED;
		f = foot_loc(h);
		f->head = h;
		/* Set f->head = h for checking in xfree_g */
	}
	if (((int64_t)(h + 1) & (ADDR_ALIGN - 1)) != 0)
		panic_g("addr %x is not %d-byte aligned", (int32_t)(int64_t)(h + 1), ADDR_ALIGN);
	/* return the true address(after the header) */
	return (void*)(h + 1);
}

void kfree_g(void* const P)
{
	header* h, *f;
	uint32_t n;
	header* lf, * rh, * lh;
	int lf_tag, rh_tag;

	h = (header*)P - 1;
	f = foot_loc(h);
	n = h->size;

	/* Check whether the block is allocated by xmalloc_g */
	if (((int64_t)P & (ADDR_ALIGN - 1)) != 0 ||
		((int64_t)f & (ADDR_ALIGN - 1)) != 0 ||
		f->head != h)
		panic_g("Addr 0x%lx is not allocated by kmalloc_g\n", (int64_t)P);

	lf = h - 1;	/* foot of the left block */
	if ((int8_t*)h - kallocbuf <= 0)
		lf_tag = 1;	/* left terminal, consider as left block is occupied */
	else
		lf_tag = lf->tag;

	rh = f + 1;	/* head of the right block */
	if ((int8_t*)rh - kallocbuf >= KSL)
		rh_tag = 1;
	else
		rh_tag = rh->tag;
	switch ((lf_tag << 1) + rh_tag)
	{
	case 3:	/* both L and F are occupied */
		h->tag = f->tag = FREE;
		f->head = h;
		if (kpav == NULL)
			kpav = h->next = h->last = h;
		else
		{	/* insert h */
			h->next = kpav;
			h->last = kpav->last;
			kpav->last->next = h;
			kpav->last = h;
			kpav = h;
		}
		break;
	case 2:	/* L is occupied */
		/* merge R and h */
		h->size = rh->size + n;
		if (rh->next == rh)
		{	/* only 1 block */
			h->next = h;
			h->last = h;
		}
		else
		{
			h->next = rh->next;
			h->last = rh->last;
		}
		f = foot_loc(h);
		f->head = h;
		h->tag = f->tag = FREE;
		h->last->next = h;
		h->next->last = h;
		kpav = h;	/* it is necessary */
		break;
	case 1:	/* R is occupied */
		/* merge L and h */
		lh = lf->head;
		lh->size += n;
		f = foot_loc(h);
		f->head = lh;
		f->tag = FREE;
		break;
	default:/* both L and F are free */
		/* merge L and h */
		lh = lf->head;
		lh->size += n + rh->size;
		lf = foot_loc(lh);
		lf->head = lh;
		/* delete node R */
		rh->last->next = rh->next;
		rh->next->last = rh->last;
		kpav = lh;	/* it is necessary */
		break;
	}
}

void kprint_avail_table()
{
	header* p;

	if (kpav == NULL)
	{
		writelog("kprint_alail_table: NULL\n");
		return;
	}
	p = kpav;
	do {
		writelog("kprint_alail_table: Start: 0x%lx, Size: %u, End: 0x%lx, Tag: %d;\n",
			(ptr_t)p, p->size, (ptr_t)p + p->size - 1U, p->tag);
	} while ((p = p->next) != kpav);
}
