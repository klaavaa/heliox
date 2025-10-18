#include "heliox_lexer.hpp"
#include "heliox_error.hpp"
#include "heliox_keywords.hpp"


namespace hx {

lexer::lexer(std::string text)
{
    this->m_text = text;
    this->m_len_text = (uint32_t)text.size();
    reset();
}
uint32_t lexer::get_line() 
{
    return m_line_number;
}
bool lexer::is_finished()
{
    return this->m_index == this->m_len_text;
}

std::vector<token> lexer::tokenize()
{
    std::vector<token> tokens;
    while (!is_finished())
    {
        tokens.push_back(get_next());
    }
    reset();
    return tokens;
}
void lexer::reset()
{
    this->m_index = 0;
    this->m_cur_char = -1;
}

token lexer::get_next()
{
    

    do
    {
        if (!advance()) return token(tk_type::END_OF_FILE, "");

    } while (m_cur_char == HX_SPACE || m_cur_char == HX_TAB || m_cur_char == HX_NEWLINE || m_cur_char == '\r');


    switch (m_cur_char)
    {

    case HX_COMMA:
    {
        return token(tk_type::COMMA, "");
    }
    case HX_DOT:
    {
        return token(tk_type::DOT, "");
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
            s += m_cur_char;
        }
        advance();
        return token(tk_type::STRING, s);

    }
    /*
    case DOLLAR:
    {
        return token(tk_type::DOLLAR, "");
    } */
    case HX_AMPERSAND:
    {
        if (peek_next() == HX_AMPERSAND)
        {
            advance();
            return token(tk_type::LOGICAL_AND, "");
        }

        return token(tk_type::BITWISE_AND, "");

    }

    case HX_CIRCUMFLEX:
    {
        return token(tk_type::BITWISE_XOR, "");
    }

    case HX_PIPE:
    {
        if (peek_next() == HX_PIPE)
        {
            advance();
            return token(tk_type::LOGICAL_OR, "");
        }
        return token(tk_type::BITWISE_OR, "");
    }

    case HX_PLUS:
    {
        return token(tk_type::PLUS, "");
    }
    case HX_MINUS:
    {
        
        if (peek_next() == HX_RIGHT_ARROW)
        {
            advance();
            return token(tk_type::ARROW, "");
        }
        
        return token(tk_type::MINUS, "");
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

                if (m_cur_char == HX_STAR)
                {
                    if (!advance())
                        goto error_label;
                    if (m_cur_char == HX_DIVIDE)
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

            } while (m_cur_char != HX_NEWLINE && m_cur_char != HX_EOF);

            return get_next();
        }


        return token(tk_type::DIVIDE, "");
    }
    case HX_STAR:
    {
        return token(tk_type::MULTIPLY, "");
    }
    case HX_LEFT_BRACE:
    {
        return token(tk_type::L_BRACE, "");
    }
    case HX_RIGHT_BRACE:
    {
        return token(tk_type::R_BRACE, "");
    }
    case HX_LEFT_PAREN:
    {
        return token(tk_type::L_PAREN, "");
    }
    case HX_RIGHT_PAREN:
    {
        return token(tk_type::R_PAREN, "");
    }
    case HX_LEFT_BRACK:
    {
        return token(tk_type::L_BRACK, "");
    }
    case HX_RIGHT_BRACK:
    {
        return token(tk_type::R_BRACK, "");
    }
    /*
    case AT:
    {
        return token(tk_type::AT, "");
    }
    */
    case HX_LEFT_ARROW:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return token(tk_type::LTE, "");
        }
        return token(tk_type::LT, "");
    }
    case HX_RIGHT_ARROW:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return token(tk_type::GTE, "");
        }
        return token(tk_type::GT, "");
    }

    case HX_EQUALS:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return token(tk_type::DOUBLE_EQU, "");
        }
        return token(tk_type::EQU, "");
    }

    case HX_EXCLAMATION_MARK:
    {

        if (peek_next() == HX_EQUALS)
        {
            advance();
            return token(tk_type::NEQU, "");
        }

        return token(tk_type::NOT, "");

    }
    /*
    case QUESTION_MARK:
    {

        return token(tk_type::NOT_A_TOKEN, "");
    }*/


    case HX_SEMICOLON:
    {
        return token(tk_type::SEMICOLON, "");
    }
    case HX_COLON:
    {
        return token(tk_type::COLON, "");
    }
    }
    if (strchr(characters, m_cur_char))
    {
        return make_identifier();
    }

    else if (strchr(numbers, m_cur_char))
    {
        return make_number();
    }
    

    return token(tk_type::NOT_A_TOKEN, "");
}

token lexer::make_number()
{
    std::string body;

    bool is_int = true;

    while (m_cur_char != HX_SPACE && m_cur_char != HX_NEWLINE && m_cur_char != HX_TAB)
    {
        if (m_cur_char == HX_DOT)
        {
            assert(is_int);//TODO ERROR;
            is_int = false;
        }


        body += m_cur_char;

    
        if (!strchr(numbers, peek_next()))
        {
            if (peek_next() != HX_DOT)
                break;
        }
        advance();

    }


    token tok = token(is_int ? tk_type::INTEGER : tk_type::FLOAT, body);

    return tok;


}

token lexer::make_identifier()
{

    std::string body;

    while (m_cur_char != HX_SPACE && m_cur_char != HX_NEWLINE && m_cur_char != HX_TAB)
    {
        body += m_cur_char;

    
        if (!strchr(characters_numbers, peek_next()))
            break;

        advance();
    }

    
    if (std::find_if(keywords.begin(), keywords.end(),
        [body](const auto& other)
        {return (body == other.first); }) != keywords.end())
    {
        return token(tk_type::KEYWORD, body);
    }
        
    token tok = token(tk_type::IDENTIFIER, body);

    return tok;


}
char lexer::peek_next(uint32_t offset)
{

    if (m_index + 1 + offset > m_len_text)
        return -1;
    
    return m_text[m_index + offset];

}
bool lexer::advance()
{

    m_index++;
    if (m_index > m_len_text)
    {
        m_index = m_len_text;
        return false;
    }
    m_cur_char = m_text[m_index - 1];

    if (m_cur_char == '\n')
        m_line_number++;

    return true;
}
}
