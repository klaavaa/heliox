#pragma once

#include "heliox_program.hpp"

namespace hx {
class Parser
{

public:
    Parser(std::vector<Token>& tokens);

    TranslationUnit parse_translation_unit();

private:	
    statement parse_toplevel_statement();
    uptr<function_statement> parse_function();
    uptr<struct_statement> parse_struct();
    expression parse_identifier();
    uptr<identifier_literal_expr> parse_identifier_literal();
    uptr<string_literal_expr> parse_string_literal();
    uptr<int_literal_expr> parse_int_literal();
    uptr<float_literal_expr> parse_float_literal();

    expression parse_expression();
    expression parse_expression_from_primary(expression primary, uint32_t min_precedence);
    expression parse_primary(); 
    
    expression parse_unary();

    uptr<variable_definition_statement> parse_variable_definition();
    uptr<variable_declaration_statement> parse_variable_declaration();
    
    Type parse_type();
	
    statement parse_statement();
    uptr<compound_statement> parse_compound_statement();
    statement parse_type_statement(); // variable_declaration or variable_defenition 
    statement parse_keyword_statement();
    uptr<return_statement> parse_return_statement();
    uptr<conditional_statement> parse_conditional_statement();
    uptr<while_statement> parse_while_statement();
    uptr<for_statement> parse_for_statement();
    uptr<break_statement> parse_break_statement();
    uptr<continue_statement> parse_continue_statement();
    uptr<asm_statement> parse_asm_statement();

	void eat(TokenType token_type);
    Token peek_next(size_t peek_amount = 0);
     

    // creates the node with source location information
    template<typename T, typename... Args>
    requires std::derived_from<T, ast_node>

    uptr<T> make_node(Args&&... args)
    {
        return std::make_unique<T>(m_current_token.filename, relevant_line, relevant_position, std::forward<Args>(args)...);
    }

private:
    std::vector<Token>& m_tokens;    
    size_t m_current_token_index = 0;
    Token m_current_token;


    bool in_module_block = false;
    uint32_t relevant_line;
    uint32_t relevant_position;
    bool equal_sign_in_current_expression = false;
};
}
