#ifndef FXVM_OP

#define FXOPS(X)\
    X(FXOP_LOAD_CONST)\
    X(FXOP_LOAD_GLOBAL_INPUT)\
    X(FXOP_LOAD_ATTRIBUTE)\
    X(FXOP_SWIZZLE)\
    X(FXOP_MOV)\
    X(FXOP_MOV_X)\
    X(FXOP_MOV_XY)\
    X(FXOP_MOV_XYZ)\
    X(FXOP_MOV_XYZW)\
    X(FXOP_MOV_MASK)\
    X(FXOP_NEG)\
    X(FXOP_ADD)\
    X(FXOP_SUB)\
    X(FXOP_MUL)\
    X(FXOP_MUL_BY_SCALAR)\
    X(FXOP_DIV)\
    X(FXOP_DIV_BY_SCALAR)\
    X(FXOP_RCP)\
    X(FXOP_RSQRT)\
    X(FXOP_SQRT)\
    X(FXOP_SIN)\
    X(FXOP_COS)\
    X(FXOP_EXP)\
    X(FXOP_EXP2)\
    X(FXOP_EXP10)\
    X(FXOP_TRUNC)\
    X(FXOP_FRACT)\
    X(FXOP_ABS)\
    X(FXOP_MIN)\
    X(FXOP_MAX)\
    X(FXOP_DOT)\
    X(FXOP_NORMALIZE)\
    X(FXOP_CLAMP01)\
    X(FXOP_CLAMP)\
    X(FXOP_INTERP)\
    X(FXOP_INTERP_BY_SCALAR)

    //FXOP(FXOP_FMA)

#define FXOP(op) op,
enum FXVM_BytecodeOp
{
    FXOPS(FXOP)
};
#undef FXOP

#define FXOP(op) #op,
const char* fxvm_opcode_string[] =
{
    FXOPS(FXOP)
};
#undef FXOP

#undef FXOPS

#define FXVM_OP
#endif
