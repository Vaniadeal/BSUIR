#include "mmemory.h"
#include <string.h>

#define PAGE_SIZE   4096
#define MAX_PAGES   1024
#define MAX_FRAMES  512

static char ram[MAX_FRAMES * PAGE_SIZE];
static int  page_frame[MAX_PAGES];    // -1 = свободна, иначе номер фрейма
static int  page_block[MAX_PAGES];    // 0 = не начало блока, N = размер блока
static int  frame_used[MAX_FRAMES];   // 0 = свободен, 1 = занят

static int num_pages;
static int num_frames;
static int alloced;
static int used;

int mm_init(size_t ps, size_t np, size_t nf) {
    if (ps != PAGE_SIZE) return MM_ERROR;
    if (np == 0 || nf == 0) return MM_ERROR;
    if (np > MAX_PAGES || nf > MAX_FRAMES) return MM_ERROR;

    num_pages  = (int)np;
    num_frames = (int)nf;
    alloced = 0;
    used = 0;

    for (int i = 0; i < num_pages;  i++) { page_frame[i] = -1; page_block[i] = 0; }
    for (int i = 0; i < num_frames; i++) frame_used[i] = 0;

    return MM_OK;
}

vaddr_t mm_malloc(size_t size) {
    if (size == 0) return MM_NULL;
    int need = ((int)size + PAGE_SIZE - 1) / PAGE_SIZE;
    if (need > num_frames - used) return MM_NULL;

    int run = 0;
    for (int i = 0; i < num_pages; i++) {
        run = (page_frame[i] == -1) ? run + 1 : 0;
        if (run == need) {
            int start = i + 1 - need;

            int taken = 0;
            for (int f = 0; f < num_frames && taken < need; f++) {
                if (!frame_used[f]) {
                    frame_used[f] = 1;
                    page_frame[start + taken] = f;
                    taken++;
                }
            }
            page_block[start] = need;
            alloced += need;
            used += need;

            return (vaddr_t)(start * PAGE_SIZE);
        }
    }
    return MM_NULL;
}

int mm_free(vaddr_t addr) {
    int page = addr / PAGE_SIZE;
    if (page < 0 || page >= num_pages) return MM_ERROR;
    if (page_block[page] <= 0) return MM_ERROR;

    int n = page_block[page];
    for (int i = page; i < page + n; i++) {
        if (page_frame[i] >= 0) frame_used[page_frame[i]] = 0;
        page_frame[i] = -1;
        page_block[i] = 0;
    }
    alloced -= n;
    used -= n;
    return MM_OK;
}

int mm_write(vaddr_t addr, const void *buf, size_t size) {
    const char *src = (const char *)buf;
    while (size > 0) {
        int page  = addr / PAGE_SIZE;
        int off   = addr % PAGE_SIZE;
        int chunk = PAGE_SIZE - off;
        if ((size_t)chunk > size) chunk = (int)size;

        if (page < 0 || page >= num_pages) return MM_ERROR;
        if (page_frame[page] < 0) return MM_ERROR;

        memcpy(ram + (size_t)page_frame[page] * PAGE_SIZE + off, src, chunk);
        addr += chunk; src += chunk; size -= chunk;
    }
    return MM_OK;
}

int mm_read(vaddr_t addr, void *buf, size_t size) {
    char *dst = (char *)buf;
    while (size > 0) {
        int page  = addr / PAGE_SIZE;
        int off   = addr % PAGE_SIZE;
        int chunk = PAGE_SIZE - off;
        if ((size_t)chunk > size) chunk = (int)size;

        if (page < 0 || page >= num_pages) return MM_ERROR;
        if (page_frame[page] < 0) return MM_ERROR;

        memcpy(dst, ram + (size_t)page_frame[page] * PAGE_SIZE + off, chunk);
        addr += chunk; dst += chunk; size -= chunk;
    }
    return MM_OK;
}

size_t mm_allocated_pages(void) { return (size_t)alloced; }
