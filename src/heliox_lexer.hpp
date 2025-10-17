#pragma once
#include <cassert>
#include <string>

#include "heliox_token.hpp"
#include "heliox_keywords.hpp"
#include "heliox_error.hpp"
#include "heliox_pointer.hpp"


namespace hx
{

class lexer
{
public:
	lexer(std::string text);

	uint32_t get_line();
	bool is_finished();

	token get_next();

private:

	token make_number();
	token make_identifier();

	char peek_next(uint32_t offset=0);
	bool advance();

private:

	std::string text;
	char cur_char;


	uint32_t len_text;
	uint32_t index=0;

	uint32_t line_number = 1;

};
}		
