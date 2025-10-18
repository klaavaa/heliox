#pragma once

#include "heliox_expression.hpp"
#include "heliox_statement.hpp"
#include "heliox_token.hpp"
#include "heliox_lexer.hpp"
#include "heliox_program.hpp"

namespace hx {
class parser
{

public:
    parser(uptr<lexer> lex);
    
    uptr<program> parse_program();

private:	

    uptr<function> parse_function();
    uptr<identifier_literal_expr> parse_identifier_literal();
    expression parse_expression();
    uptr<variable_declaration_statement> parse_variable_declaration();

    type_data parse_type();
	
    statement parse_statement();
    uptr<compound_statement> parse_compound_statement();
    statement parse_type_statement(); // variable_declaration or variable_defenition 
    uptr<expression_statement> parse_expression_statement();

	void eat(tk_type token_type);
     
private:

    token m_current_token;
    uptr<lexer> m_lexer;
};
}
