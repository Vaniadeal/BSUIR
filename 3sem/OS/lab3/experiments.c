#include "mmemory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE 4096

static void exp_fragmentation(void) {
    FILE *f = fopen("frag.csv", "w");
    fprintf(f, "block_size,wasted_bytes,wasted_percent\n");

    int sizes[] = {1, 100, 500, 1000, 2000, 4000, 4096, 4097,
                   5000, 8000, 8192, 8193, 10000, 16384};
    size_t n = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t i = 0; i < n; i++) {
        mm_init(PAGE, 1024, 256);
        mm_malloc((size_t)sizes[i]);

        size_t used = mm_allocated_pages() * PAGE;
        size_t wasted = used - (size_t)sizes[i];
        double percent = 100.0 * (double)wasted / (double)used;

        fprintf(f, "%d,%zu,%.2f\n", sizes[i], wasted, percent);
        mm_destroy();
    }
    fclose(f);
    printf("frag.csv written\n");
}

static void exp_accumulation(void) {
    mm_init(PAGE, 1024, 512);
    FILE *f = fopen("accum_frag.csv", "w");
    fprintf(f, "allocations,wasted_bytes\n");
    srand(42);
    size_t total_wasted = 0;
    for (int i = 1; i <= 500; i++) {
        size_t size = 1 + (size_t)(rand() % PAGE);
        vaddr_t addr = mm_malloc(size);
        if (addr == MM_NULL) {
            fprintf(stderr, "Allocation failed at %d\n", i);
            break;
        }
        total_wasted += PAGE - size;
        if (i % 50 == 0) {
            fprintf(f, "%d,%zu\n", i, total_wasted);
        }
    }
    fclose(f);
    mm_destroy();
    printf("accum_frag.csv written\n");
}

int main(void) {
    exp_fragmentation();
    exp_accumulation();
    return 0;
}
