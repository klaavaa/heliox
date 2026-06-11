#pragma once

#include <string>
#include <variant>
#include <vector>
#include "typedefs.hpp"
#include "heliox_token.hpp"
#include "heliox_ast_node.hpp"
#include "heliox_symbol_table.hpp"

namespace hx
{

struct int_literal_expr;
struct float_literal_expr;
struct string_literal_expr;
struct identifier_literal_expr;
struct function_call_expr;
struct binop_expr;
struct unary_expr;
struct explicit_conversion_expr;
struct noop_expression;

using expression = std::variant<
    uptr<int_literal_expr>,
    uptr<float_literal_expr>,
    uptr<string_literal_expr>,
    uptr<identifier_literal_expr>,
    uptr<function_call_expr>,
    uptr<binop_expr>,
    uptr<unary_expr>,
    uptr<explicit_conversion_expr>,
    uptr<noop_expression>
    >;


struct int_literal_expr : ast_node
{
    int_literal_expr(std::string_view filename, uint32_t line, uint32_t position, std::string value)
        : ast_node(filename, line, position), value(value) {}
    std::string value;
};

struct float_literal_expr : ast_node
{
    float_literal_expr(std::string_view filename, uint32_t line, uint32_t position, std::string value)
        : ast_node(filename, line, position), value(value) {}
    std::string value;
};
struct string_literal_expr : ast_node
{
    string_literal_expr(std::string_view filename, uint32_t line, uint32_t position, std::string value)
        : ast_node(filename, line, position), value(value) {}
    std::string value;
};

struct identifier_literal_expr : ast_node
{
    identifier_literal_expr(std::string_view filename, uint32_t line, uint32_t position, std::string name)
        : ast_node(filename, line, position), name(name) {}
    std::string name;
    Symbol symbol;
};

struct binop_expr : ast_node
{
    binop_expr(std::string_view filename, uint32_t line, uint32_t position, expression left, expression right, TokenType op_token)
        : ast_node(filename, line, position), left(std::move(left)), right(std::move(right)), op_token(op_token) {} 
    expression left;
    expression right;
    TokenType op_token;
};
struct unary_expr : ast_node
{
    unary_expr(std::string_view filename, uint32_t line, uint32_t position, expression expr, TokenType op_token)
        : ast_node(filename, line, position), expr(std::move(expr)), op_token(op_token) {}
    expression expr;
    TokenType op_token;
};
struct function_call_expr : ast_node
{
    function_call_expr(std::string_view filename, uint32_t line, uint32_t position, std::string name, std::vector<expression> parameters)
        : ast_node(filename, line, position), name(name),
          parameters(std::move(parameters))
          {}
    std::string name;
    std::vector<expression> parameters;
    Symbol symbol;
};

struct explicit_conversion_expr : ast_node
{
    explicit_conversion_expr(std::string_view filename, uint32_t line, uint32_t position, std::string _type, expression expr)
        : ast_node(filename, line, position), type(_type), expr(std::move(expr)) {}

    std::string type;
    expression expr;
};

struct noop_expression : ast_node
{
    noop_expression(std::string_view filename, uint32_t line, uint32_t position)
        : ast_node(filename, line, position) {}
};

}

