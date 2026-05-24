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
Parser::Parser(uptr<Lexer> lex)
	:
	m_lexer(std::move(lex)),
    m_current_token(m_lexer->get_next())
{
}

uptr<Program> Parser::parse_program()
{
    std::vector<uptr<function>> functions;
    std::unordered_set<std::string> imports;
    std::unordered_set<std::string> modules;

    while (m_current_token.type != TokenType::END_OF_FILE)
    {
        switch (m_current_token.type) 
        {
            case TokenType::KEYWORD:
                {
                KeyWord kword = get_kword_from_string(m_current_token.value);
                if (kword == KeyWord::FUN || kword == KeyWord::EXTERN) 
                {
                    functions.push_back(std::move(parse_function()));
                }
                else if (kword == KeyWord::MODULE)
                {
                    eat(TokenType::KEYWORD);
                    if (m_current_token.type == TokenType::SEMICOLON)
                    {
                        current_module = "";
                        eat(TokenType::SEMICOLON);
                        break;
                    }
                    current_module = m_current_token.value; 
                    modules.insert(current_module);
                    eat(TokenType::IDENTIFIER);
                    eat(TokenType::SEMICOLON);
                }
                else if (kword == KeyWord::IMPORT)
                {
                    eat(TokenType::KEYWORD);
                    uptr<identifier_literal_expr> identifier_lit = parse_identifier_literal();
                    eat(TokenType::SEMICOLON);
                    imports.insert(identifier_lit->name);
                }
                else
                {
                    Logger::error(m_current_token, HX_UNEXPECTED_KEYWORD, "Unexpected keyword");
                }
                break;
                }
            default:
                {
                    Logger::error(m_current_token, HX_UNEXPECTED_TOKEN, "Unexpected token");
                }
        }


    }

    return std::make_unique<Program>(std::move(functions), std::move(modules), std::move(imports));
}
uptr<function> Parser::parse_function()
{
    bool is_extern = false;
    KeyWord kword = get_kword_from_string(m_current_token.value);
    if (kword == KeyWord::EXTERN)
    {
        is_extern = true;
        eat(TokenType::KEYWORD);
        
        kword = get_kword_from_string(m_current_token.value);
        
    }
    if (kword != KeyWord::FUN)
    {
        Logger::error(m_current_token, HX_UNEXPECTED_KEYWORD, "Expected 'fun' keyword at start of function definition");
    }
    eat(TokenType::KEYWORD);
    

    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    
    bool varargs = false;

    eat(TokenType::L_PAREN);
    std::vector<uptr<variable_declaration_statement>> parameters;
    if (m_current_token.type != TokenType::R_PAREN) 
    {
        while (true)
        {
            if (m_current_token.type == TokenType::DOTDOTDOT)
            {
                varargs = true;
                eat(TokenType::DOTDOTDOT);
                if (m_current_token.type != TokenType::R_PAREN)
                {
                    hx::Logger::error(m_current_token, HX_VARARGS_NOT_LAST_ARG, "Varargs must be the last parameter of a function");
                }
                break;
            }
            type_data td = parse_type(); 
            
            if (m_current_token.type == TokenType::IDENTIFIER)
            {
                parameters.emplace_back(std::make_unique<variable_declaration_statement>(td, parse_identifier_literal()));
            }
            else
            {
                parameters.emplace_back(std::make_unique<variable_declaration_statement>(td,
                            std::make_unique<identifier_literal_expr>("")));
            }

            if (m_current_token.type == TokenType::R_PAREN)
            {
                break;
            }
            eat(TokenType::COMMA);
        }
    }
    eat(TokenType::R_PAREN);

    type_data td = parse_type(); 
    
    if (m_current_token.type == TokenType::SEMICOLON)
    {
        eat(TokenType::SEMICOLON);
        if (is_extern)
            return std::make_unique<function>(std::move(identifier), std::move(parameters),
                    std::move(std::vector<statement>{}), td, is_extern, varargs, "");
        return std::make_unique<function>(std::move(identifier), std::move(parameters),
                std::move(std::vector<statement>{}), td, is_extern, varargs, current_module);
    }
    if (is_extern)
    {
        Logger::error(m_current_token, HX_EXTERN_FUNC_WITH_BODY, "Extern function defined with a body"); 
    }
    eat(TokenType::L_BRACE);
    std::vector<statement> statements;
    while (m_current_token.type != TokenType::R_BRACE)
    {    
        statements.push_back(parse_statement());
    }
    eat(TokenType::R_BRACE);
    return std::make_unique<function>(std::move(identifier), std::move(parameters),
           std::move(statements), td, is_extern, varargs, current_module);
}

uptr<identifier_literal_expr> Parser::parse_identifier_literal()
{
    std::string name{m_current_token.value};
    eat(TokenType::IDENTIFIER);
    while (m_current_token.type == TokenType::COLON)
    {
        eat(TokenType::COLON);
        eat(TokenType::COLON);
        
        name += "." + std::string(m_current_token.value);
        eat(TokenType::IDENTIFIER);
    }
    return std::make_unique<identifier_literal_expr>(name);
}
expression Parser::parse_identifier()
{

    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    // check if identifier is a primitive type, if it is, explicit casting is in order
    auto primitive_opt = get_primitive_type_from_string(identifier->name);
    if (primitive_opt.has_value())
    {
        uint32_t ptr_depth = 0;
        while (m_current_token.type == TokenType::MULTIPLY)
        {
            ptr_depth++;
            eat(TokenType::MULTIPLY);
        }
        type_data conversion_type(primitive_opt.value(), ptr_depth);
        eat(TokenType::L_PAREN); 
        expression expr = parse_expression();
        eat(TokenType::R_PAREN);
        return std::make_unique<explicit_conversion_expr>(conversion_type, std::move(expr));
    }

    // check if its a function call 
    if (m_current_token.type == TokenType::L_PAREN)
    {
        eat(TokenType::L_PAREN);

        
        std::vector<expression> expressions;
        while(m_current_token.type != TokenType::R_PAREN)
        {
            expressions.push_back(parse_expression()); 
            if (m_current_token.type == TokenType::R_PAREN)
            {
                break;
            }
            eat(TokenType::COMMA);
        }
        eat(TokenType::R_PAREN);
        return std::make_unique<function_call_expr>(
                std::move(identifier), std::move(expressions), current_module);
    }
    return identifier;
}
uptr<string_literal_expr> Parser::parse_string_literal()
{
    std::string value{m_current_token.value};
    eat(TokenType::STRING);
    return std::make_unique<string_literal_expr>(value);
}
uptr<int_literal_expr> Parser::parse_int_literal()
{
    std::string value{m_current_token.value};
    eat(TokenType::INTEGER);
    return std::make_unique<int_literal_expr>(value);
}

uptr<float_literal_expr> Parser::parse_float_literal()
{
    std::string value{m_current_token.value};
    eat(TokenType::FLOAT);
    return std::make_unique<float_literal_expr>(value);
}

expression Parser::parse_unary()
{
    if (is_valid_unary_operator(m_current_token.type))
    {
        TokenType op = m_current_token.type;
        eat(m_current_token.type);

        expression expr = parse_unary();

        return std::make_unique<unary_expr>(std::move(expr), op);
    }
    return parse_primary();
}

expression Parser::parse_primary()
{
    
    switch (m_current_token.type)
    {
    case TokenType::IDENTIFIER:
        return parse_identifier();
    case TokenType::STRING:
        return parse_string_literal();
    case TokenType::INTEGER:
        return parse_int_literal();
    case TokenType::FLOAT:
        return parse_float_literal();
    case TokenType::L_PAREN:
        {
        eat(TokenType::L_PAREN);
        expression expr =  parse_expression();
        eat(TokenType::R_PAREN);
        return expr;
        }

    default:
        Logger::error(m_current_token, HX_UNEXPECTED_TOKEN, "Unexpected token, expected primary expression");
    }

}

expression Parser::parse_expression_from_primary(expression primary, uint32_t min_precedence)
{
    expression lhs = std::move(primary);
    while (is_valid_binary_operator(m_current_token.type) &&
            get_binop_precedence_level(m_current_token.type) >= min_precedence)
    {
        TokenType op = m_current_token.type;
        uint32_t op_precedence = get_binop_precedence_level(op);
        eat(op);
        expression rhs = parse_unary();
        
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
        lhs = make_unique<binop_expr>(std::move(lhs), std::move(rhs), op);
    }
    return lhs;
}

expression Parser::parse_expression()
{
    return parse_expression_from_primary(parse_unary(), 0);
}

type_data Parser::parse_type()
{
    std::optional<primitive_type> pt = get_primitive_type_from_string(m_current_token.value);
    eat(TokenType::IDENTIFIER);
    if (!pt.has_value())
    {
        // TODO OWN TYPES 
        Logger::error(m_current_token, HX_NOT_PRIMITIVE_TYPE, "Expected a primitive type");
    }
     
    uint32_t ptr_depth = 0;
    while (m_current_token.type == TokenType::MULTIPLY)
    {
        ptr_depth++;
        eat(TokenType::MULTIPLY);
    }
    return type_data(pt.value(), ptr_depth);

}

uptr<variable_declaration_statement> Parser::parse_variable_declaration()
{
    type_data td = parse_type(); 
    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    return std::make_unique<variable_declaration_statement>(td, std::move(identifier));
}

statement Parser::parse_statement()
{
    switch (m_current_token.type)
    {
        case TokenType::L_BRACE:
            return parse_compound_statement();
        
        case TokenType::KEYWORD:
            return parse_keyword_statement();
            
        case TokenType::SEMICOLON:
            eat(TokenType::SEMICOLON);
            return std::make_unique<noop_statement>();
    // assume it is an expression statement if none of the above      
        default:
            if (m_current_token.type == TokenType::IDENTIFIER)
            {
                if (get_primitive_type_from_string(m_current_token.value).has_value())
                {
                    return parse_type_statement();
                }
            }
            expression expr = parse_expression();
            eat(TokenType::SEMICOLON);
            return std::make_unique<expression_statement>(std::move(expr));
    }
}

statement Parser::parse_keyword_statement()
{
    KeyWord kw = get_kword_from_string(m_current_token.value);
    switch (kw)
    {
        case KeyWord::RETURN:
            return parse_return_statement();
        case KeyWord::IF: 
            return parse_conditional_statement();
        case KeyWord::WHILE:
            return parse_while_statement();
    default:
        Logger::error(m_current_token, HX_UNEXPECTED_KEYWORD, "Unexpected keyword in statement");
    }
} 

uptr<return_statement> Parser::parse_return_statement()
{
    eat(TokenType::KEYWORD);
    if (m_current_token.type == TokenType::SEMICOLON)
    {
        eat(TokenType::SEMICOLON);
        expression expr = std::make_unique<int_literal_expr>("0");
        return std::make_unique<return_statement>(std::move(expr));
    }
    expression expr = parse_expression();
    eat(TokenType::SEMICOLON);
    return std::make_unique<return_statement>(std::move(expr));
}

uptr<conditional_statement> Parser::parse_conditional_statement()
{
    eat(TokenType::KEYWORD);
    eat(TokenType::L_PAREN);
    expression expr = parse_expression();
    eat(TokenType::R_PAREN);
    statement stat = parse_statement();
    statement else_stat = std::make_unique<noop_statement>();
    if (m_current_token.type == TokenType::KEYWORD &&
            get_kword_from_string(m_current_token.value) == KeyWord::ELSE)
    {
        eat(TokenType::KEYWORD); 
        else_stat = parse_statement();
    }
    return std::make_unique<conditional_statement>(std::move(expr), std::move(stat), std::move(else_stat));
}

uptr<while_statement> Parser::parse_while_statement()
{
    eat(TokenType::KEYWORD);
    eat(TokenType::L_PAREN);
    expression expr = parse_expression(); 
    eat(TokenType::R_PAREN);
    statement stat = parse_statement();
    
    return std::make_unique<while_statement>(std::move(expr), std::move(stat));

}

statement Parser::parse_type_statement()
{
    // checked before calling whether it has value
    type_data type = parse_type();
    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    uptr<variable_declaration_statement> declaration = 
        std::make_unique<variable_declaration_statement>(type, std::move(identifier));

    if (m_current_token.type == TokenType::EQU)
    {
        eat(TokenType::EQU);
        expression definition = parse_expression();
        eat(TokenType::SEMICOLON);
        return std::make_unique<variable_definition_statement>
            (std::move(declaration), std::move(definition));

    }
    eat(TokenType::SEMICOLON);
    return declaration;
}



uptr<compound_statement> Parser::parse_compound_statement()
{
    eat(TokenType::L_BRACE);
    
    std::vector<statement> statements;
    while (m_current_token.type != TokenType::R_BRACE)
    {
       statements.push_back(std::move(
                   parse_statement()));
        
    }
    eat(TokenType::R_BRACE);
    return std::make_unique<compound_statement>(std::move(statements));
}

    
void Parser::eat(TokenType token_type)
{
    if (m_current_token.type != token_type)
    {
        // TODO: unexpected token error
        Logger::error(m_current_token, HX_UNEXPECTED_TOKEN, "Unexpected token");
    }
    
    m_current_token = m_lexer->get_next();
}

}
