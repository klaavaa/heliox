#include "heliox_lexer.hpp"

namespace hx {

lexer::lexer(std::string text)
{
		this->text = text;
		this->index = 0;
		this->len_text = (uint32_t)text.size();
		this->cur_char = -1;

}
uint32_t lexer::get_line() 
{
    return line_number;
}
bool lexer::is_finished()
{
    return this->index == this->len_text;
}
token lexer::get_next()
{
    

    do
    {
        if (!advance()) return token(tk_type::TK_EOF, "");

    } while (cur_char == HX_SPACE || cur_char == HX_TAB || cur_char == HX_NEWLINE || cur_char == '\r');


    switch (cur_char)
    {

    case HX_COMMA:
    {
        return token(tk_type::TK_COMMA, "");
    }
    case HX_DOT:
    {
        return token(tk_type::TK_DOT, "");
    }
    case HX_QUOT_MARK:
    {
        std::string s;
        while (peek_next() != HX_QUOT_MARK)
        {
            if (peek_next() == -1)
            {
                error error;
                error.error_type = HX_SYNTAX_ERROR;
                error.line = get_line();
                error.info = "Undisclosed quotation mark";
                logger::log_and_exit(error);
            }
            advance();
            s += cur_char;
        }
        advance();
        return token(tk_type::TK_STRING, s);

    }
    /*
    case DOLLAR:
    {
        return token(tk_type::TK_DOLLAR, "");
    } */
    case HX_AMPERSAND:
    {
        if (peek_next() == HX_AMPERSAND)
        {
            advance();
            return token(tk_type::TK_LOGICAL_AND, "");
        }

        return token(tk_type::TK_BITWISE_AND, "");

    }

    case HX_CIRCUMFLEX:
    {
        return token(tk_type::TK_BITWISE_XOR, "");
    }

    case HX_PIPE:
    {
        if (peek_next() == HX_PIPE)
        {
            advance();
            return token(tk_type::TK_LOGICAL_OR, "");
        }
        return token(tk_type::TK_BITWISE_OR, "");
    }

    case HX_PLUS:
    {
        return token(tk_type::TK_PLUS, "");
    }
    case HX_MINUS:
    {
        
        if (peek_next() == HX_RIGHT_ARROW)
        {
            advance();
            return token(tk_type::TK_ARROW, "");
        }
        
        return token(tk_type::TK_MINUS, "");
    }
    case HX_DIVIDE:
    {

        // IGNORE IF COMMENT RETURN NEXT INSTEAD
        if (peek_next() == HX_STAR)
        {
            if (!advance())
                goto error_label;
            do
            {
                if (!advance())
                    goto error_label;

                if (cur_char == HX_STAR)
                {
                    if (!advance())
                        goto error_label;
                    if (cur_char == HX_DIVIDE)
                    {
                        advance();
                        break;
                    }
                }

            } while (true);



            return get_next();

        error_label:
            hx::error error;
            error.error_type = HX_SYNTAX_ERROR;
            error.info = "No matching '*/' found";
            error.line = get_line();
            
            hx::logger::log_and_exit(error);

        }
        
        // IGNORE IF COMMENT RETURN NEXT INSTEAD
        if (peek_next() == HX_DIVIDE)
        {
            
            do
            {
                advance();

            } while (cur_char != HX_NEWLINE && cur_char != HX_EOF);

            return get_next();
        }


        return token(tk_type::TK_DIVIDE, "");
    }
    case HX_STAR:
    {
        return token(tk_type::TK_MULTIPLY, "");
    }
    case HX_LEFT_BRACE:
    {
        return token(tk_type::TK_L_BRACE, "");
    }
    case HX_RIGHT_BRACE:
    {
        return token(tk_type::TK_R_BRACE, "");
    }
    case HX_LEFT_PAREN:
    {
        return token(tk_type::TK_L_PAREN, "");
    }
    case HX_RIGHT_PAREN:
    {
        return token(tk_type::TK_R_PAREN, "");
    }
    case HX_LEFT_BRACK:
    {
        return token(tk_type::TK_L_BRACK, "");
    }
    case HX_RIGHT_BRACK:
    {
        return token(tk_type::TK_R_BRACK, "");
    }
    /*
    case AT:
    {
        return token(tk_type::TK_AT, "");
    }
    */
    case HX_LEFT_ARROW:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return token(tk_type::TK_LTE, "");
        }
        return token(tk_type::TK_LT, "");
    }
    case HX_RIGHT_ARROW:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return token(tk_type::TK_GTE, "");
        }
        return token(tk_type::TK_GT, "");
    }

    case HX_EQUALS:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return token(tk_type::TK_DOUBLE_EQU, "");
        }
        return token(tk_type::TK_EQU, "");
    }

    case HX_EXCLAMATION_MARK:
    {

        if (peek_next() == HX_EQUALS)
        {
            advance();
            return token(tk_type::TK_NEQU, "");
        }

        return token(tk_type::TK_NOT, "");

    }
    /*
    case QUESTION_MARK:
    {

        return token(tk_type::TK_NOT_A_TOKEN, "");
    }*/


    case HX_SEMICOLON:
    {
        return token(tk_type::TK_SEMICOLON, "");
    }
    case HX_COLON:
    {
        return token(tk_type::TK_COLON, "");
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
    

    return token(tk_type::TK_NOT_A_TOKEN, "");
}

token lexer::make_number()
{
    std::string body;

    bool is_int = true;

    while (cur_char != HX_SPACE && cur_char != HX_NEWLINE && cur_char != HX_TAB)
    {
        if (cur_char == HX_DOT)
        {
            assert(is_int);//TODO ERROR;
            is_int = false;
        }


        body += cur_char;

    
        if (!strchr(numbers, peek_next()))
        {
            if (peek_next() != HX_DOT)
                break;
        }
        advance();

    }


    token tok = token(is_int ? tk_type::TK_INTEGER : tk_type::TK_FLOAT, body);

    return tok;


}

token lexer::make_identifier()
{

    std::string body;

    while (cur_char != HX_SPACE && cur_char != HX_NEWLINE && cur_char != HX_TAB)
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
        token tok(tk_type::TK_KEYWORD, body);
        return tok;
        return token(tk_type::TK_KEYWORD, body);
    }
        
    token tok = token(tk_type::TK_IDENTIFIER, body);

    return tok;


}
char lexer::peek_next(uint32_t offset)
{

    if (index + 1 + offset > len_text)
        return -1;
    
    return text[index + offset];

}
bool lexer::advance()
{

    index++;
    if (index > len_text)
    {
        index = len_text;
        return false;
    }
    cur_char = text[index - 1];

    if (cur_char == '\n')
        line_number++;

    return true;
}
}
