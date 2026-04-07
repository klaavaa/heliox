#include "heliox_lexer.hpp"
#include "heliox_error.hpp"
#include "heliox_keywords.hpp"


namespace hx {

Lexer::Lexer(std::string text)
{
    this->m_text = text;
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
        if (!advance()) return Token(TokenType::END_OF_FILE, "");

    } while (m_cur_char == HX_SPACE || m_cur_char == HX_TAB || m_cur_char == HX_NEWLINE || m_cur_char == '\r');


    switch (m_cur_char)
    {

    case HX_COMMA:
    {
        return Token(TokenType::COMMA, "");
    }
    case HX_DOT:
    {
        return Token(TokenType::DOT, "");
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
            if (peek_next() == -1)
            {
                Error error;
                error.error_type = HX_SYNTAX_ERROR;
                error.line = get_line();
                error.info = "Undisclosed quotation mark";
                Logger::log_and_exit(error);
            }
            advance();
            s += m_cur_char;
        }
        advance();
        return Token(TokenType::STRING, s);

    }
    /*
    case DOLLAR:
    {
        return Token(TokenType::DOLLAR, "");
    } */
    case HX_AMPERSAND:
    {
        if (peek_next() == HX_AMPERSAND)
        {
            advance();
            return Token(TokenType::LOGICAL_AND, "");
        }

        return Token(TokenType::BITWISE_AND, "");

    }

    case HX_CIRCUMFLEX:
    {
        return Token(TokenType::BITWISE_XOR, "");
    }

    case HX_PIPE:
    {
        if (peek_next() == HX_PIPE)
        {
            advance();
            return Token(TokenType::LOGICAL_OR, "");
        }
        return Token(TokenType::BITWISE_OR, "");
    }

    case HX_PLUS:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return Token(TokenType::PLUSEQUALS, "");
        }
        return Token(TokenType::PLUS, "");
    }
    case HX_MINUS:
    {
        
        if (peek_next() == HX_RIGHT_ARROW)
        {
            advance();
            return Token(TokenType::ARROW, "");
        }
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return Token(TokenType::MINUSEQUALS, "");
        }
        
        return Token(TokenType::MINUS, "");
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
            hx::Error error;
            error.error_type = HX_SYNTAX_ERROR;
            error.info = "No matching '*/' found";
            error.line = get_line();
            
            hx::Logger::log_and_exit(error);

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
            return Token(TokenType::DIVEQUALS, "");
        }

        return Token(TokenType::DIVIDE, "");
    }
    case HX_STAR:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return Token(TokenType::MULEQUALS, "");
        }
        return Token(TokenType::MULTIPLY, "");
    }
    case HX_LEFT_BRACE:
    {
        return Token(TokenType::L_BRACE, "");
    }
    case HX_RIGHT_BRACE:
    {
        return Token(TokenType::R_BRACE, "");
    }
    case HX_LEFT_PAREN:
    {
        return Token(TokenType::L_PAREN, "");
    }
    case HX_RIGHT_PAREN:
    {
        return Token(TokenType::R_PAREN, "");
    }
    case HX_LEFT_BRACK:
    {
        return Token(TokenType::L_BRACK, "");
    }
    case HX_RIGHT_BRACK:
    {
        return Token(TokenType::R_BRACK, "");
    }
    case HX_MODULO:
    {
        return Token(TokenType::MODULO, "");
    }
    /*
    case AT:
    {
        return Token(TokenType::AT, "");
    }
    */
    case HX_LEFT_ARROW:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return Token(TokenType::LTE, "");
        }
        return Token(TokenType::LT, "");
    }
    case HX_RIGHT_ARROW:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return Token(TokenType::GTE, "");
        }
        return Token(TokenType::GT, "");
    }

    case HX_EQUALS:
    {
        if (peek_next() == HX_EQUALS)
        {
            advance();
            return Token(TokenType::DOUBLE_EQU, "");
        }
        return Token(TokenType::EQU, "");
    }

    case HX_EXCLAMATION_MARK:
    {

        if (peek_next() == HX_EQUALS)
        {
            advance();
            return Token(TokenType::NEQU, "");
        }

        return Token(TokenType::NOT, "");

    }
    /*
    case QUESTION_MARK:
    {

        return Token(TokenType::NOT_A_TOKEN, "");
    }*/


    case HX_SEMICOLON:
    {
        return Token(TokenType::SEMICOLON, "");
    }
    case HX_COLON:
    {
        return Token(TokenType::COLON, "");
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
    

    return Token(TokenType::NOT_A_TOKEN, "");
}

Token Lexer::make_number()
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


    Token tok = Token(is_int ? TokenType::INTEGER : TokenType::FLOAT, body);

    return tok;


}

Token Lexer::make_identifier()
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
        return Token(TokenType::KEYWORD, body);
    }
        
    Token tok = Token(TokenType::IDENTIFIER, body);

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
        m_line_number++;

    return true;
}
}
