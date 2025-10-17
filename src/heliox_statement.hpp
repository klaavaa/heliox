#pragma once

#include <string>
#include <utility>
#include <vector>
#include <optional>
#include "heliox_keywords.hpp"
#include "heliox_pointer.hpp"
#include "heliox_types.hpp"
#include "heliox_expression.hpp"
#include <variant>

namespace hx {

    struct compound_statement;
    struct return_statement;
    struct variable_declaration_statement;
    struct conditional_statement;
    struct while_statement;
    struct expression_statement;
    struct noop_statement;

    using statement = std::variant<
        uptr<compound_statement>,
        uptr<return_statement>,
        uptr<variable_declaration_statement>,
        uptr<conditional_statement>,
        uptr<while_statement>,
        uptr<expression_statement>,
        uptr<noop_statement>
            >;

    using stat_uptr = uptr<statement>;
    using stat_vector = std::vector<stat_uptr>;

    struct compound_statement
    {
        compound_statement(stat_vector statements)
            : statements(std::move(statements)) {}
        stat_vector statements;
    };
    struct return_statement
    {
        return_statement(expr_uptr return_expression)
            : return_expression(std::move(return_expression)) {}
        expr_uptr return_expression;
    };
    struct variable_declaration_statement
    {
        variable_declaration_statement(type_data var_type,
                uptr<identifier_literal_expr> var_identifier)
            : var_type(var_type), var_identifier(std::move(var_identifier)) {}
        type_data var_type;
        uptr<identifier_literal_expr> var_identifier;
    };
    struct conditional_statement
    {
        conditional_statement(expr_uptr condition, stat_uptr then_stat, stat_uptr else_stat)
            : condition(std::move(condition)), then_stat(std::move(then_stat)),
            else_stat(std::move(else_stat)) {}
        
        expr_uptr condition;
        stat_uptr then_stat;
        stat_uptr else_stat;
    };
    struct while_statement
    {
        while_statement(expr_uptr condition, stat_uptr loop)
            : condition(std::move(condition)), loop(std::move(loop)) {}

        expr_uptr condition;
        stat_uptr loop;
    };
    struct expression_statement
    {
        expression_statement(expr_uptr expr)
            : expr(std::move(expr)) {}
        expr_uptr expr;
    };
    struct noop_statement
    {
        noop_statement() = default;
    };
        /*
struct hx_program
{
	std::vector<hx_sptr<hx_function>> functions;
    std::vector<hx_sptr<hx_extern_statement>> externs;
	void print()
	{

		printf("--PROGRAM START--\n\n");
		for (const auto ext : externs)
		{
			ext->print();
		}
		for (const auto func : functions)
		{
			func->print();
		}

		printf("\n--PROGRAM END--\n");

	}


};
*/
}
