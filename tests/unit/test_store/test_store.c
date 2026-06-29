#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "hov.h"

#define DEFINE_TEST_SCATTER(name, scatter_func, data_type, index_type, format_data) \
void test_##name() { \
    printf("--- Testing %s ---\n", #name); \
    data_type data_array[4] = {0, 0, 0, 0}; \
    index_type index_array[4] = {3, 2, 0, 1}; \
    hov_pair_t pair = hov_create_pair(data_array, index_array, sizeof(data_type), sizeof(index_type), 4); \
    \
    /* Perform indirect stores */ \
    scatter_func((data_type)10, &pair, 0); /* data_array[3] = 10 */ \
    scatter_func((data_type)20, &pair, 1); /* data_array[2] = 20 */ \
    scatter_func((data_type)30, &pair, 2); /* data_array[0] = 30 */ \
    scatter_func((data_type)40, &pair, 3); /* data_array[1] = 40 */ \
    \
    /* Expected values in data_array */ \
    data_type expected[4] = {30, 40, 20, 10}; \
    int pass = 1; \
    \
    for (size_t j = 0; j < 4; j++) { \
        if (data_array[j] != expected[j]) { \
            printf("  [FAIL] data_array[%zu]: Expected %" format_data " | Got %" format_data "\n", \
                   j, expected[j], data_array[j]); \
            pass = 0; \
        } \
    } \
    \
    if (pass) printf("  [PASS]\n"); \
    hov_destroy_pair(&pair); \
}

DEFINE_TEST_SCATTER(f64_u64, hov_scatter_f64_u64, double, uint64_t, "f")
DEFINE_TEST_SCATTER(f64_u32, hov_scatter_f64_u32, double, uint32_t, "f")

DEFINE_TEST_SCATTER(f32_u64, hov_scatter_f32_u64, float, uint64_t, "f")
DEFINE_TEST_SCATTER(f32_u32, hov_scatter_f32_u32, float, uint32_t, "f")

DEFINE_TEST_SCATTER(u64_u64, hov_scatter_u64_u64, uint64_t, uint64_t, PRIu64)
DEFINE_TEST_SCATTER(u64_u32, hov_scatter_u64_u32, uint64_t, uint32_t, PRIu64)

DEFINE_TEST_SCATTER(u32_u64, hov_scatter_u32_u64, uint32_t, uint64_t, PRIu32)
DEFINE_TEST_SCATTER(u32_u32, hov_scatter_u32_u32, uint32_t, uint32_t, PRIu32)

DEFINE_TEST_SCATTER(u16_u64, hov_scatter_u16_u64, uint16_t, uint64_t, PRIu16)
DEFINE_TEST_SCATTER(u16_u32, hov_scatter_u16_u32, uint16_t, uint32_t, PRIu16)

DEFINE_TEST_SCATTER(u8_u64, hov_scatter_u8_u64, uint8_t, uint64_t, PRIu8)
DEFINE_TEST_SCATTER(u8_u32, hov_scatter_u8_u32, uint8_t, uint32_t, PRIu8)

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
