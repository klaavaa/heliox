#include "heliox_lexer.hpp"
#include "heliox_error.hpp"
#include "heliox_keywords.hpp"



namespace hx {

Lexer::Lexer(std::string_view text, std::string_view filename)
    :
    m_text(text),
    m_filename(filename)
{
    this->m_len_text = (uint32_t)text.size();
    reset();
}
uint32_t Lexer::get_line() 
{
    return m_line_number;
}
bool Lexer::is_finished()
{
    return this->m_index == this->m_len_text;
}

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;
    while (!is_finished())
    {
        tokens.push_back(get_next());
    }
    reset();
    return tokens;
}
void Lexer::reset()
{
    this->m_index = 0;
    this->m_cur_char = -1;
}

Token Lexer::get_next()
{
    

    do
    {
        if (!advance()) return make_token(TokenType::END_OF_FILE, "");

    } while (m_cur_char == HX_SPACE || m_cur_char == HX_TAB || m_cur_char == HX_NEWLINE || m_cur_char == '\r');


    switch (m_cur_char)
    {

    case HX_COMMA:
    {
        return make_token(TokenType::COMMA, "");
    }
    case HX_DOT:
    {
        if (peek_next() == HX_DOT)
        {
            advance();
            if (peek_next() == HX_DOT)
            {
                advance();
                return make_token(TokenType::DOTDOTDOT, "");
            }
            return make_token(TokenType::DOTDOT, "");
        }
        return make_token(TokenType::DOT, "");
    }
    case HX_QUOT_MARK:
    {
        std::string s;
        while (peek_next() != HX_QUOT_MARK)
        {
            if (peek_next() == HX_BACKSLASH)
            {
                advance();
                s += m_cur_char;    
            }
            if (peek_next() == -1 || peek_next() == HX_NEWLINE)
            {
                Logger::error(m_filename, m_line_number, m_line_position, HX_UNTERMINATED_STRING_LITERAL, "Unterminated string literal");
            }
            advance();
            s += m_cur_char;
        }
        advance();
        return make_token(TokenType::STRING, s);

    }
    /*
    case DOLLAR:
    {
        return make_token(TokenType::DOLLAR, "");
    } */
    case HX_AMPERSAND:
    {
        if (peek_next() == HX_AMPERSAND)
        {
            advance();
            return make_token(TokenType::LOGICAL_AND, "");
        }

        return make_token(TokenType::BITWISE_AND, "");

    }
    case HX_TILDE:
    {
        return make_token(TokenType::BITWISE_NOT, "");
    }

    case HX_CIRCUMFLEX:
    {
        return make_token(TokenType::BITWISE_XOR, "");
    }

    case HX_PIPE:
    {
        if (peek_next() == HX_PIPE)
        {
            advance();
            return make_token(TokenType::LOGICAL_OR, "");
        }
        return make_token(TokenType::BITWISE_OR, "");
    }

    case HX_PLUS:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return make_token(TokenType::PLUSEQUALS, "");
        }
        return make_token(TokenType::PLUS, "");
    }
    case HX_MINUS:
    {
        
        if (peek_next() == HX_RIGHT_ARROW)
        {
            advance();
            return make_token(TokenType::ARROW, "");
        }
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return make_token(TokenType::MINUSEQUALS, "");
        }
        
        return make_token(TokenType::MINUS, "");
    }
    case HX_DIVIDE:
    {

        // IGNORE IF COMMENT RETURN NEXT INSTEAD
        if (peek_next() == HX_STAR)
        {
            uint32_t start_line = m_line_number;
            uint32_t start_position = m_line_position;

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
            Logger::error(m_filename, start_line, start_position, HX_UNTERMINATED_MULTILINE_COMMENT, "Unterminated multiline comment, no matching '*/' found");

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
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return make_token(TokenType::DIVEQUALS, "");
        }

        return make_token(TokenType::DIVIDE, "");
    }
    case HX_STAR:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return make_token(TokenType::MULEQUALS, "");
        }
        return make_token(TokenType::MULTIPLY, "");
    }
    case HX_LEFT_BRACE:
    {
        return make_token(TokenType::L_BRACE, "");
    }
    case HX_RIGHT_BRACE:
    {
        return make_token(TokenType::R_BRACE, "");
    }
    case HX_LEFT_PAREN:
    {
        return make_token(TokenType::L_PAREN, "");
    }
    case HX_RIGHT_PAREN:
    {
        return make_token(TokenType::R_PAREN, "");
    }
    case HX_LEFT_BRACK:
    {
        return make_token(TokenType::L_BRACK, "");
    }
    case HX_RIGHT_BRACK:
    {
        return make_token(TokenType::R_BRACK, "");
    }
    case HX_MODULO:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return make_token(TokenType::MODEQUALS, "");
        }
        return make_token(TokenType::MODULO, "");
    }
    /*
    case AT:
    {
        return make_token(TokenType::AT, "");
    }
    */
    case HX_LEFT_ARROW:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return make_token(TokenType::LTE, "");
        }
        if (peek_next() == HX_LEFT_ARROW)
        {
            advance();
            return make_token(TokenType::SHIFT_LEFT, "");
        }
        return make_token(TokenType::LT, "");
    }
    case HX_RIGHT_ARROW:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return make_token(TokenType::GTE, "");
        }
        if (peek_next() == HX_RIGHT_ARROW)
        {
            advance();
            return make_token(TokenType::SHIFT_RIGHT, "");
        }
        return make_token(TokenType::GT, "");
    }

    case HX_EQUALS:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return make_token(TokenType::DOUBLE_EQU, "");
        }
        return make_token(TokenType::EQU, "");
    }

    case HX_EXCLAMATION_MARK:
    {

        if (peek_next() == HX_EQUALS)
        {
            advance();
            return make_token(TokenType::NEQU, "");
        }

        return make_token(TokenType::NOT, "");

    }
    /*
    case QUESTION_MARK:
    {

        return make_token(TokenType::QUESTION_MARK, "");
    }*/


    case HX_SEMICOLON:
    {
        return make_token(TokenType::SEMICOLON, "");
    }
    case HX_COLON:
    {
        return make_token(TokenType::COLON, "");
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
    
    Logger::error(m_filename, m_line_number, m_line_position, HX_UNRECONIZED_CHARACTER, "Unrecognized character");
}

Token Lexer::make_number()
{
    std::string body;

    bool is_int = true;


    while (m_cur_char != HX_SPACE && m_cur_char != HX_NEWLINE && m_cur_char != HX_TAB)
    {
        if (m_cur_char == HX_DOT)
        {
            if (!is_int)
            {
                Logger::error(m_filename, m_line_number, m_line_position, HX_INVALID_FLOAT_LITERAL, "Invalid float literal, more than 1 dot found");
            }
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


    Token tok = make_token(is_int ? TokenType::INTEGER : TokenType::FLOAT, body);

    return tok;


}

Token Lexer::make_identifier()
{

    std::string body;
    uint32_t start_position = m_line_position;

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
        return Token(TokenType::KEYWORD, body, m_filename, m_line_number, start_position);
    }
        
    Token tok = Token(TokenType::IDENTIFIER, body, m_filename, m_line_number, start_position);

    return tok;


}
char Lexer::peek_next(uint32_t offset)
{

    if (m_index + 1 + offset > m_len_text)
        return -1;
    
    return m_text[m_index + offset];

}
bool Lexer::advance()
{

    m_index++;
    if (m_index > m_len_text)
    {
        m_index = m_len_text;
        return false;
    }
    m_cur_char = m_text[m_index - 1];

    if (m_cur_char == '\n')
    {
        m_line_number++;
        m_line_position = 0;
    }
    else
    {
        m_line_position++;
    }

    return true;
}

Token Lexer::make_token(TokenType type, std::string value)
{
    return Token(type, value, m_filename, m_line_number, m_line_position);
}

} // namespace hx
