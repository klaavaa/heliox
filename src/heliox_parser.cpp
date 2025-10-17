#include "heliox_parser.hpp"
#include "heliox_keywords.hpp"
#include "heliox_statement.hpp"
#include "heliox_token.hpp"
#include <memory>

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
        switch (keyword)
        {
            case hx_kwords::FUN:
		        program->functions.push_back(parse_function());
                break;
            case hx_kwords::EXTERN:
                program->externs.push_back(parse_extern_statement());
                break;
            default:
                hx_error error;
                error.line = lexer->get_line();

                error.info = "Unexpected token: " + token.value;
                error.error_type = HX_SYNTAX_ERROR;
                hx_logger::log_and_exit(error);
		
        }
		


	}
	
	if (!(lexer->is_finished()))
	{
        hx_error error;
		error.line = lexer->get_line();
		error.info = "Unexpected token: " + token.value;
		error.error_type = HX_SYNTAX_ERROR;
        hx_logger::log_and_exit(error);
	}

	return program;

}

void hx_parser::eat(tk_type token_type)
{

	if (token.type != token_type)
	{
        hx_error error;
		error.line = lexer->get_line();
		std::string value = token.value;
		if (token.value.size() == 0)
			value = tk_type_to_str::get_str(token.type);
		
		error.info = "Expected \"" + std::string(tk_type_to_str::get_str(token_type)) + "\", got \"" + value + "\"";
		error.error_type = HX_SYNTAX_ERROR;
		
        hx_logger::log_and_exit(error);
	}

	
	token = lexer->get_next();
}

hx_sptr<hx_function> hx_parser::parse_function()
{

	hx_sptr<hx_function> function(make_shared<hx_function>());

	function->line_number = lexer->get_line();
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

    function->return_type = token.value;
    eat(TK_KEYWORD);


	function->statement = parse_statement();

	if (function->statement->s_type == statement_type::NOOP)
    {
		function->is_declaration = true;
    }
	else
	{
        function->is_declaration = false;
    }

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
		hx_sptr<hx_noop_statement> noop_stat = make_shared<hx_noop_statement>();
		noop_stat->line_number = lexer->get_line();
		return noop_stat;
	}

	
	

	default:
		return parse_expression_statement();
	}


}

hx_sptr<hx_compound_statement> hx_parser::parse_compound_statement()
{
	hx_sptr<hx_compound_statement> compound_statement(make_shared<hx_compound_statement>());
	compound_statement->line_number = lexer->get_line();
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
	case hx_kwords::IF:		return parse_if_statement();
	case hx_kwords::WHILE:	return parse_while_statement();
	default:
	{
        hx_error error;
		error.error_type = HX_SYNTAX_ERROR;
		error.line = lexer->get_line();
		error.info = "Unexpected token: " + get_string_from_kword(keyword.keyword) + " (possibly missing matching '}')";
		hx_logger::log_and_exit(error);
        return nullptr;
	}
	}
}

hx_sptr<hx_extern_statement> hx_parser::parse_extern_statement()
{
    eat(TK_KEYWORD);
    hx_sptr<hx_extern_statement> extern_statement = make_shared<hx_extern_statement>();
    std::string name = parse_identifier_literal_expression()->name;
    eat(TK_ARROW);
    extern_statement->line_number = lexer->get_line();
    extern_statement->externed_name = name;
    extern_statement->externed_function = parse_function();
    if (!extern_statement->externed_function->is_declaration)
    {
        hx_error error;
		error.error_type = HX_SYNTAX_ERROR;
		error.line = lexer->get_line();
		error.info = "Gave an extern function a body";
        hx_logger::log_and_exit(error);
    } 
     
    return extern_statement;
}

hx_sptr<hx_return_statement> hx_parser::parse_return_statement()
{
	eat(TK_KEYWORD);
	hx_sptr<hx_return_statement> return_statement(make_shared<hx_return_statement>());
	return_statement->line_number = lexer->get_line();
	return_statement->expression = parse_expression(parse_primary(), 0);
	eat(TK_SEMICOLON);
	return return_statement;
}

hx_sptr<hx_conditional_statement> hx_parser::parse_if_statement()
{
	eat(TK_KEYWORD);
	hx_sptr<hx_conditional_statement> conditional_statement(make_shared<hx_conditional_statement>());

	conditional_statement->line_number = lexer->get_line();

	eat(TK_L_PAREN);

	conditional_statement->expression = parse_expression(parse_primary(), 0);
	conditional_statement->expression->line_number = lexer->get_line();
	eat(TK_R_PAREN);

	conditional_statement->statement = parse_statement();
	conditional_statement->statement->line_number = lexer->get_line();

	if (token.type == TK_KEYWORD && token.value == get_string_from_kword(hx_kwords::ELSE))
	{
		eat(TK_KEYWORD);
		conditional_statement->else_statement = parse_statement();
		conditional_statement->else_statement.value()->line_number = lexer->get_line();
	}

	return conditional_statement;
}

hx_sptr<hx_while_statement> hx_parser::parse_while_statement()
{
	eat(TK_KEYWORD);
	hx_sptr<hx_while_statement> while_statement(make_shared<hx_while_statement>());
	while_statement->line_number = lexer->get_line();
	eat(TK_L_PAREN);
	while_statement->expression = parse_expression(parse_primary(), 0);
	while_statement->expression->line_number = lexer->get_line();
	eat(TK_R_PAREN);

	while_statement->statement = parse_statement();
	while_statement->statement->line_number = lexer->get_line();

	return while_statement;
}

hx_sptr<hx_definition_statement> hx_parser::parse_definition_statement()
{
	hx_sptr<hx_definition_statement> definition_statement(make_shared<hx_definition_statement>());
	definition_statement->type_decl = parse_type_decl_statement();
	definition_statement->line_number = lexer->get_line();

	if (token.type == TK_SEMICOLON)
	{
		eat(TK_SEMICOLON);
		definition_statement->is_declaration = false;
		return definition_statement;
	}
	definition_statement->is_declaration = true;

	eat(TK_EQU);

	definition_statement->expression = parse_expression(parse_primary(), 0);

	eat(TK_SEMICOLON);

	return definition_statement;
}

hx_sptr<hx_type_decl_statement> hx_parser::parse_type_decl_statement()
{
	hx_sptr<hx_type_decl_statement> type_decl_statement(make_shared<hx_type_decl_statement>());
	type_decl_statement->line_number = lexer->get_line();
	type_decl_statement->type = token.value;
	eat(TK_KEYWORD);
	
	// check if ptr
	// i.e. int* a;

	while (token.type == TK_MULTIPLY)
	{
		type_decl_statement->ptr_depth++;
        type_decl_statement->is_pointer = true;
		eat(TK_MULTIPLY);
	}
	
	if (token.type == TK_IDENTIFIER)
	{
		type_decl_statement->name = token.value;
		eat(TK_IDENTIFIER);
	}
	/* TODO ARRAY
	if (token.type == TK_L_BRACK)
	{
		eat(TK_L_BRACK, error);
		eat(TK_R_BRACK, error);
	}
	*/

	return type_decl_statement;
}



hx_sptr<hx_expression_statement> hx_parser::parse_expression_statement()
{
	hx_sptr<hx_expression_statement> expression_statement(make_shared<hx_expression_statement>());
	expression_statement->line_number = lexer->get_line();
	expression_statement->expression = parse_expression(parse_primary(), 0, 0);

	eat(TK_SEMICOLON);

	return expression_statement;
}

hx_sptr<hx_expression> hx_parser::parse_primary()
{

	switch (token.type)
	{
	case TK_INTEGER:	return parse_int_literal_expression();
	case TK_IDENTIFIER:	return parse_identifier_expression();
	case TK_L_PAREN:
	{
		eat(TK_L_PAREN);
		hx_sptr<hx_expression> expression = parse_expression(parse_primary(), 0);
		eat(TK_R_PAREN);
		return expression;
	} 

	
	case TK_MINUS:
	case TK_PLUS:
	case TK_NOT:
	case TK_MULTIPLY:
    case TK_BITWISE_AND:
		return parse_unary_expression();
	
	default:
        hx_error error;
		error.error_type = HX_SYNTAX_ERROR;
		error.info = "Expected int, identifier or ()";
		error.line = lexer->get_line();
		hx_logger::log_and_exit(error);
        return nullptr;
	}
}

hx_sptr<hx_expression> hx_parser::parse_expression(hx_sptr<hx_expression> lhs, uint32_t depth, uint32_t precedence)
{
	
	hx_sptr<hx_binop_expression> binop_expression(make_shared<hx_binop_expression>());

	while (true)
	{

    	std::optional<uint32_t> opt = get_precedence_level(token.type);

		if (!opt.has_value())
			break;

		uint32_t precedence_level = opt.value();
        /*
		if (token.type == TK_EQU)
		{
			// TODO: REMOVE THIS
            
			if (depth > 0)
			{
                hx_error error;
				error.error_type = HX_SYNTAX_ERROR;
				error.line = lhs->line_number;
				error.info = "Equal sign at the wrong place";
				hx_logger::log_and_exit(error);
			}
			if (lhs->e_type != expression_type::IDENTIFIER_LITERAL)
			{
                hx_error error;
				error.error_type = HX_SYNTAX_ERROR;
				error.line = lhs->line_number;
				error.info = "Left hand side of equality is not an identifier.";
				hx_logger::log_and_exit(error);
			}
		} */
	
		if (precedence_level < precedence)
			break;

		std::string op = tk_type_to_str::get_str(token.type);
		eat(token.type);

		hx_sptr<hx_expression> rhs = parse_primary();

		
		while (true)
		{
			std::optional<uint32_t> opt = get_precedence_level(token.type);
			if (!opt.has_value())
				break;

			uint32_t precedence_level_inner = opt.value();

			if (precedence_level_inner <= precedence_level)
				break;

			rhs = parse_expression(rhs, depth + 1, precedence_level_inner);

		}
		
		
		binop_expression->left = copy_expression(lhs);

		binop_expression->right = rhs;
		binop_expression->op = op;

		lhs = binop_expression;

		depth++;
	}

	return lhs;
}

hx_sptr<hx_unary_expression> hx_parser::parse_unary_expression()
{
	hx_sptr<hx_unary_expression> unary_op(make_shared<hx_unary_expression>());

	unary_op->op = tk_type_to_str::get_str(token.type);
	eat(token.type);

	unary_op->expression = parse_primary();


	//unary_op->expression = parse_expression(error, parse_primary(error), 0);

	return unary_op;
	


}

hx_sptr<hx_expression> hx_parser::parse_identifier_expression()
{
	
	hx_sptr<hx_identifier_literal_expression> identifier_literal_expression(make_shared<hx_identifier_literal_expression>());

	std::string name = token.value;


	identifier_literal_expression->line_number = lexer->get_line();
	identifier_literal_expression->name = name;
	eat(TK_IDENTIFIER);

	if (token.type == TK_L_PAREN)		// if there is a '(' after identifier then it must be a function call
	{

		hx_sptr<hx_function_call_expression> function_call_expression(make_shared<hx_function_call_expression>());
		function_call_expression->identifier = identifier_literal_expression;
		function_call_expression->line_number = lexer->get_line();

		eat(TK_L_PAREN);


		while (token.type != TK_R_PAREN)
		{
			function_call_expression->arguments.push_back(parse_expression(parse_primary(), 1));

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

	int_literal_expression->value = std::stoll(token.value);
	int_literal_expression->line_number = lexer->get_line();

	eat(TK_INTEGER);

	return int_literal_expression;
}

hx_sptr<hx_string_literal_expression> hx_parser::parse_string_literal_expression()
{
	hx_sptr<hx_string_literal_expression> string_literal_expression(make_shared<hx_string_literal_expression>());
	string_literal_expression->line_number = lexer->get_line();
	string_literal_expression->literal = token.value;
	eat(TK_STRING);

	return string_literal_expression;
}

hx_sptr<hx_identifier_literal_expression> hx_parser::parse_identifier_literal_expression()
{
	hx_sptr<hx_identifier_literal_expression> identifier_literal_expression(make_shared<hx_identifier_literal_expression>());
	identifier_literal_expression->line_number = lexer->get_line();
	identifier_literal_expression->name = token.value;

	eat(TK_IDENTIFIER);

	return identifier_literal_expression;
}

hx_sptr<hx_expression> hx_parser::copy_expression(hx_sptr<hx_expression> expr)
{
	switch (expr->e_type)
	{
	case expression_type::INT_LITERAL:
		return
			make_shared<hx_int_literal_expression>
			(*std::dynamic_pointer_cast<hx_int_literal_expression>(expr));
	case expression_type::IDENTIFIER_LITERAL:
		return
			make_shared<hx_identifier_literal_expression>
			(*std::dynamic_pointer_cast<hx_identifier_literal_expression>(expr));

	case expression_type::FUNCTION_CALL:
	{
		return
			make_shared<hx_function_call_expression>
			(*std::dynamic_pointer_cast<hx_function_call_expression>(expr));

	}
	case expression_type::BINOP:
		return
			make_shared<hx_binop_expression>(*std::dynamic_pointer_cast<hx_binop_expression>(expr));

	case expression_type::UNARYOP:
		return
			make_shared<hx_unary_expression>(*std::dynamic_pointer_cast<hx_unary_expression>(expr));
	default:
	{
		hx_error error;
		error.line = expr->line_number;
		error.error_type = HX_SYNTAX_ERROR;
		error.info = "Unknown expression type";
		hx_logger::log_and_exit(error);
        return nullptr;
	}
	}

}

