#pragma once
#include <cassert>
#include <string>

#include "heliox_token.hpp"
#include "heliox_keywords.hpp"
#include "heliox_error.hpp"
#include "heliox_pointer.hpp"


class hx_lexer
{

public:
	hx_lexer(std::string text)
	{
		this->text = text;
		this->index = 0;
		this->len_text = text.size();
		this->cur_char = -1;

	}

	uint32_t get_line() 
	{
		return line_number;
	}

	bool is_finished()
	{
		return this->index == this->len_text;
	}


	hx_token get_next(hx_sptr<hx_error> error)
	{
		

		do
		{
			if (cur_char == NEWLINE) this->line_number++;
			if (!advance()) return hx_token(tk_type::TK_EOF, "");

		} while (cur_char == SPACE || cur_char == TAB || cur_char == NEWLINE);


		switch (cur_char)
		{

		case COMMA:
		{
			return hx_token(tk_type::TK_COMMA, "");
		}
		case DOT:
		{
			return hx_token(tk_type::TK_DOT, "");
		}
		case QUOT_MARK:
		{
			std::string s;
			while (peek_next() != QUOT_MARK)
			{
				if (peek_next() == -1)
				{
					error->ok = false;
					error->error_type = HX_SYNTAX_ERROR;
					error->line = get_line();
					error->info = "Undisclosed quotation mark";
					return hx_token(tk_type::TK_NOT_A_TOKEN, "");
				}
				advance();
				s += cur_char;
			}
			advance();
			return hx_token(tk_type::TK_STRING, s);

		}
		case DOLLAR:
		{
			return hx_token(tk_type::TK_DOLLAR, "");
		}
		case AMPERSAND:
		{

			return hx_token(tk_type::TK_AND, "");

		}

		case CIRCUMFLEX:
		{
			return hx_token(tk_type::TK_XOR, "");
		}

		case PIPE:
		{
			return hx_token(tk_type::TK_OR, "");
		}

		case PLUS:
		{
			return hx_token(tk_type::TK_PLUS, "");
		}
		case MINUS:
		{

			if (peek_next() == RIGHT_ARROW)
			{
				advance();
				return hx_token(tk_type::TK_ARROW, "");
			}
			return hx_token(tk_type::TK_MINUS, "");
		}
		case DIVIDE:
		{

			// IGNORE IF COMMENT RETURN NEXT INSTEAD
			if (peek_next() == STAR)
			{
				advance();
				do
				{
					advance();

					if (cur_char == STAR)
					{
						advance();
						if (cur_char == DIVIDE)
						{
							advance();
							break;
						}
					}

				} while (cur_char != EOF);


				std::cout << cur_char << std::endl;

				return get_next(error);


			}
			
			// IGNORE IF COMMENT RETURN NEXT INSTEAD
			if (peek_next() == DIVIDE)
			{
				
				do
				{
					advance();

				} while (cur_char != NEWLINE && cur_char != EOF);

				return get_next(error);
			}


			return hx_token(tk_type::TK_DIVIDE, "");
		}
		case STAR:
		{
			return hx_token(tk_type::TK_MULTIPLY, "");
		}
		case LEFT_BRACE:
		{
			return hx_token(tk_type::TK_L_BRACE, "");
		}
		case RIGHT_BRACE:
		{
			return hx_token(tk_type::TK_R_BRACE, "");
		}
		case LEFT_PAREN:
		{
			return hx_token(tk_type::TK_L_PAREN, "");
		}
		case RIGHT_PAREN:
		{
			return hx_token(tk_type::TK_R_PAREN, "");
		}
		case LEFT_BRACK:
		{
			return hx_token(tk_type::TK_L_BRACK, "");
		}
		case RIGHT_BRACK:
		{
			return hx_token(tk_type::TK_R_BRACK, "");
		}
		case AT:
		{
			return hx_token(tk_type::TK_AT, "");
		}

		case LEFT_ARROW:
		{
			return hx_token(tk_type::TK_LT, "");
		}
		case RIGHT_ARROW:
		{
			return hx_token(tk_type::TK_GT, "");
		}

		case EQUALS:
		{

			return hx_token(tk_type::TK_EQU, "");
		}

		case EXCLAMATION_MARK:
		{
			return hx_token(tk_type::TK_NOT, "");

		}
		case QUESTION_MARK:
		{

			return hx_token(tk_type::TK_NOT_A_TOKEN, "");
		}


		case SEMICOLON:
		{
			return hx_token(tk_type::TK_SEMICOLON, "");
		}
		case COLON:
		{
			return hx_token(tk_type::TK_COLON, "");
		}
		}
		if (strchr(characters, cur_char))
		{
			return make_identifier();
		}

		else if (strchr(numbers, cur_char))
		{
			return make_number();
		}
		

		return hx_token(tk_type::TK_NOT_A_TOKEN, "");
	}




private:

	hx_token make_number()
	{

		std::string body;

		bool is_int = true;

		while (cur_char != SPACE && cur_char != NEWLINE && cur_char != TAB)
		{
			if (cur_char == DOT)
			{
				assert(is_int);//TODO ERROR;
				is_int = false;
			}
	

			body += cur_char;

		
			if (!strchr(numbers, peek_next()))
			{
				if (peek_next() != DOT)
					break;
			}
			advance();

		}


		hx_token tok = hx_token(is_int ? tk_type::TK_INTEGER : tk_type::TK_FLOAT, body);

		return tok;


	}

	hx_token make_identifier()
	{

		std::string body;

		while (cur_char != SPACE && cur_char != NEWLINE && cur_char != TAB)
		{
			body += cur_char;

		
			if (!strchr(characters_numbers, peek_next()))
				break;

			advance();
		}
	
		
		if (std::find_if(keywords.begin(), keywords.end(),
			[body](const auto& other)
			{return (body == other.first); }) != keywords.end())
		{
			hx_token tok(tk_type::TK_KEYWORD, body);
			return tok;
			return hx_token(tk_type::TK_KEYWORD, body);
		}
			
		hx_token tok = hx_token(tk_type::TK_IDENTIFIER, body);

		return tok;


	}

	char peek_next(uint32_t offset=0)
	{

		if (index + 1 + offset > len_text)
			return -1;
		
		return text[index + offset];

	}

	bool advance()
	{

		index++;
		if (index > len_text)
		{
			index = len_text;
			return false;
		}
		cur_char = text[index - 1];

		return true;
	}


private:

	std::string text;
	char cur_char;


	uint32_t len_text;
	uint32_t index=0;

	uint32_t line_number = 1;

};




		

		