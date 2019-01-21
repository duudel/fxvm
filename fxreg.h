#ifndef FXVM_REG

#include <cstdint>
#include <cmath>

struct Reg
{
    float v[4];
};

inline Reg reg_load(const uint8_t *p)
{
    float *v = (float*)p;
    return { v[0], v[1], v[2], v[3] };
}

inline Reg reg_add(Reg a, Reg b)
{ return { a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2], a.v[3] + b.v[3] }; }

inline Reg reg_sub(Reg a, Reg b)
{ return { a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2], a.v[3] - b.v[3] }; }

inline Reg reg_mul(Reg a, Reg b)
{ return { a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2], a.v[3] * b.v[3] }; }

inline Reg reg_div(Reg a, Reg b)
{ return { a.v[0] / b.v[0], a.v[1] / b.v[1], a.v[2] / b.v[2], a.v[3] / b.v[3] }; }

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

inline Reg reg_exp(Reg a)
{ return { expf(a.v[0]), expf(a.v[1]), expf(a.v[2]), expf(a.v[3]) }; }

inline Reg reg_exp2(Reg a)
{ return { exp2f(a.v[0]), exp2f(a.v[1]), exp2f(a.v[2]), exp2f(a.v[3]) }; }

inline float exp10f(float v) { return expf(v) / expf(10.0f); }

inline Reg reg_exp10(Reg a)
{ return { exp10f(a.v[0]), exp10f(a.v[1]), exp10f(a.v[2]), exp10f(a.v[3]) }; }

inline Reg reg_abs(Reg a)
{ return { fabsf(a.v[0]), fabsf(a.v[1]), fabsf(a.v[2]), fabsf(a.v[3]) }; }

inline Reg reg_min(Reg a, Reg b)
{ return { fminf(a.v[0], b.v[0]), fminf(a.v[1], b.v[1]), fminf(a.v[2], b.v[2]), fminf(a.v[3], b.v[3]) }; }

inline Reg reg_max(Reg a, Reg b)
{ return { fmaxf(a.v[0], b.v[0]), fmaxf(a.v[1], b.v[1]), fmaxf(a.v[2], b.v[2]), fmaxf(a.v[3], b.v[3]) }; }

inline Reg reg_clamp01(Reg a)
{ return { fminf(fmaxf(a.v[0], 0.0f), 1.0f), fminf(fmaxf(a.v[1], 0.0f), 1.0f), fminf(fmaxf(a.v[2], 0.0f), 1.0f), fminf(fmaxf(a.v[3], 0.0f), 1.0f) }; }

inline Reg reg_clamp(Reg x, Reg a, Reg b)
{ return { fminf(fmaxf(x.v[0], a.v[0]), b.v[0]), fminf(fmaxf(x.v[1], a.v[1]), b.v[1]), fminf(fmaxf(x.v[2], a.v[2]), b.v[2]), fminf(fmaxf(x.v[3], a.v[3]), b.v[3]) }; }

inline float interp(float a, float b, float t) { return a * (1.0f - t) + b * t; }

inline Reg reg_interp(Reg a, Reg b, Reg t)
{ return { interp(a.v[0], b.v[0], t.v[0]), interp(a.v[1], b.v[1], t.v[1]), interp(a.v[2], b.v[2], t.v[2]), interp(a.v[3], b.v[3], t.v[3]) }; }

#define FXVM_REG
#endif

