#include "heliox_parser.hpp"
#include "heliox_keywords.hpp"
#include "heliox_operator.hpp"
#include <memory>

namespace hx 
{
Parser::Parser(std::vector<Token>& tokens)
	:
    m_tokens(tokens),
    m_current_token(tokens[0])

{
    m_current_token_index++;
}


TranslationUnit Parser::parse_translation_unit()
{
    std::vector<statement> statements;
    while (m_current_token.type != TokenType::END_OF_FILE)
    {
        statements.push_back(parse_toplevel_statement());
    }

    return TranslationUnit(m_current_token.filename, statements);
}


statement Parser::parse_toplevel_statement()
{
    if (m_current_token.type == TokenType::KEYWORD)
    {
        KeyWord key = get_kword_from_string(m_current_token.value);
        switch (key)
        {
        case KeyWord::FUN:
        case KeyWord::EXTERN:
            return parse_function();
        default:
            Logger::not_implemented();
        }
    }
    Logger::not_implemented();
}

uptr<function_statement> Parser::parse_function()
{
    uint32_t fun_line = m_current_token.line;
    uint32_t fun_position = m_current_token.position;

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
        Logger::error(m_current_token, "Expected 'fun' keyword at start of function definition");
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
                    Logger::error(m_current_token, "Varargs must be the last parameter of a function");
                }
                break;
            }
            parameters.push_back(parse_variable_declaration());

            if (m_current_token.type == TokenType::R_PAREN)
            {
                break;
            }
            eat(TokenType::COMMA);
        }
    }
    eat(TokenType::R_PAREN);
    Type return_type; 
    if (m_current_token.type == TokenType::IDENTIFIER)
        return_type = parse_type(); 
    else 
        return_type = Type::Primitive(PrimitiveType::VOID, 0);
    
    if (m_current_token.type == TokenType::SEMICOLON)
    {
        eat(TokenType::SEMICOLON);
        relevant_line = fun_line;
        relevant_position = fun_position;
        return make_node<function_statement>(identifier->name, std::move(parameters),
                std::move(std::vector<statement>{}), return_type, is_extern, varargs);
    }

    if (is_extern)
    {
        Logger::error(m_current_token, "Extern function defined with a body"); 
    }
    eat(TokenType::L_BRACE);
    std::vector<statement> statements;
    while (m_current_token.type != TokenType::R_BRACE)
    {    
        statements.push_back(parse_statement());
    }
    eat(TokenType::R_BRACE);
    relevant_line = fun_line;
    relevant_position = fun_position;
    return make_node<function_statement>(identifier->name, std::move(parameters),
           std::move(statements), return_type, is_extern, varargs);
}

uptr<identifier_literal_expr> Parser::parse_identifier_literal()
{
    std::string name = m_current_token.value;
    eat(TokenType::IDENTIFIER);
    return make_node<identifier_literal_expr>(name);
}
expression Parser::parse_identifier()
{
    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    // check if identifier is a primitive type, if it is, explicit casting is in order
    // TODO EXPLICIT CONVERSION
    /*
    auto primitive_type = get_primitive_type_from_string(identifier->name);
    uint32_t ptr_depth = 0;
    while (m_current_token.type == TokenType::MULTIPLY)
    {
        ptr_depth++;
        eat(TokenType::MULTIPLY);
    }
    if (!(primitive_type == primitive_type::USER_DEFINED_STRUCT && ptr_depth == 0))
    {
        type_data conversion_type(primitive_type, ptr_depth);
        eat(TokenType::L_PAREN); 
        expression expr = parse_expression();
        eat(TokenType::R_PAREN);
        return make_node<explicit_conversion_expr>(conversion_type, std::move(expr));
    }
    */

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


        return make_node<function_call_expr>(
                identifier->name, std::move(expressions));
    }
    return identifier;
}
uptr<string_literal_expr> Parser::parse_string_literal()
{
    std::string value{m_current_token.value};
    eat(TokenType::STRING);
    return make_node<string_literal_expr>(value);
}
uptr<int_literal_expr> Parser::parse_int_literal()
{
    std::string value{m_current_token.value};
    eat(TokenType::INTEGER);
    return make_node<int_literal_expr>(value);
}

uptr<float_literal_expr> Parser::parse_float_literal()
{
    std::string value{m_current_token.value};
    eat(TokenType::FLOAT);
    return make_node<float_literal_expr>(value);
}

expression Parser::parse_unary()
{
    if (is_valid_unary_operator(m_current_token.type))
    {
        TokenType op = m_current_token.type;
        eat(m_current_token.type);

        expression expr = parse_unary();

        return make_node<unary_expr>(std::move(expr), op);
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
        Logger::error(m_current_token, "Unexpected token, expected primary expression");
    }

}

expression Parser::parse_expression_from_primary(expression primary, uint32_t min_precedence)
{
    expression lhs = std::move(primary);
    while (is_valid_binary_operator(m_current_token.type) &&
            get_binop_precedence_level(m_current_token.type) >= min_precedence)
    {
        TokenType op = m_current_token.type;
        if (is_equals_operator(op))
        {
            if (equal_sign_in_current_expression)
            {
                Logger::error(m_current_token, "Multiple equal signs in expression");
            }
            equal_sign_in_current_expression = true;
        }
        uint32_t op_precedence = get_binop_precedence_level(op);
        eat(op);
        expression rhs = parse_unary();
        
        while ( ((is_valid_binary_operator(m_current_token.type))
             && (get_binop_precedence_level(m_current_token.type) > op_precedence))
             || (get_binop_associativity(m_current_token.type) == op_associativity::RIGHT_TO_LEFT
             && (get_binop_precedence_level(m_current_token.type) == op_precedence)))
        {
            uint32_t new_precedence = op_precedence;
            if (get_binop_precedence_level(m_current_token.type) > op_precedence)
                new_precedence = op_precedence + 1;

            rhs = parse_expression_from_primary(std::move(rhs), new_precedence);

        }

        lhs = make_node<binop_expr>(std::move(lhs), std::move(rhs), op);
    }
    return lhs;
}

expression Parser::parse_expression()
{
    return parse_expression_from_primary(parse_unary(), 0);
}


Type Parser::parse_type()
{
    if (m_current_token.type != TokenType::IDENTIFIER)
        return Type::Unresolved("auto", 0);
    std::string type_name = parse_identifier_literal()->name;
    uint32_t ptr_depth = 0;
    while (m_current_token.type == TokenType::MULTIPLY)
    {
        ptr_depth++; 
        eat(TokenType::MULTIPLY);
    }
    return Type::Unresolved(type_name, ptr_depth);
}

uptr<variable_declaration_statement> Parser::parse_variable_declaration()
{
    std::string name = parse_identifier_literal()->name;
    eat(TokenType::COLON);
    Type type = parse_type(); 
    return make_node<variable_declaration_statement>(name, type);
}

uptr<variable_definition_statement> Parser::parse_variable_definition()
{
    auto declaration = parse_variable_declaration();
    eat(TokenType::EQU);
    auto expression = parse_expression();
    return make_node<variable_definition_statement>(std::move(declaration), std::move(expression));
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
            return make_node<noop_statement>();
        default:
            if (m_current_token.type == TokenType::IDENTIFIER)
            {
                if (peek_next().type == TokenType::COLON)
                {
                    auto declaration = parse_variable_declaration();
                    if (m_current_token.type == TokenType::SEMICOLON)
                        return declaration;
                    eat(TokenType::EQU);
                    auto def = make_node<variable_definition_statement>(std::move(declaration), parse_expression());
                    eat(TokenType::SEMICOLON);
                    return def;
                }
            }
            equal_sign_in_current_expression = false;
            expression expr = parse_expression();
            eat(TokenType::SEMICOLON);
            return make_node<expression_statement>(std::move(expr));
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
        case KeyWord::FOR:
            return parse_for_statement();
        case KeyWord::BREAK:
            return parse_break_statement();
        case KeyWord::CONTINUE:
            return parse_continue_statement();
        case KeyWord::ASM:
            return parse_asm_statement();
    default:
        Logger::error(m_current_token, "Unexpected keyword in statement");
    }
} 

uptr<return_statement> Parser::parse_return_statement()
{
    eat(TokenType::KEYWORD);
    if (m_current_token.type == TokenType::SEMICOLON)
    {
        eat(TokenType::SEMICOLON);
        expression expr = make_node<int_literal_expr>("0");
        return make_node<return_statement>(std::move(expr));
    }
    expression expr = parse_expression();
    eat(TokenType::SEMICOLON);
    return make_node<return_statement>(std::move(expr));
}

uptr<conditional_statement> Parser::parse_conditional_statement()
{
    eat(TokenType::KEYWORD);
    expression expr = parse_expression();

    statement stat = parse_statement();
    statement else_stat = make_node<noop_statement>();
    if (m_current_token.type == TokenType::KEYWORD &&
            get_kword_from_string(m_current_token.value) == KeyWord::ELSE)
    {
        eat(TokenType::KEYWORD); 
        else_stat = parse_statement();
    }
    return make_node<conditional_statement>(std::move(expr), std::move(stat), std::move(else_stat));
}

uptr<while_statement> Parser::parse_while_statement()
{
    eat(TokenType::KEYWORD);
    expression expr = parse_expression(); 
    statement stat = parse_statement();
    
    return make_node<while_statement>(std::move(expr), std::move(stat));

}
uptr<for_statement> Parser::parse_for_statement()
{
    eat(TokenType::KEYWORD);
    eat(TokenType::L_PAREN);
    statement init = parse_statement();
    if (std::holds_alternative<uptr<compound_statement>>(init))
    {
        eat(TokenType::SEMICOLON);
    }

    equal_sign_in_current_expression = false;
    expression condition;
    if (m_current_token.type == TokenType::SEMICOLON)
    {
        condition = make_node<int_literal_expr>("1");
    }
    else
    {
        condition = parse_expression();
    }
    equal_sign_in_current_expression = false;
    eat(TokenType::SEMICOLON);
    expression iteration;
    if (m_current_token.type == TokenType::R_PAREN)
    {
        iteration = make_node<noop_expression>();
    }
    else
    {
        iteration = parse_expression();
    }
    eat(TokenType::R_PAREN);

    statement loop = parse_statement();
    return make_node<for_statement>(std::move(init), std::move(condition), std::move(iteration), std::move(loop));
}

uptr<break_statement> Parser::parse_break_statement()
{
    eat(TokenType::KEYWORD);
    eat(TokenType::SEMICOLON);
    return make_node<break_statement>();
}


uptr<continue_statement> Parser::parse_continue_statement()
{
    eat(TokenType::KEYWORD);
    eat(TokenType::SEMICOLON);
    return make_node<continue_statement>();
}
uptr<asm_statement> Parser::parse_asm_statement()
{
    eat(TokenType::KEYWORD);
    eat(TokenType::L_BRACK); 
    std::vector<uptr<string_literal_expr>> clobber;
    while (m_current_token.type != TokenType::R_BRACK)
    {
        clobber.push_back(parse_string_literal());
        if (m_current_token.type == TokenType::R_BRACK)
            break;
        eat(TokenType::COMMA);
    }
    eat(TokenType::R_BRACK);
    eat(TokenType::L_BRACE);
    uptr<string_literal_expr> asm_body = parse_string_literal();
    eat(TokenType::R_BRACE);
    eat(TokenType::SEMICOLON);

    return make_node<asm_statement>(std::move(clobber), std::move(asm_body));
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
    return make_node<compound_statement>(std::move(statements));
}

    
void Parser::eat(TokenType token_type)
{
    if (m_current_token.type != token_type)
    {
        // TODO: unexpected token error
        Logger::error(m_current_token, std::format("Unexpected token {}", m_current_token.value));
    }
    relevant_line = m_current_token.line; 
    relevant_position = m_current_token.position;

    m_current_token = m_tokens[m_current_token_index++];
}

Token Parser::peek_next(size_t peek_amount)
{
    return m_tokens[m_current_token_index + peek_amount];
}

}
