
#include "fxvm.h"
#include "fxcomp.h"
#include <cstdio>
#include <cstring>

struct Bytecode_Writer
{
    int max_size;
    int len;
    uint8_t *code;
};


void ensure_bytes_fit(Bytecode_Writer *writer, int bytes)
{
    if (writer->len + bytes > writer->max_size)
    {
        int max_size = writer->len + bytes + 32;
        writer->code = (uint8_t*)realloc(writer->code, max_size);
        writer->max_size = max_size;
    }
}

void write_byte(Bytecode_Writer *writer, int8_t v)
{
    ensure_bytes_fit(writer, 1);
    writer->code[writer->len] = v;
    writer->len++;
}

void write_float(Bytecode_Writer *writer, float v)
{
    ensure_bytes_fit(writer, 4);
    *(float*)&writer->code[writer->len] = v;
    writer->len += 4;
}

void write_vec4(Bytecode_Writer *writer, float x, float y, float z, float w)
{
    ensure_bytes_fit(writer, 16);
    float *v = (float*)&writer->code[writer->len];
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
    writer->len += 16;
}

void write_op(Bytecode_Writer *writer, Particle_Bytecode_OP op)
{
    write_byte(writer, (uint8_t)op);
}

void write_target_reg(Bytecode_Writer *writer, uint8_t target_reg)
{
    write_byte(writer, target_reg & 0xf);
}

void write_target_source_regs(Bytecode_Writer *writer, uint8_t target_reg, uint8_t source_reg)
{
    write_byte(writer, (target_reg & 0xf) | (source_reg << 4));
}

void write_target_source2_regs(Bytecode_Writer *writer, uint8_t target_reg, uint8_t source_reg1, uint8_t source_reg2)
{
    uint8_t source_regs = (source_reg1 & 0x3) | ((source_reg2 & 0x3) << 2);
    write_byte(writer, (target_reg & 0xf) | (source_regs << 4));
}

void write_bytecode(Particle_Bytecode *bytecode)
{
    Bytecode_Writer writer = { };

    write_op(&writer, PBOP_LOAD_CONST);
    write_target_reg(&writer, 0);
    write_vec4(&writer, 4.0f, 3.0f, -2.0f, 3.14f);

    write_op(&writer, PBOP_LOAD_CONST);
    write_target_reg(&writer, 1);
    write_vec4(&writer, 2.0f, 10.0f, 21.0f, 1.14f);

    write_op(&writer, PBOP_ADD);
    write_target_source_regs(&writer, 0, 1);

    write_op(&writer, PBOP_LOAD_CONST);
    write_target_reg(&writer, 2);
    write_vec4(&writer, 0.0f, 1.0f, 0.5f, 0.15f);

    write_op(&writer, PBOP_INTERP);
    write_target_source2_regs(&writer, 0, 1, 2);

    bytecode->len = writer.len;
    bytecode->code = writer.code;
}

void free_bytecode(Particle_Bytecode *bytecode)
{
    free(bytecode->code);
    *bytecode = { };
}

int main_simple_bc(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    Particle_Bytecode bytecode = { };
    write_bytecode(&bytecode);

    printf("bytecode len = %d\n", bytecode.len);

    Particle_VM_State state = { };
    exec(state, nullptr, &bytecode);

    Reg r0 = state.r[0];
    printf("r0 = %.3f, %.3f, %.3f %.3f\n", r0.v[0], r0.v[1], r0.v[2], r0.v[3]);

    return 0;
}

void report_compile_error(const char *err)
{
    printf("Error: %s\n", err);
}

void print_indentation(int indentation)
{
    while (indentation-- > 0) putchar(' ');
}

void print_ast(FXVM_Ast *ast, int indentation);

void print_ast_root(FXVM_Ast *root, int indentation)
{
    for (int i = 0; i < root->root.node_num; i++)
    {
        print_ast(root->root.nodes[i], indentation);
    }
}

void print_ast_unary_expr(FXVM_Ast *expr, int indentation)
{
    print_indentation(indentation);
    printf("unary %s [%s]\n", unary_op_to_string(expr->unary.op), type_to_string(expr->type));
    print_ast(expr->unary.operand, indentation + 1);
}

void print_ast_binary_expr(FXVM_Ast *expr, int indentation)
{
    print_indentation(indentation);
    printf("binary %s [%s]\n", binary_op_to_string(expr->binary.op), type_to_string(expr->type));
    print_ast(expr->binary.left, indentation + 1);
    print_ast(expr->binary.right, indentation + 1);
}

void print_ast_variable_expr(FXVM_Ast *expr, int indentation)
{
    print_indentation(indentation);
    char buf[32];
    const char *start = expr->variable.token->start;
    const char *end = expr->variable.token->end;
    size_t max_len = (end - start > 31) ? 31 : (end - start);
    strncpy(buf, start, max_len);
    buf[max_len] = '\0';
    printf("variable %s [%s]\n", buf, type_to_string(expr->type));
}

void print_ast_number_expr(FXVM_Ast *expr, int indentation)
{
    print_indentation(indentation);
    char buf[32];
    const char *start = expr->number.token->start;
    const char *end = expr->number.token->end;
    size_t max_len = (end - start > 31) ? 31 : (end - start);
    strncpy(buf, start, max_len);
    buf[max_len] = '\0';
    printf("number %s [%s]\n", buf, type_to_string(expr->type));
}

void print_ast_call_expr(FXVM_Ast *expr, int indentation)
{
    print_indentation(indentation);
    char buf[32];
    const char *start = expr->call.token->start;
    const char *end = expr->call.token->end;
    size_t max_len = (end - start > 31) ? 31 : (end - start);
    strncpy(buf, start, max_len);
    buf[max_len] = '\0';
    printf("call %s [%s]\n", buf, type_to_string(expr->type));
    for (int i = 0; i < expr->call.params.node_num; i++)
    {
        print_ast(expr->call.params.nodes[i], indentation + 1);
    }
}

void print_ast(FXVM_Ast *ast, int indentation)
{
    switch (ast->kind)
    {
    case FXAST_ROOT: print_ast_root(ast, indentation); break;
    case FXAST_EXPR_UNARY: print_ast_unary_expr(ast, indentation); break;
    case FXAST_EXPR_BINARY: print_ast_binary_expr(ast, indentation); break;
    case FXAST_EXPR_VARIABLE: print_ast_variable_expr(ast, indentation); break;
    case FXAST_EXPR_NUMBER: print_ast_number_expr(ast, indentation); break;
    case FXAST_EXPR_CALL: print_ast_call_expr(ast, indentation); break;
    }
}

void print_instr(FXVM_ILInstr *instr)
{
    switch (instr->op)
    {
    case FXIL_LOAD_CONST:
        printf("%-16s r%d <- [%.6f %.6f %.6f %.6f]\n", il_op_to_string(instr->op),
                instr->target.index,
                instr->constant_load.v[0],
                instr->constant_load.v[1],
                instr->constant_load.v[2],
                instr->constant_load.v[4]);
        break;
    case FXIL_LOAD_INPUT:
        printf("%-16s r%d <- input[%d]\n", il_op_to_string(instr->op),
                instr->target.index,
                instr->input_load.input_index);
        break;
    default:
        switch (il_instr_info[instr->op].reg_operand_num)
        {
        case 0:
            printf("%-16s\n", il_op_to_string(instr->op));
            break;
        case 1:
            printf("%-16s r%d\n", il_op_to_string(instr->op),
                    instr->target.index);
            break;
        case 2:
            printf("%-16s r%d <- r%d\n", il_op_to_string(instr->op),
                    instr->target.index,
                    instr->read_operands[0].index);
            break;
        case 3:
            printf("%-16s r%d <- r%d r%d\n", il_op_to_string(instr->op),
                    instr->target.index,
                    instr->read_operands[0].index,
                    instr->read_operands[1].index);
            break;
        case 4:
            printf("%-16s r%d <- r%d r%d r%d\n", il_op_to_string(instr->op),
                    instr->target.index,
                    instr->read_operands[0].index,
                    instr->read_operands[1].index,
                    instr->read_operands[2].index);
            break;
        case 5:
            printf("%-16s r%d <- r%d r%d r%d r%d\n", il_op_to_string(instr->op),
                    instr->target.index,
                    instr->read_operands[0].index,
                    instr->read_operands[1].index,
                    instr->read_operands[2].index,
                    instr->read_operands[3].index);
            break;
        }
    }
}

void print_il(FXVM_ILContext *ctx)
{
    for (int i = 0; i < ctx->instr_num; i++)
    {
        print_instr(&ctx->instructions[i]);
    }
}

const char *source = "\
v = sqrt(vec2(1.0,2.0)) * Particle_life_time;\
v + vec2(0.5,0.5);\
v3 = vec3(3, 2, 1);\n\
";
/*
x = v3.x;\n\
yxz = v3.yxz;\n\
v4 = vec4(v3, v);\n\
";
*/

// vec4 r6 <- r5.xyz, r0.x
// swizzle r6 <- r5.x, r5.y, r5.z, r0.x
// v4_set r6 <- r5 r5 r5
//
// a = vec4(1, v.z, 0, v.x)
// =>
// load_const r1 <- 1
// mov_x r0 <- r1 x
// mov_xy r0 <- r2 z
// mov_xyz r0 <- 0.0
// mov_xyzw r0 <- r2 x
//

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    FXVM_Compiler compiler = { };
    compiler.report_error = report_compile_error;

    register_constant(&compiler, "PI", 3.1415f);
    register_input_variable(&compiler, "Particle_life_time", FXTYP_F1);

    compile(&compiler, source, source + strlen(source));

    printf("%s\n", source);
    printf("---\n");
    if (compiler.ast)
    {
        print_ast(compiler.ast, 0);
        printf("---\n");
        print_il(&compiler.il_context);
    }

    return 0;
}

