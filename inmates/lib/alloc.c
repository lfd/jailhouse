#include <inmate.h>

static unsigned long heap_pos = (unsigned long)stack_top;

void *alloc(unsigned long size, unsigned long align)
{
	unsigned long base = (heap_pos + align - 1) & ~(align - 1);

	heap_pos = base + size;
	return (void *)base;
}
