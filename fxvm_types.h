#ifndef FXVM_TYPES

enum FXVM_Type
{
    FXTYP_NONE,

    FXTYP_F1,
    FXTYP_F2,
    FXTYP_F3,
    FXTYP_F4,

    FXTYP_FUNC,

    // generic float vector type, all GENF in a function signature have the same width
    FXTYP_GENF,
};

#define FXVM_TYPES
#endif

