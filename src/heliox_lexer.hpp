#pragma once
#include <cassert>
#include <string>
#include <vector>

#include "heliox_token.hpp"

namespace hx
{

class lexer
{
public:
	lexer(std::string text);

	uint32_t get_line();
	bool is_finished();

	token get_next();

    std::vector<token> tokenize();
    void reset();
private:

	token make_number();
	token make_identifier();

	char peek_next(uint32_t offset=0);
	bool advance();

private:

	std::string m_text;
	char m_cur_char;


	uint32_t m_len_text;
	uint32_t m_index=0;

	uint32_t m_line_number = 1;

};
}		
