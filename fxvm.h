#ifndef FXVM_VM

#include "fxreg.h"
#include "fxop.h"

/*
 * b1   | b2
 * w OP | s  t
 *
 * OP opcode (6 bits)
 * w  width: 1, 2, 3 or 4 components (2 bits)
 * s  source: which register is the source (4 bits)
 * t  target: which register is the target (4 bits)
*/

struct FXVM_Bytecode
{
    int len;
    void *code;
};

struct FXVM_Machine
{
    pcg32_random_t rng;
};

FXVM_Machine fxvm_new();

struct FXVM_State
{
    enum { MAX_REGS = 16 };
    Reg r[MAX_REGS];
};

void exec(FXVM_Machine *vm, FXVM_State &S, float *global_input, float **instance_attributes, int instance_index, FXVM_Bytecode *bytecode);

template <int MAX_GROUP>
void exec(FXVM_Machine *vm, FXVM_State (&S)[MAX_GROUP], float *global_input, float **instance_attributes, int instance_index, int instance_count, FXVM_Bytecode *bytecode);

void disassemble(FXVM_Bytecode *bytecode);

#ifdef FXVM_IMPL

FXVM_Machine fxvm_new()
{
    FXVM_Machine result = { };
    result.rng = PCG32_INITIALIZER;
    return result;
}

#include <cstdio>

#ifdef TRACE_FXVM
#define FXVM_TRACE_OP() printf("%-18s ", fxvm_opcode_string[opcode] + 5)
#define FXVM_TRACE(fmt, ...) printf(fmt, ## __VA_ARGS__)
#define FXVM_TRACE_REG(i) printf("r%d={%.3f, %.3f, %.3f, %.3f}", i, S.r[i].v[0], S.r[i].v[1], S.r[i].v[2], S.r[i].v[3])
#else
#define FXVM_TRACE_OP()
#define FXVM_TRACE(fmt, ...)
#define FXVM_TRACE_REG(i)
#endif

void exec(FXVM_Machine *vm, FXVM_State &S, float *global_input, float **instance_attributes, int instance_index, FXVM_Bytecode *bytecode)
{
    const uint8_t *end = (uint8_t*)bytecode->code + bytecode->len;
    const uint8_t *p = (uint8_t*)bytecode->code;
    while (p < end)
    {
        // ww opopop
        auto opcode = (FXVM_BytecodeOp)(p[0] & 0x3f);
        switch (opcode)
        {
        case FXOP_LOAD_CONST:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_load(p + 2);
                p += 16 + 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- const: ", target_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_LOAD_GLOBAL_INPUT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t input_offset = p[2];
                S.r[target_reg] = reg_load((uint8_t*)(global_input + input_offset));
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- [%d]: ", target_reg, input_offset);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_LOAD_ATTRIBUTE:
            {
                uint8_t width = p[0] >> 6;
                if (width == 3) width = 4;
                uint8_t target_reg = p[1] & 0xf;
                uint8_t input_attribute = p[2];
                float *attribute_data = instance_attributes[input_attribute];
                S.r[target_reg] = reg_load((uint8_t*)(attribute_data + instance_index * width));
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("%d r%d <- [%d][%d]: ", width, target_reg, instance_index, input_attribute);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_SWIZZLE:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                uint8_t swizzle_mask = p[2];
                S.r[target_reg] = reg_swizzle(S.r[source_reg], swizzle_mask);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d MASK %02x: ", target_reg, source_reg, swizzle_mask);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = S.r[source_reg];
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV_X:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_mov_x(S.r[x_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, x_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV_XY:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                S.r[target_reg] = reg_mov_xy(S.r[x_reg], S.r[y_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, x_reg, y_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV_XYZ:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                uint8_t z_reg = (p[2] >> 4) & 0xf;
                S.r[target_reg] = reg_mov_xyz(S.r[x_reg], S.r[y_reg], S.r[z_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d: ", target_reg, x_reg, y_reg, z_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
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

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d r%d: ", target_reg, x_reg, y_reg, z_reg, w_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
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

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d MASK %x: ", target_reg, source_reg, mov_mask);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_NEG:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_neg(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_ADD:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_add(S.r[a_reg], S.r[b_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_SUB:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_sub(S.r[a_reg], S.r[b_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MUL:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_mul(S.r[a_reg], S.r[b_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MUL_BY_SCALAR:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_mul_by_scalar(S.r[a_reg], S.r[b_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_DIV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_div(S.r[a_reg], S.r[b_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_DIV_BY_SCALAR:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_div_by_scalar(S.r[a_reg], S.r[b_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_RCP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_rcp(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_RSQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_rsqrt(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_SQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_sqrt(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_SIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_sin(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_COS:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_cos(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_EXP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_exp(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_EXP2:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_exp2(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_EXP10:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_exp10(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_TRUNC:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_trunc(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_FRACT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_fract(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_ABS:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_abs(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_min(S.r[a_reg], S.r[b_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MAX:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                S.r[target_reg] = reg_max(S.r[a_reg], S.r[b_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_DOT:
            {
                uint8_t width = p[0] >> 6;
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                switch (width)
                {
                    case 1: S.r[target_reg].v[0] = reg_dot1(S.r[a_reg], S.r[b_reg]); break;
                    case 2: S.r[target_reg].v[0] = reg_dot2(S.r[a_reg], S.r[b_reg]); break;
                    case 3: S.r[target_reg].v[0] = reg_dot3(S.r[a_reg], S.r[b_reg]); break;
                    case 4: S.r[target_reg].v[0] = reg_dot4(S.r[a_reg], S.r[b_reg]); break;
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("%d r%d <- r%d r%d: ", width, target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_NORMALIZE:
            {
                uint8_t width = p[0] >> 6;
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                switch (width)
                {
                    case 1: S.r[target_reg] = reg_normalize1(S.r[a_reg]); break;
                    case 2: S.r[target_reg] = reg_normalize2(S.r[a_reg]); break;
                    case 3: S.r[target_reg] = reg_normalize3(S.r[a_reg]); break;
                    case 4: S.r[target_reg] = reg_normalize4(S.r[a_reg]); break;
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("%d r%d <- r%d: ", width, target_reg, a_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_CLAMP01:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_clamp01(S.r[source_reg]);
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_CLAMP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                S.r[target_reg] = reg_clamp(S.r[x_reg], S.r[a_reg], S.r[b_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d: ", target_reg, x_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_INTERP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t t_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                S.r[target_reg] = reg_interp(S.r[a_reg], S.r[b_reg], S.r[t_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d: ", target_reg, t_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_INTERP_BY_SCALAR:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t t_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                S.r[target_reg] = reg_interp_by_scalar(S.r[a_reg], S.r[b_reg], S.r[t_reg]);
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d: ", target_reg, t_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_RAND01:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_random01(&vm->rng);
                p += 16 + 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- ", target_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        default:
            printf("ERROR: invalid opcode %d\n", opcode); fflush(stdout);
            return;
        }
    }
}

template <int MAX_GROUP>
void exec(FXVM_Machine *vm, FXVM_State (&S)[MAX_GROUP], float *global_input, float **instance_attributes, int instance_index, int instance_count, FXVM_Bytecode *bytecode)
{
    const uint8_t *end = (uint8_t*)bytecode->code + bytecode->len;
    const uint8_t *p = (uint8_t*)bytecode->code;
    while (p < end)
    {
        // ww opopop
        auto opcode = (FXVM_BytecodeOp)(p[0] & 0x3f);
        switch (opcode)
        {
        case FXOP_LOAD_CONST:
            {
                uint8_t target_reg = p[1] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_load(p + 2);
                }
                p += 16 + 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- const: ", target_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_LOAD_GLOBAL_INPUT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t input_offset = p[2];
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_load((uint8_t*)(global_input + input_offset));
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- [%d]: ", target_reg, input_offset);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_LOAD_ATTRIBUTE:
            {
                uint8_t width = p[0] >> 6;
                if (width == 3) width = 4;
                uint8_t target_reg = p[1] & 0xf;
                uint8_t input_attribute = p[2];
                float *attribute_data = instance_attributes[input_attribute];
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_load((uint8_t*)(attribute_data + (instance_index + i)  * width));
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("%d r%d <- [%d][%d]: ", width, target_reg, instance_index, input_attribute);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_SWIZZLE:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                uint8_t swizzle_mask = p[2];
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_swizzle(S[i].r[source_reg], swizzle_mask);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d MASK %02x: ", target_reg, source_reg, swizzle_mask);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = S[i].r[source_reg];
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV_X:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_mov_x(S[i].r[x_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, x_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV_XY:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_mov_xy(S[i].r[x_reg], S[i].r[y_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, x_reg, y_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV_XYZ:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                uint8_t z_reg = (p[2] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_mov_xyz(S[i].r[x_reg], S[i].r[y_reg], S[i].r[z_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d: ", target_reg, x_reg, y_reg, z_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV_XYZW:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                uint8_t z_reg = (p[2] >> 4) & 0xf;
                uint8_t w_reg = p[3] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_mov_xyzw(S[i].r[x_reg], S[i].r[y_reg], S[i].r[z_reg], S[i].r[w_reg]);
                }
                p += 4;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d r%d: ", target_reg, x_reg, y_reg, z_reg, w_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MOV_MASK:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                uint8_t mov_mask = p[2];
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg].v[0] = (mov_mask & 1) ? S[i].r[source_reg].v[0] : S[i].r[target_reg].v[0];
                    S[i].r[target_reg].v[1] = (mov_mask & 2) ? S[i].r[source_reg].v[1] : S[i].r[target_reg].v[1];
                    S[i].r[target_reg].v[2] = (mov_mask & 4) ? S[i].r[source_reg].v[2] : S[i].r[target_reg].v[2];
                    S[i].r[target_reg].v[3] = (mov_mask & 8) ? S[i].r[source_reg].v[3] : S[i].r[target_reg].v[3];
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d MASK %x: ", target_reg, source_reg, mov_mask);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_NEG:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_neg(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_ADD:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_add(S[i].r[a_reg], S[i].r[b_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_SUB:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_sub(S[i].r[a_reg], S[i].r[b_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MUL:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_mul(S[i].r[a_reg], S[i].r[b_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MUL_BY_SCALAR:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_mul_by_scalar(S[i].r[a_reg], S[i].r[b_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_DIV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_div(S[i].r[a_reg], S[i].r[b_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_DIV_BY_SCALAR:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_div_by_scalar(S[i].r[a_reg], S[i].r[b_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_RCP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_rcp(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_RSQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_rsqrt(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_SQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_sqrt(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_SIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_sin(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_COS:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_cos(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_EXP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_exp(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_EXP2:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_exp2(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_EXP10:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_exp10(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_TRUNC:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_trunc(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_FRACT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_fract(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_ABS:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_abs(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_min(S[i].r[a_reg], S[i].r[b_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_MAX:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_max(S[i].r[a_reg], S[i].r[b_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d: ", target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_DOT:
            {
                uint8_t width = p[0] >> 6;
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    switch (width)
                    {
                        case 1: S[i].r[target_reg].v[0] = reg_dot1(S[i].r[a_reg], S[i].r[b_reg]); break;
                        case 2: S[i].r[target_reg].v[0] = reg_dot2(S[i].r[a_reg], S[i].r[b_reg]); break;
                        case 3: S[i].r[target_reg].v[0] = reg_dot3(S[i].r[a_reg], S[i].r[b_reg]); break;
                        case 4: S[i].r[target_reg].v[0] = reg_dot4(S[i].r[a_reg], S[i].r[b_reg]); break;
                    }
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("%d r%d <- r%d r%d: ", width, target_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_NORMALIZE:
            {
                uint8_t width = p[0] >> 6;
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    switch (width)
                    {
                        case 1: S[i].r[target_reg] = reg_normalize1(S[i].r[a_reg]); break;
                        case 2: S[i].r[target_reg] = reg_normalize2(S[i].r[a_reg]); break;
                        case 3: S[i].r[target_reg] = reg_normalize3(S[i].r[a_reg]); break;
                        case 4: S[i].r[target_reg] = reg_normalize4(S[i].r[a_reg]); break;
                    }
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("%d r%d <- r%d: ", width, target_reg, a_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_CLAMP01:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_clamp01(S[i].r[source_reg]);
                }
                p += 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d: ", target_reg, source_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_CLAMP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_clamp(S[i].r[x_reg], S[i].r[a_reg], S[i].r[b_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d: ", target_reg, x_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_INTERP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t t_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_interp(S[i].r[a_reg], S[i].r[b_reg], S[i].r[t_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d: ", target_reg, t_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_INTERP_BY_SCALAR:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t t_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_interp_by_scalar(S[i].r[a_reg], S[i].r[b_reg], S[i].r[t_reg]);
                }
                p += 3;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- r%d r%d r%d: ", target_reg, t_reg, a_reg, b_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        case FXOP_RAND01:
            {
                uint8_t target_reg = p[1] & 0xf;
                for (int i = 0; i < instance_count; i++)
                {
                    S[i].r[target_reg] = reg_random01(&vm->rng);
                }
                p += 16 + 2;

                FXVM_TRACE_OP();
                FXVM_TRACE("r%d <- ", target_reg);
                FXVM_TRACE_REG(target_reg);
                FXVM_TRACE("\n");
            } break;
        default:
            printf("ERROR: invalid opcode %d\n", opcode); fflush(stdout);
            return;
        }
    }
}

#undef FXVM_TRACE_OP
#undef FXVM_TRACE
#undef FXVM_TRACE_REG

#define FXVM_PRINT_OP() printf("%-18s ", fxvm_opcode_string[opcode] + 5)
#define FXVM_PRINT(fmt, ...) printf(fmt, ## __VA_ARGS__)

void disassemble(FXVM_Bytecode *bytecode)
{
    const uint8_t *end = (uint8_t*)bytecode->code + bytecode->len;
    const uint8_t *p = (uint8_t*)bytecode->code;
    while (p < end)
    {
        // ww opopop
        auto opcode = (FXVM_BytecodeOp)(p[0] & 0x3f);
        switch (opcode)
        {
        case FXOP_LOAD_CONST:
            {
                uint8_t target_reg = p[1] & 0xf;
                const float *v = (const float*)(p + 2);
                p += 16 + 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- const: %.3f, %.3f, %.3f, %.3f\n", target_reg, v[0], v[1], v[2], v[3]);
            } break;
        case FXOP_LOAD_GLOBAL_INPUT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t input_offset = p[2];
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- [%d]\n", target_reg, input_offset);
            } break;
        case FXOP_LOAD_ATTRIBUTE:
            {
                uint8_t width = p[0] >> 6;
                uint8_t target_reg = p[1] & 0xf;
                uint8_t input_attribute = p[2];
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("%d r%d <- [instance][%d]\n", width, target_reg, input_attribute);
            } break;
        case FXOP_SWIZZLE:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                uint8_t swizzle_mask = p[2];
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d MASK %02x\n", target_reg, source_reg, swizzle_mask);
            } break;
        case FXOP_MOV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_MOV_X:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, x_reg);
            } break;
        case FXOP_MOV_XY:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d\n", target_reg, x_reg, y_reg);
            } break;
        case FXOP_MOV_XYZ:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                uint8_t z_reg = (p[2] >> 4) & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d r%d\n", target_reg, x_reg, y_reg, z_reg);
            } break;
        case FXOP_MOV_XYZW:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t y_reg = p[2] & 0xf;
                uint8_t z_reg = (p[2] >> 4) & 0xf;
                uint8_t w_reg = p[3] & 0xf;
                p += 4;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d r%d r%d\n", target_reg, x_reg, y_reg, z_reg, w_reg);
            } break;
        case FXOP_MOV_MASK:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                uint8_t mov_mask = p[2];
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d MASK %x\n", target_reg, source_reg, mov_mask);
            } break;
        case FXOP_NEG:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_ADD:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d\n", target_reg, a_reg, b_reg);
            } break;
        case FXOP_SUB:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d\n", target_reg, a_reg, b_reg);
            } break;
        case FXOP_MUL:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d\n", target_reg, a_reg, b_reg);
            } break;
        case FXOP_MUL_BY_SCALAR:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d\n", target_reg, a_reg, b_reg);
            } break;
        case FXOP_DIV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d\n", target_reg, a_reg, b_reg);
            } break;
        case FXOP_DIV_BY_SCALAR:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d\n", target_reg, a_reg, b_reg);
            } break;
        case FXOP_RCP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_RSQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_SQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_SIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_COS:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_EXP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_EXP2:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_EXP10:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_TRUNC:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_FRACT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_ABS:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_MIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d\n", target_reg, a_reg, b_reg);
            } break;
        case FXOP_MAX:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d\n", target_reg, a_reg, b_reg);
            } break;
        case FXOP_DOT:
            {
                uint8_t width = p[0] >> 6;
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                uint8_t b_reg = p[2] & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("%d r%d <- r%d r%d\n", width, target_reg, a_reg, b_reg);
            } break;
        case FXOP_NORMALIZE:
            {
                uint8_t width = p[0] >> 6;
                uint8_t target_reg = p[1] & 0xf;
                uint8_t a_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("%d r%d <- r%d\n", width, target_reg, a_reg);
            } break;
        case FXOP_CLAMP01:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                p += 2;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d\n", target_reg, source_reg);
            } break;
        case FXOP_CLAMP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t x_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d r%d\n", target_reg, x_reg, a_reg, b_reg);
            } break;
        case FXOP_INTERP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t t_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d r%d\n", target_reg, t_reg, a_reg, b_reg);
            } break;
        case FXOP_INTERP_BY_SCALAR:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t t_reg = (p[1] >> 4) & 0xf;
                uint8_t a_reg = p[2] & 0xf;
                uint8_t b_reg = (p[2] >> 4) & 0xf;
                p += 3;

                FXVM_PRINT_OP();
                FXVM_PRINT("r%d <- r%d r%d r%d\n", target_reg, t_reg, a_reg, b_reg);
            } break;
        default:
            printf("ERROR: invalid opcode %d\n", opcode); fflush(stdout);
            return;
        }
    }
}

#endif

#define FXVM_VM
#endif

