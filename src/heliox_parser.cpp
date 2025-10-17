#include "heliox_parser.hpp"

hx::parser::parser(hx::lexer* lexer)
	:
	lexer(lexer),
	token(tk_type::TK_EOF, "")
{
	token = lexer->get_next();
}

