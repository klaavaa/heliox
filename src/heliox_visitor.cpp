#include "heliox_visitor.hpp"
#include <print>

    
namespace hx
{
    
    Visitor::Visitor() = default;
    Visitor::~Visitor() = default;


    void Visitor::visit_program(Program& prog)
    {
        for (auto& unit : prog.translation_units)
            visit_translation_unit(unit);
    }

    void Visitor::visit_translation_unit(TranslationUnit& unit)
    {
        filename = unit.filename;
        for (auto& statement : unit.statements)
            visit_statement(statement);
    }


    void Visitor::visit_expression(expression& expr)
    {
        auto* const ast_node = as_ast_node(expr);
        line_number = ast_node->line;
        column = ast_node->position;
        std::visit(overloads{
            [this](uptr<int_literal_expr>& int_literal)
            {visit_int_literal(int_literal);},
            [this](uptr<float_literal_expr>& float_literal)
            {visit_float_literal(float_literal);},
            [this](uptr<string_literal_expr>& string_literal)
            {visit_string_literal(string_literal);},
            [this](uptr<identifier_literal_expr>& identifier_literal)
            {visit_identifier_literal(identifier_literal);},
            [this](uptr<function_call_expr>& function_call)
            {visit_function_call(function_call);},
            [this](uptr<binop_expr>& binop)
            {visit_binop(binop);},
            [this](uptr<unary_expr>& unary)
            {visit_unary(unary);},
            [this](uptr<explicit_conversion_expr>& explicit_conversion)
            {visit_explicit_conversion(explicit_conversion);},
            [this](uptr<noop_expression>& noop)
            {visit_noop_e(noop);}
            }, expr);
    }

    void Visitor::visit_statement(statement& stat)
    {
        auto* const ast_node = as_ast_node(stat);
        line_number = ast_node->line;
        column = ast_node->position;

        std::visit(overloads{
           [this](uptr<compound_statement>& compound)
            { visit_compound(compound);},
           [this](uptr<return_statement>& return_s)
            { visit_return(return_s);},
           [this](uptr<variable_declaration_statement>& variable_declaration)
            { visit_variable_declaration(variable_declaration);},
           [this](uptr<variable_definition_statement>& variable_definition)
            { visit_variable_definition(variable_definition);},
           [this](uptr<conditional_statement>& conditional)
            { visit_conditional(conditional);},
           [this](uptr<while_statement>& while_s)
            { visit_while(while_s);},
           [this](uptr<for_statement>& for_s)
            { visit_for(for_s);},
           [this](uptr<expression_statement>& expression_s)
            { visit_expression_s(expression_s);},
           [this](uptr<noop_statement>& noop_s)
            { visit_noop_s(noop_s); },
           [this](uptr<break_statement>& break_s)
            { visit_break(break_s); },
           [this](uptr<continue_statement>& continue_s)
            { visit_continue(continue_s); },
           [this](uptr<asm_statement>& asm_s)
            { visit_asm(asm_s); },
           [this](uptr<function_statement>& func_s)
            { visit_function(func_s); }
            }, stat);
    }
    
    const ast_node* Visitor::as_ast_node(expression& expr) const
    {
        return std::visit([](auto& e) -> ast_node* { return e.get(); }, expr);
    }
    const ast_node* Visitor::as_ast_node(statement& stat) const
    {
        return std::visit([](auto& s) -> ast_node* { return s.get(); }, stat);
    }
}
