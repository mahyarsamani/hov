/*
 * hov_pool.h — Pool Core Layer
 *
 * Manages the raw HOV memory pool: mmap, size parsing, header region.
 * No allocation logic — that lives in hov_alloc.c.
 */
#ifndef HOV_POOL_H
#define HOV_POOL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Reserve 2 MiB at the start of the pool for shared metadata.
 * Fields are placed at 64-byte (cache-line) offsets to avoid false sharing.
 *
 * Layout:
 *   [0x000]  shared_bump   (size_t) — persistent allocation bump counter
 *   [0x040]  chunk_bump    (size_t) — arena chunk allocation bump counter
 *   [0x080]  arena_frozen  (int)    — flag: shared region is frozen
 *   [0x0C0..0x200000)               — reserved for future use
 */
#define HOV_HEADER_SIZE  (2u * 1024u * 1024u)  /* 2 MiB */

/* Offsets within the header (in bytes, each on its own cache line) */
#define HOV_HDR_SHARED_BUMP    0x000
#define HOV_HDR_CHUNK_BUMP     0x040
#define HOV_HDR_ARENA_FROZEN   0x080

/* Initialize the pool: mmap /dev/hov, parse hov_mem_size from /proc/cmdline.
 * Idempotent — safe to call from multiple threads (mutex-protected).
 * After return, the pool is mapped and header fields are accessible. */
void hov_pool_init(void);

/* Pool accessors — valid only after hov_pool_init(). */
void  *hov_pool_raw(void);           /* raw mmap base (byte 0 of pool, start of header) */
void  *hov_pool_base(void);          /* start of usable pool (after 2 MiB header) */
size_t hov_pool_size(void);          /* total pool size (including header) */
size_t hov_pool_usable_size(void);   /* pool_size - HOV_HEADER_SIZE */

/* Access a shared header field as a pointer to size_t or int.
 * offset must be one of the HOV_HDR_* constants.
 * The returned pointer is into MAP_SHARED memory — use __atomic builtins. */
static inline size_t *hov_pool_hdr_sizet(void *raw, size_t offset) {
    return (size_t *)((char *)raw + offset);
}
static inline int *hov_pool_hdr_int(void *raw, size_t offset) {
    return (int *)((char *)raw + offset);
}

/* Alignment utility: round up to 4 KiB page boundary. */
#define HOV_PAGE_SIZE  4096u
#define HOV_ALIGN_UP(x, align) (((x) + (align) - 1u) & ~((align) - 1u))

/* Exported symbols for libhov_syscalls.so discovery via dlsym.
 * These are set by hov_pool_init(). */
extern void  *g_hov_mem_pool;  /* = hov_pool_raw() */
extern size_t g_hov_mem_size;  /* = hov_pool_size() */

#ifdef __cplusplus
}
#endif

#endif /* HOV_POOL_H */
