#pragma once

#include "heliox_expression.hpp"
#include "heliox_statement.hpp"
#include "heliox_token.hpp"
#include "heliox_lexer.hpp"
#include "heliox_program.hpp"

namespace hx {
class Parser
{

public:
    Parser(uptr<Lexer> lex);
    
    uptr<TranslationUnit> parse_translation_unit();

private:	
    void parse_module();

    uptr<function> parse_function();
    expression parse_identifier();
    uptr<identifier_literal_expr> parse_identifier_literal();
    uptr<string_literal_expr> parse_string_literal();
    uptr<int_literal_expr> parse_int_literal();
    uptr<float_literal_expr> parse_float_literal();

    expression parse_expression();
    expression parse_expression_from_primary(expression primary, uint32_t min_precedence);
    expression parse_primary(); 
    
    expression parse_unary();

    uptr<variable_declaration_statement> parse_variable_declaration();

    type_data parse_type();
	
    statement parse_statement();
    uptr<compound_statement> parse_compound_statement();
    statement parse_type_statement(); // variable_declaration or variable_defenition 
    statement parse_keyword_statement();
    uptr<return_statement> parse_return_statement();
    uptr<conditional_statement> parse_conditional_statement();
    uptr<while_statement> parse_while_statement();


	void eat(TokenType token_type);
     
    // creates the node with source location information
    template<typename T, typename... Args>
    uptr<T> make_node(Args&&... args)
    {
        return std::make_unique<T>(m_current_token.filename, relevant_line, relevant_position, std::forward<Args>(args)...);
    }

private:
    uptr<Lexer> m_lexer;
    Token m_current_token;

    sptr<Module> global_module;
    sptr<Module> m_current_module;

    std::vector<uptr<import_statement>> imports;
    
    bool in_module_block = false;

    uint32_t relevant_line;
    uint32_t relevant_position;
    bool equal_sign_in_current_expression = false;
};
}
