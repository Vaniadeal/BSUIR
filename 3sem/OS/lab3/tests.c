#include "mmemory.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PAGE   4096
#define VPAGES 64
#define FRAMES 16

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, name) do {                                \
    if (cond) {                                               \
        printf("[ OK ] %s\n", name);                          \
        tests_passed++;                                       \
    } else {                                                  \
        printf("[FAIL] %s\n", name);                          \
        tests_failed++;                                       \
    }                                                         \
} while (0)

// Инициализация и уничтожение
static void test_init_and_destroy(void) {
    CHECK(mm_init(PAGE, VPAGES, FRAMES) == MM_OK, "init ok");
    CHECK(mm_allocated_pages() == 0,             "no pages allocated after init");
    CHECK(mm_used_frames() == 0,                 "no frames used after init");
    CHECK(mm_page_faults() == 0,                 "no faults after init");
    mm_destroy();
}

// Неверные аргументы mm_init
static void test_init_bad_args(void) {
    CHECK(mm_init(0,    VPAGES, FRAMES) == MM_ERROR, "init rejects pageSize=0");
    CHECK(mm_init(3000, VPAGES, FRAMES) == MM_ERROR, "init rejects non-power-of-two");
    CHECK(mm_init(PAGE, 0,      FRAMES) == MM_ERROR, "init rejects numPages=0");
    CHECK(mm_init(PAGE, VPAGES, 0)      == MM_ERROR, "init rejects numFrames=0");
}

// Простое выделение
static void test_simple_alloc(void) {
    mm_init(PAGE, VPAGES, FRAMES);

    vaddr_t a = mm_malloc(100);
    CHECK(a != MM_NULL,                "malloc(100) returns address");
    CHECK(mm_allocated_pages() == 1,   "one page allocated for 100 bytes");

    vaddr_t b = mm_malloc(PAGE);
    CHECK(b != MM_NULL,                "malloc(4096) returns address");
    CHECK(mm_allocated_pages() == 2,   "two pages allocated total");

    mm_destroy();
}

// malloc(0)
static void test_alloc_zero(void) {
    mm_init(PAGE, VPAGES, FRAMES);
    CHECK(mm_malloc(0) == MM_NULL, "malloc(0) returns MM_NULL");
    mm_destroy();
}

// Простая запись и чтение 
static void test_write_read_simple(void) {
    mm_init(PAGE, VPAGES, FRAMES);

    vaddr_t a = mm_malloc(100);
    char in[] = "hello, world";
    CHECK(mm_write(a, in, 12) == MM_OK, "write ok");

    char out[13] = {0};
    CHECK(mm_read(a, out, 12) == MM_OK,   "read ok");
    CHECK(strcmp(in, out) == 0,           "data matches");

   
    mm_destroy();
}
// Запись/чтение через границу страниц 
static void test_cross_page(void) {
    mm_init(PAGE, VPAGES, FRAMES);

    vaddr_t a = mm_malloc(PAGE * 3);
    CHECK(a != MM_NULL, "malloc(12288) ok");

    size_t n = 6000;
    char *in = malloc(n);
    for (size_t i = 0; i < n; i++) in[i] = (char)(i & 0xFF);

    vaddr_t mid = a + PAGE - 100;
    CHECK(mm_write(mid, in, n) == MM_OK, "cross-page write ok");

    char *out = malloc(n);
    CHECK(mm_read(mid, out, n) == MM_OK, "cross-page read ok");
    CHECK(memcmp(in, out, n) == 0,       "cross-page data matches");

    free(in);
    free(out);
    mm_destroy();
}

// Освобождение и повторное использование
static void test_free_and_reuse(void) {
    mm_init(PAGE, VPAGES, FRAMES);

    vaddr_t a = mm_malloc(PAGE);
    vaddr_t b = mm_malloc(PAGE);
    CHECK(a != MM_NULL && b != MM_NULL, "two blocks allocated");
    CHECK(b == a + PAGE,                "blocks are adjacent");

    CHECK(mm_free(a) == MM_OK,             "free first block");
    CHECK(mm_allocated_pages() == 1,       "one page after free");

    vaddr_t c = mm_malloc(PAGE);
    CHECK(c == a,                          "freed page is reused");

    mm_destroy();
}

// Освобождение многократного блока 
static void test_free_multi_page(void) {
    mm_init(PAGE, VPAGES, FRAMES);

    vaddr_t a = mm_malloc(PAGE * 3);
    CHECK(a != MM_NULL,                    "malloc(3 pages) ok");
    CHECK(mm_allocated_pages() == 3,       "3 pages allocated");

    CHECK(mm_free(a) == MM_OK,             "free 3-page block");
    CHECK(mm_allocated_pages() == 0,       "0 pages after free");

    mm_destroy();
}

// Некорректные вызовы free
static void test_free_bad(void) {
    mm_init(PAGE, VPAGES, FRAMES);

    CHECK(mm_free(0) == MM_ERROR,          "free never allocated -> error");
    CHECK(mm_free(999999) == MM_ERROR,     "free out of range -> error");

    vaddr_t a = mm_malloc(PAGE * 2);
    CHECK(mm_free(a + PAGE) == MM_ERROR,   "free middle of block -> error");

    CHECK(mm_free(a) == MM_OK,             "free start of block ok");

    mm_destroy();
}

// Нехватка памяти
static void test_out_of_memory(void) {
    mm_init(PAGE, 2, 1);

    CHECK(mm_malloc(PAGE) != MM_NULL,      "first page ok");
    CHECK(mm_malloc(PAGE) != MM_NULL,      "second page ok");
    CHECK(mm_malloc(PAGE) == MM_NULL,      "third page -> MM_NULL");

    mm_destroy();
}



// Точка входа
int main(void) {
    printf("=== unit tests ===\n\n");

    test_init_and_destroy();
    test_init_bad_args();
    test_simple_alloc();
    test_alloc_zero();
    test_write_read_simple();
    test_cross_page();
    test_free_and_reuse();
    test_free_multi_page();
    test_free_bad();
    test_out_of_memory();

    printf("\n=== results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed == 0 ? 0 : 1;
}