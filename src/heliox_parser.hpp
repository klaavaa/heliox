#pragma once
#include <vector>
#include <memory>

#include "heliox_lexer.hpp"
#include "heliox_statement.hpp"
#include "heliox_pointer.hpp"


class hx_parser
{

	

public:
	hx_parser(hx_lexer* lexer);
	hx_sptr<hx_program> parse();

private:	
	
	void eat(tk_type token_type);
	hx_sptr<hx_function> parse_function();
	hx_sptr<hx_statement> parse_statement();
	hx_sptr<hx_compound_statement> parse_compound_statement();
	hx_sptr<hx_statement> parse_keyword_statement();
	hx_sptr<hx_return_statement> parse_return_statement();
	hx_sptr<hx_definition_statement> parse_definition_statement();
	hx_sptr<hx_type_decl_statement> parse_type_decl_statement();
	hx_sptr<hx_expression_statement> parse_expression_statement();
	hx_sptr<hx_expression> parse_factor();
	hx_sptr<hx_expression> parse_term();
	hx_sptr<hx_expression> parse_expression();
	hx_sptr<hx_expression> parse_identifier_expression();
	hx_sptr<hx_int_literal_expression> parse_int_literal_expression();
	hx_sptr<hx_string_literal_expression> parse_string_literal_expression();
	hx_sptr<hx_identifier_literal_expression> parse_identifier_literal_expression();

private:

	hx_token token;
	hx_lexer* lexer;
};