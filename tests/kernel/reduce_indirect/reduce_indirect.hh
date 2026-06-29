#ifndef REDUCE_INDIRECT_HH
#define REDUCE_INDIRECT_HH

#include <cstdint>
#include <cassert>

#if defined(USE_SVE)
#include <arm_sve.h>
#endif

#if defined(USE_HOV)
extern "C" {
#include "hov.h"
}
#endif

#include "util.hh"

#if defined(USE_SVE)
__attribute__((noinline)) void reduce_indirect(const uint64_t *idx, const double *a, double *b, int n)
{
    assert(n % 2 == 0);
    int step = 2 * svcntd();
    for (int i = 0; i < n; i += step)
    {
        svbool_t pg = svwhilelt_b64(i, n);
        
        // Structured load of 2 interleaved vector registers.
        // vidx.v0 contains idx[i], idx[i+2], idx[i+4], ...
        // vidx.v1 contains idx[i+1], idx[i+3], idx[i+5], ...
        svuint64x2_t vidx = svld2_u64(pg, &idx[i]);
        
        svuint64_t vidx_even = svget2_u64(vidx, 0);
        svuint64_t vidx_odd  = svget2_u64(vidx, 1);
        
        // Gather values from 'a'
        svfloat64_t va_even = svld1_gather_u64index_f64(pg, a, vidx_even);
        svfloat64_t va_odd  = svld1_gather_u64index_f64(pg, a, vidx_odd);
        
        // Compute difference and sum
        svfloat64_t vdiff = svsub_f64_m(pg, va_odd, va_even);
        svfloat64_t vsum  = svadd_f64_m(pg, va_odd, va_even);
        
        // Scatter results to 'b'
        svst1_scatter_u64index_f64(pg, b, vidx_even, vdiff);
        svst1_scatter_u64index_f64(pg, b, vidx_odd, vsum);
    }
}
#elif defined(USE_HOV)
__attribute__((noinline)) void reduce_indirect(const uint64_t *idx, const double *a, double *b, int n)
{
    assert(n % 2 == 0);
    
    hov_pair_t pair_a = hov_create_pair((void *)a, (void *)idx,
                                      sizeof(double), sizeof(uint64_t),
                                      (size_t)n);
    hov_pair_t pair_b = hov_create_pair((void *)b, (void *)idx,
                                      sizeof(double), sizeof(uint64_t),
                                      (size_t)n);

    for (int i = 0; i < n; i += 2)
    {
        hov_result_f64_u64_t res_i, res_ip1;
        hov_gather_f64_u64(&res_ip1, &pair_a, (size_t)(i + 1));
        hov_gather_f64_u64(&res_i, &pair_a, (size_t)i);
        
        double diff = res_ip1.data_val - res_i.data_val;
        double sum  = res_ip1.data_val + res_i.data_val;
        
        hov_scatter_f64_u64(diff, &pair_b, (size_t)i);
        hov_scatter_f64_u64(sum, &pair_b, (size_t)(i + 1));
    }

    hov_destroy_pair(&pair_a);
    hov_destroy_pair(&pair_b);
}
#else
__attribute__((noinline)) void reduce_indirect(const uint64_t *idx, const double *a, double *b, int n)
{
    assert(n % 2 == 0);
    for (int i = 0; i < n; i += 2)
    {
        b[idx[i]] = a[idx[i+1]] - a[idx[i]];
        b[idx[i+1]] = a[idx[i+1]] + a[idx[i]];
    }
}
#endif

#endif // REDUCE_INDIRECT_HH
