#include "heliox_parser.hpp"
#include "heliox_expression.hpp"
#include "heliox_keywords.hpp"
#include "heliox_statement.hpp"
#include "heliox_token.hpp"
#include "heliox_types.hpp"
#include "heliox_operator.hpp"
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
expression parser::parse_identifier()
{

    uptr<identifier_literal_expr> identifier = parse_identifier_literal();

    // check if its a function call 
    if (m_current_token.type == tk_type::L_PAREN)
    {
        eat(tk_type::L_PAREN);
        std::vector<expression> expressions;
        while(m_current_token.type != tk_type::R_PAREN)
        {
            expressions.push_back(parse_expression()); 
            if (m_current_token.type == tk_type::R_PAREN)
            {
                eat(tk_type::R_PAREN);
                break;
            }
            eat(tk_type::COMMA);
        }
        return std::make_unique<function_call_expr>(std::move(identifier), std::move(expressions));
    }
    return identifier;
}
uptr<string_literal_expr> parser::parse_string_literal()
{
    std::string value = m_current_token.value;
    eat(tk_type::STRING);
    return std::make_unique<string_literal_expr>(value);
}
uptr<int_literal_expr> parser::parse_int_literal()
{
    std::string value = m_current_token.value;
    eat(tk_type::INTEGER);
    return std::make_unique<int_literal_expr>(value);
}

expression parser::parse_primary()
{
    
    switch (m_current_token.type)
    {
    case tk_type::IDENTIFIER:
        return parse_identifier();
    case tk_type::STRING:
        return parse_string_literal();
    case tk_type::INTEGER:
        return parse_int_literal();
    case tk_type::L_PAREN:
        {
        eat(tk_type::L_PAREN);
        expression expr =  parse_expression();
        eat(tk_type::R_PAREN);
        return expr;
        }
    default:
        std::println("Unexpected token '{}' at parser::parse_primary", 
                get_string_from_token_type(m_current_token.type)); 
        exit(-1);

    }

}

expression parser::parse_expression_from_primary(expression primary, uint32_t min_precedence)
{
    expression lhs = std::move(primary);
    while (is_valid_binary_operator(m_current_token.type) &&
            get_binop_precedence_level(m_current_token.type) >= min_precedence)
    {
        tk_type op = m_current_token.type;
        uint32_t op_precedence = get_binop_precedence_level(op);
        eat(op);
        expression rhs = parse_primary();
        
        while (is_valid_binary_operator(m_current_token.type) 
             && get_binop_precedence_level(m_current_token.type) > op_precedence
             || (get_binop_associativity(m_current_token.type) == op_associativity::RIGHT_TO_LEFT
                &&  get_binop_precedence_level(m_current_token.type) == op_precedence))
        {
            uint32_t new_precedence = op_precedence;
            if (get_binop_precedence_level(m_current_token.type) > op_precedence)
                new_precedence = op_precedence + 1;

            rhs = parse_expression_from_primary(std::move(rhs), new_precedence);

        }
        return std::make_unique<binop_expr>(std::move(lhs), std::move(rhs), op);
    }
    return lhs;
}

expression parser::parse_expression()
{
    return parse_expression_from_primary(parse_primary(), 0);
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
        
        case tk_type::KEYWORD:
            return parse_keyword_statement();
            
        case tk_type::SEMICOLON:
            eat(tk_type::SEMICOLON);
            return std::make_unique<noop_statement>();
    // assume it is an expression statement if none of the above      
        default:
            if (m_current_token.type == tk_type::IDENTIFIER)
            {
                if (get_primitive_type_from_string(m_current_token.value).has_value())
                {
                    return parse_type_statement();
                }
            }
            expression expr = parse_expression();
            eat(tk_type::SEMICOLON);
            return std::make_unique<expression_statement>(std::move(expr));
    }
}

statement parser::parse_keyword_statement()
{
    keyword kw = get_kword_from_string(m_current_token.value);
    switch (kw)
    {
        case keyword::RETURN:
            return parse_return_statement();
           
    default:
        println("Unexpected keyword '{}' at parser::parse_keyword_statement",
                get_string_from_kword(kw));
        exit(-1);
    }
} 

uptr<return_statement> parser::parse_return_statement()
{
    eat(tk_type::KEYWORD);
    expression expr = parse_expression();
    eat(tk_type::SEMICOLON);
    return std::make_unique<return_statement>(std::move(expr));
}

statement parser::parse_type_statement()
{
    // checked before calling whether it has value
    type_data type = parse_type();
    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    uptr<variable_declaration_statement> declaration = 
        std::make_unique<variable_declaration_statement>(type, std::move(identifier));

    if (m_current_token.type == tk_type::EQU)
    {
        eat(tk_type::EQU);
        expression definition = parse_expression();
        eat(tk_type::SEMICOLON);
        return std::make_unique<variable_definition_statement>
            (std::move(declaration), std::move(definition));

    }
    eat(tk_type::SEMICOLON);
    return declaration;
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
    eat(tk_type::R_BRACE);
    return std::make_unique<compound_statement>(std::move(statements));
}

    
void parser::eat(tk_type token_type)
{
    if (m_current_token.type != token_type)
    {
        // TODO: unexpected token error
        std::println("Unexpected token '{}', expected '{}' at parser::eat", 
                        get_string_from_token_type(m_current_token.type), 
                        get_string_from_token_type(token_type)); 

        exit(-1); 
    }
    
    m_current_token = m_lexer->get_next();
}

}
