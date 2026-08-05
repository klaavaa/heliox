#pragma once

#include <utility>
#include <vector>
#include "typedefs.hpp"
#include "heliox_expression.hpp"
#include "heliox_types.hpp"
#include <variant>

namespace hx {

    struct compound_statement;
    struct return_statement;
    struct variable_definition_statement;
    struct variable_declaration_statement;
    struct conditional_statement;
    struct while_statement;
    struct for_statement;
    struct expression_statement;
    struct noop_statement;
    struct break_statement;
    struct continue_statement;
    struct asm_statement;
    struct function_statement;
    struct struct_statement;

    using statement = std::variant<
        uptr<compound_statement>,
        uptr<return_statement>,
        uptr<variable_definition_statement>,
        uptr<variable_declaration_statement>,
        uptr<conditional_statement>,
        uptr<while_statement>,
        uptr<expression_statement>,
        uptr<noop_statement>,
        uptr<for_statement>,
        uptr<break_statement>,
        uptr<continue_statement>,
        uptr<asm_statement>,
        uptr<function_statement>,
        uptr<struct_statement>
        >;
    
    struct asm_statement : ast_node 
    {
        asm_statement(std::string_view filename, uint32_t line, uint32_t position, 
                std::vector<uptr<string_literal_expr>> _clobbered_registers, 
                uptr<string_literal_expr> _asm_body)
            : 
            ast_node(filename, line, position),
            clobbered_registers(std::move(_clobbered_registers)),
            asm_body(std::move(_asm_body))
        {}
        
        std::vector<uptr<string_literal_expr>> clobbered_registers;
        uptr<string_literal_expr> asm_body;
    };

    struct return_statement : ast_node
    {
        return_statement(std::string_view filename, uint32_t line, uint32_t position,expression return_expression)
            : 
            ast_node(filename, line, position),
            return_expression(std::move(return_expression)) {}
        expression return_expression;
        Symbol* symbol;
    };

    
    struct variable_declaration_statement : ast_node
    {
        variable_declaration_statement(
            std::string_view filename, uint32_t line, uint32_t position, const std::string& _var_name, Type _var_type)
            : ast_node(filename, line, position), var_name(_var_name), var_type(_var_type) {}

        std::string var_name;
        Type var_type;
        Symbol* symbol;
    };

    struct variable_definition_statement : ast_node
    {
        variable_definition_statement(
            std::string_view filename, uint32_t line, uint32_t position, uptr<variable_declaration_statement> _declaration, expression _definition)
            : ast_node(filename, line, position), declaration(std::move(_declaration)), definition(std::move(_definition)) {}

        uptr<variable_declaration_statement> declaration;
        expression definition;
    };

    struct conditional_statement : ast_node
    {
        conditional_statement(std::string_view filename, uint32_t line, uint32_t position, expression condition, statement then_stat, statement else_stat)
            : ast_node(filename, line, position), condition(std::move(condition)), then_stat(std::move(then_stat)), else_stat(std::move(else_stat)) {}
        
        expression condition;
        statement then_stat;
        statement else_stat;
    };
    struct while_statement : ast_node
    {
        while_statement(std::string_view filename, uint32_t line, uint32_t position, expression condition, statement loop)
            : ast_node(filename, line, position), condition(std::move(condition)), loop(std::move(loop)) {}

        expression condition;
        statement loop;
    };
    struct for_statement : ast_node
    {
        for_statement(std::string_view filename, uint32_t line, uint32_t position, statement init, expression condition, expression iteration, statement loop)
            : ast_node(filename, line, position), init(std::move(init)), condition(std::move(condition)), iteration(std::move(iteration)), loop(std::move(loop)) {}

        statement init;
        expression condition;
        expression iteration;
        statement loop;
    };
    struct expression_statement : ast_node
    {
        expression_statement(std::string_view filename, uint32_t line, uint32_t position, expression expr)
            : ast_node(filename, line, position), expr(std::move(expr)) {}
        expression expr;
    };
    struct noop_statement : ast_node
    {
        noop_statement(std::string_view filename, uint32_t line, uint32_t position)
            : ast_node(filename, line, position) {}
    };
    struct compound_statement : ast_node
    {
        compound_statement(std::string_view filename, uint32_t line, uint32_t position, std::vector<statement> statements)
            : ast_node(filename, line, position), statements(std::move(statements)) {}
        std::vector<statement> statements;
    };
    struct break_statement : ast_node
    {
        break_statement(std::string_view filename, uint32_t line, uint32_t position)
            : ast_node(filename, line, position) {}
    };
    struct continue_statement : ast_node
    {
        continue_statement(std::string_view filename, uint32_t line, uint32_t position)
            : ast_node(filename, line, position) {}
    };

    struct function_statement : ast_node 
    {
        function_statement(std::string_view filename, uint32_t line, uint32_t position, std::string name, 
                std::vector<uptr<variable_declaration_statement>> params,
                    std::vector<statement> statements, Type return_type,
                    bool is_extern, bool has_varargs)
            : ast_node(filename, line, position),
              name(name),
              params(std::move(params)),
              statements(std::move(statements)), 
              return_type(return_type),
              is_extern(is_extern),
              has_varargs(has_varargs)
              {}

        std::string name;
        std::vector<uptr<variable_declaration_statement>> params;
        std::vector<statement> statements;
        Type return_type;
        bool is_extern;
        bool has_varargs;
        Symbol* symbol;
    };

    struct struct_statement : ast_node
    {
        struct_statement(std::string_view filename, uint32_t line, uint32_t position, std::string name, std::vector<uptr<variable_declaration_statement>> fields)
            :
            ast_node(filename, line, position),
            name(name),
            fields(std::move(fields))
        {}

        std::string name;
        std::vector<uptr<variable_declaration_statement>> fields;
        Symbol* symbol;
    };
}
