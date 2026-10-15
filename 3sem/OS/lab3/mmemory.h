#ifndef MMEMORY_H
#define MMEMORY_H

#include <stddef.h>

typedef unsigned int vaddr_t;

#define MM_OK     0
#define MM_ERROR (-1)
#define MM_NULL  ((vaddr_t)-1)

int  mm_init(size_t pageSize, size_t numVirtualPages, size_t numFrames);
void mm_destroy(void);

vaddr_t mm_malloc(size_t size);
int     mm_free(vaddr_t addr);

int mm_write(vaddr_t addr, const void *buf, size_t size);
int mm_read (vaddr_t addr, void       *buf, size_t size);

size_t mm_page_faults(void);
size_t mm_allocated_pages(void);
size_t mm_used_frames(void);    

#endif