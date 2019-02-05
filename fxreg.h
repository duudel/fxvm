#ifndef FXVM_REG

//#define USE_SSE

#ifdef USE_SSE
#include <intrin.h>
#endif

struct Reg
{
    union
    {
        float v[4];
#ifdef USE_SSE
        __m128 v4;
#endif
    };
};

#include <cstdint>

struct pcg32_random_t
{
    uint64_t state;
    uint64_t inc;
};

#ifdef FXVM_IMPL

#define PCG32_INITIALIZER { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }

uint32_t pcg32_random_r(pcg32_random_t* rng);

void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate, uint64_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint32_t pcg32_random_fast_r(pcg32_random_t *rng)
{
    uint64_t x = rng->state;
    unsigned count = (unsigned)(x >> 61);	// 61 = 64 - 3

    rng->state = x * 0xf13283ad;
    x ^= x >> 22;
    return (uint32_t)(x >> (22 + count));	// 22 = 32 - 3 - 7
}

inline float random01_float(pcg32_random_t *rng)
{
    return (float)pcg32_random_fast_r(rng) / (1ull << 32);
}

#include <cmath>
#include "fast_math.h"


#ifndef USE_SSE
// !USE_SSE

inline Reg reg_random01(pcg32_random_t *rng)
{
    float x = random01_float(rng);
    float y = 0.0f; //random01_float(rng);
    float z = 0.0f; //random01_float(rng);
    float w = 0.0f; //random01_float(rng);
    return { x, y, z, w };
}

inline Reg reg_load(const uint8_t *p)
{
    float *v = (float*)p;
    return { v[0], v[1], v[2], v[3] };
}

inline Reg reg_swizzle(Reg a, uint8_t mask)
{
    uint8_t i0 = mask & 0x3;
    uint8_t i1 = (mask >> 2) & 0x3;
    uint8_t i2 = (mask >> 4) & 0x3;
    uint8_t i3 = (mask >> 6) & 0x3;
    return { a.v[i0], a.v[i1], a.v[i2], a.v[i3] };
}

inline Reg reg_mov_x(Reg x)
{ return { x.v[0], 0.0f, 0.0f, 0.0f }; }

inline Reg reg_mov_xy(Reg x, Reg y)
{ return { x.v[0], y.v[0], 0.0f, 0.0f }; }

inline Reg reg_mov_xyz(Reg x, Reg y, Reg z)
{ return { x.v[0], y.v[0], z.v[0], 0.0f }; }

inline Reg reg_mov_xyzw(Reg x, Reg y, Reg z, Reg w)
{ return { x.v[0], y.v[0], z.v[0], w.v[0] }; }

inline Reg reg_neg(Reg a)
{ return { -a.v[0], -a.v[1], -a.v[2], -a.v[3] }; }

inline Reg reg_add(Reg a, Reg b)
{ return { a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2], a.v[3] + b.v[3] }; }

inline Reg reg_sub(Reg a, Reg b)
{ return { a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2], a.v[3] - b.v[3] }; }

inline Reg reg_mul(Reg a, Reg b)
{ return { a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2], a.v[3] * b.v[3] }; }

inline Reg reg_mul_by_scalar(Reg a, Reg b)
{ return { a.v[0] * b.v[0], a.v[1] * b.v[0], a.v[2] * b.v[0], a.v[3] * b.v[0] }; }

inline Reg reg_div(Reg a, Reg b)
{ return { a.v[0] / b.v[0], a.v[1] / b.v[1], a.v[2] / b.v[2], a.v[3] / b.v[3] }; }

inline Reg reg_div_by_scalar(Reg a, Reg b)
{ return { a.v[0] / b.v[0], a.v[1] / b.v[0], a.v[2] / b.v[0], a.v[3] / b.v[0] }; }

inline Reg reg_rcp(Reg a)
{ return { 1.0f/a.v[0], 1.0f/a.v[1], 1.0f/a.v[2], 1.0f/a.v[3] }; }

inline Reg reg_rsqrt(Reg a)
{ return { 1.0f/sqrtf(a.v[0]), 1.0f/sqrtf(a.v[1]), 1.0f/sqrtf(a.v[2]), 1.0f/sqrtf(a.v[3]) }; }

inline Reg reg_sqrt(Reg a)
{ return { sqrtf(a.v[0]), sqrtf(a.v[1]), sqrtf(a.v[2]), sqrtf(a.v[3]) }; }

inline Reg reg_sin(Reg a)
{ return { sinf(a.v[0]), sinf(a.v[1]), sinf(a.v[2]), sinf(a.v[3]) }; }

inline Reg reg_cos(Reg a)
{ return { cosf(a.v[0]), cosf(a.v[1]), cosf(a.v[2]), cosf(a.v[3]) }; }

//inline Reg reg_sin(Reg a)
//{ return { fastest_sin_s(a.v[0]), fastest_sin_s(a.v[1]), fastest_sin_s(a.v[2]), fastest_sin_s(a.v[3]) }; }
//
//inline Reg reg_cos(Reg a)
//{ return { fastest_cos_s(a.v[0]), fastest_cos_s(a.v[1]), fastest_cos_s(a.v[2]), fastest_cos_s(a.v[3]) }; }

inline Reg reg_exp(Reg a)
{ return { expf(a.v[0]), expf(a.v[1]), expf(a.v[2]), expf(a.v[3]) }; }

inline Reg reg_exp2(Reg a)
{ return { exp2f(a.v[0]), exp2f(a.v[1]), exp2f(a.v[2]), exp2f(a.v[3]) }; }

inline float exp10f(float v) { return expf(v) / expf(10.0f); }

inline Reg reg_exp10(Reg a)
{ return { exp10f(a.v[0]), exp10f(a.v[1]), exp10f(a.v[2]), exp10f(a.v[3]) }; }

inline Reg reg_trunc(Reg a)
{ return { truncf(a.v[0]), truncf(a.v[1]), truncf(a.v[2]), truncf(a.v[3]) }; }

inline float fract(float x)
{ return x - truncf(x); }

inline Reg reg_fract(Reg a)
{ return { fract(a.v[0]), fract(a.v[1]), fract(a.v[2]), fract(a.v[3]) }; }

inline Reg reg_abs(Reg a)
{ return { fabsf(a.v[0]), fabsf(a.v[1]), fabsf(a.v[2]), fabsf(a.v[3]) }; }

inline Reg reg_min(Reg a, Reg b)
{ return { fminf(a.v[0], b.v[0]), fminf(a.v[1], b.v[1]), fminf(a.v[2], b.v[2]), fminf(a.v[3], b.v[3]) }; }

inline Reg reg_max(Reg a, Reg b)
{ return { fmaxf(a.v[0], b.v[0]), fmaxf(a.v[1], b.v[1]), fmaxf(a.v[2], b.v[2]), fmaxf(a.v[3], b.v[3]) }; }

inline float reg_dot1(Reg a, Reg b)
{ return a.v[0] * b.v[0]; }

inline float reg_dot2(Reg a, Reg b)
{ return a.v[0] * b.v[0] + a.v[1] * b.v[1]; }

inline float reg_dot3(Reg a, Reg b)
{ return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2]; }

inline float reg_dot4(Reg a, Reg b)
{ return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2] + a.v[3] * b.v[3]; }

inline Reg reg_normalize1(Reg a)
{ return { 1.0f, a.v[1], a.v[2], a.v[3] }; }

inline Reg reg_normalize2(Reg a)
{
    float len2 = reg_dot2(a, a);
    float k = 1.0f / sqrt(len2);
    return { a.v[0] * k, a.v[1] * k, a.v[2], a.v[3] };
}

inline Reg reg_normalize3(Reg a)
{
    float len2 = reg_dot3(a, a);
    float k = 1.0f / sqrt(len2);
    return { a.v[0] * k, a.v[1] * k, a.v[2] * k, a.v[3] };
}

inline Reg reg_normalize4(Reg a)
{
    float len2 = reg_dot2(a, a);
    float k = 1.0f / sqrt(len2);
    return { a.v[0] * k, a.v[1] * k, a.v[2] * k, a.v[3] * k };
}

inline Reg reg_clamp01(Reg a)
{ return { fminf(fmaxf(a.v[0], 0.0f), 1.0f), fminf(fmaxf(a.v[1], 0.0f), 1.0f), fminf(fmaxf(a.v[2], 0.0f), 1.0f), fminf(fmaxf(a.v[3], 0.0f), 1.0f) }; }

inline Reg reg_clamp(Reg x, Reg a, Reg b)
{ return { fminf(fmaxf(x.v[0], a.v[0]), b.v[0]), fminf(fmaxf(x.v[1], a.v[1]), b.v[1]), fminf(fmaxf(x.v[2], a.v[2]), b.v[2]), fminf(fmaxf(x.v[3], a.v[3]), b.v[3]) }; }

inline float interp(float a, float b, float t) { return a * (1.0f - t) + b * t; }

inline Reg reg_interp(Reg a, Reg b, Reg t)
{ return { interp(a.v[0], b.v[0], t.v[0]), interp(a.v[1], b.v[1], t.v[1]), interp(a.v[2], b.v[2], t.v[2]), interp(a.v[3], b.v[3], t.v[3]) }; }

inline Reg reg_interp_by_scalar(Reg a, Reg b, Reg t)
{ return { interp(a.v[0], b.v[0], t.v[0]), interp(a.v[1], b.v[1], t.v[0]), interp(a.v[2], b.v[2], t.v[0]), interp(a.v[3], b.v[3], t.v[0]) }; }

#else
// USE_SSE

#include <xmmintrin.h>

inline Reg reg_random01(pcg32_random_t *rng)
{
    float x = random01_float(rng);
    float y = 0.0f; //random01_float(rng);
    float z = 0.0f; //random01_float(rng);
    float w = 0.0f; //random01_float(rng);
    return { x, y, z, w };
}

inline Reg reg_load(const uint8_t *p)
{ return Reg{ .v4 = _mm_loadu_ps((const float*)p) }; }

inline Reg reg_swizzle(Reg a, uint8_t mask)
{
    //uint8_t i0 = mask & 0x3;
    //uint8_t i1 = (mask >> 2) & 0x3;
    //uint8_t i2 = (mask >> 4) & 0x3;
    //uint8_t i3 = (mask >> 6) & 0x3;
    //return { a.v[i0], a.v[i1], a.v[i2], a.v[i3] };
#define SHUFFLES(x) \
    case 0x00|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x00|x) }; \
    case 0x01|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x01|x) }; \
    case 0x02|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x02|x) }; \
    case 0x03|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x03|x) }; \
    case 0x04|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x04|x) }; \
    case 0x05|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x05|x) }; \
    case 0x06|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x06|x) }; \
    case 0x07|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x07|x) }; \
    case 0x08|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x08|x) }; \
    case 0x09|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x09|x) }; \
    case 0x0a|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x0a|x) }; \
    case 0x0b|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x0b|x) }; \
    case 0x0c|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x0c|x) }; \
    case 0x0d|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x0d|x) }; \
    case 0x0e|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x0e|x) }; \
    case 0x0f|x: return Reg{ .v4 = _mm_shuffle_ps(a.v4, a.v4, 0x0f|x) }; \

    switch (mask)
    {
        SHUFFLES(0x00)
        SHUFFLES(0x10)
        SHUFFLES(0x20)
        SHUFFLES(0x30)
        SHUFFLES(0x40)
        SHUFFLES(0x50)
        SHUFFLES(0x60)
        SHUFFLES(0x70)
        SHUFFLES(0x80)
        SHUFFLES(0x90)
        SHUFFLES(0xa0)
        SHUFFLES(0xb0)
        SHUFFLES(0xc0)
        SHUFFLES(0xd0)
        SHUFFLES(0xe0)
        SHUFFLES(0xf0)
    }
    // invalid swizzle
    return a;
}

inline Reg reg_mov_x(Reg x)
{ return x; }

inline Reg reg_mov_xy(Reg x, Reg y)
{ return { x.v[0], y.v[0], 0.0f, 0.0f }; }

inline Reg reg_mov_xyz(Reg x, Reg y, Reg z)
{ return { x.v[0], y.v[0], z.v[0], 0.0f }; }

inline Reg reg_mov_xyzw(Reg x, Reg y, Reg z, Reg w)
{ return { x.v[0], y.v[0], z.v[0], w.v[0] }; }

inline Reg reg_neg(Reg a)
{ return Reg{ .v4 = _mm_xor_ps(a.v4, _mm_set1_ps(-0.0f)) }; }

inline Reg reg_add(Reg a, Reg b)
{ return Reg{ .v4 = _mm_add_ps(a.v4, b.v4) }; }

inline Reg reg_sub(Reg a, Reg b)
{ return Reg{ .v4 = _mm_sub_ps(a.v4, b.v4) }; }

inline Reg reg_mul(Reg a, Reg b)
{ return Reg{ .v4 = _mm_mul_ps(a.v4, b.v4) }; }

inline Reg reg_mul_by_scalar(Reg a, Reg b)
{ return Reg{ .v4 = _mm_mul_ps(a.v4, _mm_shuffle_ps(b.v4, b.v4, 0)) }; }

inline Reg reg_div(Reg a, Reg b)
{ return Reg{ .v4 = _mm_div_ps(a.v4, b.v4) }; }

inline Reg reg_div_by_scalar(Reg a, Reg b)
{ return Reg{ .v4 = _mm_div_ps(a.v4, _mm_shuffle_ps(b.v4, b.v4, 0)) }; }

inline Reg reg_rcp(Reg a)
{ return Reg{ .v4 = _mm_rcp_ps(a.v4) }; }

inline Reg reg_rsqrt(Reg a)
{ return Reg{ .v4 = _mm_rsqrt_ps(a.v4) }; }

inline Reg reg_sqrt(Reg a)
{ return Reg{ .v4 = _mm_sqrt_ps(a.v4) }; }

inline Reg reg_sin(Reg a)
//{ return Reg{ .v4 = sin_ps(a.v4) }; }
{ return Reg{ .v4 = fastest_sin_v4(a.v4) }; }

inline Reg reg_cos(Reg a)
//{ return Reg{ .v4 = cos_ps(a.v4) }; }
{ return Reg{ .v4 = fastest_cos_v4(a.v4) }; }

inline Reg reg_exp(Reg a)
{ return { expf(a.v[0]), expf(a.v[1]), expf(a.v[2]), expf(a.v[3]) }; }

inline Reg reg_exp2(Reg a)
{ return { exp2f(a.v[0]), exp2f(a.v[1]), exp2f(a.v[2]), exp2f(a.v[3]) }; }

inline float exp10f(float v) { return expf(v) / expf(10.0f); }

inline Reg reg_exp10(Reg a)
{ return { exp10f(a.v[0]), exp10f(a.v[1]), exp10f(a.v[2]), exp10f(a.v[3]) }; }

inline Reg reg_trunc(Reg a)
{
    __m128i i4 = _mm_cvttps_epi32(a.v4);
    return Reg{ .v4 = _mm_cvtepi32_ps(i4) };
}

inline Reg reg_fract(Reg a)
{
    __m128i i4 = _mm_cvttps_epi32(a.v4);
    return Reg{ .v4 = _mm_sub_ps(a.v4, _mm_cvtepi32_ps(i4)) };
}

inline Reg reg_abs(Reg a)
{ return Reg{ .v4 = _mm_and_ps(a.v4, (__m128)_mm_set1_epi32(0x7fffffff)) }; }

inline Reg reg_min(Reg a, Reg b)
{ return Reg{ .v4 = _mm_min_ps(a.v4, b.v4) }; }

inline Reg reg_max(Reg a, Reg b)
{ return Reg{ .v4 = _mm_max_ps(a.v4, b.v4) }; }

inline float reg_dot1(Reg a, Reg b)
{ return a.v[0] * b.v[0]; }

inline float reg_dot2(Reg a, Reg b)
{ return a.v[0] * b.v[0] + a.v[1] * b.v[1]; }

inline float reg_dot3(Reg a, Reg b)
{
    Reg r = Reg{ .v4 = _mm_mul_ps(a.v4, b.v4) };
    return r.v[0] + r.v[1] + r.v[2];
}

inline float reg_dot4(Reg a, Reg b)
{
    Reg r = Reg{ .v4 = _mm_mul_ps(a.v4, b.v4) };
    return r.v[0] + r.v[1] + r.v[2] + r.v[3];
}

inline Reg reg_normalize1(Reg a)
{ return { 1.0f, a.v[1], a.v[2], a.v[3] }; }

inline Reg reg_normalize2(Reg a)
{
    float len2 = reg_dot2(a, a);
    __m128 k = _mm_rsqrt_ps(_mm_set1_ps(len2));
    return Reg{ .v4 = _mm_mul_ps(a.v4, k) };
}

inline Reg reg_normalize3(Reg a)
{
    float len2 = reg_dot3(a, a);
    __m128 k = _mm_rsqrt_ps(_mm_set1_ps(len2));
    return Reg{ .v4 = _mm_mul_ps(a.v4, k) };
}

inline Reg reg_normalize4(Reg a)
{
    float len2 = reg_dot2(a, a);
    __m128 k = _mm_rsqrt_ps(_mm_set1_ps(len2));
    return Reg{ .v4 = _mm_mul_ps(a.v4, k) };
}

inline Reg reg_clamp01(Reg a)
{
    __m128 zero = _mm_setzero_ps();
    __m128 one = _mm_set1_ps(1.0f);
    return Reg{ .v4 = _mm_min_ps(_mm_max_ps(a.v4, zero), one) };
}

inline Reg reg_clamp(Reg x, Reg a, Reg b)
{
    return Reg{ .v4 = _mm_min_ps(_mm_max_ps(x.v4, a.v4), b.v4) };
}

inline Reg reg_interp(Reg a, Reg b, Reg t)
{
    __m128 one_minus_t = _mm_sub_ps(_mm_set1_ps(1.0f), t.v4);
    return Reg{ .v4 = _mm_add_ps(_mm_mul_ps(a.v4, one_minus_t), _mm_mul_ps(b.v4, t.v4)) };
}

inline Reg reg_interp_by_scalar(Reg a, Reg b, Reg t)
{
    __m128 tt = _mm_shuffle_ps(t.v4, t.v4, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 one_minus_t = _mm_sub_ps(_mm_set1_ps(1.0f), tt);
    return Reg{ .v4 = _mm_add_ps(_mm_mul_ps(a.v4, one_minus_t), _mm_mul_ps(b.v4, tt)) };
}
#endif

#endif

#define FXVM_REG
#endif

