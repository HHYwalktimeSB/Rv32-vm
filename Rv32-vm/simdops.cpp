#include "simdops.h"
#include <immintrin.h>

int chk_val_in_arr_simd(unsigned int val, unsigned int mask, unsigned int* arr, unsigned arr_sz)
{
    if (arr_sz % 8 != 0)return -2;
    unsigned int sb[8];
    unsigned int dsb[8];
    for (int i = 0; i < 8; ++i) {
        sb[i] = val & mask;
        dsb[i] = mask;
    }
    __m256i cmps =  _mm256_loadu_epi32(sb);
    __m256i packedsb = _mm256_loadu_epi32(dsb);
    for (unsigned i = 0; i < arr_sz; ++i) {
        auto b = _mm256_load_si256((__m256i*)(void*)(arr + i));
        b = _mm256_and_si256(b, packedsb);
        auto res = _mm256_cmpeq_epi32(cmps, 
        b); 
    }
    return 0;
}
