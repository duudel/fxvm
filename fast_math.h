
#include <cstdint>
#include <cmath>

#include <xmmintrin.h>

float fastest_expf(float x)
{
    union {
        float f;
        uint32_t u;
    };
    f = x;
    //if (x >= -1.0f && x <= 1.0f)
    //if (!u || ((u & 0x3f80000) == 0x3f80000))
    //    return 1.0f + x + x*x*0.5f + x*x*x*0.16666f; // + x*x*x*x*0.041666f + x*x*x*x*x*0.008333333f;

    //if (x > -2.2f && x < 2.2f)
    //    return 1.0f + x + x*x*0.5f + x*x*x*0.16666f + x*x*x*x*0.041666f + x*x*x*x*x*0.008333333f;

    if (x < -2000.0f) return 0.0f;
    //x = 1.0000406f + x / 1024.0f;
    //x = 1.00000246f + x / 4095.0f;
    x = 1.0f + x / 4092.5f;
    //x = 1.0f + x / 1024.0f;
    x *= x; x *= x; x *= x; x *= x;
    x *= x; x *= x; x *= x; x *= x;
    x *= x; x *= x; x *= x; x *= x;
    return x;
    //if (x > -1.02f)
    //{
    //    return 1.0f + x + x*x*0.5f + x*x*x*0.16666f; //+ x*x*x*x*0.041666f;
    //}
    //if (x > -2.53f)
    //{
    //    return 0.52f + x*0.2f;
    //}
    //return 0.0f;
}

#ifdef USE_SSE

static const uint32_t _ps_sign_mask[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
static const uint32_t _ps_inv_sign_mask[4] = { ~0x80000000, ~0x80000000, ~0x80000000, ~0x80000000 };
static const float _ps_1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const float _ps_half[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
static const float _ps_4_over_pi[4] = { 1.27323954473516f, 1.27323954473516f, 1.27323954473516f, 1.27323954473516f };
static const uint32_t _pi32_1[4] = { 1, 1, 1, 1 };
static const int32_t _pi32_inv1[4] = { ~1, ~1, ~1, ~1 };
static const int32_t _pi32_2[4] = { 2, 2, 2, 2 };
static const int32_t _pi32_4[4] = { 4, 4, 4, 4 };
static const float _ps_minus_cephes_DP1[4] = { -0.78515625f, -0.78515625f, -0.78515625f, -0.78515625f };
static const float _ps_minus_cephes_DP2[4] = { -2.4187564849853515625e-4f, -2.4187564849853515625e-4f, -2.4187564849853515625e-4f, -2.4187564849853515625e-4f };
static const float _ps_minus_cephes_DP3[4] = { -3.77489497744594108e-8f, -3.77489497744594108e-8f, -3.77489497744594108e-8f, -3.77489497744594108e-8f };
static const float _ps_sincof_p0[4] = { -1.9515295891E-4f, -1.9515295891E-4f, -1.9515295891E-4f, -1.9515295891E-4f };
static const float _ps_sincof_p1[4] = { 8.3321608736E-3f, 8.3321608736E-3f, 8.3321608736E-3f, 8.3321608736E-3f };
static const float _ps_sincof_p2[4] = { -1.6666654611E-1f, -1.6666654611E-1f, -1.6666654611E-1f, -1.6666654611E-1f };
static const float _ps_coscof_p0[4] = { 2.443315711809948E-005f, 2.443315711809948E-005f, 2.443315711809948E-005f, 2.443315711809948E-005f };
static const float _ps_coscof_p1[4] = { -1.388731625493765E-003f, -1.388731625493765E-003f, -1.388731625493765E-003f, -1.388731625493765E-003f };
static const float _ps_coscof_p2[4] = { 4.166664568298827E-002f, 4.166664568298827E-002f, 4.166664568298827E-002f, 4.166664568298827E-002f };

__m128 sin_ps(__m128 x)
{
    __m128 xmm1, xmm2 = _mm_setzero_ps(), xmm3, sign_bit, y;

    __m128i emm0, emm2;
    sign_bit = x;
    /* take the absolute value */
    x = _mm_and_ps(x, *(__m128*)_ps_inv_sign_mask);
    /* extract the sign bit (upper one) */
    sign_bit = _mm_and_ps(sign_bit, *(__m128*)_ps_sign_mask);

    /* scale by 4/Pi */
    y = _mm_mul_ps(x, *(__m128*)_ps_4_over_pi);

    /* store the integer part of y in mm0 */
    emm2 = _mm_cvttps_epi32(y);
    /* j=(j+1) & (~1) (see the cephes sources) */
    emm2 = _mm_add_epi32(emm2, *(__m128i*)_pi32_1);
    emm2 = _mm_and_si128(emm2, *(__m128i*)_pi32_inv1);
    y = _mm_cvtepi32_ps(emm2);

    /* get the swap sign flag */
    emm0 = _mm_and_si128(emm2, *(__m128i*)_pi32_4);
    emm0 = _mm_slli_epi32(emm0, 29);
    /* get the polynom selection mask
        there is one polynom for 0 <= x <= Pi/4
        and another one for Pi/4<x<=Pi/2

        Both branches will be computed.
    */
    emm2 = _mm_and_si128(emm2, *(__m128i*)_pi32_2);
    emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());

    __m128 swap_sign_bit = _mm_castsi128_ps(emm0);
    __m128 poly_mask = _mm_castsi128_ps(emm2);
    sign_bit = _mm_xor_ps(sign_bit, swap_sign_bit);

    /* The magic pass: "Extended precision modular arithmetic"
        x = ((x - y * DP1) - y * DP2) - y * DP3; */
    xmm1 = *(__m128*)_ps_minus_cephes_DP1;
    xmm2 = *(__m128*)_ps_minus_cephes_DP2;
    xmm3 = *(__m128*)_ps_minus_cephes_DP3;
    xmm1 = _mm_mul_ps(y, xmm1);
    xmm2 = _mm_mul_ps(y, xmm2);
    xmm3 = _mm_mul_ps(y, xmm3);
    x = _mm_add_ps(x, xmm1);
    x = _mm_add_ps(x, xmm2);
    x = _mm_add_ps(x, xmm3);

    /* Evaluate the first polynom  (0 <= x <= Pi/4) */
    y = *(__m128*)_ps_coscof_p0;
    __m128 z = _mm_mul_ps(x,x);

    y = _mm_mul_ps(y, z);
    y = _mm_add_ps(y, *(__m128*)_ps_coscof_p1);
    y = _mm_mul_ps(y, z);
    y = _mm_add_ps(y, *(__m128*)_ps_coscof_p2);
    y = _mm_mul_ps(y, z);
    y = _mm_mul_ps(y, z);
    __m128 tmp = _mm_mul_ps(z, *(__m128*)_ps_half);
    y = _mm_sub_ps(y, tmp);
    y = _mm_add_ps(y, *(__m128*)_ps_1);

    /* Evaluate the second polynom  (Pi/4 <= x <= 0) */

    __m128 y2 = *(__m128*)_ps_sincof_p0;
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_add_ps(y2, *(__m128*)_ps_sincof_p1);
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_add_ps(y2, *(__m128*)_ps_sincof_p2);
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_mul_ps(y2, x);
    y2 = _mm_add_ps(y2, x);

    /* select the correct result from the two polynoms */
    xmm3 = poly_mask;
    y2 = _mm_and_ps(xmm3, y2); //, xmm3);
    y = _mm_andnot_ps(xmm3, y);
    y = _mm_add_ps(y,y2);
    /* update the sign */
    y = _mm_xor_ps(y, sign_bit);
    return y;
}

__m128 cos_ps(__m128 x)
{
    __m128 xmm1, xmm2 = _mm_setzero_ps(), xmm3, y;
    __m128i emm0, emm2;
    /* take the absolute value */
    x = _mm_and_ps(x, *(__m128*)_ps_inv_sign_mask);

    /* scale by 4/Pi */
    y = _mm_mul_ps(x, *(__m128*)_ps_4_over_pi);

    /* store the integer part of y in mm0 */
    emm2 = _mm_cvttps_epi32(y);
    /* j=(j+1) & (~1) (see the cephes sources) */
    emm2 = _mm_add_epi32(emm2, *(__m128i*)_pi32_1);
    emm2 = _mm_and_si128(emm2, *(__m128i*)_pi32_inv1);
    y = _mm_cvtepi32_ps(emm2);

    emm2 = _mm_sub_epi32(emm2, *(__m128i*)_pi32_2);

    /* get the swap sign flag */
    emm0 = _mm_andnot_si128(emm2, *(__m128i*)_pi32_4);
    emm0 = _mm_slli_epi32(emm0, 29);
    /* get the polynom selection mask */
    emm2 = _mm_and_si128(emm2, *(__m128i*)_pi32_2);
    emm2 = _mm_cmpeq_epi32(emm2, _mm_setzero_si128());

    __m128 sign_bit = _mm_castsi128_ps(emm0);
    __m128 poly_mask = _mm_castsi128_ps(emm2);

    /* The magic pass: "Extended precision modular arithmetic"
        x = ((x - y * DP1) - y * DP2) - y * DP3; */
    xmm1 = *(__m128*)_ps_minus_cephes_DP1;
    xmm2 = *(__m128*)_ps_minus_cephes_DP2;
    xmm3 = *(__m128*)_ps_minus_cephes_DP3;
    xmm1 = _mm_mul_ps(y, xmm1);
    xmm2 = _mm_mul_ps(y, xmm2);
    xmm3 = _mm_mul_ps(y, xmm3);
    x = _mm_add_ps(x, xmm1);
    x = _mm_add_ps(x, xmm2);
    x = _mm_add_ps(x, xmm3);

    /* Evaluate the first polynom  (0 <= x <= Pi/4) */
    y = *(__m128*)_ps_coscof_p0;
    __m128 z = _mm_mul_ps(x,x);

    y = _mm_mul_ps(y, z);
    y = _mm_add_ps(y, *(__m128*)_ps_coscof_p1);
    y = _mm_mul_ps(y, z);
    y = _mm_add_ps(y, *(__m128*)_ps_coscof_p2);
    y = _mm_mul_ps(y, z);
    y = _mm_mul_ps(y, z);
    __m128 tmp = _mm_mul_ps(z, *(__m128*)_ps_half);
    y = _mm_sub_ps(y, tmp);
    y = _mm_add_ps(y, *(__m128*)_ps_1);

    /* Evaluate the second polynom  (Pi/4 <= x <= 0) */

    __m128 y2 = *(__m128*)_ps_sincof_p0;
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_add_ps(y2, *(__m128*)_ps_sincof_p1);
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_add_ps(y2, *(__m128*)_ps_sincof_p2);
    y2 = _mm_mul_ps(y2, z);
    y2 = _mm_mul_ps(y2, x);
    y2 = _mm_add_ps(y2, x);

    /* select the correct result from the two polynoms */
    xmm3 = poly_mask;
    y2 = _mm_and_ps(xmm3, y2); //, xmm3);
    y = _mm_andnot_ps(xmm3, y);
    y = _mm_add_ps(y,y2);
    /* update the sign */
    y = _mm_xor_ps(y, sign_bit);

    return y;
}

// USE_SSE
#endif


float fastest_sin_s(float x)
{
    const float c_2_over_pi  = 2.0f / 3.14159265358f;
    const float c_pi_over_2  = 3.14159265358f / 2.0f;
    const float c_1_over_6   = 0.1666666666f;
    const float c_1_over_120 = 0.0078740157f; // Adjusted value for increased accuracy
    //const float c_1_over_120 = 0.008333333f;

    bool negative = (x < 0.0f);

    x = fabsf(x);
    float i = truncf(x * c_2_over_pi);
    x -= i * c_pi_over_2;

    int ii = (int)i;
    bool b1 = (ii & 1) & 1;
    bool b2 = (ii & 2) & 2;

    if (b1) x = c_pi_over_2 - x;

    float y = x - x*x*x * c_1_over_6 + x*x*x*x*x * c_1_over_120;

    if (b2 ^ negative) return -y;
    return y;
}

float fastest_cos_s(float x)
{
    const float c_2_over_pi = 2.0f / 3.14159265358f;
    const float c_pi_over_2 = 3.14159265358f / 2.0f;
    const float c_1_over_2  = 0.5f;
    const float c_1_over_24 = 0.03846153846f; // Adjusted value for increased accuracy
    //const float c_1_over_24 = 0.04166666666f;

    x = fabsf(x);
    float i = truncf(x * c_2_over_pi);
    x -= i * c_pi_over_2;

    int ii = (int)i;
    bool b1 = (ii & 1) == 1;
    bool b2 = (ii & 2) == 2;

    if (b1) x = c_pi_over_2 - x;

    float y = 1.0f - x*x * c_1_over_2 + x*x*x*x * c_1_over_24;

    if (b1 ^ b2) return -y;
    return y;
}

#ifdef USE_SSE

__m128 fastest_sin_v4(__m128 x)
{
    const __m128 c_2_over_pi  = _mm_set1_ps(2.0f / 3.14159265358f);
    const __m128 c_pi_over_2  = _mm_set1_ps(3.14159265358f / 2.0f);
    const __m128 c_1_over_6   = _mm_set1_ps(0.1666666666f);
    const __m128 c_1_over_120 = _mm_set1_ps(0.0078740157f); // Adjusted value for increased accuracy
    //const __m128 c_1_over_120 = _mm_set1_ps(0.0083333333f);
    const __m128 signmask = (__m128)_mm_set1_epi32(0x80000000);
    const __m128 inv_signmask = (__m128)_mm_set1_epi32(0x7fffffff);
    const __m128i one_i = _mm_set1_epi32(1);
    const __m128i two_i = _mm_set1_epi32(2);

    __m128 negative = _mm_cmplt_ps(x, _mm_setzero_ps());
    x = _mm_and_ps(x, inv_signmask); // abs

    __m128 i = _mm_mul_ps(x, c_2_over_pi);
    __m128i ii = _mm_cvttps_epi32(i);
    i = _mm_cvtepi32_ps(ii);
    x = _mm_sub_ps(x, _mm_mul_ps(i, c_pi_over_2));

    __m128i b1 = _mm_and_si128(ii, one_i);
    __m128i b1_mask = _mm_cmpeq_epi32(b1, one_i);

    __m128 kx = _mm_sub_ps(c_pi_over_2, x);

    x = _mm_or_ps(_mm_and_ps((__m128)b1_mask, kx), _mm_andnot_ps((__m128)b1_mask, x));

    __m128 xx = _mm_mul_ps(x, x);
    __m128 xxx = _mm_mul_ps(x, xx);

    __m128 p0 = x;
    __m128 p1 = _mm_mul_ps(x, c_1_over_6);
    __m128 p2 = _mm_mul_ps(xxx, c_1_over_120);

    __m128 yy = _mm_mul_ps(_mm_sub_ps(p2, p1), xx);
    __m128 y = _mm_add_ps(p0, yy);

    __m128i b2 = _mm_and_si128(ii, two_i);
    __m128i b2_mask = _mm_cmpeq_epi32(b2, two_i);
    __m128 negate_mask = _mm_xor_ps(negative, (__m128)b2_mask);

    //y = _mm_xor_ps(y, _mm_and_ps(_mm_set1_ps(-0.0f), negate_mask));
    y = _mm_xor_ps(y, _mm_and_ps(signmask, negate_mask));
    return y;
}

__m128 fastest_cos_v4(__m128 x)
{
    const __m128 c_2_over_pi  = _mm_set1_ps(2.0f / 3.14159265358f);
    const __m128 c_pi_over_2  = _mm_set1_ps(3.14159265358f / 2.0f);
    const __m128 c_1_over_2   = _mm_set1_ps(0.5f);
    const __m128 c_1_over_24 = _mm_set1_ps(0.03846153846f); // Adjusted value for increased accuracy
    //const __m128 c_1_over_24 = _mm_set1_ps(0.04166666666f);
    const __m128 signmask = (__m128)_mm_set1_epi32(0x80000000);
    const __m128 inv_signmask = (__m128)_mm_set1_epi32(0x7fffffff);
    const __m128i one_i = _mm_set1_epi32(1);
    const __m128i two_i = _mm_set1_epi32(2);

    x = _mm_and_ps(x, inv_signmask); // abs

    __m128 i = _mm_mul_ps(x, c_2_over_pi);
    __m128i ii = _mm_cvttps_epi32(i);
    i = _mm_cvtepi32_ps(ii);
    x = _mm_sub_ps(x, _mm_mul_ps(i, c_pi_over_2));

    __m128i b1 = _mm_and_si128(ii, one_i);
    __m128i b1_mask = _mm_cmpeq_epi32(b1, one_i);

    __m128 kx = _mm_sub_ps(c_pi_over_2, x);

    x = _mm_or_ps(_mm_and_ps((__m128)b1_mask, kx), _mm_andnot_ps((__m128)b1_mask, x));

    __m128 xx = _mm_mul_ps(x, x);
    //__m128 xxxx = _mm_mul_ps(xx, xx);

    __m128 p0 = _mm_set1_ps(1.0f);
    __m128 p1 = c_1_over_2;
    __m128 p2 = _mm_mul_ps(xx, c_1_over_24);

    __m128 yy = _mm_mul_ps(_mm_sub_ps(p2, p1), xx);
    __m128 y = _mm_add_ps(p0, yy);

    __m128i b2 = _mm_and_si128(ii, two_i);
    __m128i b2_mask = _mm_cmpeq_epi32(b2, two_i);
    __m128 negate_mask = _mm_xor_ps((__m128)b1_mask, (__m128)b2_mask);

    //y = _mm_xor_ps(y, _mm_and_ps(_mm_set1_ps(-0.0f), negate_mask));
    y = _mm_xor_ps(y, _mm_and_ps(signmask, negate_mask));
    return y;
}

// USE_SSE
#endif

