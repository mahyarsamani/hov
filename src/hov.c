#include "hov.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

static void *g_hov_mem_pool = NULL;
static size_t g_hov_mem_allocated = 0;
static size_t g_hov_mem_size = 0;
static pthread_mutex_t g_hov_mem_mutex = PTHREAD_MUTEX_INITIALIZER;

static void hov_init_pool(void) {
    if (g_hov_mem_pool != NULL) {
        return;
    }

    /* We don't need to parse /proc/cmdline for physical base anymore.
     * The hov_drv kernel module handles the physical base and size mapping.
     * We just open /dev/hov and map it! */

    FILE *fp = fopen("/proc/cmdline", "r");
    if (!fp) {
        perror("hov_init_pool: fopen /proc/cmdline failed");
        abort();
    }
    char cmdline[4096];
    if (!fgets(cmdline, sizeof(cmdline), fp)) {
        perror("hov_init_pool: fgets failed");
        abort();
    }
    fclose(fp);

    char *size_ptr = strstr(cmdline, "hov_mem_size=");
    if (!size_ptr) {
        fprintf(stderr, "hov_init_pool: hov_mem_size= not found in /proc/cmdline\n");
        abort();
    }

    g_hov_mem_size = strtoull(size_ptr + 13, NULL, 10);
    if (g_hov_mem_size == 0) {
        fprintf(stderr, "hov_init_pool: invalid hov_mem_size\n");
        abort();
    }

    int fd = open("/dev/hov", O_RDWR);
    if (fd < 0) {
        perror("hov_init_pool: open /dev/hov failed");
        abort();
    }

    /* Map at offset 0, the driver knows the physical address. */
    g_hov_mem_pool = mmap(NULL, g_hov_mem_size, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, 0);
    if (g_hov_mem_pool == MAP_FAILED) {
        perror("hov_init_pool: mmap /dev/hov failed");
        abort();
    }
    close(fd);
}

void* hov_alloc_data(size_t size) {
    pthread_mutex_lock(&g_hov_mem_mutex);
    if (g_hov_mem_pool == NULL) {
        hov_init_pool();
    }

    /* Align to 64 bytes (cache line) */
    size_t align = 64;
    size = (size + align - 1) & ~(align - 1);

    if (g_hov_mem_allocated + size > g_hov_mem_size) {
        fprintf(stderr, "hov_alloc_data: Out of memory in HOV pool\n");
        abort();
    }

    void* ret = (char*)g_hov_mem_pool + g_hov_mem_allocated;
    g_hov_mem_allocated += size;

    pthread_mutex_unlock(&g_hov_mem_mutex);
    return ret;
}

void hov_free_data(void* ptr) {
    /* Basic bump allocator: no-op free */
    (void)ptr;
}

hov_pair_t hov_create_pair(void *data, void *index,
                           size_t data_elem_size,
                           size_t index_elem_size,
                           size_t count)
{
    hov_pair_t pair;
    pair.data_base = data;
    pair.index_base = index;
    pair.data_elem_size = data_elem_size;
    pair.index_elem_size = index_elem_size;
    pair.count = count;

    /* Allocate a VA region for aliases.
     * PROT_NONE: no actual access permitted — accidental dereference
     * segfaults immediately (fail-fast).
     * MAP_ANONYMOUS: no file backing; no physical pages committed. */
    size_t alias_region_size = count * data_elem_size;
    void *alias_region = mmap(NULL, alias_region_size,
                              PROT_NONE,
                              MAP_ANONYMOUS | MAP_PRIVATE,
                              -1, 0);
    if (alias_region == MAP_FAILED) {
        /* Fallback: PROT_READ (gem5 SE mode may not support PROT_NONE) */
        alias_region = mmap(NULL, alias_region_size,
                            PROT_READ,
                            MAP_ANONYMOUS | MAP_PRIVATE,
                            -1, 0);
        if (alias_region == MAP_FAILED) {
            perror("hov_create_pair: mmap failed");
            abort();
        }
    }
    pair.base_alias = alias_region;
    return pair;
}

void hov_destroy_pair(hov_pair_t *pair)
{
    if (pair->base_alias != NULL) {
        size_t alias_region_size = pair->count * pair->data_elem_size;
        munmap(pair->base_alias, alias_region_size);
        pair->base_alias = NULL;
    }
}
