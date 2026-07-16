/*
 * hov_alloc.h — Allocation Strategy Layer
 *
 * Two-phase allocator on top of the HOV pool:
 *   Phase 1 (shared):  persistent allocations via shared atomic bump
 *   Phase 2 (arenas):  transient allocations via per-rank linked-list chunks
 */
#ifndef HOV_ALLOC_H
#define HOV_ALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Phase 1: initialize the shared pool.
 * Called lazily on first hov_alloc_data(), or explicitly. */
void hov_init_shared_pool(void);

/* Phase 2: freeze the shared region and set up per-rank arenas.
 * Call once, after all persistent allocations are complete.
 * After this, hov_alloc_data() routes to the per-rank arena. */
void hov_init_arenas(void);

/* Allocate physically contiguous memory.
 * Before hov_init_arenas(): allocates from the shared region (cross-process atomic).
 * After  hov_init_arenas(): allocates from the per-rank arena (process-local). */
void *hov_alloc_data(size_t size);

/* Free memory allocated from the per-rank arena.
 * Only valid after hov_init_arenas(). No-op for shared-region allocations. */
void hov_free_data(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* HOV_ALLOC_H */
