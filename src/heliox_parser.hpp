#pragma once

#include "heliox_expression.hpp"
#include "heliox_statement.hpp"
#include "heliox_token.hpp"
#include "heliox_lexer.hpp"
#include "heliox_program.hpp"

namespace hx {
class Parser
{

public:
    Parser(uptr<Lexer> lex);
    
    uptr<Program> parse_program();

private:	

    uptr<function> parse_function();
    expression parse_identifier();
    uptr<identifier_literal_expr> parse_identifier_literal();
    uptr<string_literal_expr> parse_string_literal();
    uptr<int_literal_expr> parse_int_literal();

    expression parse_expression();
    expression parse_expression_from_primary(expression primary, uint32_t min_precedence);
    expression parse_primary(); 


    uptr<variable_declaration_statement> parse_variable_declaration();

    type_data parse_type();
	
    statement parse_statement();
    uptr<compound_statement> parse_compound_statement();
    statement parse_type_statement(); // variable_declaration or variable_defenition 
    statement parse_keyword_statement();
    uptr<return_statement> parse_return_statement();
    uptr<conditional_statement> parse_conditional_statement();

	void eat(TokenType token_type);
     
private:

    Token m_current_token;
    uptr<Lexer> m_lexer;
};
}
