#pragma once
#include <cassert>
#include <string>
#include <vector>

#include "heliox_token.hpp"

namespace hx
{

class Lexer
{
public:
	Lexer(std::string_view text, std::string_view filename);

	uint32_t get_line();
	bool is_finished();

	Token get_next();

    std::vector<Token> tokenize();
    void reset();
private:

	Token make_number();
	Token make_identifier();

	Token make_token(TokenType type, std::string value);

	char peek_next(uint32_t offset=0);
	bool advance();

private:

	std::string_view m_text;
	std::string_view m_filename;

	char m_cur_char;


	uint32_t m_len_text;
	uint32_t m_index=0;

	uint32_t m_line_number = 1;
	uint32_t m_line_position = 0;

};
}		
