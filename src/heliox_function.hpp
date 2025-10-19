#pragma once
#include "heliox_expression.hpp"
#include "heliox_pointer.hpp"
#include "heliox_statement.hpp"
#include <vector>

namespace hx
{

struct function 
{
    function(uptr<identifier_literal_expr> identifier, 
            std::vector<uptr<variable_declaration_statement>> params,
            std::vector<statement> statements, type_data type, bool is_extern)
        : identifier(std::move(identifier)),
          params(std::move(params)),
          statements(std::move(statements)), 
          type(type),
          is_extern(is_extern) {}

    uptr<identifier_literal_expr> identifier;
    std::vector<uptr<variable_declaration_statement>> params;
    std::vector<statement> statements;
    type_data type;
    bool is_extern;


};
}

