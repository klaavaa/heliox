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
Parser::Parser(std::vector<Token>& tokens)
	:
    m_tokens(tokens),
    m_current_token(tokens[0]),
    global_module(std::make_shared<Module>()),
    m_current_module(global_module)
{
    m_current_token_index++;
}

uptr<TranslationUnit> Parser::parse_translation_unit()
{
    //std::vector<uptr<function>> functions;

    //sptr<Module> current_module = std::make_shared<Module>();

    while (m_current_token.type != TokenType::END_OF_FILE)
    {
        parse_module();
    }

    return std::make_unique<TranslationUnit>(std::move(global_module), std::move(imports), m_current_token.filename);

}

void Parser::parse_module()
{
    switch (m_current_token.type) 
     {
        case TokenType::KEYWORD:
        {
            KeyWord kword = get_kword_from_string(m_current_token.value);
            switch (kword) 
            {
                case KeyWord::FUN:
                case KeyWord::EXTERN:
                    m_current_module->insert_function(std::move(parse_function()));
                    break;
                case KeyWord::MODULE:
                {
                    if (!in_module_block)
                    {
                        m_current_module = global_module;
                    }

                    eat(TokenType::KEYWORD);
                    if (m_current_token.type == TokenType::SEMICOLON)
                    {
                        eat(TokenType::SEMICOLON);
                        return;
                    }
                    m_current_module = create_or_get_submodule(m_current_module, m_current_token.value);
                    eat(TokenType::IDENTIFIER);
                    while (m_current_token.type == TokenType::COLON)
                    {
                        eat(TokenType::COLON);
                        eat(TokenType::COLON);
                        m_current_module = create_or_get_submodule(m_current_module, m_current_token.value);
                        eat(TokenType::IDENTIFIER);
                    }
                    if (m_current_token.type == TokenType::L_BRACE)
                    {
                        bool was_in_module_block = in_module_block;
                        eat(TokenType::L_BRACE);
                        in_module_block = true;
                        parse_module();
                        eat(TokenType::R_BRACE);
                        in_module_block = was_in_module_block;
                        m_current_module = m_current_module->parent_module;
                        return;
                    }
                    eat(TokenType::SEMICOLON);
                    return;
                }
                case KeyWord::IMPORT:
                {
                    eat(TokenType::KEYWORD);
                    uptr<identifier_literal_expr> module_path = parse_identifier_literal();
                    imports.push_back(make_node<import_statement>(std::move(module_path))); 
                    eat(TokenType::SEMICOLON);
                    break;
                }
                case KeyWord::STRUCT:
                {
                    eat(TokenType::KEYWORD);
                    uptr<identifier_literal_expr> struct_name = parse_identifier_literal();
                    std::vector<uptr<variable_declaration_statement>> declarations;
                    eat(TokenType::L_BRACE);
                    while (m_current_token.type != TokenType::R_BRACE)
                    {
                        declarations.push_back(parse_variable_declaration());
                        eat(TokenType::SEMICOLON);
                    }
                    eat(TokenType::R_BRACE);
                    eat(TokenType::SEMICOLON);
                    
                    m_current_module->structs.push_back(std::make_unique<struct_declaration>(std::move(declarations)));
                    break;
                }
                default:
                {
                    Logger::error(m_current_token, HX_UNEXPECTED_KEYWORD, "Unexpected keyword");
                }
            }
            break;
            }
        default:
        {
            Logger::error(m_current_token, HX_UNEXPECTED_TOKEN, "Unexpected token");
        }
    }
}

uptr<function> Parser::parse_function()
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
                parameters.emplace_back(make_node<variable_declaration_statement>(td, parse_identifier_literal()));
            }
            else
            {
                parameters.emplace_back(make_node<variable_declaration_statement>(td,
                            make_node<identifier_literal_expr>("")));
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
                    std::move(std::vector<statement>{}), td, is_extern, varargs, m_current_token.filename, fun_line, fun_position);
        return std::make_unique<function>(std::move(identifier), std::move(parameters),
                std::move(std::vector<statement>{}), td, is_extern, varargs, m_current_token.filename, fun_line, fun_position);
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
           std::move(statements), td, is_extern, varargs, m_current_token.filename, fun_line, fun_position);
}

uptr<identifier_literal_expr> Parser::parse_identifier_literal()
{
    std::vector<std::string> name_parts{};
    name_parts.push_back(m_current_token.value);
    eat(TokenType::IDENTIFIER);
    while (m_current_token.type == TokenType::COLON)
    {
        eat(TokenType::COLON);
        eat(TokenType::COLON);
        name_parts.push_back(m_current_token.value);
        eat(TokenType::IDENTIFIER);
    }
    if (name_parts.size() == 1)
        return make_node<identifier_literal_expr>(name_parts.front());
    std::string name = name_parts.back();
    name_parts.pop_back();
    return make_node<identifier_literal_expr>(name, name_parts);
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

        std::vector<std::string> module_path = m_current_module->get_module_path();

        return make_node<function_call_expr>(
                std::move(identifier), std::move(expressions), std::move(module_path));
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
        if (is_equals_operator(op))
        {
            if (equal_sign_in_current_expression)
            {
                Logger::error(m_current_token, HX_MULTIPLE_EQUAL_SIGNS_IN_EXPRESSION, "Multiple equal signs in expression");
            }
            equal_sign_in_current_expression = true;
        }
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

        lhs = make_node<binop_expr>(std::move(lhs), std::move(rhs), op);
    }
    return lhs;
}

expression Parser::parse_expression()
{
    return parse_expression_from_primary(parse_unary(), 0);
}

type_data Parser::parse_type()
{
    primitive_type pt = get_primitive_type_from_string(m_current_token.value);
    eat(TokenType::IDENTIFIER);
     
    uint32_t ptr_depth = 0;
    while (m_current_token.type == TokenType::MULTIPLY)
    {
        ptr_depth++;
        eat(TokenType::MULTIPLY);
    }
    return type_data(pt, ptr_depth);

}

uptr<variable_declaration_statement> Parser::parse_variable_declaration()
{
    type_data td = parse_type(); 
    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    return make_node<variable_declaration_statement>(td, std::move(identifier));
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
    // assume it is an expression statement if none of the above      
        default:
            if (m_current_token.type == TokenType::IDENTIFIER)
            {
                size_t index_to_peek_next = 0;
                while (peek_next(index_to_peek_next).type == TokenType::MULTIPLY)
                {
                    index_to_peek_next += 1;
                }
                if (peek_next(index_to_peek_next).type == TokenType::IDENTIFIER &&
                    ((peek_next(index_to_peek_next + 1).type == TokenType::EQU) || (peek_next(index_to_peek_next + 1).type == TokenType::SEMICOLON)))
                {
                    return parse_type_statement();
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

statement Parser::parse_type_statement()
{
    // checked before calling whether it has value
    type_data type = parse_type();
    uptr<identifier_literal_expr> identifier = parse_identifier_literal();
    uptr<variable_declaration_statement> declaration = 
        make_node<variable_declaration_statement>(type, std::move(identifier));

    if (m_current_token.type == TokenType::EQU)
    {
        eat(TokenType::EQU);
        equal_sign_in_current_expression = true;
        expression definition = parse_expression();
        eat(TokenType::SEMICOLON);
        return make_node<variable_definition_statement>
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
    return make_node<compound_statement>(std::move(statements));
}

    
void Parser::eat(TokenType token_type)
{
    if (m_current_token.type != token_type)
    {
        // TODO: unexpected token error
        Logger::error(m_current_token, HX_UNEXPECTED_TOKEN, "Unexpected token");
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
