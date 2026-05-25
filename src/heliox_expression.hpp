#pragma once

#include <string>
#include <variant>
#include <vector>
#include "heliox_pointer.hpp"
#include "heliox_types.hpp"
#include "heliox_token.hpp"
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

using expression = std::variant<
    uptr<int_literal_expr>,
    uptr<float_literal_expr>,
    uptr<string_literal_expr>,
    uptr<identifier_literal_expr>,
    uptr<function_call_expr>,
    uptr<binop_expr>,
    uptr<unary_expr>,
    uptr<explicit_conversion_expr>
    >;


struct int_literal_expr
{
    int_literal_expr(std::string value)
        : value(value) {}
    std::string value;
};

struct float_literal_expr
{
    float_literal_expr(std::string value)
        : value(value) {}
    std::string value;
};

struct string_literal_expr
{
    string_literal_expr(std::string value)
        : value(value) {}
    std::string value;
};
struct identifier_literal_expr
{
    identifier_literal_expr(std::string name, std::vector<std::string> module_path = {})
        : name(name), module_path(std::move(module_path)) {}
    std::string name;
    std::vector<std::string> module_path;

    std::string get_full_name() const
    {
        std::string full_name;
        for (const auto& module : module_path)
        {
            full_name += module + "::";
        }
        full_name += name;
        return full_name;
    }
};
struct binop_expr
{
    binop_expr(expression left, expression right, TokenType op_token)
        : left(std::move(left)), right(std::move(right)), op_token(op_token) {} 
    expression left;
    expression right;
    TokenType op_token;
};
struct unary_expr
{
    unary_expr(expression expr, TokenType op_token)
        : expr(std::move(expr)), op_token(op_token) {}
    expression expr;
    TokenType op_token;
};
struct function_call_expr
{
    function_call_expr(uptr<identifier_literal_expr> _identifier,
            std::vector<expression> parameters, std::vector<std::string> in_module)
        : identifier(std::move(_identifier)),
          parameters(std::move(parameters)),
          in_module(std::move(in_module)) 
          {
            find_in_parent_modules = identifier->module_path.empty();
          }
    uptr<identifier_literal_expr> identifier;
    std::vector<expression> parameters;
    std::vector<std::string> in_module;
    bool find_in_parent_modules;
};

struct explicit_conversion_expr
{
    explicit_conversion_expr(type_data type_info, expression expr)
        : type_info(type_info), expr(std::move(expr)) {}

    type_data type_info;
    expression expr;
};

}

