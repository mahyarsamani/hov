#ifndef HOV_H
#define HOV_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------- */

/**
 * Indirect access pair descriptor.
 *
 * Represents the "streaming_data" abstraction:
 *   streaming_data[i] := data_base[index_base[i]]
 *
 * base_alias points to a virtual address region (no physical backing)
 * that provides unique aliases for each streaming element:
 *   alias(i) = base_alias + i * data_elem_size
 */
typedef struct {
    void   *base_alias;       /* mmap'd VA region for aliases          */
    void   *data_base;        /* base of data array                    */
    void   *index_base;       /* base of index array                   */
    size_t  data_elem_size;   /* sizeof(elem(data))                    */
    size_t  index_elem_size;  /* sizeof(elem(index))                   */
    size_t  count;            /* number of index elements              */
} hov_pair_t;

#define HOV_DECLARE_RESULT_STRUCT(name, type_data, type_index) \
    typedef struct { \
        type_data data_val; \
        type_index index_val; \
    } name;

/* 64-bit float data */
HOV_DECLARE_RESULT_STRUCT(hov_result_f64_u64_t, double, uint64_t)
HOV_DECLARE_RESULT_STRUCT(hov_result_f64_u32_t, double, uint32_t)

/* 64-bit integer data */
HOV_DECLARE_RESULT_STRUCT(hov_result_u64_u64_t, uint64_t, uint64_t)
HOV_DECLARE_RESULT_STRUCT(hov_result_u64_u32_t, uint64_t, uint32_t)

/* 32-bit float data */
HOV_DECLARE_RESULT_STRUCT(hov_result_f32_u64_t, float, uint64_t)
HOV_DECLARE_RESULT_STRUCT(hov_result_f32_u32_t, float, uint32_t)

/* 32-bit integer data */
HOV_DECLARE_RESULT_STRUCT(hov_result_u32_u64_t, uint32_t, uint64_t)
HOV_DECLARE_RESULT_STRUCT(hov_result_u32_u32_t, uint32_t, uint32_t)

/* 16-bit integer data */
HOV_DECLARE_RESULT_STRUCT(hov_result_u16_u64_t, uint16_t, uint64_t)
HOV_DECLARE_RESULT_STRUCT(hov_result_u16_u32_t, uint16_t, uint32_t)

/* 8-bit integer data */
HOV_DECLARE_RESULT_STRUCT(hov_result_u8_u64_t, uint8_t, uint64_t)
HOV_DECLARE_RESULT_STRUCT(hov_result_u8_u32_t, uint8_t, uint32_t)

/* -----------------------------------------------------------------------
 * Pair lifecycle & Alias computation
 * -------------------------------------------------------------------- */

hov_pair_t hov_create_pair(void *data, void *index, size_t data_elem_size, size_t index_elem_size, size_t count);
void hov_destroy_pair(hov_pair_t *pair);

/* -----------------------------------------------------------------------
 * Memory Allocation (two-phase: shared + arena)
 * See hov_alloc.h for full API documentation.
 * -------------------------------------------------------------------- */
#include "hov_alloc.h"

static inline uint64_t hov_alias(const hov_pair_t *pair, size_t i) {
    return (uint64_t)pair->base_alias + i * pair->data_elem_size;
}

/* -----------------------------------------------------------------------
 * Low-level assembly primitives (internal)
 * -------------------------------------------------------------------- */
hov_result_u64_u64_t __hov_ldindx_x(void *dummy, uint64_t alias, const void *data, const uint64_t *index_base, uint64_t index);
hov_result_u64_u32_t __hov_ldindx_w(void *dummy, uint64_t alias, const void *data, const uint32_t *index_base, uint32_t index);
hov_result_u32_u64_t __hov_ldindw_x(void *dummy, uint64_t alias, const void *data, const uint64_t *index_base, uint64_t index);
hov_result_u32_u32_t __hov_ldindw_w(void *dummy, uint64_t alias, const void *data, const uint32_t *index_base, uint32_t index);
hov_result_u16_u64_t __hov_ldindh_x(void *dummy, uint64_t alias, const void *data, const uint64_t *index_base, uint64_t index);
hov_result_u16_u32_t __hov_ldindh_w(void *dummy, uint64_t alias, const void *data, const uint32_t *index_base, uint32_t index);
hov_result_u8_u64_t __hov_ldindb_x(void *dummy, uint64_t alias, const void *data, const uint64_t *index_base, uint64_t index);
hov_result_u8_u32_t __hov_ldindb_w(void *dummy, uint64_t alias, const void *data, const uint32_t *index_base, uint32_t index);

/* -----------------------------------------------------------------------
 * Low-level assembly primitives for Store (internal)
 * -------------------------------------------------------------------- */
void __hov_stindx_x(uint64_t data, uint64_t alias, void *data_base, const uint64_t *index_base, uint64_t index);
void __hov_stindx_w(uint64_t data, uint64_t alias, void *data_base, const uint32_t *index_base, uint32_t index);
void __hov_stindw_x(uint32_t data, uint64_t alias, void *data_base, const uint64_t *index_base, uint64_t index);
void __hov_stindw_w(uint32_t data, uint64_t alias, void *data_base, const uint32_t *index_base, uint32_t index);
void __hov_stindh_x(uint16_t data, uint64_t alias, void *data_base, const uint64_t *index_base, uint64_t index);
void __hov_stindh_w(uint16_t data, uint64_t alias, void *data_base, const uint32_t *index_base, uint32_t index);
void __hov_stindb_x(uint8_t data, uint64_t alias, void *data_base, const uint64_t *index_base, uint64_t index);
void __hov_stindb_w(uint8_t data, uint64_t alias, void *data_base, const uint32_t *index_base, uint32_t index);

/* -----------------------------------------------------------------------
 * User-facing gather operations
 * -------------------------------------------------------------------- */
static inline void hov_gather_f64_u64(hov_result_f64_u64_t *result, const hov_pair_t *pair, size_t i) {
    typedef hov_result_f64_u64_t (*func_t)(void*, uint64_t, const void*, const uint64_t*, uint64_t);
    *result = ((func_t)(void*)__hov_ldindx_x)(NULL, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}
static inline void hov_gather_f64_u32(hov_result_f64_u32_t *result, const hov_pair_t *pair, size_t i) {
    typedef hov_result_f64_u32_t (*func_t)(void*, uint64_t, const void*, const uint32_t*, uint32_t);
    *result = ((func_t)(void*)__hov_ldindx_w)(NULL, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}
static inline void hov_gather_u64_u64(hov_result_u64_u64_t *result, const hov_pair_t *pair, size_t i) {
    *result = __hov_ldindx_x(NULL, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}

static inline void hov_gather_u64_u32(hov_result_u64_u32_t *result, const hov_pair_t *pair, size_t i) {
    *result = __hov_ldindx_w(NULL, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}

static inline void hov_gather_f32_u64(hov_result_f32_u64_t *result, const hov_pair_t *pair, size_t i) {
    typedef hov_result_f32_u64_t (*func_t)(void*, uint64_t, const void*, const uint64_t*, uint64_t);
    *result = ((func_t)(void*)__hov_ldindw_x)(NULL, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}

static inline void hov_gather_f32_u32(hov_result_f32_u32_t *result, const hov_pair_t *pair, size_t i) {
    typedef hov_result_f32_u32_t (*func_t)(void*, uint64_t, const void*, const uint32_t*, uint32_t);
    *result = ((func_t)(void*)__hov_ldindw_w)(NULL, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}

static inline void hov_gather_u32_u64(hov_result_u32_u64_t *result, const hov_pair_t *pair, size_t i) {
    *result = __hov_ldindw_x(NULL, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}

static inline void hov_gather_u32_u32(hov_result_u32_u32_t *result, const hov_pair_t *pair, size_t i) {
    *result = __hov_ldindw_w(NULL, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}

static inline void hov_gather_u16_u64(hov_result_u16_u64_t *result, const hov_pair_t *pair, size_t i) {
    *result = __hov_ldindh_x(NULL, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}

static inline void hov_gather_u16_u32(hov_result_u16_u32_t *result, const hov_pair_t *pair, size_t i) {
    *result = __hov_ldindh_w(NULL, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}

static inline void hov_gather_u8_u64(hov_result_u8_u64_t *result, const hov_pair_t *pair, size_t i) {
    *result = __hov_ldindb_x(NULL, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}

static inline void hov_gather_u8_u32(hov_result_u8_u32_t *result, const hov_pair_t *pair, size_t i) {
    *result = __hov_ldindb_w(NULL, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}

/* -----------------------------------------------------------------------
 * User-facing scatter operations
 * -------------------------------------------------------------------- */
static inline void hov_scatter_f64_u64(double data, const hov_pair_t *pair, size_t i) {
    uint64_t udata;
    __builtin_memcpy(&udata, &data, sizeof(double));
    __hov_stindx_x(udata, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}
static inline void hov_scatter_f64_u32(double data, const hov_pair_t *pair, size_t i) {
    uint64_t udata;
    __builtin_memcpy(&udata, &data, sizeof(double));
    __hov_stindx_w(udata, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}
static inline void hov_scatter_u64_u64(uint64_t data, const hov_pair_t *pair, size_t i) {
    __hov_stindx_x(data, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}
static inline void hov_scatter_u64_u32(uint64_t data, const hov_pair_t *pair, size_t i) {
    __hov_stindx_w(data, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}

static inline void hov_scatter_f32_u64(float data, const hov_pair_t *pair, size_t i) {
    uint32_t udata;
    __builtin_memcpy(&udata, &data, sizeof(float));
    __hov_stindw_x(udata, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}
static inline void hov_scatter_f32_u32(float data, const hov_pair_t *pair, size_t i) {
    uint32_t udata;
    __builtin_memcpy(&udata, &data, sizeof(float));
    __hov_stindw_w(udata, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}
static inline void hov_scatter_u32_u64(uint32_t data, const hov_pair_t *pair, size_t i) {
    __hov_stindw_x(data, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}
static inline void hov_scatter_u32_u32(uint32_t data, const hov_pair_t *pair, size_t i) {
    __hov_stindw_w(data, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}

static inline void hov_scatter_u16_u64(uint16_t data, const hov_pair_t *pair, size_t i) {
    __hov_stindh_x(data, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}
static inline void hov_scatter_u16_u32(uint16_t data, const hov_pair_t *pair, size_t i) {
    __hov_stindh_w(data, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}

static inline void hov_scatter_u8_u64(uint8_t data, const hov_pair_t *pair, size_t i) {
    __hov_stindb_x(data, hov_alias(pair, i), pair->data_base, (const uint64_t *)pair->index_base, (uint64_t)i);
}
static inline void hov_scatter_u8_u32(uint8_t data, const hov_pair_t *pair, size_t i) {
    __hov_stindb_w(data, hov_alias(pair, i), pair->data_base, (const uint32_t *)pair->index_base, (uint32_t)i);
}

#ifdef __cplusplus
}
#endif

#endif /* HOV_H */
