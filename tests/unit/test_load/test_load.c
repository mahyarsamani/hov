#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "hov.h"

#define DEFINE_TEST_GATHER(name, gather_func, result_type, data_type, index_type, format_data, format_index) \
void test_##name() { \
    printf("--- Testing %s ---\n", #name); \
    data_type data_array[4] = {10, 20, 30, 40}; \
    index_type index_array[4] = {3, 2, 0, 1}; \
    hov_pair_t pair = hov_create_pair(data_array, index_array, sizeof(data_type), sizeof(index_type), 4); \
    int pass = 1; \
    for (size_t i = 0; i < 4; i++) { \
        result_type res; \
        gather_func(&res, &pair, i); \
        if (res.data_val != data_array[index_array[i]] || res.index_val != index_array[i]) { \
            printf("  [FAIL] Iteration %zu: Expected data=%" format_data " index=%" format_index \
                   " | Got data=%" format_data " index=%" format_index "\n", \
                   i, data_array[index_array[i]], index_array[i], res.data_val, res.index_val); \
            pass = 0; \
        } \
    } \
    if (pass) printf("  [PASS]\n"); \
    hov_destroy_pair(&pair); \
}

DEFINE_TEST_GATHER(f64_u64, hov_gather_f64_u64, hov_result_f64_u64_t, double, uint64_t, "f", PRIu64)
DEFINE_TEST_GATHER(f64_u32, hov_gather_f64_u32, hov_result_f64_u32_t, double, uint32_t, "f", PRIu32)

DEFINE_TEST_GATHER(f32_u64, hov_gather_f32_u64, hov_result_f32_u64_t, float, uint64_t, "f", PRIu64)
DEFINE_TEST_GATHER(f32_u32, hov_gather_f32_u32, hov_result_f32_u32_t, float, uint32_t, "f", PRIu32)

DEFINE_TEST_GATHER(u64_u64, hov_gather_u64_u64, hov_result_u64_u64_t, uint64_t, uint64_t, PRIu64, PRIu64)
DEFINE_TEST_GATHER(u64_u32, hov_gather_u64_u32, hov_result_u64_u32_t, uint64_t, uint32_t, PRIu64, PRIu32)

DEFINE_TEST_GATHER(u32_u64, hov_gather_u32_u64, hov_result_u32_u64_t, uint32_t, uint64_t, PRIu32, PRIu64)
DEFINE_TEST_GATHER(u32_u32, hov_gather_u32_u32, hov_result_u32_u32_t, uint32_t, uint32_t, PRIu32, PRIu32)

DEFINE_TEST_GATHER(u16_u64, hov_gather_u16_u64, hov_result_u16_u64_t, uint16_t, uint64_t, PRIu16, PRIu64)
DEFINE_TEST_GATHER(u16_u32, hov_gather_u16_u32, hov_result_u16_u32_t, uint16_t, uint32_t, PRIu16, PRIu32)

DEFINE_TEST_GATHER(u8_u64, hov_gather_u8_u64, hov_result_u8_u64_t, uint8_t, uint64_t, PRIu8, PRIu64)
DEFINE_TEST_GATHER(u8_u32, hov_gather_u8_u32, hov_result_u8_u32_t, uint8_t, uint32_t, PRIu8, PRIu32)

int main() {
    test_f64_u64();
    test_f64_u32();
    test_f32_u64();
    test_f32_u32();
    test_u64_u64();
    test_u64_u32();
    test_u32_u64();
    test_u32_u32();
    test_u16_u64();
    test_u16_u32();
    test_u8_u64();
    test_u8_u32();
    printf("All tests completed.\n");
    return 0;
}
