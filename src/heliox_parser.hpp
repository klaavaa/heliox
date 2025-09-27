#pragma once
#include <vector>
#include <memory>

#include "heliox_lexer.hpp"
#include "heliox_statement.hpp"
#include "heliox_pointer.hpp"


class hx_parser
{

	

public:
	hx_parser(hx_lexer* lexer, hx_sptr<hx_error> error);
	hx_sptr<hx_program> parse(hx_sptr<hx_error> error);

private:	
	
	void eat(tk_type token_type, hx_sptr<hx_error> error);
	hx_sptr<hx_function> parse_function(hx_sptr<hx_error> error);
	hx_sptr<hx_statement> parse_statement(hx_sptr<hx_error> error);
	hx_sptr<hx_compound_statement> parse_compound_statement(hx_sptr<hx_error> error);
	hx_sptr<hx_statement> parse_keyword_statement(hx_sptr<hx_error> error);
	hx_sptr<hx_return_statement> parse_return_statement(hx_sptr<hx_error> error);
	hx_sptr<hx_conditional_statement> parse_if_statement(hx_sptr<hx_error> error);
	hx_sptr<hx_while_statement> parse_while_statement(hx_sptr<hx_error> error);
	hx_sptr<hx_definition_statement> parse_definition_statement(hx_sptr<hx_error> error);
	hx_sptr<hx_type_decl_statement> parse_type_decl_statement(hx_sptr<hx_error> error);
	hx_sptr<hx_expression_statement> parse_expression_statement(hx_sptr<hx_error> error);
	hx_sptr<hx_expression> parse_primary(hx_sptr<hx_error> error);
	hx_sptr<hx_expression> parse_expression(hx_sptr<hx_error> error, hx_sptr<hx_expression> lhs, uint32_t depth, uint32_t precedence=0);
	hx_sptr<hx_unary_expression> parse_unary_expression(hx_sptr<hx_error> error);
	hx_sptr<hx_expression> parse_identifier_expression(hx_sptr<hx_error> error);
	hx_sptr<hx_int_literal_expression> parse_int_literal_expression(hx_sptr<hx_error> error);
	hx_sptr<hx_string_literal_expression> parse_string_literal_expression(hx_sptr<hx_error> error);
	hx_sptr<hx_identifier_literal_expression> parse_identifier_literal_expression(hx_sptr<hx_error> error);
	hx_sptr<hx_expression> copy_expression(hx_sptr<hx_expression> expr);
private:

	hx_token token;
	hx_lexer* lexer;
};