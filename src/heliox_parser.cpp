#include "heliox_parser.hpp"

hx_parser::hx_parser(hx_lexer* lexer, hx_sptr<hx_error> error)
	:
	lexer(lexer),
	token(tk_type::TK_EOF, "")
{
	token = lexer->get_next(error);
}

hx_sptr<hx_program> hx_parser::parse(hx_sptr<hx_error> error)
{

	hx_sptr<hx_program> program(make_shared<hx_program>());

	while (token.type == TK_KEYWORD)
	{
		hx_kwords keyword = get_kword_from_string(token.value).keyword;
		if (keyword != hx_kwords::FUNC)
		{
			error->ok = false;
			error->line = lexer->get_line();

			error->info = "Unexpected token: " + token.value;
			error->error_type = HX_SYNTAX_ERROR;
			return nullptr;
		}

		program->functions.push_back(parse_function(error));

	}
	
	if (!(lexer->is_finished()))
	{
		error->ok = false;
		error->line = lexer->get_line();
		error->info = "Unexpected token: " + token.value;
		error->error_type = HX_SYNTAX_ERROR;
		return nullptr;
	}

	return program;

}

void hx_parser::eat(tk_type token_type, hx_sptr<hx_error> error)
{

	if (token.type != token_type)
	{
		error->ok = false;
		error->line = lexer->get_line();
		std::string value = token.value;
		if (token.value.size() == 0)
			value = tk_type_to_str::get_str(token.type);
		
		error->info = "Expected \"" + std::string(tk_type_to_str::get_str(token_type)) + "\", got \"" + value + "\"";
		error->error_type = HX_SYNTAX_ERROR;
		
		//exit(1);
	}

	
	token = lexer->get_next(error);
}

hx_sptr<hx_function> hx_parser::parse_function(hx_sptr<hx_error> error)
{
	hx_sptr<hx_function> function(make_shared<hx_function>());

	eat(TK_KEYWORD, error);

	function->name = token.value;
	eat(TK_IDENTIFIER, error);

	eat(TK_L_PAREN, error);

	while (token.type != TK_R_PAREN)
	{
		hx_sptr<hx_type_decl_statement> type_decl_statement = parse_type_decl_statement(error);
		function->parameters.push_back(type_decl_statement);
		if (token.type != TK_R_PAREN)
			eat(TK_COMMA, error);
	}
	eat(TK_R_PAREN, error);


	if (token.type == TK_ARROW)
	{
		eat(TK_ARROW, error);

		function->return_type = token.value;
		eat(TK_KEYWORD, error);
	}

	else
	{
		function->return_type = get_string_from_kword(hx_kwords::VOID);
	}


	function->statement = parse_statement(error);

	return function;
}

hx_sptr<hx_statement> hx_parser::parse_statement(hx_sptr<hx_error> error)
{

	switch (token.type)
	{
	case TK_L_BRACE: return parse_compound_statement(error);
	case TK_KEYWORD: return parse_keyword_statement(error);

	case TK_SEMICOLON:
	{
		eat(TK_SEMICOLON, error);
		return make_shared<hx_noop_statement>();
	}

	default:
		return parse_expression_statement(error);
	}


}

hx_sptr<hx_compound_statement> hx_parser::parse_compound_statement(hx_sptr<hx_error> error)
{
	hx_sptr<hx_compound_statement> compound_statement(make_shared<hx_compound_statement>());
	eat(TK_L_BRACE, error);
	while (token.type != TK_R_BRACE)
	{
		compound_statement->statements.push_back(parse_statement(error));
	}
	eat(TK_R_BRACE, error);

	return compound_statement;
}

hx_sptr<hx_statement> hx_parser::parse_keyword_statement(hx_sptr<hx_error> error)
{

	hx_keyword keyword = get_kword_from_string(token.value);


	if (keyword.type == hx_keyword::TYPE_DECL)
		return parse_definition_statement(error);

	switch (keyword.keyword)
	{
	case hx_kwords::RETURN: return parse_return_statement(error);
	}
}

hx_sptr<hx_return_statement> hx_parser::parse_return_statement(hx_sptr<hx_error> error)
{
	eat(TK_KEYWORD, error);
	hx_sptr<hx_return_statement> return_statement(make_shared<hx_return_statement>());
	return_statement->expression = parse_expression(error);
	eat(TK_SEMICOLON, error);
	return return_statement;
}

hx_sptr<hx_definition_statement> hx_parser::parse_definition_statement(hx_sptr<hx_error> error)
{
	hx_sptr<hx_definition_statement> definition_statement(make_shared<hx_definition_statement>());
	definition_statement->type_decl = parse_type_decl_statement(error);

	eat(TK_EQU, error);

	definition_statement->expression = parse_expression(error);

	eat(TK_SEMICOLON, error);

	return definition_statement;
}

hx_sptr<hx_type_decl_statement> hx_parser::parse_type_decl_statement(hx_sptr<hx_error> error)
{
	hx_sptr<hx_type_decl_statement> type_decl_statement(make_shared<hx_type_decl_statement>());
	type_decl_statement->type = token.value;
	eat(TK_KEYWORD, error);

	if (token.type == TK_L_BRACK)
	{
		eat(TK_L_BRACK, error);
		eat(TK_R_BRACK, error);
	}


	if (token.type == TK_IDENTIFIER)
	{
		type_decl_statement->name = token.value;
		eat(TK_IDENTIFIER, error);
	}

	return type_decl_statement;
}

hx_sptr<hx_expression_statement> hx_parser::parse_expression_statement(hx_sptr<hx_error> error)
{
	hx_sptr<hx_expression_statement> expression_statement(make_shared<hx_expression_statement>());
	expression_statement->expression = parse_expression(error);

	eat(TK_SEMICOLON, error);

	return expression_statement;
}

hx_sptr<hx_expression> hx_parser::parse_factor(hx_sptr<hx_error> error)
{
	switch (token.type)
	{
	case TK_INTEGER:	return parse_int_literal_expression(error);
	case TK_IDENTIFIER:	return parse_identifier_expression(error);
	case TK_L_PAREN:
	{
		eat(TK_L_PAREN, error);
		hx_sptr<hx_expression> expression = parse_expression(error);
		eat(TK_R_PAREN, error);
		return expression;
	}
	}
}

hx_sptr<hx_expression> hx_parser::parse_term(hx_sptr<hx_error> error)
{
	hx_sptr<hx_expression> expression = parse_factor(error);
	hx_sptr<hx_binop_expression> binop_expression(make_shared<hx_binop_expression>());

	bool return_binop = false;
	if (token.type == TK_MULTIPLY || token.type == TK_DIVIDE)
	{
		tk_type tok_type = token.type;
		eat(token.type, error);

		hx_sptr<hx_binop_expression> left_binop(make_shared<hx_binop_expression>());

		left_binop->left = expression;
		left_binop->op = tk_type_to_str::get_str(tok_type);
		left_binop->right = parse_factor(error);

		binop_expression = left_binop;

		return_binop = true;
		
	}
	while (token.type == TK_MULTIPLY || token.type == TK_DIVIDE)
	{
		tk_type tok_type = token.type;
		eat(token.type, error);

		hx_sptr<hx_binop_expression> left_binop(make_shared<hx_binop_expression>());

		left_binop->left = binop_expression;
		left_binop->op = tk_type_to_str::get_str(tok_type);
		left_binop->right = parse_factor(error);

		binop_expression = left_binop;

	}
	if (return_binop)
		return binop_expression;
	return expression;
}

hx_sptr<hx_expression> hx_parser::parse_expression(hx_sptr<hx_error> error)
{
	hx_sptr<hx_expression> expression = parse_term(error);
	hx_sptr<hx_binop_expression> binop_expression(make_shared<hx_binop_expression>());

	bool return_binop = false;

	if (token.type == TK_PLUS || token.type == TK_MINUS)
	{
		tk_type tok_type = token.type;
		eat(token.type, error);

		hx_sptr<hx_binop_expression> left_binop(make_shared<hx_binop_expression>());

		left_binop->left = expression;
		left_binop->op = tk_type_to_str::get_str(tok_type);
		left_binop->right = parse_term(error);

		binop_expression = left_binop;

		return_binop = true;

	}
	while (token.type == TK_PLUS || token.type == TK_MINUS)
	{
		tk_type tok_type = token.type;
		eat(token.type, error);

		hx_sptr<hx_binop_expression> left_binop(make_shared<hx_binop_expression>());

		left_binop->left = binop_expression;
		left_binop->op = tk_type_to_str::get_str(tok_type);
		left_binop->right = parse_term(error);

		binop_expression = left_binop;
	}
	if (return_binop)
		return binop_expression;
	return expression;
}

hx_sptr<hx_expression> hx_parser::parse_identifier_expression(hx_sptr<hx_error> error)
{
	std::string name = token.value;


	hx_sptr<hx_identifier_literal_expression> identifier_literal_expression(make_shared<hx_identifier_literal_expression>());
	identifier_literal_expression->name = name;
	eat(TK_IDENTIFIER, error);

	if (token.type == TK_L_PAREN)		// if there is a '(' after identifier then it must be a function call
	{

		hx_sptr<hx_function_call_expression> function_call_expression(make_shared<hx_function_call_expression>());
		function_call_expression->identifier = identifier_literal_expression;

		eat(TK_L_PAREN, error);


		while (token.type != TK_R_PAREN)
		{
			function_call_expression->arguments.push_back(parse_expression(error));

			if (token.type != TK_R_PAREN)
				eat(TK_COMMA, error);
		}

		eat(TK_R_PAREN, error);

		return function_call_expression;
	}

	return identifier_literal_expression;
}

hx_sptr<hx_int_literal_expression> hx_parser::parse_int_literal_expression(hx_sptr<hx_error> error)
{
	hx_sptr<hx_int_literal_expression> int_literal_expression(make_shared<hx_int_literal_expression>());
	int_literal_expression->value = std::atoi(token.value.c_str());

	eat(TK_INTEGER, error);

	return int_literal_expression;
}

hx_sptr<hx_string_literal_expression> hx_parser::parse_string_literal_expression(hx_sptr<hx_error> error)
{
	hx_sptr<hx_string_literal_expression> string_literal_expression(make_shared<hx_string_literal_expression>());
	string_literal_expression->literal = token.value;
	eat(TK_STRING, error);

	return string_literal_expression;
}

hx_sptr<hx_identifier_literal_expression> hx_parser::parse_identifier_literal_expression(hx_sptr<hx_error> error)
{
	hx_sptr<hx_identifier_literal_expression> identifier_literal_expression(make_shared<hx_identifier_literal_expression>());
	identifier_literal_expression->name = token.value;

	eat(TK_IDENTIFIER, error);

	return identifier_literal_expression;
}

