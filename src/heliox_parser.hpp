#pragma once
#include <vector>
#include <memory>
#include <print>

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
    uptr<function> parse_function();
    uptr<identifier_literal_expr> parse_identifier_literal();
    uptr<variable_declaration_statement> parse_variable_declaration();
    statement parse_statement();
    type_data parse_type();
private:	
	
	void eat(tk_type token_type);
     
private:

    token m_current_token;
    uptr<lexer> m_lexer;
};
}
