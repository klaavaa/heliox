#pragma once

#include <string>
#include <variant>
#include <vector>
#include "heliox_pointer.hpp"
#include "heliox_types.hpp"
#include "heliox_token.hpp"
#include "heliox_ast_node.hpp"
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
    identifier_literal_expr(std::string_view filename, uint32_t line, uint32_t position, std::string name, std::vector<std::string> module_path = {})
        : ast_node(filename, line, position), name(name), module_path(std::move(module_path)) {}
    std::string name;
    std::vector<std::string> module_path;
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
    function_call_expr(std::string_view filename, uint32_t line, uint32_t position, uptr<identifier_literal_expr> _identifier,
            std::vector<expression> parameters, std::vector<std::string> _in_module)
        : ast_node(filename, line, position), identifier(std::move(_identifier)),
          parameters(std::move(parameters)),
          in_module(std::move(_in_module)) 
          {
            find_in_parent_modules = identifier->module_path.empty();
            if (!find_in_parent_modules)
                in_module = identifier->module_path; 
          }
    uptr<identifier_literal_expr> identifier;
    std::vector<expression> parameters;
    std::vector<std::string> in_module;
    bool find_in_parent_modules;
};

struct explicit_conversion_expr : ast_node
{
    explicit_conversion_expr(std::string_view filename, uint32_t line, uint32_t position, type_data type_info, expression expr)
        : ast_node(filename, line, position), type_info(type_info), expr(std::move(expr)) {}

    type_data type_info;
    expression expr;
};

struct noop_expression : ast_node
{
    noop_expression(std::string_view filename, uint32_t line, uint32_t position)
        : ast_node(filename, line, position) {}
};

}

