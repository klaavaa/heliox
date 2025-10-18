#pragma once
#include "heliox_expression.hpp"
#include "heliox_pointer.hpp"
#include "heliox_statement.hpp"
#include <vector>
#include <format>

namespace hx
{

struct function 
{
    function(uptr<identifier_literal_expr> identifier, 
            std::vector<uptr<variable_declaration_statement>> params,
            statement body, type_data type, bool is_extern)
        : identifier(std::move(identifier)),
          params(std::move(params)),
          body(std::move(body)), 
          type(type),
          is_extern(is_extern) {}

    uptr<identifier_literal_expr> identifier;
    std::vector<uptr<variable_declaration_statement>> params;
    statement body;
    type_data type;
    bool is_extern;


};
}

template <>
struct std::formatter<hx::function> : std::formatter<std::string>
{
    auto format(const hx::function& func, auto& ctx) const
    {
        std::string formatted = std::format("function{}::begin\n", "main");
         
        return std::formatter<std::string>::format(formatted, ctx);
    }

};
