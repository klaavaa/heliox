#pragma once

#include <string>
#include <variant>
#include <vector>
#include "heliox_pointer.hpp"
#include "heliox_token.hpp"
namespace hx
{

struct int_literal_expr;
struct string_literal_expr;
struct identifier_literal_expr;
struct function_call_expr;
struct binop_expr;
struct unary_expr;

using expression = std::variant<
    uptr<int_literal_expr>,
    uptr<string_literal_expr>,
    uptr<identifier_literal_expr>,
    uptr<function_call_expr>,
    uptr<binop_expr>,
    uptr<unary_expr>
    >;

struct int_literal_expr
{
    int_literal_expr(std::string value)
        : value(value) {}
    std::string value;
};

struct string_literal_expr
{
    string_literal_expr(std::string value, uint32_t label)
        : value(value), label(label) {}
    std::string value;
    uint32_t label;
};
struct identifier_literal_expr
{
    identifier_literal_expr(std::string name)
        : name(name) {}
    std::string name;
};
struct binop_expr
{
    binop_expr(expression left, expression right, tk_type op_token)
        : left(std::move(left)), right(std::move(right)), op_token(op_token) {} 
    expression left;
    expression right;
    tk_type op_token;
};
struct unary_expr
{
    unary_expr(expression expr, tk_type op_token)
        : expr(std::move(expr)), op_token(op_token) {}
    expression expr;
    tk_type op_token;
};
struct function_call_expr
{
    function_call_expr(uptr<identifier_literal_expr> identifier,
            std::vector<expression> parameters, uint32_t label)
        : identifier(std::move(identifier)),
          parameters(std::move(parameters)),
          label(label) {}
    uptr<identifier_literal_expr> identifier;
    std::vector<expression> parameters;
    uint32_t label;
};
}

