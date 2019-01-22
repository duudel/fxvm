#ifndef FXVM_COMP

#include "fxsyms.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

enum FXVM_TokenKind
{
    FXTOK_IDENT,
    FXTOK_NUMBER,
    FXTOK_ASSIGN,
    FXTOK_PLUS,
    FXTOK_MINUS,
    FXTOK_STAR,
    FXTOK_SLASH,
    FXTOK_SEMICOLON,
    FXTOK_PAREN_L,
    FXTOK_PAREN_R,
    FXTOK_COMMA,
};

struct FXVM_Token
{
    FXVM_TokenKind kind;
    const char *start;
    const char *end;
};

struct FXVM_Ast;
struct FXVM_ILInstr;

struct FXVM_ILContext
{
    int reg_index;

    int instr_num;
    int instr_cap;
    FXVM_ILInstr *instructions;
};

struct FXVM_Compiler
{
    int token_num;
    int token_cap;
    FXVM_Token *tokens;
    FXVM_Ast *ast;
    FXVM_ILContext il_context;

    FXVM_Symbols symbols;

    int error_num;
    void (*report_error)(const char *);
};

void invalid_char(FXVM_Compiler *compiler, const char *c)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        snprintf(buf, 64, "FX error: invalid char %c", c[0]);
        compiler->report_error(buf);
    }
}

void invalid_number(FXVM_Compiler *compiler, const char *c)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        snprintf(buf, 64, "FX error: invalid number"); (void)c;
        compiler->report_error(buf);
    }
}

void ensure_token_fits(FXVM_Compiler *compiler)
{
    if (compiler->token_num + 1 > compiler->token_cap)
    {
        int new_cap = compiler->token_cap + 32;
        compiler->tokens = (FXVM_Token*)realloc(compiler->tokens, new_cap * sizeof(FXVM_Token));
        compiler->token_cap = new_cap;
    }
}

void push_token(FXVM_Compiler *compiler, FXVM_TokenKind kind, const char *start, const char *end)
{
    ensure_token_fits(compiler);
    int i = compiler->token_num;
    compiler->tokens[i] = FXVM_Token { kind, start, end };
    compiler->token_num = i + 1;
}

inline bool is_digit(char c) { return '0' <= c && c <= '9'; }
inline bool is_alpha(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }
inline bool is_ident(char c) { return is_alpha(c) || is_digit(c) || c == '_'; }

const char* lex_ident(FXVM_Compiler *compiler, const char *c, const char *source_end)
{
    const char *start = c;
    while (c < source_end && is_ident(c[0]))
    {
        c++;
    }
    push_token(compiler, FXTOK_IDENT, start, c);
    return c;
}

const char* lex_number(FXVM_Compiler *compiler, const char *c, const char *source_end)
{
    const char *start = c;
    while (c < source_end && is_digit(c[0]))
    {
        c++;
    }
    if (c < source_end && c[0] == '.')
    {
        c++;
        if (c == source_end || !is_digit(c[0]))
        {
            invalid_number(compiler, c);
            return c;
        }
        while (c < source_end && is_digit(c[0]))
        {
            c++;
        }
        if (c < source_end && (c[0] == 'e' || c[0] == 'E'))
        {
            c++;
            if (c < source_end && (c[0] == '+' || c[0] == '-'))
            {
                c++;
            }
            if (c == source_end || !is_digit(c[0]))
            {
                invalid_number(compiler, c);
                return c;
            }
            while (c < source_end && is_digit(c[0]))
            {
                c++;
            }
        }
    }
    push_token(compiler, FXTOK_NUMBER, start, c);
    return c;
}

bool tokenize(FXVM_Compiler *compiler, const char *source, const char *source_end)
{
    const char *c = source;
    while (c < source_end && compiler->error_num < 5)
    {
        switch (c[0])
        {
        case ' ': case '\n': case '\r': case '\t':
            c++;
            break;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            c = lex_number(compiler, c, source_end);
            break;
        case '=':
            push_token(compiler, FXTOK_ASSIGN, c, c + 1);
            c++;
            break;
        case '+':
            push_token(compiler, FXTOK_PLUS, c, c + 1);
            c++;
            break;
        case '-':
            push_token(compiler, FXTOK_MINUS, c, c + 1);
            c++;
            break;
        case '*':
            push_token(compiler, FXTOK_STAR, c, c + 1);
            c++;
            break;
        case '/':
            push_token(compiler, FXTOK_SLASH, c, c + 1);
            c++;
            break;
        case ';':
            push_token(compiler, FXTOK_SEMICOLON, c, c + 1);
            c++;
            break;
        case '(':
            push_token(compiler, FXTOK_PAREN_L, c, c + 1);
            c++;
            break;
        case ')':
            push_token(compiler, FXTOK_PAREN_R, c, c + 1);
            c++;
            break;
        case ',':
            push_token(compiler, FXTOK_COMMA, c, c + 1);
            c++;
            break;
        default:
            if (('A' <= c[0] && c[0] <= 'Z') || ('a' <= c[0] && c[0] <= 'z'))
            {
                c = lex_ident(compiler, c, source_end);
            }
            else
            {
                invalid_char(compiler, c);
                c++;
            }
            break;
        }
    }
    return compiler->error_num == 0;
}

// PARSER

enum FXVM_AstKind
{
    FXAST_ROOT,
    FXAST_EXPR_UNARY,
    FXAST_EXPR_BINARY,
    FXAST_EXPR_VARIABLE,
    FXAST_EXPR_NUMBER,
    FXAST_EXPR_CALL,
};

enum FXVM_AstUnaryOp
{
    FXAST_OP_PLUS,
    FXAST_OP_MINUS,
};

enum FXVM_AstBinaryOp
{
    FXAST_OP_ADD,
    FXAST_OP_SUB,
    FXAST_OP_DIV,
    FXAST_OP_MUL,

    FXAST_OP_ASSIGN,
};

struct FXVM_Ast;

struct FXAST_Nodes
{
    int node_num;
    int node_cap;
    FXVM_Ast **nodes;
};

struct FXAST_UnaryExpr
{
    FXVM_AstUnaryOp op;
    FXVM_Ast *operand;
};

struct FXAST_BinaryExpr
{
    FXVM_AstBinaryOp op;
    FXVM_Ast *left, *right;
};

struct FXAST_Variable
{
    FXVM_Token *token;
};

struct FXAST_Number
{
    FXVM_Token *token;
    float value;
};

struct FXAST_Call
{
    FXVM_Token *token;
    FXAST_Nodes params;
};

struct FXVM_Ast
{
    FXVM_AstKind kind;
    union
    {
        FXAST_Nodes root;
        FXAST_UnaryExpr unary;
        FXAST_BinaryExpr binary;
        FXAST_Variable variable;
        FXAST_Number number;
        FXAST_Call call;
    };

    FXVM_Type type;
};

struct FXVM_Tokens
{
    FXVM_Token *t;
    FXVM_Token *end;
};

FXVM_Ast* alloc_node(FXVM_Compiler *compiler, FXVM_AstKind kind)
{
    (void)compiler;
    // TODO: use pool allocator, with growing number of pools, to guarantee stable memory location for nodes.
    auto *node = (FXVM_Ast*)malloc(sizeof(FXVM_Ast));
    *node = { kind, { }, { } };
    return node;
}

void ensure_node_fits(FXAST_Nodes *root)
{
    if (root->node_num + 1 > root->node_cap)
    {
        int new_cap = root->node_cap + 32;
        root->nodes = (FXVM_Ast**)realloc(root->nodes, new_cap * sizeof(FXVM_Ast*));
        root->node_cap = new_cap;
    }
}

void push_node(FXAST_Nodes *root, FXVM_Ast *node)
{
    ensure_node_fits(root);
    int i = root->node_num;
    root->nodes[i] = node;
    root->node_num = i + 1;
}

FXVM_Token* accept(FXVM_Compiler *compiler, FXVM_Tokens *tokens, FXVM_TokenKind kind)
{
    (void)compiler;
    if (tokens->t < tokens->end && tokens->t->kind == kind)
    {
        return tokens->t++;
    }
    return nullptr;
}

const char* token_kind_to_string(FXVM_TokenKind kind)
{
    switch (kind)
    {
    case FXTOK_ASSIGN: return "=";
    case FXTOK_COMMA: return ",";
    case FXTOK_IDENT: return "identifier";
    case FXTOK_MINUS: return "-";
    case FXTOK_NUMBER: return "number";
    case FXTOK_PAREN_L: return "(";
    case FXTOK_PAREN_R: return ")";
    case FXTOK_PLUS: return "+";
    case FXTOK_SEMICOLON: return ";";
    case FXTOK_SLASH: return "/";
    case FXTOK_STAR: return "*";
    }
    return nullptr;
}

FXVM_Token* expect(FXVM_Compiler *compiler, FXVM_Tokens *tokens, FXVM_TokenKind kind)
{
    if (tokens->t < tokens->end && tokens->t->kind == kind)
    {
        return tokens->t++;
    }
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        snprintf(buf, 64, "expected %s", token_kind_to_string(kind));
        compiler->report_error(buf);
    }
    return nullptr;
}

void expected_parameter(FXVM_Compiler *compiler, FXVM_Tokens *tokens)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        snprintf(buf, 64, "expected parameter"); (void)tokens;
        compiler->report_error(buf);
    }
}

void expected_expression(FXVM_Compiler *compiler, FXVM_Tokens *tokens)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        snprintf(buf, 64, "expected expression"); (void)tokens;
        compiler->report_error(buf);
    }
}

FXVM_Ast* parse_expression(FXVM_Compiler *compiler, FXVM_Tokens *tokens);

FXVM_Ast* parse_factor(FXVM_Compiler *compiler, FXVM_Tokens *tokens)
{
    FXVM_Token *number_tok = nullptr;
    if ((number_tok = accept(compiler, tokens, FXTOK_NUMBER)))
    {
        FXVM_Ast *term = alloc_node(compiler, FXAST_EXPR_NUMBER);
        term->number.token = number_tok;
        return term;
    }
    FXVM_Token *ident_tok = nullptr;
    if ((ident_tok = accept(compiler, tokens, FXTOK_IDENT)))
    {
        if (accept(compiler, tokens, FXTOK_PAREN_L))
        {
            FXVM_Ast *term = alloc_node(compiler, FXAST_EXPR_CALL);
            term->call.token = ident_tok;
            bool comma = false;
            while (tokens->t < tokens->end)
            {
                FXVM_Ast *param = parse_expression(compiler, tokens);
                if (!param)
                {
                    if (comma)
                    {
                        expected_parameter(compiler, tokens);
                    }
                    break;
                }

                push_node(&term->call.params, param);

                if (!accept(compiler, tokens, FXTOK_COMMA))
                    break;
            }
            expect(compiler, tokens, FXTOK_PAREN_R);
            return term;
        }
        else
        {
            FXVM_Ast *term = alloc_node(compiler, FXAST_EXPR_VARIABLE);
            term->variable.token = ident_tok;
            return term;
        }
    }
    if (accept(compiler, tokens, FXTOK_PAREN_L))
    {
        FXVM_Ast *term = parse_expression(compiler, tokens);
        if (!term)
        {
            expected_expression(compiler, tokens);
        }
        expect(compiler, tokens, FXTOK_PAREN_R);
        return term;
    }
    return nullptr;
}

void invalid_unary_op(FXVM_Compiler *compiler, FXVM_Token *token)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        snprintf(buf, 64, "invalid unary operator %c", token->start[0]); // NOTE: assume operator is single char
        compiler->report_error(buf);
    }
}

FXVM_Ast* parse_unary_factor(FXVM_Compiler *compiler, FXVM_Tokens *tokens)
{
    FXVM_Token *unary_tok = nullptr;
    (unary_tok = accept(compiler, tokens, FXTOK_PLUS)) ||
    (unary_tok = accept(compiler, tokens, FXTOK_MINUS));

    FXVM_Ast *term = parse_factor(compiler, tokens);

    if (unary_tok)
    {
        if (!term)
        {
            invalid_unary_op(compiler, unary_tok);
            return nullptr;
        }

        FXVM_Ast *unary_expr = alloc_node(compiler, FXAST_EXPR_UNARY);
        switch (unary_tok->kind)
        {
        case FXTOK_PLUS: unary_expr->unary.op = FXAST_OP_PLUS; break;
        case FXTOK_MINUS: unary_expr->unary.op = FXAST_OP_MINUS; break;
        default:
            // ICE
            break;
        }
        unary_expr->unary.operand = term;
        term = unary_expr;
    }

    return term;
}

FXVM_Ast* parse_term(FXVM_Compiler *compiler, FXVM_Tokens *tokens)
{
    FXVM_Ast *expr = parse_unary_factor(compiler, tokens);
    if (expr)
    {
        while (true)
        {
            FXVM_Token *op_token = nullptr;
            (op_token = accept(compiler, tokens, FXTOK_STAR)) ||
            (op_token = accept(compiler, tokens, FXTOK_SLASH));
            if (!op_token) break;

            FXVM_Ast *binary_expr = alloc_node(compiler, FXAST_EXPR_BINARY);
            switch (op_token->kind)
            {
            case FXTOK_STAR: binary_expr->binary.op = FXAST_OP_MUL; break;
            case FXTOK_SLASH: binary_expr->binary.op = FXAST_OP_DIV; break;
            default:
                // ICE
                break;
            }
            binary_expr->binary.left = expr;
            binary_expr->binary.right = parse_unary_factor(compiler, tokens);

            expr = binary_expr;
        }
    }
    return expr;
}

FXVM_Ast* parse_addition(FXVM_Compiler *compiler, FXVM_Tokens *tokens)
{
    FXVM_Ast *expr = parse_term(compiler, tokens);
    if (expr)
    {
        while (true)
        {
            FXVM_Token *op_token = nullptr;
            (op_token = accept(compiler, tokens, FXTOK_PLUS)) ||
            (op_token = accept(compiler, tokens, FXTOK_MINUS));
            if (!op_token) break;

            FXVM_Ast *binary_expr = alloc_node(compiler, FXAST_EXPR_BINARY);
            switch (op_token->kind)
            {
            case FXTOK_PLUS: binary_expr->binary.op = FXAST_OP_ADD; break;
            case FXTOK_MINUS: binary_expr->binary.op = FXAST_OP_SUB; break;
            default:
                // ICE
                break;
            }
            binary_expr->binary.left = expr;
            binary_expr->binary.right = parse_term(compiler, tokens);

            expr = binary_expr;
        }
    }
    return expr;
}

FXVM_Ast* parse_assignment(FXVM_Compiler *compiler, FXVM_Tokens *tokens)
{
    FXVM_Ast *left = parse_addition(compiler, tokens);
    if (left && accept(compiler, tokens, FXTOK_ASSIGN))
    {
        FXVM_Ast *assignment = alloc_node(compiler, FXAST_EXPR_BINARY);
        assignment->binary.op = FXAST_OP_ASSIGN;
        assignment->binary.left = left;
        assignment->binary.right = parse_addition(compiler, tokens);
        if (!assignment->binary.right)
        {
            expected_expression(compiler, tokens);
        }
        return assignment;
    }
    return left;
}

FXVM_Ast* parse_expression(FXVM_Compiler *compiler, FXVM_Tokens *tokens)
{
    return parse_assignment(compiler, tokens);
}

void invalid_token(FXVM_Compiler *compiler, FXVM_Tokens *tokens)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        snprintf(buf, 64, "invalid token(%d): %s",
                -(int)(compiler->tokens - tokens->t),
                token_kind_to_string(tokens->t->kind));
        compiler->report_error(buf);
    }
}

bool parse(FXVM_Compiler *compiler)
{
    FXVM_Token *token = compiler->tokens;
    FXVM_Token *tokens_end = token + compiler->token_num;
    FXVM_Tokens tokens = { token, tokens_end };
    FXVM_Ast *ast = alloc_node(compiler, FXAST_ROOT);
    while (tokens.t < tokens_end && compiler->error_num < 5)
    {
        if (accept(compiler, &tokens, FXTOK_SEMICOLON)) continue;

        FXVM_Ast *node = parse_expression(compiler, &tokens);
        if (node)
        {
            expect(compiler, &tokens, FXTOK_SEMICOLON);
            push_node(&ast->root, node);
            continue;
        }

        invalid_token(compiler, &tokens);
        tokens.t++;
    }
    compiler->ast = ast;
    return compiler->error_num == 0;
}

// TYPE CHECKING

void variable_not_defined(FXVM_Compiler *compiler, const char *sym_start, const char *sym_end)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        char sym_buf[32];

        int max_len = (sym_end - sym_start > 31) ? 31 : (sym_end - sym_start);
        memcpy(sym_buf, sym_start, max_len);
        sym_buf[max_len] = '\0';

        snprintf(buf, 64, "Variable %.32s not defined", sym_buf);
        compiler->report_error(buf);
    }
}

void function_not_defined(FXVM_Compiler *compiler, const char *sym_start, const char *sym_end)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        char sym_buf[32];

        int max_len = (sym_end - sym_start > 31) ? 31 : (sym_end - sym_start);
        memcpy(sym_buf, sym_start, max_len);
        sym_buf[max_len] = '\0';

        snprintf(buf, 64, "Function %.32s not defined", sym_buf);
        compiler->report_error(buf);
    }
}

void symbol_not_a_function(FXVM_Compiler *compiler, const char *sym_start, const char *sym_end)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        char sym_buf[32];

        int max_len = (sym_end - sym_start > 31) ? 31 : (sym_end - sym_start);
        memcpy(sym_buf, sym_start, max_len);
        sym_buf[max_len] = '\0';

        snprintf(buf, 64, "Symbol %.32s is not a function", sym_buf);
        compiler->report_error(buf);
    }
}

void invalid_parameter_count(FXVM_Compiler *compiler, const char *sym_start, const char *sym_end, int found_num, int expected_num)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        char sym_buf[32];

        int max_len = (sym_end - sym_start > 31) ? 31 : (sym_end - sym_start);
        memcpy(sym_buf, sym_start, max_len);
        sym_buf[max_len] = '\0';

        snprintf(buf, 64, "Parameters specified for %.32s was %d, expected %d parameters", sym_buf, found_num, expected_num);
        compiler->report_error(buf);
    }
}

void invalid_parameter_type(FXVM_Compiler *compiler, const char *sym_start, const char *sym_end, int parameter_num)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        char sym_buf[32];

        int max_len = (sym_end - sym_start > 31) ? 31 : (sym_end - sym_start);
        memcpy(sym_buf, sym_start, max_len);
        sym_buf[max_len] = '\0';

        snprintf(buf, 64, "Parameter type of parameter %d specified for %.32s is invalid", parameter_num, sym_buf);
        compiler->report_error(buf);
    }
}

const char* unary_op_to_string(FXVM_AstUnaryOp op)
{
    switch (op)
    {
        case FXAST_OP_PLUS: return "+";
        case FXAST_OP_MINUS: return "-";
    }
    return nullptr;
}

const char* binary_op_to_string(FXVM_AstBinaryOp op)
{
    switch (op)
    {
        case FXAST_OP_ADD: return "+";
        case FXAST_OP_SUB: return "-";
        case FXAST_OP_MUL: return "*";
        case FXAST_OP_DIV: return "/";
        case FXAST_OP_ASSIGN: return "=";
    }
    return nullptr;
}

const char* type_to_string(FXVM_Type type)
{
    switch (type)
    {
        case FXTYP_NONE: return "[none]";
        case FXTYP_F1: return "float";
        case FXTYP_F2: return "vec2";
        case FXTYP_F3: return "vec3";
        case FXTYP_F4: return "vec4";
        case FXTYP_FUNC: return "func";
        case FXTYP_GENF: return "genF";
    }
    return nullptr;
}

void invalid_operand_types(FXVM_Compiler *compiler, FXVM_AstBinaryOp op, FXVM_Type left, FXVM_Type right)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        const char *op_str = binary_op_to_string(op);
        const char *left_str = type_to_string(left);
        const char *right_str = type_to_string(right);
        snprintf(buf, 64, "Operand types for %s are incompatible: %s %s %s", op_str, left_str, op_str, right_str);
        compiler->report_error(buf);
    }
}

void invalid_assignment_to_non_variable(FXVM_Compiler *compiler, FXVM_Ast *node)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        snprintf(buf, 64, "Invalid assignment to non-varible"); (void)node;
        compiler->report_error(buf);
    }
}

void invalid_assignment_of_varible_with_new_type(FXVM_Compiler *compiler,
        const char *sym_start, const char *sym_end,
        FXVM_Type original_type, FXVM_Type new_type)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        char sym_buf[32];

        int max_len = (sym_end - sym_start > 31) ? 31 : (sym_end - sym_start);
        memcpy(sym_buf, sym_start, max_len);
        sym_buf[max_len] = '\0';

        const char *orig_str = type_to_string(original_type);
        const char *new_str = type_to_string(new_type);
        snprintf(buf, 64, "Invalid assignment of variable %s. Original type %s, new type %s", sym_buf, orig_str, new_str);
        compiler->report_error(buf);
    }
}

FXVM_Type type_check(FXVM_Compiler *compiler, FXVM_Ast *ast);

FXVM_Type type_check_unary(FXVM_Compiler *compiler, FXVM_Ast *ast)
{
    return ast->type = type_check(compiler, ast->unary.operand);
}

FXVM_Type type_check_assign(FXVM_Compiler *compiler, FXVM_Ast *ast)
{
    FXVM_Ast *left = ast->binary.left;
    if (left->kind != FXAST_EXPR_VARIABLE)
    {
        invalid_assignment_to_non_variable(compiler, left);
        return FXTYP_NONE;
    }

    FXVM_Type type_right = type_check(compiler, ast->binary.right);

    FXVM_Symbols *syms = &compiler->symbols;
    const char *sym_start = left->variable.token->start;
    const char *sym_end = left->variable.token->end;
    int sym_index = symbols_find(syms, sym_start, sym_end);
    if (sym_index != -1)
    {
        FXVM_Type sym_type = syms->types[sym_index];
        if (sym_type != type_right)
        {
            invalid_assignment_of_varible_with_new_type(compiler, sym_start, sym_end, sym_type, type_right);
        }
        else if (syms->sym_types[sym_index] != FXSYM_Variable)
        {
            invalid_assignment_to_non_variable(compiler, left);
        }
    }
    else
    {
        push_symbol(&compiler->symbols, sym_start, sym_end, type_right);
    }

    ast->type = type_right;
    left->type = type_right;
    return type_right;
}

FXVM_Type type_check_binary(FXVM_Compiler *compiler, FXVM_Ast *ast)
{
    switch (ast->binary.op)
    {
    case FXAST_OP_ADD:
    case FXAST_OP_SUB:
        {
            FXVM_Type type_left = type_check(compiler, ast->binary.left);
            FXVM_Type type_right = type_check(compiler, ast->binary.right);
            if ((type_left != FXTYP_NONE && type_right != FXTYP_NONE) && type_left != type_right)
            {
                invalid_operand_types(compiler, ast->binary.op, type_left, type_right);
            }
            return ast->type = type_left;
        }
    case FXAST_OP_MUL:
    case FXAST_OP_DIV:
        {
            FXVM_Type type_left = type_check(compiler, ast->binary.left);
            FXVM_Type type_right = type_check(compiler, ast->binary.right);
            // If no error in typing and neither operand is just scalar, the types must be equal.
            if ((type_left != FXTYP_NONE && type_right != FXTYP_NONE) &&
                (type_left != FXTYP_F1 && type_right != FXTYP_F1))
            {
                if (type_left != type_right)
                {
                    invalid_operand_types(compiler, ast->binary.op, type_left, type_right);
                }
            }
            return ast->type = type_left;
        }
    case FXAST_OP_ASSIGN:
        return type_check_assign(compiler, ast);
    }
    return ast->type = FXTYP_F1;
}

void too_long_floating_point_constant(FXVM_Compiler *compiler)
{
    compiler->error_num++;
    if (compiler->report_error)
    {
        char buf[64];
        snprintf(buf, 64, "Too long floating point number");
        compiler->report_error(buf);
    }
}

FXVM_Type type_check_number(FXVM_Compiler *compiler, FXVM_Ast *ast)
{
    float value = 0.0f;
    const char *start = ast->number.token->start;
    char *end = (char*)ast->number.token->end;
    if (end - start > 31)
    {
        too_long_floating_point_constant(compiler);
    }
    else
    {
        char buf[32];
        int len = end - start;
        strncpy(buf, start, len);
        buf[len] = '\0';
        value = atof(buf);
    }
    ast->number.value = value;
    return ast->type = FXTYP_F1;
}

FXVM_Type type_check_variable_ref(FXVM_Compiler *compiler, FXVM_Ast *ast)
{
    const char *sym_start = ast->variable.token->start;
    const char *sym_end = ast->variable.token->end;
    int sym_index = symbols_find(&compiler->symbols, sym_start, sym_end);
    if (sym_index == -1)
    {
        variable_not_defined(compiler, sym_start, sym_end);
        return FXTYP_NONE;
    }
    else
    {
        return ast->type = compiler->symbols.types[sym_index];
    }
}

FXVM_Type type_check_call(FXVM_Compiler *compiler, FXVM_Ast *ast)
{
    FXVM_Symbols *syms = &compiler->symbols;
    const char *sym_start = ast->call.token->start;
    const char *sym_end = ast->call.token->end;
    int sym_index = symbols_find(syms, sym_start, sym_end);
    if (sym_index == -1)
    {
        function_not_defined(compiler, sym_start, sym_end);
        return FXTYP_NONE;
    }

    FXVM_Type sym_type = syms->types[sym_index];
    if (sym_type != FXTYP_FUNC)
    {
        symbol_not_a_function(compiler, sym_start, sym_end);
        return FXTYP_NONE;
    }

    auto function_type = syms->function_types[sym_index];
    int param_num = function_type.parameter_num;
    if (param_num != ast->call.params.node_num)
    {
        invalid_parameter_count(compiler, sym_start, sym_end, param_num, ast->call.params.node_num);
        return FXTYP_NONE;
    }

    FXVM_Type gen_type = FXTYP_NONE; // The type of generic vectors, if any parameters have such type.
    for (int param_index = 0; param_index < param_num; param_index++)
    {
        FXVM_Ast *param_node = ast->call.params.nodes[param_index];
        FXVM_Type param_type = type_check(compiler, param_node);
        if (param_type == FXTYP_NONE)
        {
            // There already was an error
            return FXTYP_NONE;
        }

        FXVM_Type def_param_type = function_type.parameter_types[param_index];
        if (def_param_type == FXTYP_GENF)
        {
            if (gen_type == FXTYP_NONE)
            {
                gen_type = param_type;
                def_param_type = param_type;
            }
        }

        if (param_type != def_param_type)
        {
            invalid_parameter_type(compiler, sym_start, sym_end, param_index + 1);
            return FXTYP_NONE;
        }
    }

    FXVM_Type ret_type = syms->function_types[sym_index].return_type;
    if (ret_type == FXTYP_GENF)
    {
        ret_type = gen_type;
    }

    return ast->type = ret_type;
}

FXVM_Type type_check_root(FXVM_Compiler *compiler, FXVM_Ast *ast)
{
    FXVM_Type result = FXTYP_NONE;
    for (int i = 0; i < ast->root.node_num; i++)
    {
        result = type_check(compiler, ast->root.nodes[i]);
    }
    return result;
}

FXVM_Type type_check(FXVM_Compiler *compiler, FXVM_Ast *ast)
{
    switch (ast->kind)
    {
        case FXAST_ROOT:            return type_check_root(compiler, ast); break;
        case FXAST_EXPR_UNARY:      return type_check_unary(compiler, ast); break;
        case FXAST_EXPR_BINARY:     return type_check_binary(compiler, ast); break;
        case FXAST_EXPR_NUMBER:     return type_check_number(compiler, ast); break;
        case FXAST_EXPR_VARIABLE:   return type_check_variable_ref(compiler, ast); break;
        case FXAST_EXPR_CALL:       return type_check_call(compiler, ast); break;
    }
    return FXTYP_NONE;
}

bool type_check(FXVM_Compiler *compiler)
{
    type_check(compiler, compiler->ast);
    return compiler->error_num == 0;
}

// INTERMEDIATE LANGUAGE GENERATION

enum FXVM_ILOp
{
    FXIL_LOAD_CONST,
    FXIL_LOAD_INPUT,
    FXIL_MOV,
    FXIL_MOV_X,
    FXIL_MOV_XY,
    FXIL_MOV_XYZ,
    FXIL_MOV_XYZW,
    FXIL_NEG,
    FXIL_ADD,
    FXIL_SUB,
    FXIL_MUL,
    FXIL_DIV,

    FXIL_RCP,
    FXIL_RSQRT,
    FXIL_SQRT,
    FXIL_SIN,
    FXIL_COS,
    FXIL_EXP,
    FXIL_EXP2,
    FXIL_EXP10,
    FXIL_ABS,
    FXIL_MIN,
    FXIL_MAX,
    FXIL_CLAMP01,
    FXIL_CLAMP,
    FXIL_INTERP,
};

struct FXVM_ILInstrInfo
{
    const char *name;
    int reg_operand_num;

};

FXVM_ILInstrInfo il_instr_info[] = {
    [FXIL_LOAD_CONST] = {"FXIL_LOAD_CONST", 1},
    [FXIL_LOAD_INPUT] = {"FXIL_LOAD_INPUT", 1},
    [FXIL_MOV] =        {"FXIL_MOV", 2},
    [FXIL_MOV_X] =      {"FXIL_MOV_X", 2},
    [FXIL_MOV_XY] =     {"FXIL_MOV_XY", 3},
    [FXIL_MOV_XYZ] =    {"FXIL_MOV_XYZ", 4},
    [FXIL_MOV_XYZW] =   {"FXIL_MOV_XYZW", 5},
    [FXIL_NEG] =        {"FXIL_NEG", 2},
    [FXIL_ADD] =        {"FXIL_ADD", 3},
    [FXIL_SUB] =        {"FXIL_SUB", 3},
    [FXIL_MUL] =        {"FXIL_MUL", 3},
    [FXIL_DIV] =        {"FXIL_DIV", 3},
    [FXIL_RCP] =        {"FXIL_RCP", 2},
    [FXIL_RSQRT] =      {"FXIL_RSQRT", 2},
    [FXIL_SQRT] =       {"FXIL_SQRT", 2},
    [FXIL_SIN] =        {"FXIL_SIN", 2},
    [FXIL_COS] =        {"FXIL_COS", 2},
    [FXIL_EXP] =        {"FXIL_EXP", 2},
    [FXIL_EXP2] =       {"FXIL_EXP2", 2},
    [FXIL_EXP10] =      {"FXIL_EXP10", 2},
    [FXIL_ABS] =        {"FXIL_ABS", 2},
    [FXIL_MIN] =        {"FXIL_MIN", 3},
    [FXIL_MAX] =        {"FXIL_MAX", 3},
    [FXIL_CLAMP01] =    {"FXIL_CLAMP01", 2},
    [FXIL_CLAMP] =      {"FXIL_CLAMP", 4},
    [FXIL_INTERP] =     {"FXIL_INTERP", 4},
};

const char* il_op_to_string(FXVM_ILOp op)
{
    return il_instr_info[op].name;
}

struct FXIL_Reg
{
    int index;
    //FXVM_Type type;
};

struct FXIL_Operands {
    FXIL_Reg ops[5];
};

struct FXIL_ConstantLoad {
    FXIL_Reg target;
    float v[4];
};

struct FXIL_InputLoad {
    FXIL_Reg target;
    int input_index;
};

struct FXVM_ILInstr
{
    FXVM_ILOp op;
    union {
        FXIL_Operands operands;
        FXIL_ConstantLoad constant_load;
        FXIL_InputLoad input_load;
    };
};


void ensure_il_instr_fits(FXVM_ILContext *ctx)
{
    if (ctx->instr_num + 1 > ctx->instr_cap)
    {
        int new_cap = ctx->instr_cap + 32;
        ctx->instructions = (FXVM_ILInstr*)realloc(ctx->instructions, new_cap * sizeof(FXVM_ILInstr));
        ctx->instr_cap = new_cap;
    }
}

void push_il(FXVM_ILContext *ctx, FXVM_ILOp op, FXIL_Reg a, FXIL_Reg b)
{
    ensure_il_instr_fits(ctx);
    int i = ctx->instr_num;
    ctx->instructions[i] = FXVM_ILInstr { op, a, b };
    ctx->instr_num = i + 1;
}

void push_il(FXVM_ILContext *ctx, FXVM_ILOp op, FXIL_Reg a, FXIL_Reg b, FXIL_Reg c)
{
    ensure_il_instr_fits(ctx);
    int i = ctx->instr_num;
    ctx->instructions[i] = FXVM_ILInstr { op, a, b, c };
    ctx->instr_num = i + 1;
}

void push_il_mov_x(FXVM_ILContext *ctx, FXIL_Reg target, FXIL_Reg x)
{
    ensure_il_instr_fits(ctx);
    int i = ctx->instr_num;
    ctx->instructions[i] = FXVM_ILInstr { FXIL_MOV_X, target, x };
    ctx->instr_num = i + 1;
}

void push_il_mov_xy(FXVM_ILContext *ctx, FXIL_Reg target, FXIL_Reg x, FXIL_Reg y)
{
    ensure_il_instr_fits(ctx);
    int i = ctx->instr_num;
    ctx->instructions[i] = FXVM_ILInstr { FXIL_MOV_XY, target, x, y };
    ctx->instr_num = i + 1;
}

void push_il_mov_xyz(FXVM_ILContext *ctx, FXIL_Reg target, FXIL_Reg x, FXIL_Reg y, FXIL_Reg z)
{
    ensure_il_instr_fits(ctx);
    int i = ctx->instr_num;
    ctx->instructions[i] = FXVM_ILInstr { FXIL_MOV_XYZ, target, x, y, z };
    ctx->instr_num = i + 1;
}

void push_il_mov_xyzw(FXVM_ILContext *ctx, FXIL_Reg target, FXIL_Reg x, FXIL_Reg y, FXIL_Reg z, FXIL_Reg w)
{
    ensure_il_instr_fits(ctx);
    int i = ctx->instr_num;
    ctx->instructions[i] = FXVM_ILInstr { FXIL_MOV_XYZW, target, x, y, z, w };
    ctx->instr_num = i + 1;
}

void push_il_load_const(FXVM_ILContext *ctx, FXIL_Reg target, float v)
{
    ensure_il_instr_fits(ctx);
    int i = ctx->instr_num;
    ctx->instructions[i] = FXVM_ILInstr { FXIL_LOAD_CONST, .constant_load = {.target = target, v } };
    ctx->instr_num = i + 1;
}

void push_il_load_const(FXVM_ILContext *ctx, FXIL_Reg target, float *v)
{
    ensure_il_instr_fits(ctx);
    int i = ctx->instr_num;
    ctx->instructions[i] = FXVM_ILInstr { FXIL_LOAD_CONST, .constant_load = {.target = target, v[0], v[1], v[2], v[3] } };
    ctx->instr_num = i + 1;
}

void push_il_load_input(FXVM_ILContext *ctx, FXIL_Reg target, int input_index)
{
    ensure_il_instr_fits(ctx);
    int i = ctx->instr_num;
    ctx->instructions[i] = FXVM_ILInstr { FXIL_LOAD_INPUT, .input_load = {.target = target, input_index} };
    ctx->instr_num = i + 1;
}

FXIL_Reg new_il_reg(FXVM_ILContext *ctx)
{
    return { ctx->reg_index++ };
}

FXIL_Reg generate_expr(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr);

FXIL_Reg generate_plus(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr)
{
    return generate_expr(compiler, ctx, expr->unary.operand);
}

FXIL_Reg generate_negate(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr)
{
    FXIL_Reg operand = generate_expr(compiler, ctx, expr->unary.operand);
    FXIL_Reg result = new_il_reg(ctx);
    push_il(ctx, FXIL_NEG, result, operand);
    return result;
}

FXIL_Reg generate_unary_expr(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr)
{
    switch (expr->unary.op)
    {
    case FXAST_OP_PLUS:  return generate_plus(compiler, ctx, expr);
    case FXAST_OP_MINUS: return generate_negate(compiler, ctx, expr);
        default:
        // ICE
        break;
    }
    return { -1 };
}

FXIL_Reg generate_add(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *add)
{
    FXIL_Reg left = generate_expr(compiler, ctx, add->binary.left);
    FXIL_Reg right = generate_expr(compiler, ctx, add->binary.right);
    FXIL_Reg result = new_il_reg(ctx);
    push_il(ctx, FXIL_ADD, result, left, right);
    return result;
}

FXIL_Reg generate_sub(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *add)
{
    FXIL_Reg left = generate_expr(compiler, ctx, add->binary.left);
    FXIL_Reg right = generate_expr(compiler, ctx, add->binary.right);
    FXIL_Reg result = new_il_reg(ctx);
    push_il(ctx, FXIL_SUB, result, left, right);
    return result;
}

FXIL_Reg generate_mul(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *add)
{
    FXIL_Reg left = generate_expr(compiler, ctx, add->binary.left);
    FXIL_Reg right = generate_expr(compiler, ctx, add->binary.right);
    FXIL_Reg result = new_il_reg(ctx);
    push_il(ctx, FXIL_MUL, result, left, right);
    return result;
}

FXIL_Reg generate_div(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *add)
{
    FXIL_Reg left = generate_expr(compiler, ctx, add->binary.left);
    FXIL_Reg right = generate_expr(compiler, ctx, add->binary.right);
    FXIL_Reg result = new_il_reg(ctx);
    push_il(ctx, FXIL_DIV, result, left, right);
    return result;
}

FXIL_Reg generate_variable_target(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr)
{
    FXIL_Reg result = new_il_reg(ctx);
    const char *sym_start = expr->variable.token->start;
    const char *sym_end = expr->variable.token->end;
    int sym_index = symbols_find(&compiler->symbols, sym_start, sym_end);
    if (sym_index == -1)
    {
        sym_index = push_symbol(&compiler->symbols, sym_start, sym_end, expr->type);
    }
    compiler->symbols.additional_data[sym_index].variable_reg = result.index;
    return result;
}

FXIL_Reg generate_assign(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *assign)
{
    FXIL_Reg left = generate_variable_target(compiler, ctx, assign->binary.left);
    FXIL_Reg right = generate_expr(compiler, ctx, assign->binary.right);
    push_il(ctx, FXIL_MOV, left, right);
    return left;
}

FXIL_Reg generate_binary_expr(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr)
{
    switch (expr->binary.op)
    {
    case FXAST_OP_ADD: return generate_add(compiler, ctx, expr);
    case FXAST_OP_SUB: return generate_sub(compiler, ctx, expr);
    case FXAST_OP_MUL: return generate_mul(compiler, ctx, expr);
    case FXAST_OP_DIV: return generate_div(compiler, ctx, expr);
    case FXAST_OP_ASSIGN: return generate_assign(compiler, ctx, expr);
    default:
        // ICE
        break;
    }
    return { -1 };
}

FXIL_Reg generate_variable(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr, int sym_index)
{
    (void)compiler;
    (void)ctx;
    (void)expr;
    //assert(sym_index != -1)
    int variable_reg = compiler->symbols.additional_data[sym_index].variable_reg;
    return { variable_reg };
}

FXIL_Reg generate_constant(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr, int sym_index)
{
    (void)compiler;
    (void)expr;
    FXIL_Reg target = new_il_reg(ctx);
    float *v = compiler->symbols.additional_data[sym_index].constant; // TODO: here we assume constant of 4-wide
    push_il_load_const(ctx, target, v);
    return target;
}

FXIL_Reg generate_input_variable(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr, int sym_index)
{
    (void)expr;
    FXIL_Reg target = new_il_reg(ctx);
    int input_index = compiler->symbols.additional_data[sym_index].input_index;
    push_il_load_input(ctx, target, input_index);
    return target;
}

FXIL_Reg generate_variable_expr(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr)
{
    int sym_index = symbols_find(&compiler->symbols, expr->variable.token->start, expr->variable.token->end);
    switch (compiler->symbols.sym_types[sym_index])
    {
    case FXSYM_Variable:        return generate_variable(compiler, ctx, expr, sym_index);
    case FXSYM_BuiltinConstant: return generate_constant(compiler, ctx, expr, sym_index);
    case FXSYM_InputVariable:   return generate_input_variable(compiler, ctx, expr, sym_index);
    case FXSYM_BuiltinFunction:
        // ICE
        break;
    }
    return { -1 };
}

FXIL_Reg generate_number(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr)
{
    (void)compiler;
    FXIL_Reg result = new_il_reg(ctx);
    push_il_load_const(ctx, result, expr->number.value);
    return result;
}

template <FXVM_ILOp OP>
FXIL_Reg emit_single_param_func(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{
    FXIL_Reg result = new_il_reg(ctx);
    FXIL_Reg param = generate_expr(compiler, ctx, call->call.params.nodes[0]);
    push_il(ctx, OP, result, param);
    return result;
}

FXIL_Reg emit_rcp(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_RCP>(compiler, ctx, call); }

FXIL_Reg emit_rsqrt(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_RSQRT>(compiler, ctx, call); }

FXIL_Reg emit_sqrt(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_SQRT>(compiler, ctx, call); }

FXIL_Reg emit_sin(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_SIN>(compiler, ctx, call); }

FXIL_Reg emit_cos(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_COS>(compiler, ctx, call); }

FXIL_Reg emit_exp(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_EXP>(compiler, ctx, call); }

FXIL_Reg emit_exp2(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_EXP2>(compiler, ctx, call); }

FXIL_Reg emit_exp10(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_EXP10>(compiler, ctx, call); }

FXIL_Reg emit_abs(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_ABS>(compiler, ctx, call); }

FXIL_Reg emit_min(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_MIN>(compiler, ctx, call); }

FXIL_Reg emit_max(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_MAX>(compiler, ctx, call); }

FXIL_Reg emit_clamp01(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_CLAMP01>(compiler, ctx, call); }

FXIL_Reg emit_clamp(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_CLAMP>(compiler, ctx, call); }

FXIL_Reg emit_lerp(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{ return emit_single_param_func<FXIL_INTERP>(compiler, ctx, call); }

FXIL_Reg emit_vec4(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{
    FXIL_Reg result = new_il_reg(ctx);
    FXVM_Ast *param0 = call->call.params.nodes[0];
    FXVM_Ast *param1 = call->call.params.nodes[1];
    FXVM_Ast *param2 = call->call.params.nodes[2];
    FXVM_Ast *param3 = call->call.params.nodes[3];
    if ((param0->kind == FXAST_EXPR_NUMBER) &&
        (param1->kind == FXAST_EXPR_NUMBER) &&
        (param2->kind == FXAST_EXPR_NUMBER) &&
        (param3->kind == FXAST_EXPR_NUMBER))
    {
        float v[4] = { param0->number.value, param1->number.value, param2->number.value, param3->number.value };
        push_il_load_const(ctx, result, v);
    }
    else
    {
        FXIL_Reg x = generate_expr(compiler, ctx, param0);
        FXIL_Reg y = generate_expr(compiler, ctx, param1);
        FXIL_Reg z = generate_expr(compiler, ctx, param2);
        FXIL_Reg w = generate_expr(compiler, ctx, param3);
        push_il_mov_xyzw(ctx, result, x, y, z, w);
    }
    return result;
}

FXIL_Reg emit_vec3(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{
    FXIL_Reg result = new_il_reg(ctx);
    FXVM_Ast *param0 = call->call.params.nodes[0];
    FXVM_Ast *param1 = call->call.params.nodes[1];
    FXVM_Ast *param2 = call->call.params.nodes[2];
    if ((param0->kind == FXAST_EXPR_NUMBER) &&
        (param1->kind == FXAST_EXPR_NUMBER) &&
        (param2->kind == FXAST_EXPR_NUMBER))
    {
        float v[4] = { param0->number.value, param1->number.value, param2->number.value, 0.0f };
        push_il_load_const(ctx, result, v);
    }
    else
    {
        FXIL_Reg x = generate_expr(compiler, ctx, param0);
        FXIL_Reg y = generate_expr(compiler, ctx, param1);
        FXIL_Reg z = generate_expr(compiler, ctx, param2);
        push_il_mov_xyz(ctx, result, x, y, z);
    }
    return result;
}

FXIL_Reg emit_vec2(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *call)
{
    FXIL_Reg result = new_il_reg(ctx);
    FXVM_Ast *param0 = call->call.params.nodes[0];
    FXVM_Ast *param1 = call->call.params.nodes[1];
    if ((param0->kind == FXAST_EXPR_NUMBER) &&
        (param1->kind == FXAST_EXPR_NUMBER))
    {
        float v[4] = { param0->number.value, param1->number.value, 0.0f, 0.0f };
        push_il_load_const(ctx, result, v);
    }
    else
    {
        FXIL_Reg x = generate_expr(compiler, ctx, param0);
        FXIL_Reg y = generate_expr(compiler, ctx, param1);
        push_il_mov_xy(ctx, result, x, y);
    }
    return result;
}

FXIL_Reg generate_call_expr(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *expr)
{
    struct BuiltinInfo
    {
        const char *name;
        FXIL_Reg (*emit)(FXVM_Compiler*, FXVM_ILContext*, FXVM_Ast*);
    };
    const BuiltinInfo builtins[] = {
        {"vec4", emit_vec4},
        {"vec3", emit_vec3},
        {"vec2", emit_vec2},
        {"rcp", emit_rcp},
        {"rsqrt", emit_rsqrt},
        {"sqrt", emit_sqrt},
        {"sin", emit_sin},
        {"cos", emit_cos},
        {"exp", emit_exp},
        {"exp2", emit_exp2},
        {"exp10", emit_exp10},
        {"abs", emit_abs},
        {"min", emit_min},
        {"max", emit_max},
        {"clamp01", emit_clamp01},
        {"clamp", emit_clamp},
        {"lerp", emit_lerp},
    };
    const int builtin_num = sizeof(builtins) / sizeof(BuiltinInfo);

    const char *func_name = expr->call.token->start;
    const char *func_name_end = expr->call.token->end;
    size_t func_name_len = func_name_end - func_name;
    for (int i = 0; i < builtin_num; i++)
    {
        if (string_eq(func_name, func_name_len, builtins[i].name, strlen(builtins[i].name)))
        {
            return builtins[i].emit(compiler, ctx, expr);
        }
    }
    // ICE
    return new_il_reg(ctx);
}

FXIL_Reg generate_expr(FXVM_Compiler *compiler, FXVM_ILContext *ctx, FXVM_Ast *node)
{
    switch (node->kind)
    {
    case FXAST_EXPR_NUMBER:     return generate_number(compiler, ctx, node);
    case FXAST_EXPR_VARIABLE:   return generate_variable_expr(compiler, ctx, node);
    case FXAST_EXPR_UNARY:      return generate_unary_expr(compiler, ctx, node);
    case FXAST_EXPR_BINARY:     return generate_binary_expr(compiler, ctx, node);
    case FXAST_EXPR_CALL:       return generate_call_expr(compiler, ctx, node);
    default:
        // ICE
        break;
    }
    return { -1 };
}

bool generate_il(FXVM_Compiler *compiler)
{
    auto root = compiler->ast->root;
    for (int i = 0; i < root.node_num; i++)
    {
        generate_expr(compiler, &compiler->il_context, root.nodes[i]);
    }
    return true;
}

void register_constant(FXVM_Compiler *compiler, const char *name, float *value, int width)
{
    push_symbol_builtin_constant(&compiler->symbols, name, name + strlen(name), value, width);
}

void register_constant(FXVM_Compiler *compiler, const char *name, float value)
{
    register_constant(compiler, name, &value, 1);
}

int register_builtin_function(FXVM_Compiler *compiler, const char *name, FXVM_Type return_type, int parameter_num)
{
    int sym_index = push_symbol(&compiler->symbols, name, name + strlen(name), FXTYP_FUNC);
    compiler->symbols.function_types[sym_index].parameter_num = parameter_num;
    compiler->symbols.function_types[sym_index].return_type = return_type;
    compiler->symbols.sym_types[sym_index] = FXSYM_BuiltinFunction;
    return sym_index;
}

void set_function_parameter_type(FXVM_Compiler *compiler, int sym_index, int param_index, FXVM_Type param_type)
{
    compiler->symbols.function_types[sym_index].parameter_types[param_index] = param_type;
}

void register_input_variable(FXVM_Compiler *compiler, const char *name, FXVM_Type type)
{
    int sym_index = push_symbol(&compiler->symbols, name, name + strlen(name), type);
    compiler->symbols.additional_data[sym_index].input_index = compiler->symbols.input_index++;
    compiler->symbols.sym_types[sym_index] = FXSYM_InputVariable;
}

bool compile(FXVM_Compiler *compiler, const char *source, const char *source_end)
{
    int vec4_i = register_builtin_function(compiler, "vec4", FXTYP_F4, 4);
    set_function_parameter_type(compiler, vec4_i, 0, FXTYP_F1);
    set_function_parameter_type(compiler, vec4_i, 1, FXTYP_F1);
    set_function_parameter_type(compiler, vec4_i, 2, FXTYP_F1);
    set_function_parameter_type(compiler, vec4_i, 3, FXTYP_F1);

    int vec3_i = register_builtin_function(compiler, "vec3", FXTYP_F3, 3);
    set_function_parameter_type(compiler, vec3_i, 0, FXTYP_F1);
    set_function_parameter_type(compiler, vec3_i, 1, FXTYP_F1);
    set_function_parameter_type(compiler, vec3_i, 2, FXTYP_F1);

    int vec2_i = register_builtin_function(compiler, "vec2", FXTYP_F2, 2);
    set_function_parameter_type(compiler, vec2_i, 0, FXTYP_F1);
    set_function_parameter_type(compiler, vec2_i, 1, FXTYP_F1);

    int sqrt_i = register_builtin_function(compiler, "sqrt", FXTYP_GENF, 1);
    set_function_parameter_type(compiler, sqrt_i, 0, FXTYP_GENF);

    bool result =
        tokenize(compiler, source, source_end) &&
        parse(compiler) &&
        type_check(compiler) &&
        generate_il(compiler) && true;
        //write_bytecode(compile);
    return result;
}

#define FXVM_COMP
#endif
