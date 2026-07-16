/*
 * hov.c — Pair lifecycle & alias management
 *
 * Pool management has moved to hov_pool.c.
 * Allocation logic has moved to hov_alloc.c.
 * This file only handles hov_create_pair / hov_destroy_pair.
 */
#include "hov.h"
#include "hov_pool.h"

#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

#define HOV_LOG(fmt, ...) \
    fprintf(stderr, "[hov] " fmt "\n", ##__VA_ARGS__)

hov_pair_t hov_create_pair(void *data, void *index,
                           size_t data_elem_size,
                           size_t index_elem_size,
                           size_t count)
{
    HOV_LOG("create_pair: data=%p index=%p data_elem_size=%zu "
            "index_elem_size=%zu count=%zu",
            data, index, data_elem_size, index_elem_size, count);

    hov_pair_t pair;
    pair.data_base = data;
    pair.index_base = index;
    pair.data_elem_size = data_elem_size;
    pair.index_elem_size = index_elem_size;
    pair.count = count;

    /* Allocate a VA region for aliases. */
    size_t alias_region_size = count * data_elem_size;
    HOV_LOG("create_pair: mmap alias region of %zu bytes (PROT_NONE)...",
            alias_region_size);
    void *alias_region = mmap(NULL, alias_region_size,
                              PROT_NONE,
                              MAP_ANONYMOUS | MAP_PRIVATE,
                              -1, 0);
    if (alias_region == MAP_FAILED) {
        HOV_LOG("create_pair: WARNING: PROT_NONE mmap failed, "
                "falling back to PROT_READ...");
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
    HOV_LOG("create_pair: alias region at %p (size=%zu)", alias_region,
            alias_region_size);
    return pair;
}

void hov_destroy_pair(hov_pair_t *pair)
{
    if (pair->base_alias != NULL) {
        size_t alias_region_size = pair->count * pair->data_elem_size;
        HOV_LOG("destroy_pair: unmapping alias at %p (size=%zu)",
                pair->base_alias, alias_region_size);
        munmap(pair->base_alias, alias_region_size);
        pair->base_alias = NULL;
    }
}
