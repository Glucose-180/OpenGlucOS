/*
 * Memory allocation without compression.
 * Modified from the program of "Data Structure" course by Glucose180.
 */
#include <os/malloc-g.h>

/* Memory space to be allocated (Kernel, User) */
int8_t* kallocbuf, * uallocbuf;

static header* kpav, * upav;

/* point to the foot of the node p points */
static inline header* foot_loc(header* p)
{
	return (header*)((int8_t*)p + p->size - sizeof(header));
}

void malloc_init()
{
	header* p, *q;

	kallocbuf = allocKernelPage(KSL);
	uallocbuf = allocUserPage(USL);

	kpav = (header*)kallocbuf;
	p = kpav;
	q = foot_loc(p);

	p->next = p;
	p->size = KSL;
	p->tag = FREE;
	p->last = p;

	q->head = p;
	q->tag = FREE;

	upav = (header*)uallocbuf;
	p = upav;
	q = foot_loc(p);

	p->next = p;
	p->size = USL;
	p->tag = FREE;
	p->last = p;

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
	}
	if (((int64_t)(h + 1) & (ADDR_ALIGN - 1)) != 0)
		panic_g("kmalloc_g: addr %x is not %d-byte aligned", (int32_t)(int64_t)(h + 1), ADDR_ALIGN);
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
		printk("NULL\n");
		return;
	}
	p = kpav;
	do {
		printk("Start: %lld, Size: %u, End: %lld, Tag: %d;\n", (int8_t*)p - kallocbuf, p->size, (int8_t*)p - kallocbuf + (ptrdiff_t)p->size - 1, p->tag);
	} while ((p = p->next) != kpav);
}

/* umalloc: First fit */
void* umalloc_g(const uint32_t Size)
{
	uint32_t n;
	header* h, *f;

	n = Size + 2U * sizeof(header);
	n = ROUND(n, ADDR_ALIGN);	/* ensure that n%8==0 */
	for (h = upav; h != NULL && h->size < n && h->next != upav; h = h->next)
		;
	if (h == NULL || h->size < n)
		return NULL;	/* Not found */
	f = foot_loc(h);
	upav = h->next;	/* Convention */
	if (h->size - n < EU)
	{	/* allocate the single block */
		if (h == upav)
			/* no free space left */
			upav = NULL;
		else
		{
			h->last->next = upav;
			upav->last = h->last;
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
	}
	if (((int64_t)(h + 1) & (ADDR_ALIGN - 1)) != 0)
		panic_g("umalloc_g: addr %x is not %d-byte aligned", (int32_t)(int64_t)(h + 1), ADDR_ALIGN);
	/* return the true address(after the header) */
	return (void*)(h + 1);
}

void ufree_g(void* const P)
{
	header* h, *f;
	uint32_t n;
	header* lf, * rh, * lh;
	int lf_tag, rh_tag;

	h = (header*)P - 1;
	f = foot_loc(h);
	n = h->size;

	lf = h - 1;	/* foot of the left block */
	if ((int8_t*)h - kallocbuf <= 0)
		lf_tag = 1;	/* left terminal, consider as left block is occupied */
	else
		lf_tag = lf->tag;

	rh = f + 1;	/* head of the right block */
	if ((int8_t*)rh - kallocbuf >= USL)
		rh_tag = 1;
	else
		rh_tag = rh->tag;
	switch ((lf_tag << 1) + rh_tag)
	{
	case 3:	/* both L and F are occupied */
		h->tag = f->tag = FREE;
		f->head = h;
		if (upav == NULL)
			upav = h->next = h->last = h;
		else
		{	/* insert h */
			h->next = upav;
			h->last = upav->last;
			upav->last->next = h;
			upav->last = h;
			upav = h;
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
		upav = h;	/* it is necessary */
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
		upav = lh;	/* it is necessary */
		break;
	}
}