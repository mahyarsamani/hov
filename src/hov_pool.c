/*
 * hov_pool.c — Pool Core Layer
 *
 * Manages the raw HOV memory pool: mmap /dev/hov, parse size from
 * /proc/cmdline, expose pool base/size/header.
 *
 * No allocation logic lives here — see hov_alloc.c.
 */
#include "hov_pool.h"
#include "hov_ioctl.h"

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define HOV_LOG(fmt, ...) \
    fprintf(stderr, "[hov_pool] " fmt "\n", ##__VA_ARGS__)

/* -----------------------------------------------------------------------
 * Process-local state (private after fork, each process gets its own copy)
 * -------------------------------------------------------------------- */
void  *g_hov_mem_pool = NULL;   /* exported for dlsym discovery */
size_t g_hov_mem_size = 0;      /* exported for dlsym discovery */
static pthread_mutex_t g_init_mutex = PTHREAD_MUTEX_INITIALIZER;

/* -----------------------------------------------------------------------
 * hov_pool_init — idempotent, thread-safe
 * -------------------------------------------------------------------- */
void hov_pool_init(void) {
    /* Double-checked locking: fast path (no lock) */
    if (g_hov_mem_pool != NULL) {
        return;
    }

    pthread_mutex_lock(&g_init_mutex);
    /* Re-check under lock */
    if (g_hov_mem_pool != NULL) {
        pthread_mutex_unlock(&g_init_mutex);
        return;
    }

    /* --- Open /dev/hov --- */
    HOV_LOG("init: opening /dev/hov...");
    int fd = open("/dev/hov", O_RDWR);
    if (fd < 0) {
        perror("hov_pool_init: open /dev/hov failed");
        abort();
    }
    HOV_LOG("init: /dev/hov opened as fd=%d", fd);

    /* --- Query pool size from the driver via ioctl --- */
    unsigned long long ioctl_size = 0;
    if (ioctl(fd, HOV_GET_MEM_SIZE, &ioctl_size) < 0) {
        perror("hov_pool_init: ioctl HOV_GET_MEM_SIZE failed");
        close(fd);
        abort();
    }
    g_hov_mem_size = (size_t)ioctl_size;

    if (g_hov_mem_size == 0) {
        HOV_LOG("init: ERROR: driver returned hov_mem_size = 0");
        close(fd);
        abort();
    }
    if (g_hov_mem_size <= HOV_HEADER_SIZE) {
        HOV_LOG("init: ERROR: pool size %zu <= header size %u",
                g_hov_mem_size, HOV_HEADER_SIZE);
        close(fd);
        abort();
    }
    HOV_LOG("init: hov_mem_size = %zu bytes (%.2f GiB)",
            g_hov_mem_size, (double)g_hov_mem_size / (1024.0 * 1024 * 1024));

    /* --- mmap the full pool --- */
    HOV_LOG("init: calling mmap(NULL, %zu, PROT_READ|PROT_WRITE, "
            "MAP_SHARED, fd=%d, 0)...", g_hov_mem_size, fd);
    void *pool = mmap(NULL, g_hov_mem_size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);
    if (pool == MAP_FAILED) {
        perror("hov_pool_init: mmap /dev/hov failed");
        abort();
    }
    close(fd);

    /* --- Initialize shared header (CAS: only first process wins) --- */
    size_t *bump = hov_pool_hdr_sizet(pool, HOV_HDR_SHARED_BUMP);
    size_t expected = 0;
    __atomic_compare_exchange_n(bump, &expected, (size_t)HOV_HEADER_SIZE,
                                0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

    size_t *chunk = hov_pool_hdr_sizet(pool, HOV_HDR_CHUNK_BUMP);
    expected = 0;
    /* chunk_bump is not initialized until hov_init_arenas(), leave at 0 */
    (void)chunk;

    /* Publish the pool pointer (makes the fast-path check above succeed) */
    g_hov_mem_pool = pool;

    HOV_LOG("init: SUCCESS. pool=%p size=%zu (%.2f GiB) "
            "header=%u usable=%zu shared_bump=%zu",
            pool, g_hov_mem_size,
            (double)g_hov_mem_size / (1024.0 * 1024 * 1024),
            HOV_HEADER_SIZE, g_hov_mem_size - HOV_HEADER_SIZE,
            __atomic_load_n(bump, __ATOMIC_SEQ_CST));

    pthread_mutex_unlock(&g_init_mutex);
}

/* -----------------------------------------------------------------------
 * Accessors
 * -------------------------------------------------------------------- */
void *hov_pool_raw(void) {
    return g_hov_mem_pool;
}

void *hov_pool_base(void) {
    return (char *)g_hov_mem_pool + HOV_HEADER_SIZE;
}

size_t hov_pool_size(void) {
    return g_hov_mem_size;
}

size_t hov_pool_usable_size(void) {
    return g_hov_mem_size - HOV_HEADER_SIZE;
}
