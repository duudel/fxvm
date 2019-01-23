
/*
struct ParticleData
{
    float *life_time;
    float *particle_seed;
    int alive_start;
    int alive_end;
    int max_size;
};

struct SwirlData
{

};

void particle_update(ParticleData *input, void *particle_system_data)
{
    SwirlData *data = (SwirlData*)particle_system_data;
    int alive_start = input->alive_start;
    int alive_end = input->alive_end;
    if (alive_end < alive_start) alive_end += input->max_size;
    int max_size = input->max_size;
    int new_alive_start = alive_start;
    for (int i = alive_start; i < alive_end; i++)
    {
        int index = i & max_size;
        if (input->life_time[index] >= 1.0f)
        {
            if (new_alive_start == i) new_alive_start++;
        }
    }
}
*/

#include "fxreg.h"
#include "fxop.h"
#include <cstdio>

/*
 * b1   | b2
 * w OP | s  t
 *
 * OP opcode (6 bits)
 * w  width: 1, 2, 3 or 4 components (2 bits)
 * s  source: which register is the source (4 bits)
 * t  target: which register is the target (4 bits)
*/

struct Particle_Bytecode
{
    int len;
    uint8_t *code;
};

struct Particle_VM_State
{
    enum { MAX_REGS = 16 };
    Reg r[MAX_REGS];
};

struct InputSetup
{

};

/*
void setup_input(InputSetup *setup, int input_count, void *input)
{
}
*/

void exec(Particle_VM_State &S, uint8_t *input, Particle_Bytecode *bytecode)
{
    const uint8_t *end = bytecode->code + bytecode->len;
    const uint8_t *p = bytecode->code;
    while (p < end)
    {
        auto opcode = (FXVM_BytecodeOp)(p[0] & 0x3f);
        switch (opcode)
        {
        case FXOP_LOAD_CONST:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_load(p + 2);
                p += 16 + 2;

                auto r = S.r[target_reg];
                printf("load const %d <- %.3f %.3f %.3f %.3f\n", target_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_LOAD_INPUT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t input_offset = p[2];
                S.r[target_reg] = reg_load(input + input_offset);
                p += 3;

                auto r = S.r[target_reg];
                printf("load input %d <- [%d] (%.3f %.3f %.3f %.3f)\n", target_reg, input_offset, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_SWIZZLE:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                uint8_t swizzle_mask = p[2];
                S.r[target_reg] = reg_swizzle(S.r[source_reg], swizzle_mask);
                p += 3;
            } break;
        case FXOP_MOV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = S.r[source_reg];
                p += 2;

                printf("mov x %d <- %d\n", target_reg, source_reg);
            } break;
        case FXOP_MOV_X:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_mov_x(S.r[x_reg]);
                p += 2;

                auto r = S.r[target_reg];
                printf("mov x %d <- (%.3f %.3f %.3f %.3f)\n", target_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_MOV_XY:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                S.r[target_reg] = reg_mov_xy(S.r[x_reg], S.r[y_reg]);
                p += 3;

                auto r = S.r[target_reg];
                printf("mov xy %d <- (%.3f %.3f %.3f %.3f)\n", target_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_MOV_XYZ:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                uint8_t z_reg = (p[2] >> 4) & 0xf;
                S.r[target_reg] = reg_mov_xyz(S.r[x_reg], S.r[y_reg], S.r[z_reg]);
                p += 3;

                auto r = S.r[target_reg];
                printf("mov xyz %d <- (%.3f %.3f %.3f %.3f)\n", target_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_MOV_XYZW:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                uint8_t z_reg = (p[2] >> 4) & 0xf;
                uint8_t w_reg = p[3] & 0xf;
                S.r[target_reg] = reg_mov_xyzw(S.r[x_reg], S.r[y_reg], S.r[z_reg], S.r[w_reg]);
                p += 4;

                auto r = S.r[target_reg];
                printf("mov xyzw %d <- (%.3f %.3f %.3f %.3f)\n", target_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_MOV_MASK:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                uint8_t mov_mask = p[2];
                S.r[target_reg].v[0] = (mov_mask & 1) ? S.r[source_reg].v[0] : S.r[target_reg].v[0];
                S.r[target_reg].v[1] = (mov_mask & 2) ? S.r[source_reg].v[1] : S.r[target_reg].v[1];
                S.r[target_reg].v[2] = (mov_mask & 4) ? S.r[source_reg].v[2] : S.r[target_reg].v[2];
                S.r[target_reg].v[3] = (mov_mask & 8) ? S.r[source_reg].v[3] : S.r[target_reg].v[3];
                p += 3;
            } break;
        case FXOP_NEG:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_neg(S.r[source_reg]);
                p += 2;
            } break;
        case FXOP_ADD:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_add(S.r[a_reg], S.r[b_reg]);
                p += 3;

                auto r = S.r[target_reg];
                printf("add %d <- %d + %d (%.3f %.3f %.3f %.3f)\n", target_reg, a_reg, b_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_SUB:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_sub(S.r[a_reg], S.r[b_reg]);
                p += 3;

                auto r = S.r[target_reg];
                printf("sub %d <- %d - %d (%.3f %.3f %.3f %.3f)\n", target_reg, a_reg, b_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_MUL:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_mul(S.r[a_reg], S.r[b_reg]);
                p += 3;

                auto r = S.r[target_reg];
                printf("mul %d <- %d * %d (%.3f %.3f %.3f %.3f)\n", target_reg, a_reg, b_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_DIV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_div(S.r[a_reg], S.r[b_reg]);
                p += 3;

                auto r = S.r[target_reg];
                printf("div %d <- %d / %d (%.3f %.3f %.3f %.3f)\n", target_reg, a_reg, b_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        //case FXOP_FMA:
        case FXOP_RCP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_rcp(S.r[source_reg]);
                p += 2;

                auto r = S.r[target_reg];
                printf("rcp %d <- %d (%.3f %.3f %.3f %.3f)\n", target_reg, source_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_RSQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_rsqrt(S.r[source_reg]);
                p += 2;

                auto r = S.r[target_reg];
                printf("rsqrt %d <- %d (%.3f %.3f %.3f %.3f)\n", target_reg, source_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_SQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_sqrt(S.r[source_reg]);
                p += 2;

                auto r = S.r[target_reg];
                printf("sqrt %d <- %d (%.3f %.3f %.3f %.3f)\n", target_reg, source_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_SIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_sin(S.r[source_reg]);
                p += 2;

                auto r = S.r[target_reg];
                printf("sin %d <- %d (%.3f %.3f %.3f %.3f)\n", target_reg, source_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_COS:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_cos(S.r[source_reg]);
                p += 2;

                auto r = S.r[target_reg];
                printf("cos %d <- %d (%.3f %.3f %.3f %.3f)\n", target_reg, source_reg, r.v[0], r.v[1], r.v[2], r.v[3]);
            } break;
        case FXOP_EXP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_exp(S.r[source_reg]);
                p += 2;
            } break;
        case FXOP_EXP2:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_exp2(S.r[source_reg]);
                p += 2;
            } break;
        case FXOP_EXP10:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_exp10(S.r[source_reg]);
                p += 2;
            } break;
        //case FXOP_DOT:
        case FXOP_ABS:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_abs(S.r[source_reg]);
                p += 2;
            } break;
        case FXOP_MIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg1 = (p[1] >> 4) & 0xf;
                uint8_t source_reg2 = p[2] & 0xf;
                S.r[target_reg] = reg_min(S.r[source_reg1], S.r[source_reg2]);
                p += 3;
            } break;
        case FXOP_MAX:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg1 = (p[1] >> 4) & 0xf;
                uint8_t source_reg2 = p[2] & 0xf;
                S.r[target_reg] = reg_max(S.r[source_reg1], S.r[source_reg2]);
                p += 3;
            } break;
        case FXOP_CLAMP01:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_clamp01(S.r[source_reg]);
                p += 2;
            } break;
        case FXOP_CLAMP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                S.r[target_reg] = reg_clamp(S.r[x_reg], S.r[a_reg], S.r[b_reg]);
                p += 3;
            } break;
        case FXOP_INTERP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t t_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                S.r[target_reg] = reg_interp(S.r[a_reg], S.r[b_reg], S.r[t_reg]);
                p += 3;
            } break;
        }
    }
}

