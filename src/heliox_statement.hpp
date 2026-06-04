#pragma once

#include <utility>
#include <vector>
#include "heliox_pointer.hpp"
#include "heliox_types.hpp"
#include "heliox_expression.hpp"
#include <variant>
#include <optional>

namespace hx {


    struct compound_statement;
    struct return_statement;
    struct variable_declaration_statement;
    struct variable_definition_statement;
    struct conditional_statement;
    struct while_statement;
    struct for_statement;
    struct expression_statement;
    struct noop_statement;
    struct import_statement;
    struct break_statement;
    struct continue_statement;

    using statement = std::variant<
        uptr<compound_statement>,
        uptr<return_statement>,
        uptr<variable_declaration_statement>,
        uptr<variable_definition_statement>,
        uptr<conditional_statement>,
        uptr<while_statement>,
        uptr<expression_statement>,
        uptr<noop_statement>,
        uptr<import_statement>,
        uptr<for_statement>,
        uptr<break_statement>,
        uptr<continue_statement>
        >;


    struct import_statement : ast_node
    {
        import_statement(std::string_view filename, uint32_t line, uint32_t position,uptr<identifier_literal_expr> module_path)
            : ast_node(filename, line, position), module_path(std::move(module_path)) {}
        
        uptr<identifier_literal_expr> module_path;
    };

    struct return_statement : ast_node
    {
        return_statement(std::string_view filename, uint32_t line, uint32_t position,expression return_expression)
            : 
            ast_node(filename, line, position),
            return_expression(std::move(return_expression)) {}
        expression return_expression;
    };
    struct variable_declaration_statement : ast_node
    {
        variable_declaration_statement(std::string_view filename, uint32_t line, uint32_t position, type_data var_type,
                uptr<identifier_literal_expr> var_identifier)
            : ast_node(filename, line, position), var_type(var_type), var_identifier(std::move(var_identifier)) {}
        type_data var_type;
        uptr<identifier_literal_expr> var_identifier;
    };
    
    struct variable_definition_statement : ast_node
    {
        variable_definition_statement(
            std::string_view filename, uint32_t line, uint32_t position,
            uptr<variable_declaration_statement> declaration, expression definition)
            : ast_node(filename, line, position), declaration(std::move(declaration)), definition(std::move(definition)) {}

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

}
