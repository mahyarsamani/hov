#ifndef DOT_INDIRECT_HH
#define DOT_INDIRECT_HH

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

#include "util.hh"

#if defined(USE_SVE)
#include <arm_sve.h>
#endif

#if defined(USE_HOV)
extern "C" {
#include "hov.h"
}
#endif

#if defined(USE_SVE)
__attribute__((noinline)) double dot_indirect(const uint64_t *idx_a, const uint64_t *idx_b,
                                              const double *data_a, const double *data_b, int n)
{
    svfloat64_t acc = svdup_f64(0.0);

    for (int i = 0; i < n; i += svcntd())
    {
        svbool_t pg = svwhilelt_b64(i, n);
        svuint64_t vidx_a = svld1_u64(pg, &idx_a[i]);
        svuint64_t vidx_b = svld1_u64(pg, &idx_b[i]);
        
        svfloat64_t va = svld1_gather_u64index_f64(pg, data_a, vidx_a);
        svfloat64_t vb = svld1_gather_u64index_f64(pg, data_b, vidx_b);
        
        svfloat64_t vprod = svmul_f64_m(pg, va, vb);
        acc = svadd_f64_m(pg, acc, vprod);
    }
    return svaddv_f64(svptrue_b64(), acc);
}
#elif defined(USE_HOV)
__attribute__((noinline)) double dot_indirect(const uint64_t *idx_a, const uint64_t *idx_b,
                                              const double *data_a, const double *data_b, int n)
{
    hov_pair_t pair_a = hov_create_pair((void *)data_a, (void *)idx_a,
                                      sizeof(double), sizeof(uint64_t),
                                      (size_t)n);
    hov_pair_t pair_b = hov_create_pair((void *)data_b, (void *)idx_b,
                                      sizeof(double), sizeof(uint64_t),
                                      (size_t)n);
    double sum = 0.0;
    for (int i = 0; i < n; ++i)
    {
        hov_result_f64_u64_t res_a, res_b;
        hov_gather_f64_u64(&res_a, &pair_a, (size_t)i);
        hov_gather_f64_u64(&res_b, &pair_b, (size_t)i);
        sum += res_a.data_val * res_b.data_val;
    }
    hov_destroy_pair(&pair_a);
    hov_destroy_pair(&pair_b);
    return sum;
}
#else
__attribute__((noinline)) double dot_indirect(const uint64_t *idx_a, const uint64_t *idx_b,
                                              const double *data_a, const double *data_b, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; ++i)
    {
        sum += data_a[idx_a[i]] * data_b[idx_b[i]];
    }
    return sum;
}
#endif

#endif // DOT_INDIRECT_HH
