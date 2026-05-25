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
            std::vector<statement> statements, type_data type,
            bool is_extern, bool has_varargs, std::string_view filename, uint32_t line, uint32_t position)
        : identifier(std::move(identifier)),
          params(std::move(params)),
          statements(std::move(statements)), 
          type(type),
          is_extern(is_extern),
          has_varargs(has_varargs),
          filename(filename),
          line_number(line),
          position(position) {}


    std::vector<type_data> get_parameter_type_data() const 
    {
        std::vector<type_data> types;
        for (auto& param : params)
        {
            types.push_back(param->var_type);
        }
        return types;
    }
    uptr<identifier_literal_expr> identifier;
    std::vector<uptr<variable_declaration_statement>> params;
    std::vector<statement> statements;
    type_data type;
    bool is_extern;
    bool has_varargs;
    std::string_view filename;
    uint32_t line_number;
    uint32_t position;
};

struct struct_declaration
{
    struct_declaration(std::vector<uptr<variable_declaration_statement>> fields)
        : fields(std::move(fields)) {}

    std::vector<uptr<variable_declaration_statement>> fields;
};


} // namespace hx

