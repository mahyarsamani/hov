/*
 * hov_alloc.c — Allocation Strategy Layer
 *
 * Phase 1 (shared):
 *   All processes share a single atomic bump counter at the start of the pool.
 *   Allocations are persistent (never freed).  Any rank can allocate; offsets
 *   from different ranks may be interleaved — each individual allocation is
 *   contiguous.
 *
 * Phase 2 (arenas):
 *   After hov_init_arenas(), the shared bump is frozen.  Each process gets a
 *   linked list of arena chunks, allocated on demand from a shared chunk bump
 *   counter.  Allocations within a chunk use a fast process-local bump.
 *   Deallocation resets the chunk bump (LIFO / stack semantics for v1).
 */
#include "hov_alloc.h"
#include "hov_pool.h"
#include "hov_config.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* -----------------------------------------------------------------------
 * Default arena chunk size: 128 MiB (overridable via env var)
 * -------------------------------------------------------------------- */
#define HOV_DEFAULT_CHUNK_SIZE  (128u * 1024u * 1024u)

/* -----------------------------------------------------------------------
 * Arena chunk header — lives at the start of each chunk (in MAP_SHARED pool).
 * Each chunk's usable memory starts at the next page boundary after the header.
 * -------------------------------------------------------------------- */
typedef struct hov_arena_chunk {
    struct hov_arena_chunk *next;    /* next chunk in this rank's list       */
    size_t chunk_size;               /* usable bytes (excluding header page) */
    size_t used;                     /* bump offset within usable region     */
} hov_arena_chunk_t;

/* Size of the chunk header (rounded up to a page) */
#define HOV_CHUNK_HDR_PAGES  1
#define HOV_CHUNK_HDR_SIZE   (HOV_CHUNK_HDR_PAGES * HOV_PAGE_SIZE)

/* -----------------------------------------------------------------------
 * Process-local state (per-fork, NOT in the pool)
 * -------------------------------------------------------------------- */
static int g_arenas_active = 0;                   /* 0 = shared phase, 1 = arena phase */
static hov_arena_chunk_t *arena_head = NULL;       /* first chunk in this process's list */
static hov_arena_chunk_t *arena_cur  = NULL;       /* current chunk for allocations      */
static size_t g_chunk_alloc_size = HOV_DEFAULT_CHUNK_SIZE; /* chunk size (incl. header)  */
static pthread_mutex_t g_alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

/* -----------------------------------------------------------------------
 * Shared header helpers
 * -------------------------------------------------------------------- */
static inline size_t *shared_bump(void) {
    return hov_pool_hdr_sizet(hov_pool_raw(), HOV_HDR_SHARED_BUMP);
}
static inline size_t *chunk_bump(void) {
    return hov_pool_hdr_sizet(hov_pool_raw(), HOV_HDR_CHUNK_BUMP);
}
static inline int *arena_frozen_flag(void) {
    return hov_pool_hdr_int(hov_pool_raw(), HOV_HDR_ARENA_FROZEN);
}

/* -----------------------------------------------------------------------
 * hov_init_shared_pool — explicit Phase 1 init (also called lazily)
 * -------------------------------------------------------------------- */
void hov_init_shared_pool(void) {
    hov_pool_init();  /* idempotent */
}

/* -----------------------------------------------------------------------
 * grab_new_chunk — atomically grab a chunk from the pool
 * -------------------------------------------------------------------- */
static hov_arena_chunk_t *grab_new_chunk(void) {
    size_t alloc_size = g_chunk_alloc_size;
    size_t offset = __atomic_fetch_add(chunk_bump(), alloc_size,
                                       __ATOMIC_SEQ_CST);

    /* Guard: check we haven't exceeded the pool */
    if (offset + alloc_size > hov_pool_size()) {
        HOV_LOG("hov_alloc", "ERROR: arena chunk allocation exceeds pool. "
                "offset=%zu chunk_size=%zu pool_size=%zu",
                offset, alloc_size, hov_pool_size());
        abort();
    }

    /* The chunk lives at pool_raw + offset */
    hov_arena_chunk_t *chunk =
        (hov_arena_chunk_t *)((char *)hov_pool_raw() + offset);

    chunk->next = NULL;
    chunk->chunk_size = alloc_size - HOV_CHUNK_HDR_SIZE;
    chunk->used = 0;

    HOV_LOG("hov_alloc", "grab_chunk: new chunk at pool offset=%zu, "
            "usable=%zu, chunk_bump now=%zu / %zu",
            offset, chunk->chunk_size,
            __atomic_load_n(chunk_bump(), __ATOMIC_SEQ_CST),
            hov_pool_size());

    return chunk;
}

/* -----------------------------------------------------------------------
 * usable_base — start of usable memory in a chunk (after header page)
 * -------------------------------------------------------------------- */
static inline void *chunk_usable_base(hov_arena_chunk_t *chunk) {
    return (char *)chunk + HOV_CHUNK_HDR_SIZE;
}

/* -----------------------------------------------------------------------
 * hov_init_arenas — freeze shared region, set up per-rank arena
 * -------------------------------------------------------------------- */
void hov_init_arenas(void) {
    hov_pool_init();  /* ensure pool is mapped */

    /* Read optional chunk size from env */
    const char *env = getenv("HOV_ARENA_CHUNK_SIZE");
    if (env) {
        size_t val = strtoull(env, NULL, 10);
        if (val >= HOV_CHUNK_HDR_SIZE + HOV_PAGE_SIZE) {
            g_chunk_alloc_size = HOV_ALIGN_UP(val, HOV_PAGE_SIZE);
            HOV_LOG("hov_alloc", "init_arenas: chunk size overridden to %zu from env",
                    g_chunk_alloc_size);
        }
    }

    /* Freeze the shared region */
    __atomic_store_n(arena_frozen_flag(), 1, __ATOMIC_SEQ_CST);

    /* Set the chunk bump to start right after the current shared bump.
     * Use CAS so only the first process to call init_arenas sets it. */
    size_t shared_end = __atomic_load_n(shared_bump(), __ATOMIC_SEQ_CST);
    size_t aligned_start = HOV_ALIGN_UP(shared_end, HOV_PAGE_SIZE);

    size_t expected = 0;
    __atomic_compare_exchange_n(chunk_bump(), &expected, aligned_start,
                                0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

    HOV_LOG("hov_alloc", "init_arenas: shared region frozen at %zu, "
            "chunk_bump starts at %zu, pool_size=%zu",
            shared_end,
            __atomic_load_n(chunk_bump(), __ATOMIC_SEQ_CST),
            hov_pool_size());

    /* Grab this process's first arena chunk */
    hov_arena_chunk_t *first = grab_new_chunk();
    arena_head = first;
    arena_cur  = first;
    g_arenas_active = 1;

    HOV_LOG("hov_alloc", "init_arenas: arena ready for this process. "
            "first chunk at %p, usable=%zu",
            (void *)first, first->chunk_size);
}

/* -----------------------------------------------------------------------
 * alloc_from_shared — Phase 1 allocation (atomic bump)
 * -------------------------------------------------------------------- */
static void *alloc_from_shared(size_t size) {
    /* Check frozen flag */
    if (__atomic_load_n(arena_frozen_flag(), __ATOMIC_SEQ_CST)) {
        HOV_LOG("hov_alloc", "ERROR: shared allocation attempted after arenas frozen. "
                "Call hov_init_arenas() only after all persistent allocs.");
        abort();
    }

    size_t aligned = HOV_ALIGN_UP(size, HOV_PAGE_SIZE);
    size_t offset = __atomic_fetch_add(shared_bump(), aligned,
                                       __ATOMIC_SEQ_CST);

    if (offset + aligned > hov_pool_size()) {
        HOV_LOG("hov_alloc", "ERROR: shared region OOM. requested=%zu (aligned=%zu) "
                "offset=%zu pool_size=%zu",
                size, aligned, offset, hov_pool_size());
        abort();
    }

    void *ret = (char *)hov_pool_raw() + offset;

    HOV_LOG("hov_alloc", "shared_alloc: %zu bytes (aligned=%zu) at %p "
            "(pool offset=%zu, bump=%zu / %zu)",
            size, aligned, ret, offset,
            __atomic_load_n(shared_bump(), __ATOMIC_SEQ_CST),
            hov_pool_size());

    return ret;
}

/* -----------------------------------------------------------------------
 * alloc_from_arena — Phase 2 allocation (process-local chunk bump)
 * -------------------------------------------------------------------- */
static void *alloc_from_arena(size_t size) {
    size_t aligned = HOV_ALIGN_UP(size, HOV_PAGE_SIZE);

    /* Try current chunk */
    if (arena_cur && arena_cur->used + aligned <= arena_cur->chunk_size) {
        void *ret = (char *)chunk_usable_base(arena_cur) + arena_cur->used;
        arena_cur->used += aligned;

        HOV_LOG("hov_alloc", "arena_alloc: %zu bytes (aligned=%zu) at %p "
                "(chunk offset=%zu, chunk used=%zu / %zu)",
                size, aligned, ret,
                arena_cur->used - aligned,
                arena_cur->used, arena_cur->chunk_size);
        return ret;
    }

    /* Current chunk full — grab a new one */
    HOV_LOG("hov_alloc", "arena_alloc: current chunk exhausted "
            "(need %zu, remaining=%zu). Grabbing new chunk...",
            aligned,
            arena_cur ? arena_cur->chunk_size - arena_cur->used : 0);

    hov_arena_chunk_t *new_chunk = grab_new_chunk();

    /* Coalescing check: is new chunk physically adjacent to current? */
    if (arena_cur) {
        char *cur_end = (char *)chunk_usable_base(arena_cur) +
                        arena_cur->chunk_size;
        if (cur_end == (char *)new_chunk) {
            /* Adjacent — extend current chunk instead of adding a node */
            arena_cur->chunk_size += g_chunk_alloc_size;
            HOV_LOG("hov_alloc", "arena_alloc: coalesced adjacent chunk, "
                    "new chunk_size=%zu", arena_cur->chunk_size);

            /* Now try allocation again in the extended chunk */
            if (arena_cur->used + aligned <= arena_cur->chunk_size) {
                void *ret = (char *)chunk_usable_base(arena_cur) +
                            arena_cur->used;
                arena_cur->used += aligned;
                return ret;
            }
        }
    }

    /* Not adjacent or still doesn't fit — add as new node */
    if (aligned > new_chunk->chunk_size) {
        HOV_LOG("hov_alloc", "ERROR: single allocation %zu exceeds chunk usable size %zu",
                aligned, new_chunk->chunk_size);
        abort();
    }

    if (arena_cur) {
        arena_cur->next = new_chunk;
    }
    arena_cur = new_chunk;

    void *ret = (char *)chunk_usable_base(arena_cur) + arena_cur->used;
    arena_cur->used += aligned;

    HOV_LOG("hov_alloc", "arena_alloc: %zu bytes (aligned=%zu) at %p "
            "(new chunk, used=%zu / %zu)",
            size, aligned, ret,
            arena_cur->used, arena_cur->chunk_size);
    return ret;
}

/* -----------------------------------------------------------------------
 * hov_alloc_data — public API, routes based on phase
 * -------------------------------------------------------------------- */
void *hov_alloc_data(size_t size) {
    /* Lazy init */
    if (g_hov_mem_pool == NULL) {
        pthread_mutex_lock(&g_alloc_mutex);
        if (g_hov_mem_pool == NULL) {
            HOV_LOG("hov_alloc", "alloc_data: pool not initialized, calling hov_pool_init()...");
            hov_pool_init();
        }
        pthread_mutex_unlock(&g_alloc_mutex);
    }

    if (g_arenas_active) {
        return alloc_from_arena(size);
    } else {
        return alloc_from_shared(size);
    }
}

/* -----------------------------------------------------------------------
 * hov_free_data — free arena allocation (v1: LIFO reset)
 * -------------------------------------------------------------------- */
void hov_free_data(void *ptr) {
    if (!g_arenas_active) {
        HOV_LOG("hov_alloc", "free_data: ignoring free for %p (shared region, never freed)",
                ptr);
        return;
    }

    if (ptr == NULL) return;

    /* v1: simple approach — check if ptr is in current chunk and reset.
     * For now, we log but don't reclaim. Full LIFO reset will be done
     * when the entire arena is released (e.g., in hov_context::destroy). */
    HOV_LOG("hov_alloc", "free_data: marking %p for deallocation (arena, v1: no-op reclaim)", ptr);
    (void)ptr;
}
