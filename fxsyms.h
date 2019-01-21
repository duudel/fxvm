#ifndef FXVM_SYMS

#include "fxvm_types.h"
#include <cstdlib>
#include <cstring>

enum FXVM_SymbolType
{
    FXSYM_Variable,

    FXSYM_InputVariable,
    FXSYM_BuiltinConstant,
    FXSYM_BuiltinFunction,
};

struct FXVM_Symbols
{
    int symbol_num;
    int symbol_cap;
    struct SymbolName
    {
        const char *start;
        const char *end;
    } *names;

    FXVM_Type *types;

    enum { MAX_PARAMETER_NUM = 4 };
    struct FunctionType
    {
        int parameter_num;
        FXVM_Type return_type;
        FXVM_Type parameter_types[MAX_PARAMETER_NUM];
    } *function_types;

    struct Constant
    {
        float v[4];
    } *constants;

    FXVM_SymbolType *sym_types;
};

void ensure_symbol_fits(FXVM_Symbols *syms)
{
    if (syms->symbol_num + 1 > syms->symbol_cap)
    {
        int new_cap = syms->symbol_cap + 32;
        syms->names = (FXVM_Symbols::SymbolName*)realloc(syms->names, new_cap * sizeof(FXVM_Symbols::SymbolName));
        syms->types = (FXVM_Type*)realloc(syms->types, new_cap * sizeof(FXVM_Type));
        syms->function_types = (FXVM_Symbols::FunctionType*)realloc(syms->function_types, new_cap * sizeof(FXVM_Symbols::FunctionType));
        syms->constants = (FXVM_Symbols::Constant*)realloc(syms->constants, new_cap * sizeof(FXVM_Symbols::Constant));
        syms->sym_types = (FXVM_SymbolType*)realloc(syms->sym_types, new_cap * sizeof(FXVM_SymbolType));
        syms->symbol_cap = new_cap;
    }
}

int push_symbol(FXVM_Symbols *syms, const char *sym, const char *sym_end, FXVM_Type type)
{
    ensure_symbol_fits(syms);
    int i = syms->symbol_num;
    syms->names[i] = { sym, sym_end };
    syms->types[i] = type;
    syms->function_types[i] = { };
    syms->constants[i] = { };
    syms->sym_types[i] = FXSYM_Variable;
    syms->symbol_num = i + 1;
    return i;
}

int push_symbol_builtin_constant(FXVM_Symbols *syms, const char *sym, const char *sym_end, float *value, int width)
{
    int sym_index = -1;
    switch (width)
    {
    case 1: sym_index = push_symbol(syms, sym, sym_end, FXTYP_F1); break;
    case 2: sym_index = push_symbol(syms, sym, sym_end, FXTYP_F2); break;
    case 3: sym_index = push_symbol(syms, sym, sym_end, FXTYP_F3); break;
    case 4: sym_index = push_symbol(syms, sym, sym_end, FXTYP_F4); break;
    }
    // TODO: assert sym_index != -1
    memcpy(&syms->constants[sym_index], value, width * sizeof(float));
    syms->sym_types[sym_index] = FXSYM_BuiltinConstant;
    return sym_index;
}

bool string_eq(const char *s1, size_t len1, const char *s2, size_t len2)
{
    if (len1 != len2) return false;
    for (size_t i = 0; i < len1; i++)
    {
        if (s1[i] != s2[i]) return false;
    }
    return true;
}

int symbols_find(FXVM_Symbols *syms, const char *start, const char *end)
{
    size_t len = end - start;
    for (int index = 0; index < syms->symbol_num; index++)
    {
        const char *s_start = syms->names[index].start;
        const char *s_end = syms->names[index].end;
        if (string_eq(s_start, s_end - s_start, start, len))
        {
            return index;
        }
    }
    return -1;
}



#define FXVM_SYMS
#endif
