#pragma once

#include <utility>
#include <vector>
#include "heliox_pointer.hpp"
#include "heliox_types.hpp"
#include "heliox_expression.hpp"
#include <variant>

namespace hx {

    struct compound_statement;
    struct return_statement;
    struct variable_declaration_statement;
    struct variable_definition_statement;
    struct conditional_statement;
    struct while_statement;
    struct expression_statement;
    struct noop_statement;


    using statement = std::variant<
        uptr<compound_statement>,
        uptr<return_statement>,
        uptr<variable_declaration_statement>,
        uptr<variable_definition_statement>,
        uptr<conditional_statement>,
        uptr<while_statement>,
        uptr<expression_statement>,
        uptr<noop_statement>
            >;

    
    struct return_statement
    {
        return_statement(expression return_expression)
            : return_expression(std::move(return_expression)) {}
        expression return_expression;
    };
    struct variable_declaration_statement
    {
        variable_declaration_statement(type_data var_type,
                uptr<identifier_literal_expr> var_identifier)
            : var_type(var_type), var_identifier(std::move(var_identifier)) {}
        type_data var_type;
        uptr<identifier_literal_expr> var_identifier;
    };
    
    struct variable_definition_statement
    {
        variable_definition_statement(
            uptr<variable_declaration_statement> declaration, expression definition)
            : declaration(std::move(declaration)), definition(std::move(definition)) {}

        uptr<variable_declaration_statement> declaration;
        expression definition;
    };

    struct conditional_statement
    {
        conditional_statement(expression condition, statement then_stat, statement else_stat)
            : condition(std::move(condition)), then_stat(std::move(then_stat)),
            else_stat(std::move(else_stat)) {}
        
        expression condition;
        statement then_stat;
        statement else_stat;
    };
    struct while_statement
    {
        while_statement(expression condition, statement loop)
            : condition(std::move(condition)), loop(std::move(loop)) {}

        expression condition;
        statement loop;
    };
    struct expression_statement
    {
        expression_statement(expression expr)
            : expr(std::move(expr)) {}
        expression expr;
    };
    struct noop_statement
    {
        noop_statement() = default;
    };
    struct compound_statement
    {
        compound_statement(std::vector<statement> statements)
            : statements(std::move(statements)) {}
        std::vector<statement> statements;
    };
}
