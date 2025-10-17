#pragma once
#include <vector>
#include <memory>

#include "heliox_token.hpp"
#include "heliox_lexer.hpp"
#include "heliox_statement.hpp"
#include "heliox_pointer.hpp"

namespace hx {
class parser
{

public:
    parser(hx::lexer* lexer);

private:	
	
	void eat(tk_type token_type);
     
private:

    token token;
    lexer* lexer;
};
}
