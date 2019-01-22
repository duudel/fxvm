
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

/*
 * b1   | b2
 * w OP | s  t
 *
 * OP opcode (6 bits)
 * w  width: 1, 2, 3 or 4 components (2 bits)
 * s  source: which register is the source (4 bits)
 * t  target: which register is the target (4 bits)
*/

enum Particle_Bytecode_OP
{
    PBOP_LOAD_CONST,
    PBOP_LOAD_INPUT,
    PBOP_SWIZZLE,
    PBOP_MOV,
    PBOP_MOV_MASK,
    PBOP_ADD,
    PBOP_SUB,
    PBOP_MUL,
    PBOP_DIV,
    //PBOP_FMA,
    PBOP_RCP,
    PBOP_RSQRT,
    PBOP_SQRT,
    PBOP_SIN,
    PBOP_COS,
    PBOP_EXP,
    PBOP_EXP2,
    PBOP_EXP10,
    //PBOP_DOT,
    PBOP_ABS,
    PBOP_MIN,
    PBOP_MAX,
    PBOP_CLAMP01,
    PBOP_CLAMP,
    PBOP_INTERP,
};

struct Instr_Info
{
    Particle_Bytecode_OP opcode;

};

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
        auto opcode = (Particle_Bytecode_OP)(p[0] & 0x3f);
        switch (opcode)
        {
        case PBOP_LOAD_CONST:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_load(p + 2);
                p += 16 + 2;
            } break;
        case PBOP_LOAD_INPUT:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t input_offset = p[2];
                S.r[target_reg] = reg_load(input + input_offset);
                p += 3;
            } break;
        case PBOP_SWIZZLE:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                uint8_t swizzle_mask = p[2];
                S.r[target_reg] = reg_swizzle(S.r[source_reg], swizzle_mask);
                p += 3;
            } break;
        case PBOP_MOV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = S.r[source_reg];
                p += 2;
            } break;
        case PBOP_MOV_MASK:
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
        case PBOP_ADD:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_add(S.r[target_reg], S.r[source_reg]);
                p += 2;
            } break;
        case PBOP_SUB:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_sub(S.r[target_reg], S.r[source_reg]);
                p += 2;
            } break;
        case PBOP_MUL:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_mul(S.r[target_reg], S.r[source_reg]);
                p += 2;
            } break;
        case PBOP_DIV:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_div(S.r[target_reg], S.r[source_reg]);
                p += 2;
            } break;
        //case PBOP_FMA:
        case PBOP_RCP:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_rcp(S.r[target_reg]);
                p += 2;
            } break;
        case PBOP_RSQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_rsqrt(S.r[target_reg]);
                p += 2;
            } break;
        case PBOP_SQRT:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_sqrt(S.r[target_reg]);
                p += 2;
            } break;
        case PBOP_SIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_sin(S.r[target_reg]);
                p += 2;
            } break;
        case PBOP_COS:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_cos(S.r[target_reg]);
                p += 2;
            } break;
        case PBOP_EXP:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_exp(S.r[target_reg]);
                p += 2;
            } break;
        case PBOP_EXP2:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_exp2(S.r[target_reg]);
                p += 2;
            } break;
        case PBOP_EXP10:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_exp10(S.r[target_reg]);
                p += 2;
            } break;
        //case PBOP_DOT:
        case PBOP_ABS:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_abs(S.r[target_reg]);
                p += 2;
            } break;
        case PBOP_MIN:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_min(S.r[target_reg], S.r[source_reg]);
                p += 2;
            } break;
        case PBOP_MAX:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg = (p[1] >> 4) & 0xf;
                S.r[target_reg] = reg_max(S.r[target_reg], S.r[source_reg]);
                p += 2;
            } break;
        case PBOP_CLAMP01:
            {
                uint8_t target_reg = p[1] & 0xf;
                S.r[target_reg] = reg_clamp01(S.r[target_reg]);
                p += 2;
            } break;
        case PBOP_CLAMP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg1 = (p[1] >> 4) & 0x3;
                uint8_t source_reg2 = (p[1] >> 6) & 0x3;
                S.r[target_reg] = reg_clamp(S.r[target_reg], S.r[source_reg1], S.r[source_reg2]);
                p += 2;
            } break;
        case PBOP_INTERP:
            {
                uint8_t target_reg = p[1] & 0xf;
                uint8_t source_reg1 = (p[1] >> 4) & 0x3;
                uint8_t source_reg2 = (p[1] >> 6) & 0x3;
                S.r[target_reg] = reg_interp(S.r[target_reg], S.r[source_reg1], S.r[source_reg2]);
                p += 2;
            } break;
        }
    }
}

