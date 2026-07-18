/*
 * hov_syscalls.c - LD_PRELOAD interposer for HOV memory
 *
 * Build as a standalone shared library:
 *   gcc -shared -fPIC -o libhov_syscalls.so hov_syscalls.c -ldl
 *
 * Usage:
 *   LD_PRELOAD=/path/to/libhov_syscalls.so ./ume_mpi_gradzatz_hov ...
 *
 * When loaded via LD_PRELOAD, this library interposes on read() and write().
 * If the buffer falls within the HOV /dev/hov mmap'd region (VM_PFNMAP),
 * the kernel's copy_to_user()/copy_from_user() would fail with -EFAULT.
 * We detect this and transparently bounce through a malloc'd buffer.
 *
 * The HOV memory range is discovered via dlsym(RTLD_DEFAULT) which finds
 * g_hov_mem_pool and g_hov_mem_size exported from the main binary's
 * libhov.a (linked with -rdynamic).
 */

#define _GNU_SOURCE
#include "hov_config.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

typedef ssize_t (*real_read_t)(int, void *, size_t);
typedef ssize_t (*real_write_t)(int, const void *, size_t);

static real_read_t  real_read_fn  = NULL;
static real_write_t real_write_fn = NULL;

/* Pointers to the HOV pool variables in the main binary */
static void  **hov_pool_ptr = NULL;
static size_t *hov_size_ptr = NULL;
static int     discovery_done = 0;

/* Bounce-buffer counters (to avoid flooding logs) */
static unsigned long bounce_read_count  = 0;
static unsigned long bounce_write_count = 0;

/*
 * Look up g_hov_mem_pool and g_hov_mem_size from the main binary.
 * These are non-static globals in hov.c (part of libhov.a), exported
 * to the dynamic symbol table via -rdynamic.
 */
static void discover_hov_pool(void) {
    if (discovery_done) return;
    discovery_done = 1;

    HOV_LOG("hov_syscalls", "discovery: looking up g_hov_mem_pool via dlsym(RTLD_DEFAULT)...");
    hov_pool_ptr = (void **)dlsym(RTLD_DEFAULT, "g_hov_mem_pool");
    if (!hov_pool_ptr) {
        HOV_LOG("hov_syscalls", "discovery: FAILED to find g_hov_mem_pool: %s", dlerror());
    } else {
        HOV_LOG("hov_syscalls", "discovery: found &g_hov_mem_pool at %p (current value: %p)",
                (void *)hov_pool_ptr, *hov_pool_ptr);
    }

    HOV_LOG("hov_syscalls", "discovery: looking up g_hov_mem_size via dlsym(RTLD_DEFAULT)...");
    hov_size_ptr = (size_t *)dlsym(RTLD_DEFAULT, "g_hov_mem_size");
    if (!hov_size_ptr) {
        HOV_LOG("hov_syscalls", "discovery: FAILED to find g_hov_mem_size: %s", dlerror());
    } else {
        HOV_LOG("hov_syscalls", "discovery: found &g_hov_mem_size at %p (current value: %zu)",
                (void *)hov_size_ptr, *hov_size_ptr);
    }

    if (hov_pool_ptr && hov_size_ptr) {
        if (*hov_pool_ptr == NULL) {
            HOV_LOG("hov_syscalls", "discovery: WARNING: g_hov_mem_pool is NULL. "
                    "Pool not yet initialized? Bounce will be checked live.");
        }
        if (*hov_size_ptr == 0) {
            HOV_LOG("hov_syscalls", "discovery: WARNING: g_hov_mem_size is 0. "
                    "Pool not yet initialized? Bounce will be checked live.");
        }
        HOV_LOG("hov_syscalls", "discovery: OK. Will check pool range on every read/write.");
    } else {
        HOV_LOG("hov_syscalls", "discovery: DISABLED. Bounce buffers will NOT be used. "
                "Ensure binary is linked with -rdynamic.");
    }
}

static inline int is_hov_memory(const void *buf) {
    if (!hov_pool_ptr || *hov_pool_ptr == NULL || *hov_size_ptr == 0)
        return 0;
    const char *b = (const char *)buf;
    const char *pool = (const char *)*hov_pool_ptr;
    return (b >= pool && b < pool + *hov_size_ptr);
}

ssize_t read(int fd, void *buf, size_t count) {
    if (!real_read_fn) {
        real_read_fn = (real_read_t)dlsym(RTLD_NEXT, "read");
        if (!real_read_fn) {
            HOV_LOG("hov_syscalls", "FATAL: dlsym(RTLD_NEXT, \"read\") failed: %s", dlerror());
            abort();
        }
        HOV_LOG("hov_syscalls", "init: resolved real read() at %p", (void *)real_read_fn);
    }

    if (!discovery_done) {
        discover_hov_pool();
    }

    if (count > 0 && is_hov_memory(buf)) {
        bounce_read_count++;
        if (bounce_read_count <= 5 || (bounce_read_count % 1000 == 0)) {
            HOV_LOG("hov_syscalls", "bounce-read #%lu: fd=%d buf=%p count=%zu "
                    "(pool=%p size=%zu)",
                    bounce_read_count, fd, buf, count,
                    *hov_pool_ptr, *hov_size_ptr);
        }

        void *bounce = malloc(count);
        if (!bounce) {
            HOV_LOG("hov_syscalls", "ERROR: bounce-read malloc(%zu) failed: %s",
                    count, strerror(errno));
            return -1;
        }

        ssize_t ret = real_read_fn(fd, bounce, count);
        if (ret < 0) {
            HOV_LOG("hov_syscalls", "ERROR: bounce-read real_read(fd=%d, count=%zu) "
                    "failed: %s", fd, count, strerror(errno));
        } else if (ret > 0) {
            memcpy(buf, bounce, ret);
        }
        free(bounce);
        return ret;
    }

    return real_read_fn(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
    if (!real_write_fn) {
        real_write_fn = (real_write_t)dlsym(RTLD_NEXT, "write");
        if (!real_write_fn) {
            HOV_LOG("hov_syscalls", "FATAL: dlsym(RTLD_NEXT, \"write\") failed: %s", dlerror());
            abort();
        }
        HOV_LOG("hov_syscalls", "init: resolved real write() at %p", (void *)real_write_fn);
    }

    if (!discovery_done) {
        discover_hov_pool();
    }

    if (count > 0 && is_hov_memory(buf)) {
        bounce_write_count++;
        if (bounce_write_count <= 5 || (bounce_write_count % 1000 == 0)) {
            HOV_LOG("hov_syscalls", "bounce-write #%lu: fd=%d buf=%p count=%zu "
                    "(pool=%p size=%zu)",
                    bounce_write_count, fd, buf, count,
                    *hov_pool_ptr, *hov_size_ptr);
        }

        void *bounce = malloc(count);
        if (!bounce) {
            HOV_LOG("hov_syscalls", "ERROR: bounce-write malloc(%zu) failed: %s",
                    count, strerror(errno));
            return -1;
        }

        memcpy(bounce, buf, count);
        ssize_t ret = real_write_fn(fd, bounce, count);
        if (ret < 0) {
            HOV_LOG("hov_syscalls", "ERROR: bounce-write real_write(fd=%d, count=%zu) "
                    "failed: %s", fd, count, strerror(errno));
        }

        free(bounce);
        return ret;
    }

    return real_write_fn(fd, buf, count);
}
