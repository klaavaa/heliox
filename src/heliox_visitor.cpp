#include "heliox_visitor.hpp"

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

    
namespace hx
{
    
    Visitor::Visitor() = default;
    Visitor::~Visitor() = default;

    void Visitor::visit_program(uptr<Program>& prog)
    {
        for (auto& func : prog->functions)
        {
            visit_function(func);
        }
    }

    void Visitor::visit_expression(expression& expr)
  {
        std::visit(overloads{
            [this](uptr<int_literal_expr>& int_literal)
            {visit_int_literal(int_literal);},
            [this](uptr<string_literal_expr>& string_literal)
            {visit_string_literal(string_literal);},
            [this](uptr<identifier_literal_expr>& identifier_literal)
            {visit_identifier_literal(identifier_literal);},
            [this](uptr<function_call_expr>& function_call)
            {visit_function_call(function_call);},
            [this](uptr<binop_expr>& binop)
            {visit_binop(binop);},
            [this](uptr<unary_expr>& unary)
            {visit_unary(unary);}
            }, expr);
    }

    void Visitor::visit_statement(statement& stat)
    {
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
           [this](uptr<expression_statement>& expression_s)
            { visit_expression_s(expression_s);},
            [this](uptr<noop_statement>& noop_s)
            { visit_noop(noop_s); }
            }, stat);
    }
    
}
