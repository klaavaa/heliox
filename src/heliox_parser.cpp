#include "heliox_parser.hpp"
#include "heliox_expression.hpp"
#include "heliox_keywords.hpp"
#include "heliox_statement.hpp"
#include "heliox_token.hpp"
#include "heliox_types.hpp"
#include <memory>
#include <print>

namespace hx 
{
parser::parser(uptr<lexer> lex)
	:
	m_lexer(std::move(lex))
{
	m_current_token = m_lexer->get_next();
}

uptr<program> parser::parse_program()
{
    std::vector<uptr<function>> functions;
    while (m_current_token.type != tk_type::END_OF_FILE)
    {
        switch (m_current_token.type) 
        {
            case tk_type::KEYWORD:
                functions.push_back(std::move(parse_function()));
                break;
           

            default:
                // TODO: unexpected token error
                std::println("Unexpected token '{}' at parser::parse_program", 
                        get_string_from_token_type(m_current_token.type)); 
                exit(-1);
        }


    }

    return std::make_unique<program>(std::move(functions));
}
uptr<function> parser::parse_function()
{
    bool is_extern = false;
    keyword kword = get_kword_from_string(m_current_token.value);
    if (kword == keyword::EXTERN)
    {
        is_extern = true;
        eat(tk_type::KEYWORD);
        
        kword = get_kword_from_string(m_current_token.value);
        
    }
    if (kword != keyword::FUN)
    {
        // TODO: unexpected token error (expected fun)
        std::println("Unexpected token '{}' at parser::parse_function", 
                get_string_from_token_type(m_current_token.type)); 
        exit(-1);
    }
    eat(tk_type::KEYWORD);
    
    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    
    eat(tk_type::L_PAREN);
    std::vector<uptr<variable_declaration_statement>> parameters;
    if (m_current_token.type != tk_type::R_PAREN) 
    {
        while (true)
        {
            uptr<variable_declaration_statement> parameter = parse_variable_declaration();
            parameters.push_back(std::move(parameter));
            if (m_current_token.type == tk_type::R_PAREN)
            {
                break;
            }
            eat(tk_type::COMMA);
        }
    }
    eat(tk_type::R_PAREN);

    type_data td = parse_type(); 

   statement body = parse_statement();
   return std::make_unique<function>(std::move(identifier), std::move(parameters),
           std::move(body), td, is_extern);
}

uptr<identifier_literal_expr> parser::parse_identifier_literal()
{
    std::string name = m_current_token.value;
    eat(tk_type::IDENTIFIER);
    return std::make_unique<identifier_literal_expr>(name);
}

expression parser::parse_expression()
{

}

type_data parser::parse_type()
{
    std::optional<primitive_type> pt = get_primitive_type_from_string(m_current_token.value);
    eat(tk_type::IDENTIFIER);
    if (!pt.has_value())
    {
        // TODO OWN TYPES 
        // TODO error
        std::println("{} is not a primitive type in parser::parse_type",
                m_current_token.value);
        exit(-1); 
    }
     
    uint32_t ptr_depth = 0;
    while (m_current_token.type == tk_type::MULTIPLY)
    {
        ptr_depth++;
        eat(tk_type::MULTIPLY);
    }
    return type_data(pt.value(), ptr_depth);

}

uptr<variable_declaration_statement> parser::parse_variable_declaration()
{
    type_data td = parse_type(); 
    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    return std::make_unique<variable_declaration_statement>(td, std::move(identifier));
}

statement parser::parse_statement()
{
    switch (m_current_token.type)
    {
        case tk_type::L_BRACE:
            return parse_compound_statement();

        case tk_type::SEMICOLON:
            return std::make_unique<noop_statement>();
        
        case tk_type::IDENTIFIER:
            if (get_primitive_type_from_string(m_current_token.value).has_value())
                return parse_type_statement();
            else
                return parse_expression_statement();

    default:

       // TOOD ERROR
        std::println("Unexpected token '{}' at parser::parse_statement", 
                get_string_from_token_type(m_current_token.type)); 
        exit(-1);
    }
}

statement parser::parse_type_statement()
{
    // checked before calling whether it has value
    type_data type = parse_type();
    uptr<identifier_literal_expr> identifier = 
        std::make_unique<identifier_literal_expr>(m_current_token.value);
    eat(tk_type::IDENTIFIER);
    
    uptr<variable_declaration_statement> declaration = 
        std::make_unique<variable_declaration_statement>(type, std::move(identifier));

    if (m_current_token.type == tk_type::EQU)
    {
        eat(tk_type::EQU);
        expression definition = parse_expression();
        return std::make_unique<variable_definition_statement>
            (std::move(declaration), std::move(definition));
    }
}

uptr<expression_statement> parser::parse_expression_statement()
{
    return nullptr;
}

uptr<compound_statement> parser::parse_compound_statement()
{
    eat(tk_type::L_BRACE);
    
    std::vector<statement> statements;
    while (m_current_token.type != tk_type::R_BRACE)
    {
       statements.push_back(std::move(
                   parse_statement()));
        
    }
    return nullptr;
}

void parser::eat(tk_type token_type)
{
    if (m_current_token.type != token_type)
    {
        // TODO: unexpected token error
        std::println("Unexpected token '{}' at parser::eat", 
                        get_string_from_token_type(m_current_token.type)); 

        exit(-1); 
    }
    
    m_current_token = m_lexer->get_next();
}

}
