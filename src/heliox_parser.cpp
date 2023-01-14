#include "heliox_parser.hpp"

hx_parser::hx_parser(hx_lexer* lexer)
	:
	lexer(lexer),
	token(tk_type::TK_EOF, "")
{
	token = lexer->get_next();
}

hx_sptr<hx_program> hx_parser::parse()
{

	hx_sptr<hx_program> program(make_shared<hx_program>());

	while (token.type == TK_KEYWORD)
	{
		hx_kwords keyword = get_kword_from_string(token.value).keyword;
		if (keyword != hx_kwords::FUNC)
		{
			printf("ERROR, INVALID KEYWORD, %s", token.value);
			exit(-1);
		}

		program->functions.push_back(parse_function());

	}

	return program;

}

void hx_parser::eat(tk_type token_type)
{

	if (token.type != token_type)
	{
		printf("EXPECTED %s, GOT %s\n", tk_type_to_str::get_str(token_type), tk_type_to_str::get_str(token.type));
		exit(1);
	}

	
	token = lexer->get_next();
}

hx_sptr<hx_function> hx_parser::parse_function()
{
	hx_sptr<hx_function> function(make_shared<hx_function>());

	eat(TK_KEYWORD);

	function->name = token.value;
	eat(TK_IDENTIFIER);

	eat(TK_L_PAREN);

	while (token.type != TK_R_PAREN)
	{
		hx_sptr<hx_type_decl_statement> type_decl_statement = parse_type_decl_statement();
		function->parameters.push_back(type_decl_statement);
		if (token.type != TK_R_PAREN)
			eat(TK_COMMA);
	}
	eat(TK_R_PAREN);


	if (token.type == TK_ARROW)
	{
		eat(TK_ARROW);

		function->return_type = token.value;
		eat(TK_KEYWORD);
	}

	else
	{
		function->return_type = get_string_from_kword(hx_kwords::VOID);
	}


	function->statement = parse_statement();

	return function;
}

hx_sptr<hx_statement> hx_parser::parse_statement()
{

	switch (token.type)
	{
	case TK_L_BRACE: return parse_compound_statement();
	case TK_KEYWORD: return parse_keyword_statement();

	case TK_SEMICOLON:
	{
		eat(TK_SEMICOLON);
		return make_shared<hx_noop_statement>();
	}

	default:
		return parse_expression_statement();
	}


}

hx_sptr<hx_compound_statement> hx_parser::parse_compound_statement()
{
	hx_sptr<hx_compound_statement> compound_statement(make_shared<hx_compound_statement>());
	eat(TK_L_BRACE);
	while (token.type != TK_R_BRACE)
	{
		compound_statement->statements.push_back(parse_statement());
	}
	eat(TK_R_BRACE);

	return compound_statement;
}

hx_sptr<hx_statement> hx_parser::parse_keyword_statement()
{

	hx_keyword keyword = get_kword_from_string(token.value);


	if (keyword.type == hx_keyword::TYPE_DECL)
		return parse_definition_statement();

	switch (keyword.keyword)
	{
	case hx_kwords::RETURN: return parse_return_statement();
	}
}

hx_sptr<hx_return_statement> hx_parser::parse_return_statement()
{
	eat(TK_KEYWORD);
	hx_sptr<hx_return_statement> return_statement(make_shared<hx_return_statement>());
	return_statement->expression = parse_expression();
	eat(TK_SEMICOLON);
	return return_statement;
}

hx_sptr<hx_definition_statement> hx_parser::parse_definition_statement()
{
	hx_sptr<hx_definition_statement> definition_statement(make_shared<hx_definition_statement>());
	definition_statement->type_decl = parse_type_decl_statement();

	eat(TK_EQU);

	definition_statement->expression = parse_expression();

	eat(TK_SEMICOLON);

	return definition_statement;
}

hx_sptr<hx_type_decl_statement> hx_parser::parse_type_decl_statement()
{
	hx_sptr<hx_type_decl_statement> type_decl_statement(make_shared<hx_type_decl_statement>());
	type_decl_statement->type = token.value;
	eat(TK_KEYWORD);

	if (token.type == TK_L_BRACK)
	{
		eat(TK_L_BRACK);
		eat(TK_R_BRACK);
	}


	if (token.type == TK_IDENTIFIER)
	{
		type_decl_statement->name = token.value;
		eat(TK_IDENTIFIER);
	}

	return type_decl_statement;
}

hx_sptr<hx_expression_statement> hx_parser::parse_expression_statement()
{
	hx_sptr<hx_expression_statement> expression_statement(make_shared<hx_expression_statement>());
	expression_statement->expression = parse_expression();

	eat(TK_SEMICOLON);

	return expression_statement;
}

hx_sptr<hx_expression> hx_parser::parse_factor()
{
	switch (token.type)
	{
	case TK_INTEGER:	return parse_int_literal_expression();
	case TK_IDENTIFIER:	return parse_identifier_expression();
	case TK_L_PAREN:
	{
		eat(TK_L_PAREN);
		hx_sptr<hx_expression> expression = parse_expression();
		eat(TK_R_PAREN);
		return expression;
	}
	}
}

hx_sptr<hx_expression> hx_parser::parse_term()
{
	hx_sptr<hx_expression> expression = parse_factor();
	hx_sptr<hx_binop_expression> binop_expression(make_shared<hx_binop_expression>());
	binop_expression->left = expression;
	bool return_binop = false;
	while (token.type == TK_MULTIPLY || token.type == TK_DIVIDE)
	{
		tk_type tok_type = token.type;
		eat(token.type);

		binop_expression->left = expression;
		binop_expression->op = tk_type_to_str::get_str(tok_type);
		binop_expression->right = parse_factor();
		return_binop = true;
	}
	if (return_binop)
		return binop_expression;
	return expression;
}

hx_sptr<hx_expression> hx_parser::parse_expression()
{
	hx_sptr<hx_expression> expression = parse_term();
	hx_sptr<hx_binop_expression> binop_expression(make_shared<hx_binop_expression>());
	binop_expression->left = expression;
	bool return_binop = false;
	while (token.type == TK_PLUS || token.type == TK_MINUS)
	{
		tk_type tok_type = token.type;
		eat(token.type);

		
		binop_expression->left = expression;
		binop_expression->op = tk_type_to_str::get_str(tok_type);
		binop_expression->right = parse_term();
		return_binop = true;
	}
	if (return_binop)
		return binop_expression;
	return expression;
}

hx_sptr<hx_expression> hx_parser::parse_identifier_expression()
{
	std::string name = token.value;


	hx_sptr<hx_identifier_literal_expression> identifier_literal_expression(make_shared<hx_identifier_literal_expression>());
	identifier_literal_expression->name = name;
	eat(TK_IDENTIFIER);

	if (token.type == TK_L_PAREN)		// if there is a '(' after identifier then it must be a function call
	{

		hx_sptr<hx_function_call_expression> function_call_expression(make_shared<hx_function_call_expression>());
		function_call_expression->identifier = identifier_literal_expression;

		eat(TK_L_PAREN);


		while (token.type != TK_R_PAREN)
		{
			function_call_expression->arguments.push_back(parse_expression());

			if (token.type != TK_R_PAREN)
				eat(TK_COMMA);
		}

		eat(TK_R_PAREN);

		return function_call_expression;
	}

	return identifier_literal_expression;
}

hx_sptr<hx_int_literal_expression> hx_parser::parse_int_literal_expression()
{
	hx_sptr<hx_int_literal_expression> int_literal_expression(make_shared<hx_int_literal_expression>());
	int_literal_expression->value = std::atoi(token.value.c_str());

	eat(TK_INTEGER);

	return int_literal_expression;
}

hx_sptr<hx_string_literal_expression> hx_parser::parse_string_literal_expression()
{
	hx_sptr<hx_string_literal_expression> string_literal_expression(make_shared<hx_string_literal_expression>());
	string_literal_expression->literal = token.value;
	eat(TK_STRING);

	return string_literal_expression;
}

hx_sptr<hx_identifier_literal_expression> hx_parser::parse_identifier_literal_expression()
{
	hx_sptr<hx_identifier_literal_expression> identifier_literal_expression(make_shared<hx_identifier_literal_expression>());
	identifier_literal_expression->name = token.value;

	eat(TK_IDENTIFIER);

	return identifier_literal_expression;
}

